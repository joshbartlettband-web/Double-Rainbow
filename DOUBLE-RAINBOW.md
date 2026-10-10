# Double Rainbow

Double Rainbow is a firmware for the M-VAVE FM-1, built on [Melodee](https://github.com/keremimo/melodee). It turns the
FM-1 into a game-like instrument for ages 10 to 14 and up, and for grown-ups who like the same things they do. Think of the
taste of a good YA novel: a little dark, a little funny, never babyish.

It is the older sibling of [Rainbow mode](https://github.com/joshbartlettband-web/felucca-rainbow), the toy we made for a
four-year-old on Felucca. Rainbow mode was bright and almost wordless. Double Rainbow is eerie and cool without being
grim. It uses more text, and its sound is much richer, because Melodee has more engines and more effects.

This file is the plan. Nothing in it is built yet. Items marked **(open)** still need a decision.

## The core idea

- **Fun first.** Every feature has to be fun to touch before it teaches anything.
- **Learn the instrument by playing it.** Each character is built on one of Melodee's engines. Each one teaches you how
  that engine makes sound.
- **A path into the full synth.** Double Rainbow is the front door. Melodee, with all of its depth, is one button combo
  away.

## Theme: Rainbow after dark

A rainbow at night is real. It is called a moonbow. Double Rainbow lives in that world: night skies, aurora, glowing
deep-sea creatures, lantern light and a slightly spooky night carnival.

- Palette: deep indigo, plum, ember orange and bioluminescent teal, with warm accents.
- Pixel art in the Rainbow style: vector first, snapped to pixels, dark outlines. All characters are new drawings.
- The tone is eerie and cool, never grim. Nothing gory, nothing that would scare a ten-year-old for real.

## Starting up and getting to Melodee

- The FM-1 starts in Double Rainbow every time it powers on.
- A button combo takes you to full Melodee, as HOME and SAVE held for 3 seconds did in Rainbow mode. Double Rainbow
  never writes Melodee's autosave, so playing in Double Rainbow cannot overwrite your own work.
- **(open)** Which combo, and whether Double Rainbow should remember a choice to start in Melodee instead.

## The cast

Characters with attitude, each built on one engine. The engine families work like factions, so getting to know a
character means getting to know an engine.

| Engine | Its characters feel | What they teach |
| --- | --- | --- |
| PROPHET | big, warm and dramatic | two oscillators, a four-pole filter, envelopes, poly-mod (the 1978 Prophet-5) |
| FM6 | glassy, metallic, bell-like | frequency modulation: operators, ratios and algorithms (the DX7) |
| CZ-1 | strange and digital | phase distortion with eight-step envelopes (Casio's CZ) |
| PHASE | sharp and shifting | phase distortion in a simpler form |
| LOFI | 8-bit gremlins | pulse waves, noise and wave RAM, like old game consoles |
| VOICE | singers and choirs | formants: how vowels are made |
| NOISE | weather, machines and ghosts | noise colors, crackle and metallic tones |
| DRUM | the rhythm section | the TR-808's circuits |

Each character has:

- **A name and one line of personality.**
- **A codex entry.** Short readable text on how its engine works, with one real piece of history or one thing to try.
- **Four macro knobs with names** (BITE, BREATH, HAUNT, GRIT and so on). Each one moves several real parameters at once.
  Holding a button shows what is underneath, for example "BITE = cutoff + resonance + filter envelope amount".
- **Its own sound,** a sky and a favorite beat.

Watch for accidental look-alikes of known characters (a lesson from Rainbow mode). Change colors and features until each
one reads as its own thing.

## Ways to play (the spellbook modes)

Turned on ALGORITHM, as Rainbow mode's modes were. The names are spells. Each mode's codex entry says what musicians
call it.

| Mode | What it does | Grew from |
| --- | --- | --- |
| **SOLO** **(open name)** | Each key plays its note. | Rainbow's 1 FRIEND |
| **WEAVE** | Strum. The black keys pick a chord and the white keys strum its notes, like an Omnichord. Real chord names, sevenths and borrowed chords. | Rainbow's STRUM (u/veecheech) |
| **CAST** | Chords under a tune. The black keys hold chords and the white keys play a melody. Its codex entry explains that jazz players call this comping. Roman numerals ("this one is borrowed from the minor key, that's why it sounds bittersweet"). | Rainbow's ACCORDION |
| **SCRY** | Ear training. Hear it, then name it: notes, intervals, major or minor, chord types. Levels get harder. | Rainbow's GUESS (u/nutty_cartoon) |
| **TRACE** | Learn a song. The next key lights and you play at your own pace. | Rainbow's FOLLOW (u/nutty_cartoon) |

Every mode shows the notes and chord names you play, as Melodee's HOME screen already does.

## Songs for TRACE

The melody of a current pop song is copyrighted on its own, separately from any recording. Shipping one in a public
firmware could get the repository taken down. So there are two routes.

**1. Public-domain tunes people will recognize,** each with a codex note on where you have heard it. Each one is to be
checked before it ships: we use the old melody, never a modern arrangement.

- Korobeiniki (the Tetris theme, a Russian folk song)
- Wellerman (an 1800s sea shanty, viral in 2021)
- Dies Irae (a medieval chant that film composers use for doom)
- In the Hall of the Mountain King (Grieg)
- Danse Macabre (Saint-Saëns)
- Toccata in D minor (attributed to Bach)
- Night on Bald Mountain (Mussorgsky)
- Funeral March of a Marionette (Gounod; Alfred Hitchcock's TV theme)
- Mars, from The Planets (Holst; the ancestor of movie villain music)
- Carol of the Bells (Leontovych)
- Moonlight Sonata and Für Elise (Beethoven)
- Gymnopédie No. 1 (Satie; it sounds like lofi)
- Clair de lune (Debussy)
- Greensleeves and House of the Rising Sun (traditional melodies)

**2. Bring your own songs.** Load any melody for your own use from the web editor or a MIDI file. We never ship it, and
it never leaves your FM-1. That is how people get today's songs into TRACE.

**3. Our own riffs.** Short riffs we write in a genre's style (a synthwave arpeggio, a metal riff, lofi chords). They
teach the genre and belong to us.

## Effects: the pedalboard

Melodee's effects shown as a row of pedals, each with one or two knobs and plain labels: distortion, the SLICER (a gate
and stutter), chorus, reverb (room or spring), and the FX layer's repeat, reverse, filter sweep, tape stop, freeze and an
octave harmonizer with shimmer.

- Melodee 0.12 removed the shared delay to free memory for recording. An echo has to come from repeat, or we spend memory
  putting a small delay back. **(open)**
- **(open)** A mockup at the FM-1's real 240 x 240 size comes first, for Josh to see before anything is built.

## More to build

- **Sound Lab.** Start from any character, change it, name it on the keys and keep it as your own. Melodee's native slots
  (128 Prophet, 64 FM6, 128 CZ-1) do most of the saving already.
- **Sound Match.** Hear a target sound and turn the knobs to match it, with a meter that shows how close you are. Scored on
  the parameter values, so it is cheap to run.
- **Strange Scales.** Melodee's 70 tunings (maqam, just intonation, historical temperaments, Bohlen-Pierce), each with a
  short card on where it comes from.
- **Loop builder.** A simple front end to Melodee's four-track sequencer: record a loop, then add bass, chords and drums.
- **Codex and unlocks.** Characters, skies, scales and songs are found by playing, and the codex fills in as you go.

## Sounds

- **Melodee's factory patches stay,** including Sequential's 200 Prophet-5 programs. We will revisit them down the road.
- Each character gets its own program, tuned to it and to its macro knobs.
- Level every character by measured loudness, not by peak (Rainbow mode's lesson; spread under 4 dB).

## Limits to plan around

- **Space.** Melodee's app slot is 540,604 bytes, and Melodee already uses most of it. Measure before planning features.
  Art and text will run out first: compress pictures (run-length) and keep strings short. Sequential's factory bank takes
  about 31 KB and is the first thing to cut if we run short.
- **Voices.** 16 shared units. A PROPHET voice costs 3 units and an FM6 voice 1. Chords on PROPHET plus a bass and drums
  will run short, so chord modes have to plan for it.
- **Retired engines.** PHYS, TRIO and WHEEL are silent in Melodee. Don't plan sounds on them.
- **Saved settings.** Audit every spare byte in Melodee's settings before storing anything, and validate it on load so
  old installs never break.
- **USB name.** The installer has to recognize Double Rainbow's USB MIDI port name, or it will report "FM-1 not found".
- **No Bluetooth.** Only M-VAVE's own firmware has it.

## Setup, in order

1. **This plan.** (done)
2. **Automatic build.** (done) `.github/workflows/double-rainbow.yml` builds the firmware on every push to `main` as
   `vX.Y.Z-dr`, ported from Rainbow mode's `rainbow.yml`.
3. **Web installer.** (done) https://joshbartlettband-web.github.io/Double-Rainbow/ is Melodee's installer and web
   editor. An unchanged Melodee 0.13.1 build installed and ran on Josh's FM-1.
4. **Browser version.** (done) `web/emu/`, Felucca's emulator ported to Melodee: the firmware's own code compiled to
   WebAssembly, at `/webapp/try/` on the site. `web/emu/emu_test.mjs` runs on every push (boot, keys, MIDI, PLAY,
   saves that survive a reload, and the same audio as a native build, bit for bit). A four-track song costs about 2 %
   of real time on a desktop.
5. **Measure space.** (done) Melodee 0.13.1 is 425,468 B of the 540,604 B slot: **115,136 B (about 112 KB) free.**
   Every build's summary page shows the current numbers.
6. **First features,** talked through before building.

## Credits and license

Double Rainbow is GPL-3.0-only, like Melodee and Felucca. Credit it everywhere as: Double Rainbow, built on Melodee by
Kerem Kilic (Ellic Studio), a modified version of Felucca by Leo Kuroshita, Hügelton Instruments.

Ideas from the r/MVaveFM1 community carried over from Rainbow mode: u/veecheech (strum), u/nutty_cartoon (the ear game,
learning songs, chord names), u/theskyisfalling1 (note lengths on the staff), u/lastapoc (the song writer). New ideas get
their author's username here, on the website and on the installer page.
