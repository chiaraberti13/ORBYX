#!/usr/bin/env python3
"""Check genuine decoded/aligned RDAS products against their native-pixel affine.

Requires cv2, numpy and cbor2. This checks execution and relative image
registration; the acquisition has no ground-truth geolocation/calibration.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import cv2
import numpy as np
import cbor2


def digest(path):
    checksum = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024*1024), b''):
            checksum.update(chunk)
    return checksum.hexdigest()


def check(decoded, aligned, profile):
    if not cv2.__version__.startswith('4.6.'):
        raise RuntimeError('This pinned reference requires OpenCV 4.6; do not compare interpolation across major versions')
    coefficients = json.loads(profile.read_text())
    report = {'opencv_reference_version': cv2.__version__, 'geolocation_verified': False,
              'scope': 'Native-resolution affine application and unmodified source metadata; no ground-truth geometry.',
              'profile_sha256': digest(profile), 'products': []}
    for vis in (1, 2):
        source = decoded / f'MSUGS_VIS{vis}'
        output = aligned / f'MSUGS_VIS-{vis}_Aligned'
        meta = cbor2.loads((source/'product.cbor').read_bytes())
        assert meta['instrument'] == 'msugs_vis' and meta['type'] == 'image'
        assert [x['file'] for x in meta['images']] == [f'MSUGS-VIS-{i}.png' for i in (1,2,3)]
        assert all(x['transform']['type'] == 1 and x['bit_depth'] == 10 for x in meta['images']), 'Unknown metadata: review transforms before copying CBOR'
        assert (source/'product.cbor').read_bytes() == (output/'product.cbor').read_bytes()
        reference = cv2.imread(str(source/'MSUGS-VIS-1.png'), cv2.IMREAD_UNCHANGED)
        assert reference.shape == (17200,6004) and reference.dtype == np.uint16
        cropped = reference[:-3606]
        for channel in (1,2,3):
            filename = f'MSUGS-VIS-{channel}.png'
            before = digest(source/filename)
            original = cv2.imread(str(source/filename), cv2.IMREAD_UNCHANGED)
            actual = cv2.imread(str(output/filename), cv2.IMREAD_UNCHANGED)
            assert original.shape == reference.shape and original.dtype == np.uint16
            assert actual.shape == (13594,6004) and actual.dtype == np.uint16
            if channel == 1:
                expected = cropped
            else:
                m = coefficients[f'L4VIS{vis}'][f'CH{channel}']
                matrix = np.array([[m['a'],m['b'],m['tx']],[m['c'],m['d'],m['ty']]], dtype=np.float32)
                expected = cv2.warpAffine(original, matrix, (6004,17200), flags=cv2.INTER_LINEAR|cv2.WARP_INVERSE_MAP)[:-3606]
            # Exact codec/affine reconstruction at original resolution. Compare
            # in strips to avoid an additional full-frame signed allocation.
            maximum = 0
            for y in range(0,actual.shape[0],128):
                delta = actual[y:y+128].astype(np.int32)-expected[y:y+128].astype(np.int32)
                maximum = max(maximum, int(np.abs(delta).max()))
            assert maximum <= 2, (vis,channel,maximum)
            assert digest(source/filename) == before, 'Source changed during verification'
            report['products'].append({'vis':vis,'channel':channel,'input_sha256':before,
                'output_sha256':digest(output/filename),'shape':list(actual.shape),
                'depth':16,'max_affine_reference_difference':maximum,'cbor_transform_type':1})
            del original, actual, expected
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--decoded', required=True, type=Path)
    parser.add_argument('--aligned', required=True, type=Path)
    parser.add_argument('--profile', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    result = check(args.decoded,args.aligned,args.profile)
    with args.report.open('x') as stream:
        json.dump(result,stream,indent=2)
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
