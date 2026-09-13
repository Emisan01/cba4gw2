"""Scratch generator for the CBA toolbar icon (32x32, 3 states).
Not part of the build - run once, output baked into CbaIcon.h, then kept
alongside for whoever adjusts this next.

2026-09-13, round 6: rounds 4 (three thin concentric rings) and 5 (offset
"Oo", tilted ellipses, a full trefoil-knot weave) were all tried live and
rejected - either too dark/thin next to Nexus's filled native icons, or too
elaborate to read at 32px. Emi sketched a rough 3-ring idea by hand instead
("ich will mich nicht zu sehr verkuenstlen") and iterated on renders from
there: three rings, each with its own slightly off-center placement (a loose,
hand-drawn feel rather than a mechanical bullseye), stepped from bright
(outer) to dark (inner) - "hat was von einer Linse sowie auch von einem
Tunnel". Final adjustment: keep the outer ring's radius/stroke exactly as
established (matches the other Nexus icons' footprint) and pull the inner
two rings in further with a slightly thinner stroke, so the gaps between
bands read clearly and the offset/depth effect is a bit stronger.

Colors are the same ground truth as rounds 4/5 (sampled from Emi's local
Nexus checkout, C:/Users/Emi/Desktop/Nexus/src/Resources/Images/QuickAccess/
{Nexus,Nexus_Hover,Generic,Generic_Hover}.png):
  - Normal (resting): muted warm tan/khaki, ~(222, 218, 176).
  - Hover: near-white/cream, ~(252, 252, 246).
  - Inactive: 0.66x of Normal (round 4's 0.45x read as "super dunkel").
"""
from PIL import Image, ImageDraw

SIZE = 32
SS = 12  # supersample factor for anti-aliasing
BIG = SIZE * SS

NORMAL_RGB = (222, 218, 176)
HOVER_RGB = (252, 252, 246)
INACTIVE_DIM = 0.66
INACTIVE_RGB = tuple(int(c * INACTIVE_DIM) for c in NORMAL_RGB)
INACTIVE_ALPHA = 235

# Three rings, each with its own slightly off-center placement (not a
# mechanical bullseye) and its own brightness step (outer brightest, inner
# darkest - the "tunnel" read). (cx, cy, radius, stroke, brightness).
RINGS = [
    (16.3 * SS, 15.3 * SS, 12.5 * SS, 3.6 * SS, 1.00),  # outer - footprint anchor, unchanged since round 4
    (15.6 * SS, 16.6 * SS,  8.2 * SS, 3.0 * SS, 0.85),  # middle - pulled in, slightly thinner
    (16.7 * SS, 16.8 * SS,  4.2 * SS, 3.0 * SS, 0.68),  # inner - pulled in, slightly thinner
]


def clamp(rgb):
    return tuple(max(0, min(255, int(v))) for v in rgb)


def render(base_rgb, alpha, name):
    img = Image.new("RGBA", (BIG, BIG), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    for cx, cy, r, stroke, brightness in RINGS:
        col = clamp(tuple(c * brightness for c in base_rgb))
        draw.ellipse([cx - r, cy - r, cx + r, cy + r], outline=col + (alpha,), width=int(stroke))
    img = img.resize((SIZE, SIZE), Image.LANCZOS)
    img.save(name)
    print(f"wrote {name} base_rgb={base_rgb} alpha={alpha}")


if __name__ == "__main__":
    render(NORMAL_RGB, 255, "icon_active.png")
    render(INACTIVE_RGB, INACTIVE_ALPHA, "icon_inactive.png")
    render(HOVER_RGB, 255, "icon_hover.png")
