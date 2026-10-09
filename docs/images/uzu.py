#!/usr/bin/env python3
"""Draws docs/images/uzu.svg, the uzu icon.

Uzu (渦) is a whirlpool: a factor graph's nodes wind inward along it, cooling to warm as they near
the eye, where the optimization lands. Dashed chords are loop closures between neighbouring turns.

    python3 docs/images/uzu.py               # rewrites docs/images/uzu.svg
    python3 docs/images/uzu.py other.svg     # or writes elsewhere
"""
import math
import os
import sys

W = 256
C = W / 2
TURNS = 2.4            # how far the whirlpool winds
NODES = 14             # graph nodes along it, outermost first
R_OUT, R_IN = 100.0, 0.0

def raw(t):
    """t in [0, 1]: 0 the outer rim, 1 the centre. Tightens towards the eye, like water does."""
    r = R_IN + (R_OUT - R_IN) * (1 - t) ** 1.25
    a = -math.pi / 2 - t * TURNS * 2 * math.pi
    return r * math.cos(a), r * math.sin(a)

# A spiral is not symmetric about its eye, so centre the drawing on its bounding box instead.
_xs, _ys = zip(*(raw(k / 2000) for k in range(2001)))
DX = C - (min(_xs) + max(_xs)) / 2
DY = C - (min(_ys) + max(_ys)) / 2

def spiral(t):
    x, y = raw(t)
    return x + DX, y + DY

def lerp(a, b, t):
    return a + (b - a) * t

def mix(c0, c1, t):
    return "#%02x%02x%02x" % tuple(round(lerp(x, y, t)) for x, y in zip(c0, c1))

COOL = (0x5e, 0xd4, 0xe6)   # outer nodes: far from the answer
WARM = (0xff, 0xb0, 0x3b)   # the eye: the answer

# Sample the curve finely, then place the nodes at equal arc length along it, stopping a little
# short of the eye so the last edge runs into the answer rather than the nodes piling up there.
FINE = 4000
samples = [spiral(k / FINE) for k in range(FINE + 1)]
arc = [0.0]
for (x0, y0), (x1, y1) in zip(samples, samples[1:]):
    arc.append(arc[-1] + math.hypot(x1 - x0, y1 - y0))
end = arc[-1]
STOP = 0.93
def t_at(fraction):
    target = fraction * end
    k = next(i for i, s in enumerate(arc) if s >= target)
    return k / FINE

node_t = [t_at(STOP * i / (NODES - 1)) for i in range(NODES)]

def path(t0, t1, steps=16):
    pts = [spiral(t0 + (t1 - t0) * k / steps) for k in range(steps + 1)]
    return " ".join(("M" if i == 0 else "L") + f"{x:.2f},{y:.2f}" for i, (x, y) in enumerate(pts))

out = []
o = out.append
o(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {W}" width="{W}" height="{W}" role="img" aria-labelledby="t">')
o('<title id="t">uzu: a factor graph winding into a whirlpool, solved at its eye</title>')
o('<defs>')
o('<linearGradient id="bg" x1="0" y1="0" x2="1" y2="1">'
  '<stop offset="0" stop-color="#1a2a48"/><stop offset="1" stop-color="#0b1424"/></linearGradient>')
o('<radialGradient id="glow" cx="0.5" cy="0.5" r="0.5">'
  '<stop offset="0" stop-color="#ffb03b" stop-opacity="0.6"/>'
  '<stop offset="1" stop-color="#ffb03b" stop-opacity="0"/></radialGradient>')
o('</defs>')
o(f'<rect x="2" y="2" width="{W-4}" height="{W-4}" rx="54" fill="url(#bg)" stroke="#5ed4e6" stroke-opacity="0.22" stroke-width="2"/>')

# The water: a broad, faint band along the whole spiral.
o(f'<path d="{path(0.0, 1.0, 180)}" fill="none" stroke="#5ed4e6" stroke-opacity="0.10" stroke-width="16" stroke-linecap="round" stroke-linejoin="round"/>')

# Loop closures: factors between nodes on neighbouring turns.
for i in range(0, NODES - 3, 3):
    ti = node_t[i]
    # the node nearest one full turn further in
    j = min(range(i + 1, NODES), key=lambda k: abs(node_t[k] - (ti + 1 / TURNS)))
    (x0, y0), (x1, y1) = spiral(node_t[i]), spiral(node_t[j])
    o(f'<line x1="{x0:.2f}" y1="{y0:.2f}" x2="{x1:.2f}" y2="{y1:.2f}" stroke="#a9bde0" '
      f'stroke-opacity="0.38" stroke-width="2" stroke-dasharray="3 4" stroke-linecap="round"/>')

# The chain: each edge is the stretch of spiral between two consecutive nodes, then on to the eye.
stops = node_t + [1.0]
for i in range(len(stops) - 1):
    t0, t1 = stops[i], stops[i + 1]
    col = mix(COOL, WARM, ((t0 + t1) / 2) ** 0.8)
    o(f'<path d="{path(t0, t1)}" fill="none" stroke="{col}" stroke-width="{lerp(4.6, 3.0, t0):.2f}" stroke-linecap="round"/>')

# The answer's glow, under the nodes.
xc, yc = spiral(1.0)
o(f'<circle cx="{xc:.2f}" cy="{yc:.2f}" r="30" fill="url(#glow)"/>')

# Nodes, outer first; they shrink and warm as they wind in.
for t in node_t:
    x, y = spiral(t)
    r = lerp(9.0, 4.6, t)
    o(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="{r:.2f}" fill="#0f1b31" stroke="{mix(COOL, WARM, t ** 0.8)}" stroke-width="3.2"/>')

# The eye: where the optimization lands.
o(f'<circle cx="{xc:.2f}" cy="{yc:.2f}" r="10" fill="#ffb03b"/>')
o(f'<circle cx="{xc:.2f}" cy="{yc:.2f}" r="3.8" fill="#fff4dd"/>')
o('</svg>')

target = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "uzu.svg")
with open(target, "w") as f:
    f.write("\n".join(out) + "\n")
