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
python3 tools/art/procchar.py art/gemini/hush_e.jpg hush 104 96 90 4      # the Listener: a scope locket (dr.c draws the Line in it)
python3 tools/art/procchar.py art/gemini/tumbler_a.jpg tumbler 104 80 90 4
python3 tools/art/procchar.py art/gemini/glint_a.jpg glint 104 100 90 4
python3 tools/art/procchar.py art/gemini/hermit_a.jpg hermit 100 110 90 4
python3 tools/art/procchar.py art/gemini/burrow_a.jpg burrow 104 100 90 4 \
    --keep=248,176,72 --keep=255,226,150                                          # his lantern's flame
python3 tools/art/procchar.py art/gemini/pip_a.jpg pip 100 100 90 4
python3 tools/art/procchar.py art/gemini/echo_a.jpg echo 104 96 90 4
python3 tools/art/procchar.py art/gemini/fork_a.jpg fork 104 90 90 4
python3 tools/art/procchar.py art/gemini/ants_b.jpg ants 96 110 90 4            # three ants (ants_a had seven: too small)
python3 tools/art/procchar.py art/gemini/rabbit_a.jpg rabbit 104 90 90 4
python3 tools/art/procchar.py art/gemini/top_a.jpg top 104 80 90 4
python3 tools/art/procchar.py art/gemini/cairn_a.jpg cairn 100 90 90 4
python3 tools/art/procchar.py art/gemini/stray_a.jpg stray 104 70 90 4
python3 tools/art/procchar.py art/gemini/tuppence_a.jpg tuppence 100 100 90 4
python3 tools/art/procchar.py art/gemini/gloam_a.jpg gloam 104 80 90 4
python3 tools/art/procchar.py art/gemini/tinker_a.jpg tinker 104 90 90 4
python3 tools/art/clear_fringe.py stray tuppence                              # the last of the magenta, in the gaps
