"""Scratch generator for the CBA toolbar icon (32x32, 3 states).
Not part of the build - run once, output baked into CbaIcon.h, then kept
alongside for whoever adjusts this next.

2026-09-13, round 5: live in Nexus's own Quick Access row next to filled
icons (shield, lion, sword), the round-4 three-CONCENTRIC-thin-rings design
read as too dark / didn't visually integrate - not a wrong color, a wrong
shape. A 2px stroke covers a tiny fraction of the 32x32 canvas next to solid
native icons, so even exact-ground-truth colors look faint by comparison.
Emi's fix: two OFFSET rings instead of three concentric ones - "Oo", the same
idea as Nexus's own overlapping "XX" mark, just circles instead of X's -
plus a thicker stroke for more visual weight, and a lighter Inactive tone
(0.45x read as "super dunkel", moved to 0.65x).

Colors are unchanged ground truth, re-verified this round by sampling actual
pixels out of Emi's local Nexus checkout (not just trusting the round-4
docstring): Nexus.png/Generic.png brightest opaque pixels average to the
same Normal tone, Nexus_Hover.png/Generic_Hover.png to the same Hover tone.
  - Normal (resting): muted warm tan/khaki, ~(218, 214, 171).
  - Hover: near-white/cream, ~(250, 250, 244) - every native icon brightens
    the same way on hover regardless of what it does.
"""
from PIL import Image, ImageDraw

SIZE = 32
SS = 8  # supersample factor for anti-aliasing
BIG = SIZE * SS

# Nexus's own RES_ICON_NEXUS / RES_ICON_GENERIC ground truth (re-sampled
# 2026-09-13 from Nexus.png, Nexus_Hover.png, Generic.png, Generic_Hover.png).
NORMAL_RGB = (218, 214, 171)
HOVER_RGB = (250, 250, 244)
# Inactive has no native-family equivalent (static shortcuts don't have an
# off state) - dimmed version of the Normal tone, CBA's own addition.
# 0.45x read as "super dunkel, etwas zu dunkel" live in the QA bar - lifted
# to 0.65x, still clearly dimmer than Active but no longer looks broken/off.
INACTIVE_DIM = 0.65
INACTIVE_RGB = tuple(int(c * INACTIVE_DIM) for c in NORMAL_RGB)

# "Oo" - a big ring and a small ring, offset like two overlapping letters,
# not concentric. Mirrors Nexus's own "XX" mark (two overlapping shapes)
# with circles instead of X's.
BIG_C, BIG_R = (12.5, 13.0), 10.0
SMALL_C, SMALL_R = (20.5, 19.5), 6.5
STROKE = 3.25  # up from 2.0 - more visual weight next to filled native icons


def render(base_rgb, alpha, name):
    img = Image.new("RGBA", (BIG, BIG), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    r, g, b = base_rgb
    stroke_px = STROKE * SS
    for (cx, cy), radius in ((BIG_C, BIG_R), (SMALL_C, SMALL_R)):
        rad_px = radius * SS
        bbox = [
            cx * SS - rad_px, cy * SS - rad_px,
            cx * SS + rad_px, cy * SS + rad_px,
        ]
        draw.ellipse(bbox, outline=(r, g, b, alpha), width=int(stroke_px))
    img = img.resize((SIZE, SIZE), Image.LANCZOS)
    img.save(name)
    print(f"wrote {name} base_rgb={base_rgb} alpha={alpha}")


if __name__ == "__main__":
    render(NORMAL_RGB, 255, "icon_active.png")
    render(INACTIVE_RGB, 235, "icon_inactive.png")
    render(HOVER_RGB, 255, "icon_hover.png")
