#!/bin/sh
# Every Double Rainbow sprite from its Gemini source (art/gemini/), the same way each time:
#   sh tools/art/make_sprites.sh       -> art/sprites/*.png (packed into the firmware by tools/art/gen_dr_art.py)
set -e
cd "$(dirname "$0")/../.."
python3 tools/art/procchar.py art/gemini/vesper_b.jpg vesper 112 100 70 4
python3 tools/art/procchar.py art/gemini/shard_d.jpg shard 108 110 90 4    # the faint light in his glass (dr.c sweeps it)
python3 tools/art/fix_squall.py                                                      # glowing eyes, a yellow bolt
python3 tools/art/procchar.py art/gemini/squall_c_fix.png squall 96 116 90 0 \
    --keep=255,236,150 --keep=250,204,56 --keep=206,140,26                         # the glow and the bolt's yellows
python3 tools/art/procchar.py art/gemini/wick_a.jpg wick 104 90 90 4
python3 tools/art/procchar.py art/gemini/tock_a.jpg tock 100 110 90 4
python3 tools/art/fix_mote.py                                                        # the marble a jar stopper
python3 tools/art/procchar.py art/gemini/mote_a_fix.png mote 100 110 90 4
python3 tools/art/procchar.py art/gemini/murk_a.jpg murk 104 96 90 4
python3 tools/art/procchar.py art/gemini/bog_a.jpg bog 92 110 90 4
python3 tools/art/procchar.py art/gemini/hush_d.jpg hush 104 96 90 4      # her own wing tip in the ink pot, no pen
python3 tools/art/procchar.py art/gemini/tumbler_a.jpg tumbler 104 80 90 4
python3 tools/art/procchar.py art/gemini/glint_a.jpg glint 104 100 90 4
python3 tools/art/procchar.py art/gemini/hermit_a.jpg hermit 100 110 90 4
