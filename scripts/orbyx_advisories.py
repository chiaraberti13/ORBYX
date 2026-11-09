#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Correlate an exact Ubuntu runtime SBOM with official Ubuntu data in OSV."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import stat
import sys
import urllib.request

API = 'https://api.osv.dev/v1/querybatch'
LIMIT = 16 * 1024 * 1024

def load(path: Path):
    if path.is_symlink() or not path.is_file() or path.stat().st_size > LIMIT:
        raise ValueError('Invalid or excessive JSON input')
    def unique(pairs):
        result={}
        for key,value in pairs:
            if key in result:raise ValueError('Duplicate JSON key')
            result[key]=value
        return result
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        if not stat.S_ISREG(os.fstat(fd).st_mode):raise ValueError('Expected regular JSON file')
        with os.fdopen(os.dup(fd),'rb') as stream:raw=stream.read(LIMIT+1)
    finally:os.close(fd)
    if len(raw)>LIMIT:raise ValueError('JSON exceeds bounds')
    def invalid(value):raise ValueError('Non-finite JSON number')
    return json.loads(raw,object_pairs_hook=unique,parse_constant=invalid)

def canonical(value):return json.dumps(value,sort_keys=True,separators=(',',':')).encode()

def source_package(binary: str, version: str):
    if not re.fullmatch(r'[a-z0-9][a-z0-9+.:_-]{0,199}',binary):raise ValueError('Invalid package name')
    result=subprocess.run(['dpkg-query','-W','-f=${Version}\t${source:Package}\t${source:Version}',binary],capture_output=True,text=True,check=True,timeout=20,env=dict(os.environ,LC_ALL='C'))
    installed,source,source_version=result.stdout.strip().split('\t')
    if installed!=version:raise ValueError('SBOM does not match installed package version')
    if not source or not source_version:raise ValueError('Source package unavailable')
    return source,source_version

def queries_for(inventory, ecosystem, resolver=source_package):
    if not isinstance(inventory,dict) or inventory.get('schema')!='orbyx-sbom/1':raise ValueError('Expected native ORBYX inventory')
    if not re.fullmatch(r'Ubuntu:\d{2}\.(04|10)(:LTS)?',ecosystem):raise ValueError('Supported scope: standard Ubuntu release ecosystems')
    queries={};unknown=[]
    binaries=inventory.get('binaries')
    if not isinstance(binaries,list) or len(binaries)>10000:raise ValueError('Invalid binary inventory')
    for binary in binaries:
        if not isinstance(binary,dict) or not isinstance(binary.get('libraries'),list) or len(binary['libraries'])>10000:raise ValueError('Invalid library inventory')
        for library in binary['libraries']:
            if not isinstance(library,dict):raise ValueError('Invalid runtime library entry')
            package,version=library.get('package'),library.get('version')
            if not package or not version:
                unknown.append({'library':library.get('soname'),'reason':'Unmapped runtime library'});continue
            try:
                source,source_version=resolver(package,version)
                key=(source,source_version)
                queries[key]={'package':{'name':source,'ecosystem':ecosystem},'version':source_version}
            except (ValueError,OSError,subprocess.SubprocessError,RecursionError) as error:
                unknown.append({'package':package,'reason':str(error)})
    if not queries or len(queries)>2000:raise ValueError('No resolved packages or excessive runtime inventory')
    return [queries[key] for key in sorted(queries)],unknown

def post(payload):
    request=urllib.request.Request(API,canonical(payload),{'Content-Type':'application/json','User-Agent':'ORBYX-advisories/1'},method='POST')
    with urllib.request.urlopen(request,timeout=60) as response:
        if response.url!=API:raise ValueError('Unexpected advisory redirect')
        raw=response.read(LIMIT+1)
    if len(raw)>LIMIT:raise ValueError('Advisory response exceeds bounds')
    return json.loads(raw)

