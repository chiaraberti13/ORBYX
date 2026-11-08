#!/usr/bin/env python3
"""Validate Natural Color against an independent scalar coloursys reference.

Checks rendering, mapping and retained depth on actual aligned image samples.
It does not establish spectral calibration or scientific true colour.
"""
import argparse
import colorsys
import hashlib
import json
from pathlib import Path
import cv2
import numpy as np


def digest(path):
    value=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):
            value.update(block)
    return value.hexdigest()


def check(channels, image):
    samples=[]
    hashes=[]
    shape=None
    dtype=None
    for channel in (1,2,3):
        path=channels/f'MSUGS-VIS-{channel}.png'
        before=digest(path)
        frame=cv2.imread(str(path),cv2.IMREAD_UNCHANGED)
        if frame is None or frame.ndim!=2 or frame.dtype not in (np.uint8,np.uint16):
            raise ValueError('Expected unsigned grayscale 8/16-bit input')
        if shape is None:shape,dtype=frame.shape,frame.dtype
        if frame.shape!=shape or frame.dtype!=dtype:raise ValueError('Input mismatch')
        samples.append(frame[::97,::67].copy().ravel())
        del frame
        if digest(path)!=before:raise ValueError('Input changed during validation')
        hashes.append(before)
    rendered=cv2.imread(str(image),cv2.IMREAD_UNCHANGED)
    if rendered is None or rendered.shape!=(*shape,3) or rendered.dtype!=dtype:
        raise ValueError('Natural Color output changed dimensions or depth')
    actual=rendered[::97,::67].copy().reshape(-1,3)
    del rendered
    maximum=np.iinfo(dtype).max
    error=0.0
    checked=0
    boundary=0
    changed=0
    for bgr,pixel in zip(zip(*samples),actual):
        rgb=[float(x)/maximum for x in bgr[::-1]]
        h,s,v=colorsys.rgb_to_hsv(*rgb)
        degrees=h*360
        # The discontinuous rule boundaries are sensitive to float32 hue
        # rounding. Report them separately; do not hide errors elsewhere.
        if s>0 and min(abs(degrees-x) for x in (40,100,340))<0.001:
            boundary+=1
            continue
        if degrees>=340 or degrees<=40:degrees+=60
        elif degrees<=100:degrees-=6
        corrected=colorsys.hsv_to_rgb((degrees%360)/360,s,v)
        expected=[(0.6*a+0.4*b)*maximum for a,b in zip(rgb,corrected)][::-1]
        delta=max(abs(float(a)-b) for a,b in zip(pixel,expected))
        error=max(error,delta)
        checked+=1
        changed+=any(abs(float(a)-float(b))>2 for a,b in zip(pixel,bgr))
    if checked==0 or error>2:raise ValueError(f'Independent colour reference failed: {checked=} {error=}')
    return {'mapping':'CH3->R CH2->G CH1->B','shape':[*shape,3], 'depth':dtype.itemsize*8,
            'sampled_pixels_checked':checked,'boundary_pixels_reported_separately':boundary,
            'max_scalar_reference_error_levels':error,'sampled_pixels_changed_from_rgb321':changed,
            'input_sha256':hashes,'output_sha256':digest(image),
            'scientific_true_colour_verified':False}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--channels',required=True,type=Path)
    parser.add_argument('--image',required=True,type=Path)
    parser.add_argument('--report',required=True,type=Path)
    args=parser.parse_args()
    result=check(args.channels,args.image)
    with args.report.open('x') as stream:json.dump(result,stream,indent=2)
    print(json.dumps(result,indent=2))


if __name__=='__main__':main()
