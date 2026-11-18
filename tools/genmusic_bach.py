#!/usr/bin/env python3
# genmusic_bach.py — transcribe "Sheep May Safely Graze" (Bach, BWV 208,
# "Schafe koennen sicher weiden") into the game's step-sequencer arrays.
#
# Source: Mutopia Project, typeset from Breitkopf & Haertel 1881 —
# PUBLIC DOMAIN (ftp/BachJS/BWV208/Sheep/Sheep.ly, Mutopia-2011/10/25-1794).
# The four voices below are copied verbatim from that file (LilyPond
# \relative mode, German note names: b = Bb, h = Bnat, bes/es/as/aes =
# flats, fis/cis = sharps). This script parses the relative notation,
# slices the Da Capo A section (bars 1-21: ritornello + first vocal
# strain + ritornello return + fermata), and emits three 16th-note-grid
# arrays: LEAD = soprano (Pales) with Flute I filling her silences,
# MID = Flute II, BASS = the continuo's throbbing eighths.
import re, sys

FLUTE1 = r"""
f8 f16( d) f8 f16( d) f8( g) g( bes)
| es,8 es16( c) es8 es16( c) es8( a) a( c)
| bes8~ bes32( c d16) bes( d) c( f,) bes8~ bes32( c d16) bes( d) c( f,)
| bes8 d32( es f16) d( bes) c( a) bes8 d32( es f16) d( bes) c( a)
| bes4 r r2
| r1
| r1
| r1
| f8 f16( d) f8 f16( d) f8( g) g( bes)
| g g16( es) g8 g16( es) g8( a) a( c)
| r8 bes32( c d16) bes( d) c( f,) bes8 r8 r4
| r8 bes32( c d16) bes( d) c( f,) bes8 r8 r4
| r1
| r2 f8 f16( d) f8 f16( d)
| f8( g) g( bes) es,8 es16( c) es8 es16( c)
| a'8( bes) bes( c) r8 d32( es f16) d( bes) f( a)
| bes8 f16( d) f8 f16( d) f8( g) g( bes)
| es,8 es16( c) es8 es16( c) es8( a) a( c)
| bes8~ bes32( c d16) bes( d) c( f,) bes8~ bes32( c d16) bes( d) c( f,)
| bes8 d32( es f16) d( bes) c( a) bes8 d32( es f16) d( bes) c( a)
| bes4 r4 r2
| r1
| r1
| r1
| r1
| r2 g8 g16( es) g8 g16( es)
| g8( as) as( c) f,8 f16( d) f8 f16( d)
| f8( b) b( d) c~ c32( d es16) c( es) d( g,)
| c8~ c32( d es16) c( es) d( g,) c8 es32( f g16) es( c) d( b)
| c8 es32( f g16) es( c) d( b) c4 r4
| r1
| r2 a8 a16( f) a8 a16( f)
| a8( bes) bes( d) g,8 g16( e) g8 g16( e)
| g8( cis) cis( e) a,8 a16( fis) a8 a16( fis)
| a8( bes) bes( d) g,8 g16( e) g8 g16( e)
| g8( a) a( c) c,8 c16( a) c8 c16( a)
| c8( d) d( bes) d8 d16( b) d8 d16( b)
| d8( e) e( g) g8 g16( e) a8 a16( f)
| bes8( c) bes( e) r2
| r1
"""

FLUTE2 = r"""
d8 d16( bes) d8 d16( bes) d8( es) es( g)
| c,8 c16( a) c8 c16( a) c8( es) c( es)
| d8~ d32( es f16) d( f) es( c) d8~ d32( es f16) d( f) es( c)
| d8 bes'32( c d16) bes( d,) es( f) d8 bes'32( c d16) bes( d,) es( c)
| d4 r r2
| r1
| r1
| r1
| d8 d16( bes) d8 d16( bes) d8( es) es( g)
| es es16( c) es8 es16( c) es8( f) f( a)
| r8 d,32( es f16) d( f) es( c) d8 r8 r4
| r8 d32( es f16) d( f) es( c) d8 r8 r4
| r1
| r2 d8 d16( bes) d8 d16( bes)
| d8( es) es( g) c,8 c16( a) c8 c16( a)
| c8( d) d( es) r8 bes'32( c d16) bes( d,) es( c)
| d8 d16( bes) d8 d16( bes) d8( es) es( g)
| c,8 c16( a) c8 c16( a) c8( es) c( es)
| d8~ d32( es f16) d( f) es( c) d8~ d32( es f16) d( f) es( c)
| d8 bes'32( c d16) bes( d,) es( f) d8 bes'32( c d16) bes( d,) es( c)
| d4 r4 r2
| r1
| r1
| r1
| r1
| r2 es8 es16( c) es8 es16( c)
| es8( f) f( as) d,8 d16( b) d8 d16( b)
| d8( f) d( f) es~ es32( f g16) es( g) f( d)
| es8~ es32( f g16) es( g) f( d) es8 c'32( d es16) c( es,) f( g)
| es8 c'32( d es16) c( es,) f( g) es4 r4
| r1
| r2 f8 f16( d) f8 f16( d)
| f8( g) g( bes) bes,8 bes16( g) bes8 bes16( g)
| e'8( g) e( g) es8 es16( c) fis8 fis16( d)
| fis8( g) g( bes) c8 c16( g) e8 e16( c)
| e8( f) f( a) a,8 a16( f) a8 a16( f)
| a8( bes) bes( d) b8 b16( g) b8 b16( g)
| b8( c) c( e) e8 e16( c) f8 f16( d)
| e8( g) e( g) r2
| r1
"""

