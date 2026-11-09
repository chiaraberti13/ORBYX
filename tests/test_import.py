# SPDX-License-Identifier: GPL-3.0-only
import importlib.util
from pathlib import Path
import hashlib
import shutil
import struct
import tempfile
import json
import subprocess
import sys
import os
import unittest

ROOT=Path(__file__).resolve().parents[1]
def module(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'scripts'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
imp=module('orbyx_import');sim=module('orbyx_simulate')
# Deterministic CBOR encoder for independently constructed test metadata.
def cbor(value):
    def head(kind,n):
        return bytes([kind*32+n]) if n<24 else bytes([kind*32+24,n]) if n<256 else bytes([kind*32+25])+struct.pack('>H',n)
    if type(value) is int:return head(0,value)
    if isinstance(value,str):
        b=value.encode();return head(3,len(b))+b
    if isinstance(value,list):return head(4,len(value))+b''.join(cbor(x) for x in value)
    if isinstance(value,dict):return head(5,len(value))+b''.join(cbor(k)+cbor(v) for k,v in value.items())
    raise ValueError()
def metadata(sat='L4'):
    return {'type':'image','instrument':'msugs_vis','satellite':sat,'images':[{'file':f'MSUGS-VIS-{c}.png','name':str(c),'bit_depth':8} for c in (1,2,3)]}

class ImportTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.parent=Path(self.temp.name);self.product=self.parent/'acquisition'
        sim.build_dataset(self.product,16,3620,4,1)
        for sensor in (1,2):(self.product/f'MSUGS_VIS{sensor}/product.cbor').write_bytes(cbor(metadata()))
    def test_ready_and_read_only(self):
        before={str(p):hashlib.sha256(p.read_bytes()).digest() for p in self.product.rglob('*') if p.is_file()}
        result=imp.discover(self.parent,0)
        self.assertEqual(result[0]['status'],'ready');self.assertEqual(result[0]['satellite'],'L4')
        self.assertEqual(before,{str(p):hashlib.sha256(p.read_bytes()).digest() for p in self.product.rglob('*') if p.is_file()})
    def test_same_size_concurrent_write_is_withheld(self):
        path=self.product/'MSUGS_VIS1/product.cbor'
        def writer(_):path.write_bytes(path.read_bytes().replace(b"L4",b"L3"))
        self.assertEqual(imp.discover(self.parent,0,sleep=writer)[0]['status'],'unstable')
    def test_conflicting_satellites_rejected(self):
        (self.product/'MSUGS_VIS2/product.cbor').write_bytes(cbor(metadata('L3')))
        self.assertEqual(imp.discover(self.parent,0)[0]['status'],'rejected')
    def test_duplicate_channel_rejected(self):
        m=metadata();m['images'][1]=m['images'][0]
        (self.product/'MSUGS_VIS1/product.cbor').write_bytes(cbor(m))
        self.assertEqual(imp.discover(self.parent,0)[0]['status'],'rejected')
    def test_real_satdump_preview_is_not_native_import(self):
        product=ROOT/'assets/sample-elektro-l4'
        with self.assertRaisesRegex(ValueError,'Preview|depth'):imp.inspect_product(product)
        real=imp.decode_cbor((product/'MSUGS_VIS1/product.cbor').read_bytes())
        self.assertEqual(real['instrument'],'msugs_vis');self.assertEqual(len(real['images']),3)
    def test_cbor_truncation_duplicates_and_nesting_rejected(self):
        data=cbor(metadata())
        for end in range(len(data)):
            with self.assertRaises((ValueError,UnicodeError)):imp.decode_cbor(data[:end])
        for bad in (b'\xa2\x61a\x01\x61a\x02',b'\x81'*30+b'\x00',b'\x9f\xff',b'\x00\x00'):
            with self.assertRaises(ValueError):imp.decode_cbor(bad)
    def test_missing_and_symlink_not_imported(self):
        p=self.product/'MSUGS_VIS1/MSUGS-VIS-1.png';p.unlink();p.symlink_to(self.product/'MSUGS_VIS2/MSUGS-VIS-1.png')
        self.assertTrue(all(p['status']!='ready' for p in imp.discover(self.parent,0)))
    def test_incomplete_product_is_visible_but_withheld(self):
        (self.product/'MSUGS_VIS1/product.cbor').unlink()
        report=imp.discover(self.parent,0)
        self.assertEqual(len(report),1);self.assertEqual(report[0]['status'],'incomplete')
    def test_truncated_png_is_rejected(self):
        p=self.product/'MSUGS_VIS1/MSUGS-VIS-1.png';p.write_bytes(p.read_bytes()[:-12])
        self.assertEqual(imp.discover(self.parent,0)[0]['status'],'rejected')
    def test_batch_rechecks_import_fingerprint_before_start(self):
        fingerprint=imp.inspect_product(self.product)['fingerprint']
        for vis in (1,2):
            path=self.product/f'MSUGS_VIS{vis}/product.cbor';path.write_bytes(path.read_bytes().replace(b'L4',b'L3'))
        profile=self.parent/'profile.json';profile.write_text('{}')
        engine=self.parent/'engine';marker=self.parent/'called'
        engine.write_text('#!'+sys.executable+'\nfrom pathlib import Path\nPath('+repr(str(marker))+').touch()\n');engine.chmod(0o700)
        manifest=self.parent/'batch.json';manifest.write_text(json.dumps({'jobs':[{'id':'imported','input':str(self.product),'config':str(profile),'satellite':'L4','output':str(self.parent/'output'),'acquisition_fingerprint':fingerprint}]}))
        result=subprocess.run([sys.executable,str(ROOT/'scripts/orbyx_batch.py'),str(manifest),'--orbyx',str(engine)],capture_output=True,text=True)
        self.assertEqual(result.returncode,1,result.stdout+result.stderr);self.assertIn('changed after import',result.stdout+result.stderr);self.assertFalse(marker.exists())
    def test_nonfinite_settle_rejected(self):
        with self.assertRaises(ValueError):imp.discover(self.parent,float('nan'))

if __name__=='__main__':unittest.main()