def query_all(queries, fetch=post):
    output=[{} for _ in queries];pending=[(i,q) for i,q in enumerate(queries)];pages=[];seen=set()
    for _ in range(100):
        if not pending:return [sorted(x.values(),key=lambda v:v['id']) for x in output],pages
        # Bound each API batch even when a large ELF dependency graph was resolved.
        current,pending=pending[:100],pending[100:]
        payload={'queries':[q for _,q in current]};response=fetch(payload)
        results=response.get('results') if isinstance(response,dict) else None
        if not isinstance(results,list) or len(results)!=len(current):raise ValueError('Incomplete OSV batch response')
        pages.append({'request':payload,'response':response})
        for (index,query),result in zip(current,results):
            if not isinstance(result,dict) or set(result)-{'vulns','next_page_token'}:raise ValueError('Invalid OSV result')
            for vulnerability in result.get('vulns',[]):
                if not isinstance(vulnerability,dict) or not isinstance(vulnerability.get('id'),str):raise ValueError('Invalid vulnerability entry')
                output[index][vulnerability['id']]=vulnerability
            token=result.get('next_page_token')
            if token:
                if not isinstance(token,str) or len(token)>8192 or (index,token) in seen:raise ValueError('Invalid or repeated OSV page token')
                seen.add((index,token));pending.append((index,dict(query,page_token=token)))
    raise ValueError('Advisory pagination limit; scan incomplete')

def scan(inventory,ecosystem,*,resolver=source_package,fetch=post):
    queries,unknown=queries_for(inventory,ecosystem,resolver)
    vulnerabilities,pages=query_all(queries,fetch)
    return {'schema':'orbyx-advisories/1','inventory_sha256':hashlib.sha256(canonical(inventory)).hexdigest(),
            'source':API,'ecosystem':ecosystem,'retrieved_at':datetime.now(timezone.utc).isoformat(),
            'coverage':'incomplete' if unknown else 'resolved-runtime-packages',
            'unknown':unknown,'packages':[dict(query,vulnerabilities=v) for query,v in zip(queries,vulnerabilities)],
            'evidence':pages,'evidence_sha256':hashlib.sha256(canonical(pages)).hexdigest(),
            'limitations':'OSV Ubuntu source-package matches only; absence of matches is not a guarantee of safety. dlopen/plugins/custom builds outside the SBOM are not covered.'}

def replay(report):
    if not isinstance(report,dict) or report.get('schema')!='orbyx-advisories/1' or report.get('source')!=API:raise ValueError('Invalid advisory snapshot')
    pages=report.get('evidence',[])
    if hashlib.sha256(canonical(pages)).hexdigest()!=report.get('evidence_sha256'):raise ValueError('Snapshot evidence changed')
    recorded=iter(pages)
    def fetch(request):
        try:page=next(recorded)
        except StopIteration as error:raise ValueError('Incomplete snapshot') from error
        if page['request']!=request:raise ValueError('Snapshot query mismatch')
        return page['response']
    queries=[{key:value for key,value in p.items() if key!='vulnerabilities'} for p in report['packages']]
    matches,_=query_all(queries,fetch)
    if matches!=[p['vulnerabilities'] for p in report['packages']]:raise ValueError('Snapshot findings changed')
    if next(recorded,None) is not None:raise ValueError('Extra snapshot pages')
    return report

def default_ecosystem():
    release={}
    for line in Path('/etc/os-release').read_text().splitlines():
        if '=' in line:
            k,v=line.split('=',1);release[k]=v.strip('"')
    if release.get('ID')!='ubuntu':raise ValueError('Online resolver supports installed Ubuntu packages only')
    version=release['VERSION_ID'];year,month=map(int,version.split('.'))
    return 'Ubuntu:'+version+(':LTS' if year%2==0 and month==4 else '')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    source=parser.add_mutually_exclusive_group(required=True);source.add_argument('--inventory',type=Path);source.add_argument('--replay',type=Path)
    parser.add_argument('--online',action='store_true',help='send source package names/versions to OSV; never sends binary paths')
    parser.add_argument('--output',type=Path);args=parser.parse_args()
    try:
        if args.replay:report=replay(load(args.replay))
        else:
            if not args.online:raise ValueError('Online access requires --online')
            report=scan(load(args.inventory),default_ecosystem())
        if args.output:
            with args.output.open('x',encoding='utf-8') as stream:json.dump(report,stream,sort_keys=True,indent=2)
        found=sum(len(p['vulnerabilities']) for p in report['packages'])
        print(f"Pacchetti / Packages: {len(report['packages'])}; segnalazioni / findings: {found}; copertura / coverage: {report['coverage']}")
        return 2 if report['unknown'] else 1 if found else 0
    except (ValueError,OSError,KeyError,TypeError,subprocess.SubprocessError,RecursionError) as error:
        print(f'orbyx-advisories: {error}',file=sys.stderr);return 2

if __name__=='__main__':raise SystemExit(main())
