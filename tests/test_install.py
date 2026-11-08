"""Installer control-flow tests with explicitly stubbed package/build commands."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

REPO=Path(__file__).resolve().parents[1]

@unittest.skipUnless(sys.platform.startswith('linux'), 'Ubuntu installer')
class InstallTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.root=Path(self.temp.name)
        self.repo=self.root/'checkout with spaces'
        (self.repo/'scripts/lib').mkdir(parents=True)
        for name in ('install.sh','start.sh','orbyx.sh','scripts/lib/session.sh'):
            shutil.copyfile(REPO/name,self.repo/name)
        self.tools=self.root/'tools';self.tools.mkdir()
        self.home=self.root/'home';self.home.mkdir()
        self.trace=self.root/'trace'
        common='''#!/bin/bash
printf '%s %s\\n' "$(basename "$0")" "$*" >> "$ORBYX_TEST_TRACE"
case "$(basename "$0")" in
 sudo) if [[ "$1" == -v ]]; then exit 0;fi;exec "$@" ;;
 apt-get)
  if [[ "$*" == *update* && ! -f "$ORBYX_TEST_TRACE.locked" ]]; then
   touch "$ORBYX_TEST_TRACE.locked";echo 'Could not get lock: unattended-upgr' >&2;exit 100
  fi ;;
 sleep) exit 0 ;;
 ctest) [[ "$ORBYX_TEST_FAIL" != test ]] || exit 42 ;;
 cmake)
  if [[ "$1" == -S ]]; then mkdir -p "$4";fi
  if [[ "$1" == --build && "$ORBYX_TEST_FAIL" == build ]]; then echo 'synthetic build failure on stdout';exit 23;fi
  if [[ "$1" == --install ]]; then
   target="$XDG_DATA_HOME/orbyx/application/bin";mkdir -p "$target"
   printf '#!/bin/bash\\necho ORBYX-stub\\n' > "$target/orbyx"
   printf '#!/bin/bash\\necho GUI-stub\\n' > "$target/orbyx-gui"
   chmod +x "$target/"*
  fi ;;
esac
exit 0
'''
        for name in ('sudo','apt-get','sleep','cmake','ctest','ninja','c++'):
            p=self.tools/name;p.write_text(common);p.chmod(0o755)
        self.env=dict(os.environ,HOME=str(self.home),XDG_DATA_HOME=str(self.home/'data'),
            PATH=str(self.tools)+os.pathsep+os.environ['PATH'],ORBYX_TEST_TRACE=str(self.trace),ORBYX_TEST_FAIL='')
        self.preexec=None
        if os.geteuid()==0:
            uid=65534;gid=65534
            self.root.chmod(0o755)
            try:
                for p in [self.root,*self.root.rglob('*')]:os.chown(p,uid,gid)
            except OSError:
                self.skipTest('Runtime root senza UID secondario mappato; verifica non-root eseguita in CI')
            def demote():os.setgid(gid);os.setuid(uid)
            self.preexec=demote
    def tearDown(self):self.temp.cleanup()
    def run_script(self,name,*args):
        return subprocess.run(['bash',str(self.repo/name),*args],cwd=self.home,env=self.env,
            capture_output=True,text=True,preexec_fn=self.preexec,timeout=30)
    def test_wait_install_and_launch_without_paths(self):
        p=self.run_script('install.sh');self.assertEqual(p.returncode,0,p.stdout+p.stderr)
        trace=self.trace.read_text()
        self.assertEqual(trace.count('apt-get -o DPkg::Lock::Timeout=30 -o Acquire::Retries=3 update'),2)
        for package in ('cmake','ninja-build','libopencv-dev','libjsoncpp-dev','qt6-base-dev','libgl-dev','gnupg'):
            self.assertIn(package,trace)
        self.assertTrue((self.repo/'build-desktop/.orbyx-install-success').exists())
        self.assertEqual(self.run_script('start.sh','--smoke-test').returncode,0)
        self.assertEqual(self.run_script('orbyx.sh','--help').returncode,0)
        self.assertTrue(list((self.repo/'logs').glob('install-*.log')))
    def test_failure_stops_before_test_install_and_records_stdout(self):
        self.env['ORBYX_TEST_FAIL']='build'
        p=self.run_script('install.sh','--no-deps');self.assertEqual(p.returncode,23,p.stdout+p.stderr)
        trace=self.trace.read_text();self.assertNotIn('ctest',trace);self.assertNotIn('--install',trace)
        errors=''.join(p.read_text() for p in (self.repo/'logs').glob('*.errors.log'))
        self.assertIn('synthetic build failure on stdout',errors)
        self.assertFalse((self.repo/'build-desktop/.orbyx-install-success').exists())
        self.assertEqual(self.run_script('start.sh').returncode,2)
    def test_failed_tests_do_not_install(self):
        self.env['ORBYX_TEST_FAIL']='test'
        p=self.run_script('install.sh','--no-deps');self.assertEqual(p.returncode,42,p.stdout+p.stderr)
        self.assertNotIn('--install',self.trace.read_text())
    def test_log_symlink_is_rejected(self):
        (self.repo/'logs').symlink_to(self.home,target_is_directory=True)
        p=self.run_script('start.sh');self.assertNotEqual(p.returncode,0)
        self.assertFalse(list(self.home.glob('desktop-*.log')))

if __name__=='__main__':unittest.main()
