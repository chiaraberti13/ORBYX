#!/usr/bin/env python3
"""Real CLI composition and validation on explicitly synthetic fixtures."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile

fixture, binary = map(lambda value: str(Path(value).resolve()), sys.argv[1:3])
with tempfile.TemporaryDirectory() as name:
    root = Path(name)
    subprocess.run([fixture, '--fixture', str(root/'fixture')], check=True)
    channels = root/'fixture/input/MSUGS_VIS1'
    flags = [item for i in (1,2,3) for item in (f'--ch{i}', str(channels/f'MSUGS-VIS-{i}.png'))]
    for command in ('natural-color', 'compose-rgb'):
        output = root/f'{command}.png'
        result = subprocess.run([binary, command, *flags, '--output', output.name, '--lang', 'it'], cwd=root, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        payload = output.read_bytes()
        assert payload[:8] == b'\x89PNG\r\n\x1a\n' and payload[24:26] == b'\x08\x02'
        before = hashlib.sha256(payload).hexdigest()
        result = subprocess.run([binary, command, *flags, '--output', output.name], cwd=root, capture_output=True)
        assert result.returncode == 2
        assert hashlib.sha256(output.read_bytes()).hexdigest() == before
    for extra in (['--ch1', 'duplicate'], ['--cpu'], ['--config', 'unexpected.json']):
        result = subprocess.run([binary, 'natural-color', *flags, '--output', str(root/'invalid.png'), *extra], capture_output=True)
        assert result.returncode == 2 and not (root/'invalid.png').exists()
    result = subprocess.run([binary, 'natural-color', '--ch1', 'x', '--output', 'unused.png', '--lang', 'it'], capture_output=True, text=True)
    assert result.returncode == 2 and 'Valore mancante per --ch2' in result.stderr
print('Composition CLI, relative output, no-overwrite and strict options: OK (synthetic input)')
