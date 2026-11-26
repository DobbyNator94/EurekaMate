#!/usr/bin/env python3
"""Draw approximate mockups of the EurekaMate AMOLED screens (280x456) -> docs/img/ui/display_*.png.
Not a pixel-exact LVGL render; layout follows src/ui/screens/grind_sync_ui.cpp."""
import math, os, shutil, subprocess
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "docs", "img", "ui"); os.makedirs(OUT, exist_ok=True)
ACC = "#FF3B30"; BLUE = "#0A84FF"; OK = "#30D158"

def arc(cx, cy, r, a0, a1):
    p0 = (cx + r * math.cos(math.radians(a0)), cy + r * math.sin(math.radians(a0)))
    p1 = (cx + r * math.cos(math.radians(a1)), cy + r * math.sin(math.radians(a1)))
    large = 1 if (a1 - a0) % 360 > 180 else 0
    return f"M{p0[0]:.1f},{p0[1]:.1f} A{r},{r} 0 {large} 1 {p1[0]:.1f},{p1[1]:.1f}"

def frame(body, name):
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="320" height="500" font-family="DejaVu Sans, Arial">
<rect x="0" y="0" width="320" height="500" rx="34" fill="#1b1b1f"/>
<rect x="20" y="22" width="280" height="456" rx="22" fill="#000"/>
<g transform="translate(20,22)">{body}</g></svg>'''
    p = os.path.join(OUT, f"display_{name}.svg"); open(p, "w").write(svg)
    if shutil.which("rsvg-convert"):
        subprocess.run(["rsvg-convert", "-z", "2", "-o", p[:-4] + ".png", p], check=True)
        os.remove(p)

def t(x, y, s, size=14, fill="#fff", anchor="middle", w="normal"):
    return f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" text-anchor="{anchor}" font-weight="{w}">{s}</text>'

# Splash
frame(f'''<path d="{arc(140,170,85,0,359.9)}" stroke="#26262C" stroke-width="10" fill="none"/>
<path d="{arc(140,170,85,-90,170)}" stroke="{ACC}" stroke-width="10" fill="none" stroke-linecap="round"/>
{t(140,160,"Eureka",34,w="bold")}{t(140,200,"Mate",34,w="bold")}
{t(140,300,"Grind-by-Sync for GaggiMate",14,ACC)}
{t(140,390,"based on smart-grind-by-weight",11,"#8A8A94")}{t(140,406,"by jaapp  -  GPLv3",11,"#8A8A94")}''', "splash")

# Grind Sync page
recipes = ["Single", "Double", "Custom", "Filter", "Decaf", "Preset 6"]
btns = "".join(f'<rect x="{12 + (i % 3) * 88}" y="{300 + (i // 3) * 46}" width="80" height="38" rx="10" fill="{ACC if i == 1 else "#26262C"}"/>'
               + t(52 + (i % 3) * 88, 324 + (i // 3) * 46, r, 13) for i, r in enumerate(recipes))
frame(f'''{t(16,30,"Grind Sync",18,anchor="start",w="bold")}
<path d="{arc(140,130,70,135,405)}" stroke="#26262C" stroke-width="12" fill="none" stroke-linecap="round"/>
<path d="{arc(140,130,70,135,135+270*0.158)}" stroke="{ACC}" stroke-width="12" fill="none" stroke-linecap="round"/>
{t(140,135,"15.8",32,w="bold")}{t(140,160,"Double",13,"#8A8A94")}
<rect x="20" y="214" width="240" height="8" rx="4" fill="#26262C"/><rect x="20" y="214" width="150" height="8" rx="4" fill="{BLUE}"/>
{t(140,240,"to 15.8  -  62%  -  1.4 s",12,"#8A8A94")}
<rect x="12" y="252" width="256" height="38" rx="10" fill="#141418" stroke="#2A2A30"/>
{t(24,276,"✓  GaggiMate ready  ·  last 30.4 s  (+0.3)",12,OK,"start")}
{btns}
<rect x="12" y="400" width="256" height="44" rx="12" fill="{ACC}"/>{t(140,428,"START SHOT",16,w="bold")}''', "grind_sync")

# Calibration wizard
jog = "".join(f'<rect x="{12 + i * 44}" y="210" width="40" height="40" rx="8" fill="#26262C"/>' + t(32 + i * 44, 235, s, 12)
              for i, s in enumerate(["-800", "-100", "-10", "+10", "+100", "+800"]))
frame(f'''{t(16,30,"Dial Calibration",18,anchor="start",w="bold")}
{t(16,70,"Step 1/3: jog FINER until the",13,"#ccc","start")}{t(16,88,"burrs just touch (grinder OFF),",13,"#ccc","start")}{t(16,106,"then press  Set 0.",13,"#ccc","start")}
{t(140,170,"steps: -1240",22,w="bold")}
{jog}
<rect x="12" y="270" width="124" height="44" rx="10" fill="{ACC}"/>{t(74,298,"Set 0",15,w="bold")}
<rect x="144" y="270" width="124" height="44" rx="10" fill="#26262C"/>{t(206,298,"Set 100",15)}
{t(16,346,"Safety margin: 5",13,"#ccc","start")}
<rect x="16" y="356" width="248" height="6" rx="3" fill="#26262C"/><circle cx="40" cy="359" r="9" fill="{ACC}"/>
<rect x="12" y="394" width="124" height="44" rx="10" fill="#26262C"/>{t(74,422,"Cancel",15)}
<rect x="144" y="394" width="124" height="44" rx="10" fill="{OK}"/>{t(206,422,"Finish",15,"#000",w="bold")}''', "calibration")
print("ok")
