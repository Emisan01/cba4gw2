"""Scratch generator for the CBA toolbar icon (32x32, 3 states).
Not part of the build - run once, output baked into CbaIcon.h, then kept
alongside for whoever adjusts this next.

2026-09-10, round 4: Emi asked to fully integrate into the Nexus icon
family ("wir integrieren uns voll in die nexus icon familie") - pulled exact
pixel colors from Emi's local Nexus source checkout
(C:/Users/Emi/Desktop/Nexus), sampling RES_ICON_NEXUS's actual baked PNGs
(src/Resources/Images/QuickAccess/Nexus.png / Nexus_Hover.png) instead of
estimating off a screenshot. Two ground-truth tones, confirmed identical in
Generic.png/Generic_Hover.png too so it's a real family convention, not a
one-off:
  - Normal (resting): a muted warm tan/khaki, ~(218, 214, 171).
  - Hover: jumps to near-white/cream, ~(247, 247, 238) - every native icon
    brightens the same way on hover regardless of what it does.
CBA has its own extra state native icons don't need (on/off), so Inactive
stays a dimmed version of the Normal tone - but Hover is now the exact same
near-white for both Active and Inactive, and (see ModuleMain.cpp /
MainWindow.cpp) now actually fires on hover in the Active state too, which
it didn't before.
"""
from PIL import Image, ImageDraw

SIZE = 32
SS = 8  # supersample factor for anti-aliasing
BIG = SIZE * SS

# Nexus's own RES_ICON_NEXUS ground truth (sampled, not estimated).
NORMAL_RGB = (218, 214, 171)
HOVER_RGB = (247, 247, 238)
# Inactive has no native-family equivalent (static shortcuts don't have an
# off state) - dimmed version of the Normal tone, CBA's own addition.
INACTIVE_RGB = tuple(int(c * 0.45) for c in NORMAL_RGB)

CX, CY = SIZE * 0.5, SIZE * 0.46
RADII = [13.5, 9.5, 5.5]              # outer -> inner, ~4px steps
RING_BRIGHTNESS = [1.00, 0.85, 0.70]  # slight falloff toward center for depth
STROKE = 2.0


def render(base_rgb, alpha, name):
    img = Image.new("RGBA", (BIG, BIG), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    for radius, ring_b in zip(RADII, RING_BRIGHTNESS):
        r, g, b = [int(c * ring_b) for c in base_rgb]
        rad_px = radius * SS
        stroke_px = STROKE * SS
        bbox = [
            CX * SS - rad_px, CY * SS - rad_px,
            CX * SS + rad_px, CY * SS + rad_px,
        ]
        draw.ellipse(bbox, outline=(r, g, b, alpha), width=int(stroke_px))
    img = img.resize((SIZE, SIZE), Image.LANCZOS)
    img.save(name)
    print(f"wrote {name} base_rgb={base_rgb} alpha={alpha}")


if __name__ == "__main__":
    render(NORMAL_RGB, 255, "icon_active.png")
    render(INACTIVE_RGB, 235, "icon_inactive.png")
    render(HOVER_RGB, 255, "icon_hover.png")
