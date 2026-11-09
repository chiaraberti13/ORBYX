#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verified per-user native updates and rollback. No shell, sudo or unsigned fallback."""
from __future__ import annotations
import argparse
from contextlib import contextmanager
import ctypes
import errno
import fcntl
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
import uuid

REPO='chiaraberti13/ORBYX'
WORKFLOW=REPO+'/.github/workflows/attest.yml'
MAX_ARCHIVE=1024*1024*1024
MAX_PAYLOAD=512*1024*1024
ID=re.compile(r'[0-9a-f]{12}-[0-9a-f]{32}')

def machine():
    release={}
    for line in Path('/etc/os-release').read_text().splitlines():
        if '=' in line:
            k,v=line.split('=',1);release[k]=v.strip('"')
    return {'id':release.get('ID'),'version_id':release.get('VERSION_ID'),'architecture':platform.machine()}

def private_copy(source: Path,target: Path,limit: int):
    fd=os.open(source,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or not 0 < info.st_size <= limit:raise ValueError('Invalid or excessive update input')
        count=0
        with os.fdopen(os.dup(fd),'rb') as reader,target.open('xb') as writer:
            while True:
                block=reader.read(min(65536,limit-count+1))
                if not block:break
                count+=len(block)
                if count>limit:raise ValueError('Update input grew beyond bounds')
                writer.write(block)
            writer.flush();os.fsync(writer.fileno())
    finally:os.close(fd)

def verify_attestation(archive: Path,bundle: Path,commit: str):
    result=subprocess.run(['gh','attestation','verify',str(archive),'--bundle',str(bundle),
        '--repo',REPO,'--signer-workflow',WORKFLOW,'--source-ref','refs/heads/main',
        '--source-digest',commit,'--deny-self-hosted-runners','--format','json'],
        capture_output=True,text=True,timeout=180)
    if result.returncode:raise ValueError('Signature or signer policy failed: '+result.stderr[-4000:])
    if not result.stdout.strip():raise ValueError('Verifier returned no evidence')

def extract_verified(archive: Path,stage: Path):
    unpack=stage/'unpack';unpack.mkdir(mode=0o700)
    roots=set();seen=set();total=0;count=0
    with tarfile.open(archive,'r|gz') as package:
        for member in package:
            count+=1
            if count>10000:raise ValueError('Too many archive entries')
            path=PurePosixPath(member.name)
            if path.is_absolute() or '..' in path.parts or '\\' in member.name or '\x00' in member.name:raise ValueError('Unsafe archive path')
            if not path.parts:
                if member.isdir():continue
                raise ValueError('Empty archive filename')
            if not (member.isdir() or member.isfile()):raise ValueError('Links/devices are not allowed in native updates')
            name=str(path)
            if name in seen:raise ValueError('Duplicate archive member')
            seen.add(name);roots.add(path.parts[0])
            if len(roots)!=1:raise ValueError('Expected a single package root')
            destination=unpack.joinpath(*path.parts)
            if member.isdir():destination.mkdir(parents=True,exist_ok=True,mode=0o700);continue
            total+=member.size
            if member.size<0 or total>MAX_PAYLOAD:raise ValueError('Archive expands beyond limits')
            destination.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
            source=package.extractfile(member)
            if source is None:raise ValueError('Missing archive content')
            remaining=member.size
            with destination.open('xb') as output:
                while remaining:
                    block=source.read(min(65536,remaining))
                    if not block:raise ValueError('Truncated archive member')
                    output.write(block);remaining-=len(block)
                output.flush();os.fsync(output.fileno())
            destination.chmod(0o755 if member.mode&0o111 else 0o644)
    if len(roots)!=1:raise ValueError('Empty package')
    payload=unpack/next(iter(roots))
    if not payload.is_dir():raise ValueError('Invalid package root')
    payload.rename(stage/'application');unpack.rmdir()
    for directory in sorted((p for p in (stage/'application').rglob('*') if p.is_dir()),key=lambda p:len(p.parts),reverse=True)+[stage/'application']:
        fd=os.open(directory,os.O_RDONLY|os.O_DIRECTORY)
        try:os.fsync(fd)
        finally:os.close(fd)
    return stage/'application'

def health_check(payload: Path):
    metadata=read_json(payload/'share/orbyx/build-platform.json')
    if metadata!=machine():raise ValueError('Package is for a different distribution/version/architecture')
    executable=payload/'bin/orbyx'
    if executable.is_symlink() or not executable.is_file() or not os.access(executable,os.X_OK):raise ValueError('Missing native executable')
    probe=subprocess.run([str(executable),'--version'],capture_output=True,text=True,timeout=30)
    if probe.returncode or not probe.stdout.startswith('ORBYX '):raise ValueError('New CLI failed its health check')
    gui=payload/'bin/orbyx-gui'
    if gui.exists():
        probe=subprocess.run([str(gui),'--smoke-test'],capture_output=True,text=True,timeout=30,env=dict(os.environ,QT_QPA_PLATFORM='offscreen'))
        if probe.returncode:raise ValueError('New desktop failed its health check')

def read_json(path: Path):
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size>1024*1024:raise ValueError('Invalid update metadata')
        with os.fdopen(os.dup(fd),'rb') as stream:raw=stream.read(1024*1024+1)
        if len(raw)>1024*1024:raise ValueError('Update metadata exceeds bounds')
        def unique(pairs):
            result={}
            for key,value in pairs:
                if key in result:raise ValueError('Duplicate update metadata key')
                result[key]=value
            return result
        value=json.loads(raw,object_pairs_hook=unique)
        if not isinstance(value,dict):raise ValueError('Expected update metadata object')
        return value
    finally:os.close(fd)

def default_root():return Path(os.environ.get('XDG_DATA_HOME',str(Path.home()/'.local/share')))/'orbyx'

@contextmanager
def locked(root: Path):
    if platform.system()!='Linux':raise ValueError('Managed native updates currently support Linux only')
    root=Path(os.path.abspath(root))
    if root.is_symlink():raise ValueError('Update root must not be a symlink')
    root.mkdir(parents=True,exist_ok=True,mode=0o700)
    store=root/'updates'
    for path in (store,store/'releases'):
        if path.is_symlink():raise ValueError('Update store must not contain directory symlinks')
        path.mkdir(exist_ok=True,mode=0o700)
    fd=os.open(store/'lock',os.O_CREAT|os.O_RDWR|os.O_NOFOLLOW|os.O_NONBLOCK,0o600)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_nlink!=1:raise ValueError('Invalid update lock')
        fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
        yield root,store
    finally:os.close(fd)

def active_id(root: Path,store: Path):
    prefix=root/'application'
    if not prefix.is_symlink():return None
    target=os.readlink(prefix);path=Path(os.path.abspath(prefix.parent/target))
    expected=store/'releases'
    if path.name!='application' or path.parent.parent!=expected or not ID.fullmatch(path.parent.name):raise ValueError('Unrecognized active application pointer')
    return path.parent.name

def exchange(left: Path,right: Path):
    libc=ctypes.CDLL(None,use_errno=True)
    call=getattr(libc,'renameat2',None)
    if call is None:raise ValueError('Atomic legacy migration is unsupported on this system')
    call.argtypes=[ctypes.c_int,ctypes.c_char_p,ctypes.c_int,ctypes.c_char_p,ctypes.c_uint]
    if call(-100,os.fsencode(left),-100,os.fsencode(right),2):
        number=ctypes.get_errno();raise OSError(number,os.strerror(number))

def activate(root: Path,store: Path,release: str):
    prefix=root/'application';link=root/('.switch-'+uuid.uuid4().hex)
    link.symlink_to(Path('updates/releases')/release/'application',target_is_directory=True)
    backup=None
    try:
        if prefix.exists() and not prefix.is_symlink():
            if not prefix.is_dir():raise ValueError('Application prefix is not a directory')
            exchange(link,prefix)  # atomically replaces a legacy directory with the signed pointer
            backup=store/('legacy-'+uuid.uuid4().hex)
            link.rename(backup)  # retain, never execute it as signed rollback
        else:os.replace(link,prefix)
        fd=os.open(root,os.O_RDONLY|os.O_DIRECTORY)
        try:os.fsync(fd)
        finally:os.close(fd)
    finally:
        if link.is_symlink():link.unlink()
    return backup

def install(archive: Path,bundle: Path,commit: str,root: Path,*,verifier=verify_attestation,health=health_check):
    commit=commit.strip().lower()
    if not re.fullmatch(r'[0-9a-f]{40}',commit):raise ValueError('Pin the full approved source commit SHA')
    with locked(root) as (root,store):
        return install_locked(archive,bundle,commit,root,store,verifier,health)

def install_locked(archive,bundle,commit,root,store,verifier,health):
    previous=active_id(root,store)
    stage=Path(tempfile.mkdtemp(prefix='.stage-',dir=store/'releases'))
    release=commit[:12]+'-'+uuid.uuid4().hex
    try:
        private_copy(archive,stage/'archive.tar.gz',MAX_ARCHIVE)
        private_copy(bundle,stage/'provenance.sigstore.json',64*1024*1024)
        verifier(stage/'archive.tar.gz',stage/'provenance.sigstore.json',commit)  # before extraction/execution
        payload=extract_verified(stage/'archive.tar.gz',stage)
        current=root/'application'
        if (current/'bin/orbyx-gui').exists() and not (payload/'bin/orbyx-gui').exists():raise ValueError('Update would remove the installed desktop')
        health(payload)
        info={'schema':'orbyx-update/1','commit':commit,'previous':previous,
              'archive_sha256':hash_file(stage/'archive.tar.gz'),'repository':REPO,'workflow':WORKFLOW}
        with (stage/'info.json').open('x') as stream:json.dump(info,stream,sort_keys=True);stream.flush();os.fsync(stream.fileno())
        fd=os.open(stage,os.O_RDONLY|os.O_DIRECTORY)
        try:os.fsync(fd)
        finally:os.close(fd)
        destination=store/'releases'/release;stage.rename(destination)
        fd=os.open(store/'releases',os.O_RDONLY|os.O_DIRECTORY)
        try:os.fsync(fd)
        finally:os.close(fd)
        backup=activate(root,store,release)
        if backup is not None:info["legacy_backup"]=str(backup)
        return info
    finally:
        if stage.exists():shutil.rmtree(stage)

def hash_file(path):
    digest=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(65536),b''):digest.update(block)
    return digest.hexdigest()

