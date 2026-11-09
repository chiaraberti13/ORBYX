# SPDX-License-Identifier: GPL-3.0-only
import importlib.util
import json
from pathlib import Path
import unittest
spec=importlib.util.spec_from_file_location('advisories',Path(__file__).resolve().parents[1]/'scripts/orbyx_advisories.py');a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)

class AdvisoryTests(unittest.TestCase):
    def inventory(self):return {'schema':'orbyx-sbom/1','binaries':[{'libraries':[{'soname':'libexample.so','package':'example','version':'1:2.0-1ubuntu1'}]}]}
    def test_source_versions_and_epoch_preserved(self):
        seen=[]
        def resolve(p,v):seen.append((p,v));return 'example-source',v
        queries,unknown=a.queries_for(self.inventory(),'Ubuntu:24.04:LTS',resolve)
        self.assertEqual(seen,[('example','1:2.0-1ubuntu1')]);self.assertFalse(unknown);self.assertEqual(queries[0]['version'],'1:2.0-1ubuntu1');self.assertEqual(queries[0]['package']['name'],'example-source')
    def test_pagination_is_not_lost(self):
        calls=[]
        def fetch(payload):
            calls.append(payload)
            return {'results':[{'vulns':[{'id':'UBUNTU-CVE-test-2'}]}]} if 'page_token' in payload['queries'][0] else {'results':[{'vulns':[{'id':'UBUNTU-CVE-test-1'}],'next_page_token':'next'}]}
        report=a.scan(self.inventory(),'Ubuntu:24.04:LTS',resolver=lambda p,v:('example',v),fetch=fetch)
        self.assertEqual(len(report['packages'][0]['vulnerabilities']),2);self.assertEqual(a.replay(report),report)
        report['evidence'][0]['response']['results'][0]['vulns']=[]
        with self.assertRaises(ValueError):a.replay(report)
    def test_incomplete_or_unknown_is_never_clear(self):
        report=a.scan({'schema':'orbyx-sbom/1','binaries':[{'libraries':[{'soname':'unknown'},*self.inventory()['binaries'][0]['libraries']]}]},'Ubuntu:24.04:LTS',resolver=lambda p,v:('example',v),fetch=lambda p:{'results':[{}]})
        self.assertEqual(report['coverage'],'incomplete');self.assertTrue(report['unknown'])
        with self.assertRaises(ValueError):a.query_all([{'package':{'name':'x'}}],fetch=lambda p:{'results':[]})
    def test_repeat_token_rejected(self):
        with self.assertRaises(ValueError):a.query_all([{}],fetch=lambda p:{'results':[{'next_page_token':'same'}]})
    def test_vendor_fixture_vulnerable_and_fixed_versions(self):
        fixture=json.loads((Path(__file__).parent/'fixtures/osv-ubuntu-php8.3.json').read_text())
        matches,_=a.query_all(fixture['request']['queries'],fetch=lambda request: fixture['response'] if request==fixture['request'] else {})
        self.assertIn('UBUNTU-CVE-2025-6491',[v['id'] for v in matches[0]])
        self.assertNotIn('UBUNTU-CVE-2025-6491',[v['id'] for v in matches[1]])

if __name__=='__main__':unittest.main()