PALES = r"""
r1
| r1
| r1
| r1
| bes4 d8( c) c4. d8
| es4 g8( f) d4 c8( bes)
| d( bes) a( g) c4. d8
| a4 g8( f) f2
| f4 aes8( g) g4. a8
| bes4 d8( c) a4 g8( f)
| f'4 g8( f) f( d) c( bes)
| f'4 g8( f) f( d) c( bes)
| es4. f8 d( c) bes( c)
| a4. bes8 bes2
| r2 g'8( es) d( c)
| f4. g8 d4. c16 bes
| bes4 r4 r2
| r1
| r1
| r1
| d4 g fis4. g8
| es4 d bes a8 g
| c4 d8( b) c4. d8
| es4 f8( d) es4 g8( f)
| g4 as b, c8( d)
| es4 d8( c) c2
| r1
| r1
| r1
| r2 g4 bes8( a)
| bes4. d8 c4 bes8( a16 bes)
| a4 g8 f f'4 e8 d
| d4. f8 e f16 g e8 d
| cis4 b8( a) c2~
| c4 bes8( c16 a) bes2~
| bes8( a) a4 es'2~
| es4 d8( f16 e) f2~
| f8( e) d( e) e4 d8( c)
| bes4 a8( bes16 g) c([ bes a bes c bes)] c d
| a4 g8 f f2
"""

CONTINUO = r"""
bes8( bes bes bes) bes( bes bes bes)
| bes( bes bes bes) bes( bes bes bes)
| bes'( bes a a) g( g f f)
| es( es f f) g f16 es f8 f,
| bes( bes bes' bes) bes( bes bes bes)
| a( a a a) bes( bes bes bes)
| bes,( bes bes bes) bes( bes a bes)
| c( c c c) f( f es es)
| d( d bes bes) es( es d d)
| c( c c c) f( f es es)
| d( d es es) bes( bes c c)
| d( d es es) bes( bes d d)
| c( c f f) bes, c d es
| f( f f, f) bes( bes bes bes)
| bes( bes bes bes) bes( bes a a)
| bes( bes es es) f( f f, f)
| bes( bes bes bes) bes( bes bes bes)
| bes( bes bes bes) bes( bes bes bes)
| bes'( bes a a) g( g f f)
| es( es f f) g f16 es f8 f,
| bes8( bes bes' bes) a( a bes bes)
| c( c d d) g,( g f f)
| es( es d d) es( es b b)
| c( c b b) c( c d d)
| es( es f f) f( f es f)
| g( g g, g) c( c c c)
| c( c c c) c( c c c)
| c( c c c) c'( c bes bes)
| as( as g g) f( f g g)
| as g16 f g8 g, c( c c c)
| d( d d d) e( e e e)
| f( f e e) d( d c c)
| bes( bes a a) g( g g g)
| a( a g g) fis( fis d d)
| g( g f f) e( e c c)
| f( f f f) f( f f f)
| bes( bes bes bes) g( g g g)
| c( c c c) bes( bes a a)
| g( g g g) a( a bes bes)
| c c c, c f2
"""

# German/LilyPond note names -> (diatonic step, semitone adjustment)
# b is B-flat, h is B-natural (German convention used by this source).
NAMES = {
    'c': (0, 0), 'd': (1, 0), 'e': (2, 0), 'f': (3, 0), 'g': (4, 0),
    'a': (5, 0), 'b': (6, -1), 'h': (6, 0),
    'es': (2, -1), 'as': (5, -1), 'aes': (5, -1), 'bes': (6, -1),
    'fes': (3, -1),
    'cis': (0, 1), 'dis': (1, 1), 'eis': (2, 1), 'fis': (3, 1),
    'gis': (4, 1), 'ais': (5, 1), 'his': (6, 1),
    'ces': (0, -1), 'des': (1, -1), 'ges': (4, -1),
}
DUR32 = {1: 32, 2: 16, 4: 8, 8: 4, 16: 2, 32: 1, 64: 0}

