"""Exercise the actual async desktop colour worker and compare CLI bytes."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

fixture, gui, cli = map(lambda value: str(Path(value).resolve()), sys.argv[1:4])
with tempfile.TemporaryDirectory(prefix='orbyx-gui-colour-') as name:
    root=Path(name)
    env=dict(os.environ, QT_QPA_PLATFORM='offscreen', XDG_CONFIG_HOME=str(root/'settings'))
    subprocess.run([fixture,'--fixture',str(root/'fixture')],check=True,env=env,timeout=20)
    channels=root/'fixture/input/MSUGS_VIS1'
    subprocess.run([cli,'natural-color',*[arg for i in (1,2,3) for arg in (f'--ch{i}',str(channels/f'MSUGS-VIS-{i}.png'))],
                    '--output',str(root/'cli.png')],check=True,env=env,timeout=30)
    for language in ('it','en'):
        output=root/f'gui-{language}.png'
        command=[gui,'--input',str(channels),'--output',str(output),'--natural-color','--lang',language,'--run-and-exit']
        subprocess.run(command,check=True,env=env,timeout=30)
        assert output.read_bytes()==(root/'cli.png').read_bytes()
        failed=subprocess.run(command,env=env,timeout=30)
        assert failed.returncode==2
        assert output.read_bytes()==(root/'cli.png').read_bytes()
print('GUI asynchronous Natural Color EN/IT matches CLI; overwrite refused')
