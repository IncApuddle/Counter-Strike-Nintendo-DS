#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Fixes the signs of the quaternions in DSA animation files.
#
# A rotation can be written as quaternion q or -q. MD5 files always store
# w <= 0, so the sign of a joint can flip between two frames even if the
# rotation barely changes. Nitro Engine interpolates quaternions linearly
# (between frames and when blending two animations) without checking the
# sign: between q and almost -q the result goes near 0 and the joint is
# squashed for a moment (the mesh looks flat).
#
# This script chooses, for every joint of every frame of every file, the sign
# that is closest to the same joint in the first frame of the first file. All
# the animations then use the same "side", so interpolation and blending work.
#
# Usage: python3 fix_dsa_signs.py idle_dsa.bin run_dsa.bin ...
# (the files are modified in place, the first one is the reference)

import struct
import sys

def load(path):
    data = bytearray(open(path, 'rb').read())
    version, frames, joints = struct.unpack_from('<III', data, 0)
    if version != 1:
        raise SystemExit(f'{path}: unknown DSA version {version}')
    return data, frames, joints

def orient_offset(frame, joint, joints):
    # Header (12 bytes), then per joint: pos[3] + orient[4] (int32)
    return 12 + (frame * joints + joint) * 28 + 12

def main(paths):
    reference = None
    for path in paths:
        data, frames, joints = load(path)
        if reference is None:
            reference = [struct.unpack_from('<4i', data, orient_offset(0, j, joints)) for j in range(joints)]
        elif joints != len(reference):
            raise SystemExit(f'{path}: {joints} joints, the first file has {len(reference)}')

        flipped = 0
        for f in range(frames):
            for j in range(joints):
                off = orient_offset(f, j, joints)
                q = struct.unpack_from('<4i', data, off)
                dot = sum(a * b for a, b in zip(q, reference[j]))
                if dot < 0:
                    struct.pack_into('<4i', data, off, *(-v for v in q))
                    flipped += 1

        open(path, 'wb').write(data)
        print(f'{path}: {frames} frames, {flipped} joint orientations flipped')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit(__doc__ if False else 'Usage: fix_dsa_signs.py first_dsa.bin [other_dsa.bin ...]')
    main(sys.argv[1:])