def parse_voice(text, ref_pos):
    """\relative parse. ref_pos: diatonic position of the reference pitch
    (octave*7 + step, C4 = 4*7+0 = 28). Returns [(start32, dur32, midi)]."""
    events = []
    t = 0
    last_pos = ref_pos          # diatonic position of previous note
    last_dur = 8                # default: quarter
    tie_next = False
    for raw in text.replace('|', ' ').split():
        tok = raw.split('\\')[0]                    # drop \commands suffixes
        tok = tok.strip('()[]_^')
        if not tok:
            continue
        m = re.match(r"^([a-z]+|r)([',]*)([0-9]*)(\.*)(~?)$", tok)
        if not m:
            sys.exit('unparsed token: %r' % raw)
        name, octs, durnum, dots, tie = m.groups()
        if durnum:
            last_dur = DUR32[int(durnum)]
            d = last_dur
            add = d // 2
            for _ in dots:
                d += add; add //= 2
        else:
            d = last_dur
        if name == 'r':
            t += d
            tie_next = False
            continue
        if name not in NAMES:
            sys.exit('unknown note name: %r' % name)
        step, semi = NAMES[name]
        # relative: pick the octave within a fourth of the previous note
        base = (last_pos // 7) * 7 + step
        while base - last_pos > 3: base -= 7
        while base - last_pos < -3: base += 7
        base += 7 * (octs.count("'") - octs.count(','))
        # midi from the diatonic position + the accidental (b/h special:
        # German b is B-flat by definition, h is B-natural)
        NAT = {0: 0, 1: 2, 2: 4, 3: 5, 4: 7, 5: 9, 6: 11}
        midi = (base // 7 + 1) * 12 + NAT[step] + semi
        if name == 'b': midi = (base // 7 + 1) * 12 + 10
        if name == 'h': midi = (base // 7 + 1) * 12 + 11
        if tie_next and events and events[-1][2] == midi:
            s0, d0, m0 = events[-1]
            events[-1] = (s0, d0 + d, m0)
        else:
            events.append((t, d, midi))
        last_pos = base
        tie_next = bool(tie)
        t += d
    return events, t

def grid(events, total32, step32=2):
    """Onset grid: value at each step = midi of the event starting there."""
    n = total32 // step32
    g = [-1] * n
    for (s, d, m) in events:
        if s % step32 == 0 and s // step32 < n:
            g[s // step32] = m
    return g

def emit(name, g):
    out = ['static const signed char %s[%d] = {' % (name, len(g))]
    for i in range(0, len(g), 16):
        out.append('    ' + ''.join('%5d,' % v for v in g[i:i+16]))
    out.append('};')
    return '\n'.join(out)

REF_TREBLE = 5 * 7 + 0      # \relative c'' -> C5
REF_BASS = 3 * 7 + 0        # \relative c   -> C3
SLICE = 21 * 32             # bars 1-21 (the Da Capo A section)

f1, t1 = parse_voice(FLUTE1, REF_TREBLE)
f2, t2 = parse_voice(FLUTE2, REF_TREBLE)
pa, tp = parse_voice(PALES, REF_TREBLE)
co, tc = parse_voice(CONTINUO, REF_BASS)
for nm, tt in (('flute1', t1), ('flute2', t2), ('pales', tp), ('continuo', tc)):
    assert tt == 40 * 32, (nm, tt)
    print(nm, 'total 32nds:', tt, 'OK', file=sys.stderr)

lead = grid(pa, SLICE)
f1g = grid(f1, SLICE)
lead = [a if a >= 0 else b for a, b in zip(lead, f1g)]   # soprano first, flute fills
mid = grid(f2, SLICE)
bass = grid(co, SLICE)

# v1.23 rehash — a continuo REALIZATION of the bass: where the
# throbbing pulse repeats a pitch on the next eighth, bounce the
# repeat to the octave (the score's own bes' gestures, generalised).
# Rhythm, counts and the flute/soprano lines are untouched.
for bar in range(21):
    prev = -1
    for k in range(8):
        s = bar * 16 + k * 2
        v = bass[s]
        if v >= 0:
            if v == prev: bass[s] = v + 12
            prev = v

NAMES_OUT = {v: k for k, v in
             [(60, 'C4'), (62, 'D4'), (64, 'E4'), (65, 'F4'), (67, 'G4'), (69, 'A4'), (70, 'Bb4'),
              (72, 'C5'), (74, 'D5'), (75, 'Eb5'), (77, 'F5'), (79, 'G5'), (81, 'A5'), (82, 'Bb5'), (84, 'C6')]}
for bar in range(4):
    seg = lead[bar*16:(bar+1)*16]
    print('LEAD bar', bar+1, [NAMES_OUT.get(v, v) if v >= 0 else '-' for v in seg], file=sys.stderr)
print('BASS bar1', bass[:16], file=sys.stderr)
print('// Sheep May Safely Graze — BWV 208 (Bach, public domain);')
print('// transcribed from the Mutopia/Breitkopf & Haertel 1881 score')
print('// by tools/genmusic_bach.py. 336 steps = 21 bars of 16ths.')
print(emit('M4_BASS', bass))
print(emit('M4_MID', mid))
print(emit('M4_LEAD', lead))
