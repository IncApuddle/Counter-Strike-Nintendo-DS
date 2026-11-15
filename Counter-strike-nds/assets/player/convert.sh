#!/bin/sh
# Converts the player model and animations (MD5, exported from Blender with
# Scale 0.01) to the DSM/DSA files used by the game (data/gign_*.bin).
# Run it from this folder in the MSYS2 terminal: sh convert.sh
set -e
MD5_TO_DSMA=${MD5_TO_DSMA:-$(find /opt/blocksds -name md5_to_dsma.py | head -n 1)}

# Looping animations: remove the copy of the first frame at the end
mkdir -p tmp
python3 trim_md5_loop.py idle.md5anim tmp/idle.md5anim
python3 trim_md5_loop.py run.md5anim tmp/run.md5anim
python3 trim_md5_loop.py fall.md5anim tmp/fall.md5anim
cp jump.md5anim tmp/jump.md5anim # Played once, keep all the frames

python3 "$MD5_TO_DSMA" \
    --model gign.md5mesh \
    --name gign \
    --output ../../data \
    --texture 128 128 \
    --anim tmp/idle.md5anim tmp/run.md5anim tmp/jump.md5anim tmp/fall.md5anim \
    --bin \
    --blender-fix

# Make the signs of the quaternions match between frames and animations
# (otherwise the mesh can look flat for a moment, see fix_dsa_signs.py)
python3 fix_dsa_signs.py ../../data/gign_idle_dsa.bin ../../data/gign_run_dsa.bin \
    ../../data/gign_jump_dsa.bin ../../data/gign_fall_dsa.bin

rm -rf tmp