def rollback(root: Path,*,verifier=verify_attestation,health=health_check):
    with locked(root) as (root,store):
        active=active_id(root,store)
        if not active:raise ValueError('No managed installation')
        current=read_json(store/'releases'/active/'info.json');previous=current.get('previous')
        if not isinstance(previous,str) or not ID.fullmatch(previous):raise ValueError('No previous signed release; legacy source installations are preserved separately')
        target=store/'releases'/previous;info=read_json(target/'info.json')
        if info.get('schema')!='orbyx-update/1' or not re.fullmatch(r'[0-9a-f]{40}',str(info.get('commit',''))):raise ValueError('Invalid cached release identity')
        # Re-verify and reconstruct from the signed archive. Mutable installed
        # files or an unsigned local hash manifest never become the trust anchor.
        return install_locked(target/'archive.tar.gz',target/'provenance.sigstore.json',info['commit'],root,store,verifier,health)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=default_root())
    commands=parser.add_subparsers(dest='command',required=True)
    update=commands.add_parser('install');update.add_argument('--archive',type=Path,required=True);update.add_argument('--bundle',type=Path,required=True);update.add_argument('--commit',required=True)
    commands.add_parser('rollback');commands.add_parser('status');args=parser.parse_args()
    try:
        if args.command=='status':
            root=Path(os.path.abspath(args.root));active=active_id(root,root/'updates')
            print(json.dumps({'active':active,'managed':bool(active)}));return 0
        if os.geteuid()==0:raise ValueError('Run managed updates as your regular user, without sudo')
        result=install(args.archive,args.bundle,args.commit,args.root) if args.command=='install' else rollback(args.root)
        if result.get('legacy_backup'):print('Backup installazione precedente / Legacy backup: '+result['legacy_backup'])
        print('Aggiornamento verificato / Verified update: '+result['commit']);print('Riavvia l’app per usare la versione attivata / Restart the app to use the activated version');return 0
    except (ValueError,OSError,subprocess.SubprocessError,tarfile.TarError,EOFError,KeyError,TypeError,RecursionError) as error:
        print('orbyx-update: '+str(error),file=sys.stderr);return 2

if __name__=='__main__':raise SystemExit(main())
