# SPDX-License-Identifier: GPL-3.0-only
"""Filesystem/transaction tests use an injected verifier; real signing is tested in OIDC CI."""
import importlib.util
import io
import json
from pathlib import Path
import shutil
import sys
import tarfile
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
CLI=Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else ROOT/'build/orbyx'
spec=importlib.util.spec_from_file_location('update',ROOT/'scripts/orbyx_update.py');u=importlib.util.module_from_spec(spec);spec.loader.exec_module(u)
SHA='a'*40
class UpdateTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name);self.managed=self.root/'managed'
        self.archive=self.root/'package.tar.gz';self.bundle=self.root/'bundle.json';self.bundle.write_text('{}');self.verified=0
        self.make_archive()
    def make_archive(self,extra=None,platform=None):
        with tarfile.open(self.archive,'w:gz') as archive:
            entries={'pkg/bin/orbyx':CLI.read_bytes(),'pkg/share/orbyx/build-platform.json':json.dumps(platform or u.machine()).encode(),'pkg/share/test':b'original'}
            if extra:entries.update(extra)
            for path,data in entries.items():
                member=tarfile.TarInfo(path);member.size=len(data);member.mode=0o755 if path.endswith('orbyx') else 0o644;archive.addfile(member,io.BytesIO(data))
    def verify(self,archive,bundle,commit):
        self.verified+=1
        self.assertIn(commit,(SHA,'b'*40))
        self.assertTrue(archive.parent.name.startswith('.stage-'))
        self.assertFalse((archive.parent/'unpack').exists(), 'Extraction preceded verification')
    def install(self):return u.install(self.archive,self.bundle,SHA,self.managed,verifier=self.verify)
    def test_install_and_verified_clean_rollback(self):
        self.install();first=(self.managed/'application').resolve();self.make_archive({'pkg/share/test':b'next-version'});u.install(self.archive,self.bundle,'b'*40,self.managed,verifier=self.verify)
        (first/'share/test').write_text('changed local files')
        result=u.rollback(self.managed,verifier=self.verify)
        self.assertEqual(result['commit'],SHA);self.assertEqual(self.verified,3)
        self.assertEqual((self.managed/'application/share/test').read_bytes(),b'original')
    def test_signature_failure_cannot_activate_or_extract(self):
        def reject(*args):raise ValueError('invalid signature')
        with self.assertRaises(ValueError):u.install(self.archive,self.bundle,SHA,self.managed,verifier=reject)
        self.assertFalse((self.managed/'application').exists());self.assertFalse(list((self.managed/'updates/releases').iterdir()))
    def test_path_traversal_rejected(self):
        self.make_archive({'pkg/../../outside':b'bad'})
        with self.assertRaises(ValueError):self.install()
        self.assertFalse((self.root/'outside').exists());self.assertFalse((self.managed/'application').exists())
    def test_archive_symlink_rejected(self):
        with tarfile.open(self.archive,'w:gz') as archive:
            member=tarfile.TarInfo('pkg/link');member.type=tarfile.SYMTYPE;member.linkname='/tmp';archive.addfile(member)
        with self.assertRaises(ValueError):self.install()
    def test_platform_mismatch_preserves_active_version(self):
        self.install();before=(self.managed/'application').resolve();self.make_archive(platform={'id':'unsupported'})
        with self.assertRaises(ValueError):self.install()
        self.assertEqual((self.managed/'application').resolve(),before)
    def test_legacy_migration_is_atomic_and_legacy_not_signed_rollback(self):
        prefix=self.managed/'application';prefix.mkdir(parents=True);(prefix/'keep').write_text('legacy')
        self.install();self.assertTrue(prefix.is_symlink())
        legacy=list((self.managed/'updates').glob('legacy-*'));self.assertEqual(len(legacy),1);self.assertEqual((legacy[0]/'keep').read_text(),'legacy')
        with self.assertRaises(ValueError):u.rollback(self.managed,verifier=self.verify)
    def test_cache_tampering_still_passes_through_verifier(self):
        self.install();first=(self.managed/'application').resolve().parent;self.install()
        (first/'archive.tar.gz').write_bytes(b'tampered')
        def reject(archive,*args):
            self.assertEqual(archive.read_bytes(),b'tampered');raise ValueError('signature rejected')
        before=(self.managed/'application').resolve()
        with self.assertRaises(ValueError):u.rollback(self.managed,verifier=reject)
        self.assertEqual((self.managed/'application').resolve(),before)
    def test_concurrent_update_is_refused(self):
        with u.locked(self.managed):
            with self.assertRaises(BlockingIOError):self.install()
        self.assertFalse((self.managed/'application').exists())
    def test_invalid_pin_and_symlink_input_rejected(self):
        with self.assertRaises(ValueError):u.install(self.archive,self.bundle,'main',self.managed,verifier=self.verify)
        alias=self.root/'alias';alias.symlink_to(self.archive)
        with self.assertRaises(OSError):u.install(alias,self.bundle,SHA,self.managed,verifier=self.verify)
        self.assertEqual(self.verified,0)

if __name__=='__main__':unittest.main()
