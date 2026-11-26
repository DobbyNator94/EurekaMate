#!/usr/bin/env python3
"""Generate the EurekaMate wiring schematic (SVG; PNG via rsvg-convert if present).

    python3 tools/gen_schematic.py   ->  docs/img/schaltplan.svg / .png

Beginner-oriented block schematic: every wire is drawn, colour = function.
Pins must match src/config/grind_sync.h and src/config/hardware.h.
"""
import os, shutil, subprocess

W, H = 1900, 1320
out = []
def add(s): out.append(s)

C = {"12v": "#d62728", "5v": "#ff7f0e", "3v3": "#c51b8a", "gnd": "#222222",
     "sig": "#1f77b4", "uart": "#17becf", "mot": "#2ca02c", "i2c": "#8c564b", "hx": "#9467bd"}
pins = {}

def text(x, y, s, size=14, anchor="start", weight="normal", fill="#111", italic=False):
    st = ' font-style="italic"' if italic else ""
    s = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    add(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" fill="{fill}"{st}>{s}</text>')

def block(key, x, y, w, h, title, sub="", left=(), right=(), top=(), fill="#f7f7f7", step=32, y0=None):
    add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" fill="{fill}" stroke="#333" stroke-width="2"/>')
    text(x + w / 2, y + 24, title, 17, "middle", "bold")
    if sub:
        for i, line in enumerate(sub.split("\n")):
            text(x + w / 2, y + 44 + i * 16, line, 12, "middle", fill="#555")
    start = y0 if y0 is not None else y + 70
    for i, p in enumerate(left):
        if p is None: continue
        py = start + i * step
        add(f'<circle cx="{x}" cy="{py}" r="4.5" fill="#fff" stroke="#333" stroke-width="2"/>')
        text(x + 10, py + 5, p[1], 13)
        pins[(key, p[0])] = (x, py)
    for i, p in enumerate(right):
        if p is None: continue
        py = start + i * step
        add(f'<circle cx="{x + w}" cy="{py}" r="4.5" fill="#fff" stroke="#333" stroke-width="2"/>')
        text(x + w - 10, py + 5, p[1], 13, "end")
        pins[(key, p[0])] = (x + w, py)
    for i, p in enumerate(top):
        px = x + 40 + i * 70
        add(f'<circle cx="{px}" cy="{y}" r="4.5" fill="#fff" stroke="#333" stroke-width="2"/>')
        text(px, y + 18, p[1], 12, "middle")
        pins[(key, p[0])] = (px, y)

def wire(pts, color, label=None, lpos=0, dash=False, width=3):
    d = " ".join(f"{x},{y}" for x, y in pts)
    da = ' stroke-dasharray="8,6"' if dash else ""
    add(f'<polyline points="{d}" fill="none" stroke="{color}" stroke-width="{width}" stroke-linejoin="round"{da}/>')
    for x, y in (pts[0], pts[-1]):
        add(f'<circle cx="{x}" cy="{y}" r="3" fill="{color}"/>')
    if label:
        (x1, y1), (x2, y2) = pts[lpos], pts[lpos + 1]
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        add(f'<rect x="{mx - len(label) * 3.6 - 4}" y="{my - 11}" width="{len(label) * 7.2 + 8}" height="17" rx="4" fill="#fff" opacity="0.92"/>')
        text(mx, my + 2, label, 12, "middle", "bold", color)

def gnd(x, y, dx=26):
    """ground symbol to the right (dx>0) or left of a pin"""
    ex = x + dx
    wire([(x, y), (ex, y), (ex, y + 10)], C["gnd"])
    for i, w in enumerate((18, 12, 6)):
        add(f'<line x1="{ex - w / 2}" y1="{y + 10 + i * 5}" x2="{ex + w / 2}" y2="{y + 10 + i * 5}" stroke="#222" stroke-width="2.5"/>')

def P(k, n): return pins[(k, n)]

add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="DejaVu Sans, Arial, sans-serif">')
add(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
text(30, 40, "EurekaMate – Schaltplan (Eureka Mignon Specialità + GaggiMate Grind-by-Sync + Motor am Mahlgradrad)", 22, weight="bold")
text(30, 64, "Primäre Stromversorgung: interne ~12 V DC-Schiene der Mühle (nach DF10S-Gleichrichter, vor 78L05). Die 230-V-Seite wird NIE angefasst. Alle GND-Symbole sind miteinander verbunden (gemeinsame Masse).", 13, fill="#444")

# ---------------- Eureka power board ----------------
add('<rect x="30" y="110" width="330" height="200" rx="10" fill="#ffe5e5" stroke="#d62728" stroke-width="2" stroke-dasharray="10,6"/>')
text(195, 135, "⚠ 230 V-Bereich – NICHT anfassen", 15, "middle", "bold", "#d62728")
text(195, 158, "Netz → Trafo 230 V/9 V AC → Relaiskontakte → Motor", 12, "middle", fill="#a00")
text(195, 176, "Vor dem Öffnen IMMER Netzstecker ziehen!", 12, "middle", "bold", "#a00")
text(195, 200, "Trafo 9 V AC → DF10S-Brücke → ~12 V DC", 12, "middle", fill="#555")
text(195, 218, "→ 78L05 → 5 V (max. <100 mA Reserve!)", 12, "middle", fill="#555")
text(195, 245, "Relaisspule läuft an ~12 V, NPN-Treiber Q1", 12, "middle", fill="#555")
text(195, 263, "wird über Stecker-Pin 3 angesteuert (aktiv HIGH)", 12, "middle", fill="#555")
block("eu", 30, 330, 330, 330, "Eureka Powerboard (Niederspannung)", "Abgriff am Siebelko: + (nach DF10S, vor 78L05)\nMessen! 11–14 V im Leerlauf",
      right=[("v12", "+12 V Abgriff (Elko +)"), ("g12", "GND (Elko –)"), None,
             ("p1", "Stecker Pin 1: +5 V"), ("p2", "Stecker Pin 2: Taste"), ("p3", "Stecker Pin 3: Motor/Relais"),
             ("p4", "Stecker Pin 4: GND")], fill="#fff4f4", y0=420)
x1, y1 = P("eu", "p1")
add(f'<line x1="{x1}" y1="{y1}" x2="{x1 + 40}" y2="{y1}" stroke="{C["5v"]}" stroke-width="3" stroke-dasharray="4,4"/>')
text(x1 + 46, y1 + 5, "✗ NICHT verbinden (isolieren)", 13, weight="bold", fill="#d62728")

# ---------------- fuse + buck ----------------
block("fu", 430, 110, 130, 90, "Sicherung", "Polyfuse/träge 1 A", left=[("i", "")], right=[("o", "")], y0=170, fill="#fffbe6")
block("bk", 430, 230, 210, 150, "5-V-Step-Down", "MP1584 / Mini560\nVOR dem Anschließen auf 5,0 V einstellen!",
      left=[("in", "IN+"), ("ing", "IN–")], right=[("out", "OUT+ 5 V"), ("outg", "OUT– GND")], y0=320)

# ---------------- ESP32-S3 ----------------
block("esp", 760, 300, 270, 640, "Waveshare ESP32-S3", "Touch-AMOLED 1.64\" (EurekaMate)",
      left=[("5v", "5V (VBUS)"), ("gnd", "GND"), None, ("g18", "GPIO18 Relais"), ("g17", "GPIO17 Brüh-Taste"), None,
            ("g2", "GPIO2 HX711 SCK"), ("g3", "GPIO3 HX711 DOUT"), ("3v3a", "3V3"), ("gnd2", "GND")],
      right=[("g7", "GPIO7 EN"), ("g5", "GPIO5 STEP"), ("g6", "GPIO6 DIR"), ("g8", "GPIO8 UART TX"),
             ("g1", "GPIO1 DIAG"), ("3v3b", "3V3"), ("gnd3", "GND"), None, ("g15", "GPIO15 SDA"), ("g16", "GPIO16 SCL"),
             None, ("g44", "GPIO44 Endschalter"), None, ("gnd4", "GND")], y0=380, step=34)
text(895, 932, "+ 1000 µF / 10 V an 5V–GND (wie Original-SGBW)", 11, "middle", fill="#555")

# ---------------- TMC2209 ----------------
block("tmc", 1250, 300, 250, 420, "TMC2209 Modul", "Treiber für Schrittmotor\n(z. B. BTT / FYSETC, Kühlkörper drauf)",
      left=[("en", "EN"), ("step", "STEP"), ("dir", "DIR"), ("pdn", "PDN_UART"), ("diag", "DIAG"), ("vio", "VIO / VDD"),
            ("gl", "GND"), ("ms1", "MS1"), ("ms2", "MS2")],
      right=[("vm", "VM (12 V)"), ("gp", "GND (Power)"), None, ("a1", "1A / M1A"), ("a2", "1B / M1B"),
             ("b1", "2A / M2A"), ("b2", "2B / M2B")], y0=380, step=34)

# ---------------- bulk cap ----------------
block("cap", 1560, 140, 210, 110, "Elko 100–470 µF", "≥ 25 V, direkt an VM/GND\n(+ an VM!)", fill="#fffbe6")

# ---------------- motor ----------------
block("mot", 1620, 440, 250, 270, "NEMA17 Schrittmotor", "17HS4401 (~0,45 Nm)\nSpule A: schwarz/grün\nSpule B: rot/blau\n(je ~1,5–2 Ω messen!)",
      left=[("a1", "A+ (schwarz)"), ("a2", "A– (grün)"), ("b1", "B+ (rot)"), ("b2", "B– (blau)")], y0=600, step=28, fill="#eef7ee")

# ---------------- AS5600 ----------------
block("as", 1250, 800, 250, 200, "AS5600 (optional)", "Magnet-Encoder auf Ritzelachse\n(Magnet am hinteren Wellenende)",
      left=[("vcc", "VCC 3V3"), ("gnd", "GND"), ("sda", "SDA"), ("scl", "SCL"), ("dir", "DIR → GND")], y0=870, step=26, fill="#f4efe9")

# ---------------- endstop ----------------
block("es", 1560, 1040, 220, 110, "Endschalter (optional)", "Grob-Ende, schließt nach GND", left=[("s", "NO"), ("g", "COM")], y0=1100, step=26)

# ---------------- HX711 + load cell ----------------
block("hx", 330, 960, 230, 230, "HX711", "Wägezellen-Verstärker", left=[("ep", "E+"), ("em", "E–"), ("am", "A–"), ("ap", "A+")],
      right=[("vcc", "VCC"), ("sck", "SCK"), ("dout", "DT / DOUT"), ("gnd", "GND")], y0=1040, step=32)
block("lc", 30, 960, 220, 230, "Wägezelle", "Load Cell (Halterung)", right=[("r", "rot E+"), ("k", "schwarz E–"), ("w", "weiß A–"), ("g", "grün A+"), ("y", "gelb Schirm")], y0=1040, step=32, fill="#f0f0ff")

# ---------------- extra brew button ----------------
block("bt", 430, 690, 200, 90, "Brüh-Taste (opt.)", "Taster nach GND", right=[("s", "")], y0=745, fill="#eef4fb")

# ======================= WIRES =======================
# 12 V tap -> fuse -> buck IN+ and -> TMC VM (+cap)
ex, ey = P("eu", "v12"); fx, fy = P("fu", "i")
wire([(ex, ey), (395, ey), (395, fy), (fx, fy)], C["12v"], "~12 V", 0)
ox, oy = P("fu", "o"); bx, by = P("bk", "in")
wire([(ox, oy), (600, oy), (600, 215), (415, 215), (415, by), (bx, by)], C["12v"])
vx, vy = P("tmc", "vm")
wire([(ox, oy), (1540, oy), (1540, vy), (vx, vy)], C["12v"], "+12 V (VM)", 0)
wire([(1540, 205), (1560, 205)], C["12v"])
# GND from the board -> buck IN- and TMC power GND
gx, gy = P("eu", "g12"); bgx, bgy = P("bk", "ing")
wire([(gx, gy), (405, gy), (405, bgy), (bgx, bgy)], C["gnd"], "GND", 0)
gpx, gpy = P("tmc", "gp")
gnd(gpx, gpy, 30)
gnd(1770, 225, 26)
text(1765, 220, "–", 16, "end", "bold")
text(1568, 210, "+", 16, "start", "bold", C["12v"])
# buck out -> ESP 5V / GND
o5x, o5y = P("bk", "out"); e5x, e5y = P("esp", "5v")
wire([(o5x, o5y), (700, o5y), (700, e5y), (e5x, e5y)], C["5v"], "5 V", 0)
ogx, ogy = P("bk", "outg"); egx, egy = P("esp", "gnd")
wire([(ogx, ogy), (680, ogy), (680, egy), (egx, egy)], C["gnd"], "GND", 1)
# harness: relay + button + GND
p3x, p3y = P("eu", "p3"); e18x, e18y = P("esp", "g18")
wire([(p3x, p3y), (650, p3y), (650, e18y), (e18x, e18y)], C["sig"], "Relais (aktiv HIGH)", 0)
p2x, p2y = P("eu", "p2"); e17x, e17y = P("esp", "g17")
wire([(p2x, p2y), (670, p2y), (670, e17y), (e17x, e17y)], C["sig"], "Taste (aktiv LOW)", 0)
btx, bty = P("bt", "s")
wire([(btx, bty), (670, bty), (670, e17y)], C["sig"], dash=True)
p4x, p4y = P("eu", "p4"); eg2x, eg2y = P("esp", "gnd2")
wire([(p4x, p4y), (690, p4y), (690, 676), (690, eg2y), (eg2x, eg2y)], C["gnd"], "GND (gemeinsam)", 0)
# HX711
hx, hy = P("hx", "sck"); e2x, e2y = P("esp", "g2")
wire([(hx, hy), (620, hy), (620, e2y), (e2x, e2y)], C["hx"], "SCK", 2)
hx, hy = P("hx", "dout"); e3x, e3y = P("esp", "g3")
wire([(hx, hy), (640, hy), (640, e3y), (e3x, e3y)], C["hx"], "DOUT", 2)
hx, hy = P("hx", "vcc"); e33x, e33y = P("esp", "3v3a")
wire([(hx, hy), (600, hy), (600, e33y), (e33x, e33y)], C["3v3"], "3V3", 1)
hx, hy = P("hx", "gnd"); gnd(hx, hy, 24)
for a, b in (("r", "ep"), ("k", "em"), ("w", "am"), ("g", "ap")):
    (x1, y1), (x2, y2) = P("lc", a), P("hx", b)
    wire([(x1, y1), (x2, y2)], C["hx"])
yx, yy = P("lc", "y"); gnd(yx, yy, 30)
# TMC logic
for e, t, lab, col in (("g7", "en", "EN (LOW = an)", C["sig"]), ("g5", "step", "STEP", C["sig"]), ("g6", "dir", "DIR", C["sig"]),
                       ("g1", "diag", "DIAG", C["sig"]), ("3v3b", "vio", "3V3", C["3v3"])):
    (x1, y1), (x2, y2) = P("esp", e), P("tmc", t)
    mx = 1130 + (hash(e) % 1) * 0
    wire([(x1, y1), (1100, y1), (1100, y1), (1170, y1), (1170, y2), (x2, y2)] if y1 != y2 else [(x1, y1), (x2, y2)], col, lab, 0)
# UART with 1k resistor
(x1, y1), (x2, y2) = P("esp", "g8"), P("tmc", "pdn")
wire([(x1, y1), (1080, y1)], C["uart"])
add(f'<rect x="1080" y="{y1 - 9}" width="60" height="18" fill="#fff" stroke="{C["uart"]}" stroke-width="2.5"/>')
text(1110, y1 + 5, "1 kΩ", 12, "middle", "bold", C["uart"])
wire([(1140, y1), (1170, y1), (1170, y2), (x2, y2)], C["uart"])
text(1110, y1 - 14, "UART TX", 12, "middle", "bold", C["uart"])
g3x, g3y = P("esp", "gnd3"); gnd(g3x, g3y, 26)
for k in ("gl", "ms1", "ms2"):
    x2, y2 = P("tmc", k); gnd(x2, y2, -30)
text(1375, 742, "MS1+MS2 an GND: UART-Adresse 0", 11, "middle", fill="#555")
text(1375, 758, "(ohne UART: 8 Mikroschritte)", 11, "middle", fill="#555")
# motor coils
for t, m in (("a1", "a1"), ("a2", "a2"), ("b1", "b1"), ("b2", "b2")):
    (x1, y1), (x2, y2) = P("tmc", t), P("mot", m)
    xm = 1560 + {"a1": 0, "a2": 12, "b1": 24, "b2": 36}[t]
    wire([(x1, y1), (xm, y1), (xm, y2), (x2, y2)], C["mot"])
text(1590, 462, "Spulenpaare!", 12, "middle", "bold", C["mot"])
# AS5600
for e, a, lab, col, xm in (("g15", "sda", "SDA", C["i2c"], 1150), ("g16", "scl", "SCL", C["i2c"], 1170)):
    (x1, y1), (x2, y2) = P("esp", e), P("as", a)
    wire([(x1, y1), (xm, y1), (xm, y2), (x2, y2)], col, lab, 0)
(x1, y1), (x2, y2) = P("esp", "3v3b"), P("as", "vcc")
wire([(1100, y1), (1100, y2), (x2, y2)], C["3v3"], "3V3", 1)
for k in ("gnd", "dir"):
    x2, y2 = P("as", k); gnd(x2, y2, -30)
# endstop
(x1, y1), (x2, y2) = P("esp", "g44"), P("es", "s")
wire([(x1, y1), (1120, y1), (1120, y2), (x2, y2)], C["sig"], "Endschalter", 2, dash=True)
x2, y2 = P("es", "g"); gnd(x2, y2, -30)
g4x, g4y = P("esp", "gnd4"); gnd(g4x, g4y, 26)

# legend
lx, ly = 30, 1225
text(lx, ly, "Legende:", 14, weight="bold")
for i, (k, lab) in enumerate((("12v", "~12 V (aus der Mühle)"), ("5v", "5 V (Step-Down)"), ("3v3", "3,3 V (vom ESP)"), ("gnd", "GND / Masse"),
                              ("sig", "Steuersignal"), ("uart", "TMC-UART"), ("mot", "Motorspulen"), ("i2c", "I²C AS5600"), ("hx", "Waage HX711"))):
    x = lx + 90 + i * 195
    add(f'<line x1="{x}" y1="{ly - 5}" x2="{x + 34}" y2="{ly - 5}" stroke="{C[k]}" stroke-width="4"/>')
    text(x + 40, ly, lab, 12)
text(lx, ly + 28, "Gestrichelt = optional. Pin-Nummern: src/config/grind_sync.h / hardware.h – vor dem Löten mit dem Waveshare-Schaltplan abgleichen. "
     "Fallback bei Spannungseinbruch (<10 V unter Last oder ESP-Reset): externes 12 V / 2 A Netzteil statt Abgriff, GND trotzdem verbinden.", 12, fill="#444")
text(lx, ly + 48, "Stecker-Pins wie im Original-SGBW gezählt (Rippen zu dir, von links). Pin 2 vor dem Anschließen messen: liegt dort offen >3,3 V an, 10 kΩ/20 kΩ-Teiler oder Diode einsetzen.", 12, fill="#444")
add("</svg>")

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
svg = os.path.join(root, "docs", "img", "schaltplan.svg")
os.makedirs(os.path.dirname(svg), exist_ok=True)
open(svg, "w", encoding="utf-8").write("\n".join(out))
print("wrote", svg)
if shutil.which("rsvg-convert"):
    png = svg[:-4] + ".png"
    subprocess.run(["rsvg-convert", "-b", "white", "-o", png, svg], check=True)
    print("wrote", png)
