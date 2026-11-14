"""Exercise the actual async desktop colour worker and compare it to the CLI.

The desktop export presents the image North-up (MSU-GS acquires South-up), so the
GUI output must equal the CLI Natural Color rotated 180°. The comparison is on
decoded pixels, not raw bytes, because the GUI re-encodes the rotated image."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from PIL import Image

Image.MAX_IMAGE_PIXELS=None
fixture, gui, cli = map(lambda value: str(Path(value).resolve()), sys.argv[1:4])

def pixels(path, rotate180=False):
    image=Image.open(path).convert('RGB')
    if rotate180: image=image.rotate(180)
    return image.tobytes()

with tempfile.TemporaryDirectory(prefix='orbyx-gui-colour-') as name:
    root=Path(name)
    env=dict(os.environ, QT_QPA_PLATFORM='offscreen', XDG_CONFIG_HOME=str(root/'settings'))
    subprocess.run([fixture,'--fixture',str(root/'fixture')],check=True,env=env,timeout=20)
    channels=root/'fixture/input/MSUGS_VIS1'
    subprocess.run([cli,'natural-color',*[arg for i in (1,2,3) for arg in (f'--ch{i}',str(channels/f'MSUGS-VIS-{i}.png'))],
                    '--output',str(root/'cli.png')],check=True,env=env,timeout=30)
    expected=pixels(root/'cli.png', rotate180=True)
    for language in ('it','en'):
        output=root/f'gui-{language}.png'
        command=[gui,'--input',str(channels),'--output',str(output),'--natural-color','--lang',language,'--run-and-exit']
        subprocess.run(command,check=True,env=env,timeout=30)
        assert pixels(output)==expected, f'GUI {language} output is not the CLI image rotated 180°'
        failed=subprocess.run(command,env=env,timeout=30)
        assert failed.returncode==2
        assert pixels(output)==expected
print('GUI asynchronous Natural Color EN/IT matches CLI rotated 180° (North-up); overwrite refused')
