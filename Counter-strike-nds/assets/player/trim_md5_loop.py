#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Removes the frames at the end of a looping MD5 animation that are equal to
# the first frame. In Blender the last key of a loop is usually a copy of the
# first one; in the game the animation goes from the last frame back to frame
# 0, so that copy makes the animation stop for one frame at every loop.
#
# Usage: python3 trim_md5_loop.py input.md5anim output.md5anim

import re
import sys

def frame_values(block):
    return [float(v) for v in block.split()]

def main(src, dst):
    text = open(src).read()
    frames = re.findall(r'frame (\d+) \{([^}]*)\}', text)
    first = frame_values(frames[0][1])

    keep = len(frames)
    while keep > 2:
        last = frame_values(frames[keep - 1][1])
        if max(abs(a - b) for a, b in zip(first, last)) > 1e-4:
            break
        keep -= 1

    if keep == len(frames):
        open(dst, 'w').write(text)
        print(f'{src}: {len(frames)} frames, nothing to remove')
        return

    # Remove the frames, their bounds lines, and fix numFrames
    for number, _ in frames[keep:]:
        text = re.sub(r'frame %s \{[^}]*\}\s*' % number, '', text)
    bounds = re.search(r'bounds \{\n(.*?)\n\}', text, re.S)
    lines = bounds.group(1).split('\n')[:keep]
    text = text[:bounds.start(1)] + '\n'.join(lines) + text[bounds.end(1):]
    text = re.sub(r'numFrames \d+', 'numFrames %d' % keep, text)

    open(dst, 'w').write(text)
    print(f'{src}: {len(frames)} -> {keep} frames')

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
