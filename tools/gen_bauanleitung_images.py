#!/usr/bin/env python3
"""Generate the simple illustrations for docs/BAUANLEITUNG.md / docs/BUILD_GUIDE_EN.md.

All images are plain SVG drawn here and converted to PNG with rsvg-convert.
Usage: python3 tools/gen_bauanleitung_images.py
Output: docs/img/bauanleitung/*.png (+ .svg sources)
Images with German/English labels side by side so both guides can share them.
"""
import base64, os, subprocess, html

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "docs", "img", "bauanleitung")
os.makedirs(OUT, exist_ok=True)

FONT = "font-family='DejaVu Sans, Arial, sans-serif'"
COL = dict(red="#d62828", black="#222", orange="#f77f00", blue="#1d6fd8", purple="#7b2cbf",
           teal="#00a6a6", green="#2a9d3c", brown="#8d5524", grey="#c8c8c8", yellow="#ffd60a")


def esc(s):
    return html.escape(str(s))


def svg(w, h, body, bg="#ffffff"):
    return (f"<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' "
            f"width='{w}' height='{h}' viewBox='0 0 {w} {h}'>"
            f"<rect width='{w}' height='{h}' fill='{bg}'/>{body}</svg>")


def text(x, y, s, size=18, fill="#222", anchor="start", weight="normal", style="normal"):
    lines = str(s).split("\n")
    out = ""
    for i, ln in enumerate(lines):
        out += (f"<text x='{x}' y='{y + i * size * 1.25}' {FONT} font-size='{size}' fill='{fill}' "
                f"text-anchor='{anchor}' font-weight='{weight}' font-style='{style}'>{esc(ln)}</text>")
    return out


def box(x, y, w, h, fill="#f4f7fb", stroke="#333", r=10, sw=2, dash=None):
    d = f" stroke-dasharray='{dash}'" if dash else ""
    return f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='{r}' fill='{fill}' stroke='{stroke}' stroke-width='{sw}'{d}/>"


def title(w, s, sub=None):
    b = f"<rect width='{w}' height='{70 if sub else 52}' fill='#2b2d42'/>" + text(20, 34, s, 24, "#fff", weight="bold")
    if sub:
        b += text(20, 60, sub, 15, "#d0d4e4", style="italic")
    return b


def save(name, content):
    p = os.path.join(OUT, name + ".svg")
    with open(p, "w") as f:
        f.write(content)
    subprocess.run(["rsvg-convert", "-o", os.path.join(OUT, name + ".png"), p], check=True)
    print("wrote", name + ".png")


def warn(x, y, w, s, h=None):
    lines = s.count("\n") + 1
    h = h or 26 + lines * 22
    return (box(x, y, w, h, "#fff4e5", "#ff9500", 8, 3) + text(x + 14, y + 30, "⚠", 24, "#d00000")
            + text(x + 46, y + 28, s, 17, "#7a4100", weight="bold"))


# --------------------------------------------------------------- component drawings
def pcb(x, y, w, h, name, color, pins, side="right", label_col="#fff", pin_gap=30, pin_y0=None):
    """Board with labelled pins. pins: list of names. Returns (svg, {pin: (x,y)})."""
    s = box(x, y, w, h, color, "#111", 8, 2) + text(x + w / 2, y + 24, name, 16, label_col, "middle", "bold")
    pos = {}
    y0 = pin_y0 if pin_y0 is not None else y + 48
    for i, p in enumerate(pins):
        py = y0 + i * pin_gap
        if side == "right":
            px = x + w
            s += f"<circle cx='{px}' cy='{py}' r='6' fill='#e9c46a' stroke='#111'/>" + text(px - 12, py + 5, p, 14, label_col, "end")
        else:
            px = x
            s += f"<circle cx='{px}' cy='{py}' r='6' fill='#e9c46a' stroke='#111'/>" + text(px + 12, py + 5, p, 14, label_col)
        pos[p] = (px, py)
    return s, pos


def wire(a, b, color, active=True, label=None, dash=None, width=None):
    (x1, y1), (x2, y2) = a, b
    mx = (x1 + x2) / 2
    c = color if active else "#d8d8d8"
    sw = width or (5 if active else 3)
    d = f" stroke-dasharray='{dash}'" if dash else ""
    s = f"<path d='M{x1},{y1} C{mx},{y1} {mx},{y2} {x2},{y2}' fill='none' stroke='{c}' stroke-width='{sw}' stroke-linecap='round'{d}/>"
    if label and active:
        s += box(mx - 60, (y1 + y2) / 2 - 14, 120, 26, "#fff", c, 6, 2) + text(mx, (y1 + y2) / 2 + 5, label, 13, "#222", "middle", "bold")
    return s


# ---------------------------------------------------------------- 01 shopping list
def img_shopping():
    items = [
        ("ESP32-S3 AMOLED 1.64\" (Waveshare)", "~25 €", "esp"), ("Wägezelle + HX711\nLoad cell + HX711", "~8 €", "load"),
        ("NEMA17 17HS4401\nSchrittmotor / stepper", "10–15 €", "motor"), ("TMC2209 + Kühlkörper\n+ heatsink", "~6 €", "tmc"),
        ("Step-Down 5 V\nMP1584 / Mini560", "~3 €", "buck"), ("Elkos 100–470 µF/25 V\n+ 1000 µF/10 V", "~1 €", "cap"),
        ("Widerstand 1 kΩ\nresistor 1 kΩ", "~0,10 €", "res"), ("Sicherung 1 A\nfuse 1 A", "~1 €", "fuse"),
        ("Zahnrad-Knopf-Kit Mignon\ngear knob kit", "–", "gear"), ("Litze, Stecker, Schrumpf.\nwire, plugs, heatshrink", "~10 €", "wire"),
        ("Schrauben M3 + Muttern\nM3 screws + nuts", "~5 €", "screw"), ("PETG + TPU Filament", "–", "fil"),
    ]
    W, cw, ch = 1200, 290, 210
    b = title(W, "Schritt 1 · Was du brauchst / What you need", "Gesamt ca. 75–90 € (ohne Optionen) · total approx. 75–90 € (without options)")
    for i, (name, price, kind) in enumerate(items):
        cx, cy = 15 + (i % 4) * (cw + 8), 90 + (i // 4) * (ch + 10)
        b += box(cx, cy, cw, ch, "#f8f9fb", "#9aa", 12, 1.5)
        ix, iy = cx + cw / 2, cy + 75
        b += icon(kind, ix, iy)
        b += text(cx + cw / 2, cy + 152, name, 14, "#222", "middle", "bold")
        b += box(cx + cw - 82, cy + 8, 74, 26, "#2a9d3c", "#2a9d3c", 13) + text(cx + cw - 45, cy + 26, price, 14, "#fff", "middle", "bold")
        b += text(cx + 12, cy + 26, f"{i + 1}", 18, "#888", weight="bold")
    b += text(20, 785, "Optional: AS5600 + Magnet 6×2,5 mm (~3 €), Endschalter / endstop (~1 €), 12 V/2 A Netzteil / PSU (~10 €)", 15, "#555")
    save("01_einkaufsliste", svg(W, 800, b))


def icon(kind, x, y):
    if kind == "esp":
        return (box(x - 70, y - 50, 140, 100, "#111", "#000", 14) + box(x - 58, y - 40, 116, 70, "#0b3d91", "#000", 6)
                + text(x, y, "88.0 g", 22, "#fff", "middle", "bold") + box(x - 12, y + 42, 24, 10, "#999", "#555", 2))
    if kind == "load":
        return (box(x - 90, y - 14, 140, 28, "#c0c4c8", "#666", 3) + box(x + 55, y - 30, 45, 60, "#2a7a2a", "#111", 4)
                + text(x + 77, y + 5, "HX711", 10, "#fff", "middle"))
    if kind == "motor":
        return (box(x - 50, y - 50, 100, 100, "#333", "#000", 6) + box(x - 44, y - 44, 88, 88, "#555", "#222", 4)
                + f"<circle cx='{x}' cy='{y}' r='18' fill='#888' stroke='#222'/><circle cx='{x}' cy='{y}' r='5' fill='#ddd'/>")
    if kind == "tmc":
        return (box(x - 40, y - 50, 80, 100, "#b3122e", "#111", 4) + box(x - 30, y - 60, 60, 40, "#c0c0c0", "#666", 2)
                + "".join(f"<line x1='{x-26+k*10}' y1='{y-60}' x2='{x-26+k*10}' y2='{y-22}' stroke='#888' stroke-width='3'/>" for k in range(6))
                + text(x, y + 20, "TMC2209", 12, "#fff", "middle", "bold"))
    if kind == "buck":
        return (box(x - 55, y - 30, 110, 60, "#1d4ed8", "#111", 4) + f"<circle cx='{x-25}' cy='{y}' r='12' fill='#333'/>"
                + box(x + 5, y - 12, 30, 24, "#666", "#222", 2) + text(x, y + 50, "IN → OUT 5 V", 12, "#333", "middle"))
    if kind == "cap":
        return (box(x - 50, y - 40, 34, 76, "#222", "#000", 8) + box(x + 10, y - 30, 26, 56, "#1d3557", "#000", 7)
                + f"<line x1='{x-40}' y1='{y+36}' x2='{x-40}' y2='{y+55}' stroke='#999' stroke-width='3'/><line x1='{x-26}' y1='{y+36}' x2='{x-26}' y2='{y+55}' stroke='#999' stroke-width='3'/>"
                + text(x - 33, y - 10, "−", 18, "#fff", "middle"))
    if kind == "res":
        return (f"<line x1='{x-80}' y1='{y}' x2='{x+80}' y2='{y}' stroke='#999' stroke-width='4'/>" + box(x - 40, y - 15, 80, 30, "#e9d8a6", "#8a6", 12)
                + "".join(f"<rect x='{x-28+k*16}' y='{y-15}' width='7' height='30' fill='{c}'/>" for k, c in enumerate(["#8d5524", "#111", "#d62828", "#c9a227"])))
    if kind == "fuse":
        return (box(x - 60, y - 16, 120, 32, "#e8f1ff", "#557", 14) + box(x - 70, y - 18, 22, 36, "#bbb", "#666", 3)
                + box(x + 48, y - 18, 22, 36, "#bbb", "#666", 3) + f"<line x1='{x-45}' y1='{y}' x2='{x+45}' y2='{y}' stroke='#666' stroke-width='2'/>" + text(x, y + 40, "1 A", 14, "#333", "middle", "bold"))
    if kind == "gear":
        return gear_svg(x - 25, y, 48, 24, "#b0b0b0") + gear_svg(x + 50, y + 10, 18, 10, "#ff9500")
    if kind == "wire":
        return "".join(f"<path d='M{x-80},{y-30+k*15} q40,-20 80,0 t80,0' stroke='{c}' stroke-width='5' fill='none'/>" for k, c in enumerate(["#d62828", "#222", "#1d6fd8", "#7b2cbf", "#2a9d3c"]))
    if kind == "screw":
        out = ""
        for k in range(4):
            sx = x - 60 + k * 40
            out += box(sx - 10, y - 40, 20, 10, "#888", "#444", 2) + f"<rect x='{sx-4}' y='{y-30}' width='8' height='{40+k*8}' fill='#aaa' stroke='#555'/>"
        return out + f"<polygon points='{x+70},{y+20} {x+82},{y+13} {x+94},{y+20} {x+94},{y+34} {x+82},{y+41} {x+70},{y+34}' fill='#999' stroke='#444'/>"
    if kind == "fil":
        return (f"<circle cx='{x-35}' cy='{y}' r='42' fill='#ff9500' stroke='#7a4100' stroke-width='3'/><circle cx='{x-35}' cy='{y}' r='12' fill='#fff'/>"
                + f"<circle cx='{x+45}' cy='{y}' r='38' fill='#333' stroke='#000' stroke-width='3'/><circle cx='{x+45}' cy='{y}' r='11' fill='#fff'/>"
                + text(x - 35, y + 62, "PETG", 12, "#333", "middle") + text(x + 45, y + 62, "TPU", 12, "#333", "middle"))
    if kind == "tools":
        return ""
    return ""


def gear_svg(cx, cy, r, teeth, color):
    import math
    pts = []
    for i in range(teeth * 2):
        a = math.pi * i / teeth
        rr = r if i % 2 == 0 else r * 0.85
        pts.append(f"{cx + rr * math.cos(a):.1f},{cy + rr * math.sin(a):.1f}")
    return f"<polygon points='{' '.join(pts)}' fill='{color}' stroke='#444'/><circle cx='{cx}' cy='{cy}' r='{r*0.3:.1f}' fill='#fff' stroke='#444'/>"


# ---------------------------------------------------------------- 03 tools
def img_tools():
    W = 1200
    tools = [("Lötkolben ≥ 60 W\nsoldering iron", "iron"), ("Multimeter", "mm"), ("Seitenschneider +\nAbisolierzange\ncutters + stripper", "cut"),
             ("Schraubendreher\nKreuz + Innensechskant\nscrewdrivers + hex keys", "sd"), ("Messschieber\ncalipers", "cal"),
             ("Schrumpfschlauch +\nHeißluft/Feuerzeug\nheatshrink + heat", "hs"), ("3D-Drucker\n3D printer", "pr"), ("PC/Mac + Chrome/Edge\n+ USB-C-Datenkabel", "pc")]
    b = title(W, "Schritt 3 · Werkzeug / Tools")
    for i, (n, k) in enumerate(tools):
        cx, cy = 15 + (i % 4) * 298, 75 + (i // 4) * 270
        b += box(cx, cy, 288, 255, "#f8f9fb", "#9aa", 12, 1.5) + tool_icon(k, cx + 144, cy + 90) + text(cx + 144, cy + 190, n, 15, "#222", "middle", "bold")
    save("03_werkzeug", svg(W, 630, b))


def tool_icon(k, x, y):
    if k == "iron":
        return (box(x - 90, y - 12, 110, 24, "#1d6fd8", "#123", 10) + f"<rect x='{x+20}' y='{y-5}' width='50' height='10' fill='#aaa'/>"
                + f"<polygon points='{x+70},{y-5} {x+95},{y} {x+70},{y+5}' fill='#666'/>" + f"<path d='M{x-90},{y} q-30,10 -20,50' stroke='#222' stroke-width='4' fill='none'/>")
    if k == "mm":
        return multimeter(x, y, "12.3", 0.8)
    if k == "cut":
        return (f"<path d='M{x-70},{y+50} L{x},{y-10} L{x+20},{y-60}' stroke='#d62828' stroke-width='14' fill='none' stroke-linecap='round'/>"
                + f"<path d='M{x+70},{y+50} L{x},{y-10} L{x-20},{y-60}' stroke='#d62828' stroke-width='14' fill='none' stroke-linecap='round'/>"
                + f"<circle cx='{x}' cy='{y-10}' r='8' fill='#555'/>")
    if k == "sd":
        return (box(x - 80, y - 14, 70, 28, "#ffd60a", "#222", 10) + f"<rect x='{x-10}' y='{y-4}' width='80' height='8' fill='#aaa'/>"
                + f"<path d='M{x-60},{y+40} l90,0 l0,25' stroke='#444' stroke-width='9' fill='none'/>")
    if k == "cal":
        return (f"<rect x='{x-100}' y='{y-8}' width='200' height='16' fill='#ccc' stroke='#555'/>"
                + "".join(f"<line x1='{x-95+j*10}' y1='{y-8}' x2='{x-95+j*10}' y2='{y-2}' stroke='#333'/>" for j in range(20))
                + f"<rect x='{x-100}' y='{y-50}' width='14' height='60' fill='#aaa' stroke='#555'/><rect x='{x-30}' y='{y-50}' width='14' height='60' fill='#aaa' stroke='#555'/>")
    if k == "hs":
        return "".join(f"<rect x='{x-90+j*45}' y='{y-30}' width='22' height='70' rx='4' fill='{c}'/>" for j, c in enumerate(["#d62828", "#222", "#1d6fd8", "#2a9d3c"]))
    if k == "pr":
        return (box(x - 70, y - 60, 140, 120, "#eee", "#333", 6) + f"<rect x='{x-60}' y='{y-40}' width='120' height='6' fill='#555'/>"
                + box(x - 10, y - 46, 20, 18, "#444", "#222", 2) + box(x - 40, y + 25, 80, 22, "#ff9500", "#7a4100", 3))
    if k == "pc":
        return (box(x - 80, y - 60, 160, 100, "#333", "#111", 8) + box(x - 72, y - 52, 144, 84, "#cfe8ff", "#111", 3)
                + text(x, y - 5, "Chrome / Edge", 14, "#123", "middle", "bold") + f"<rect x='{x-100}' y='{y+42}' width='200' height='10' rx='4' fill='#666'/>")
    return ""


def multimeter(x, y, val, sc=1.0, unit="V⎓"):
    w, h = 110 * sc, 170 * sc
    return (box(x - w / 2, y - h / 2, w, h, "#ffd60a", "#333", 12 * sc, 2) + box(x - w / 2 + 10 * sc, y - h / 2 + 12 * sc, w - 20 * sc, 46 * sc, "#c8d5b9", "#333", 4)
            + text(x, y - h / 2 + 46 * sc, f"{val} {unit}", 22 * sc, "#111", "middle", "bold")
            + f"<circle cx='{x}' cy='{y+15*sc}' r='{28*sc}' fill='#333'/><line x1='{x}' y1='{y+15*sc}' x2='{x+14*sc}' y2='{y-5*sc}' stroke='#fff' stroke-width='{4*sc}'/>"
            + f"<circle cx='{x-22*sc}' cy='{y+65*sc}' r='{7*sc}' fill='#111'/><circle cx='{x+22*sc}' cy='{y+65*sc}' r='{7*sc}' fill='#d62828'/>")


# ---------------------------------------------------------------- 04 unplug
def img_unplug():
    W = 1000
    b = title(W, "Schritt 4 · ZUERST: Netzstecker ziehen! / FIRST: unplug!")
    # socket
    b += box(80, 130, 220, 220, "#eee", "#777", 30, 3) + "<circle cx='190' cy='240' r='80' fill='#ddd' stroke='#999' stroke-width='3'/>"
    b += "<circle cx='160' cy='240' r='12' fill='#333'/><circle cx='220' cy='240' r='12' fill='#333'/>"
    # plug moved away
    b += box(420, 180, 150, 120, "#222", "#000", 20) + "<rect x='570' y='225' width='60' height='10' fill='#aaa'/><rect x='570' y='245' width='60' height='10' fill='#aaa'/>"
    b += "<path d='M420,240 C360,240 380,400 300,420' stroke='#222' stroke-width='14' fill='none'/>"
    b += "<path d='M340,170 L400,170' stroke='#d00000' stroke-width='10' marker-end='url(#ar)'/>"
    b += "<defs><marker id='ar' markerWidth='6' markerHeight='6' refX='3' refY='3' orient='auto'><path d='M0,0 L6,3 L0,6 z' fill='#d00000'/></marker></defs>"
    b += text(370, 150, "raus / out", 20, "#d00000", "middle", "bold")
    # triangle
    b += "<polygon points='800,110 940,350 660,350' fill='#ffd60a' stroke='#111' stroke-width='8' stroke-linejoin='round'/>"
    b += "<path d='M815,160 L770,250 L810,250 L780,330 L850,220 L810,220 L835,160 z' fill='#111'/>"
    b += text(800, 395, "230 V", 34, "#d00000", "middle", "bold")
    b += warn(40, 430, 920, "Ausschalten reicht NICHT. Stecker raus, 2 Minuten warten (Motorkondensator).\nSwitching off is NOT enough. Unplug, wait 2 minutes (motor capacitor).")
    b += text(40, 560, "✔ Nur im Niederspannungsteil arbeiten (12 V / 5 V). An 230-V-Teile nie löten.", 18, "#2a7a2a", weight="bold")
    b += text(40, 590, "✔ Work only on the low-voltage side (12 V / 5 V). Never solder to 230 V parts.", 16, "#2a7a2a")
    save("04_stecker_ziehen", svg(W, 620, b))


# ---------------------------------------------------------------- 05 wiring steps
ESP_PINS = ["5V", "GND", "3V3", "GPIO1", "GPIO2", "GPIO3", "GPIO5", "GPIO6", "GPIO7", "GPIO8", "GPIO15", "GPIO16", "GPIO17", "GPIO18", "GPIO44"]


def wiring_base(step):
    """Draw all components; return svg and pin dict."""
    s = ""
    P = {}
    # grinder power board (left top)
    t, p = pcb(30, 110, 230, 150, "Mühle: Powerboard\nGrinder power board", "#6c757d",
               ["+12 V (DF10S/Elko +)", "GND (Elko −)"], "right", pin_y0=190, pin_gap=40)
    s += t; P.update({"M12": p["+12 V (DF10S/Elko +)"], "MGND": p["GND (Elko −)"]})
    s += text(40, 290, "230 V-Teile: NICHT anfassen", 13, "#d00000", weight="bold")
    # fuse
    s += box(330, 168, 80, 34, "#e8f1ff", "#557", 14) + text(370, 191, "1 A", 15, "#223", "middle", "bold")
    P["F_in"], P["F_out"] = (330, 185), (410, 185)
    # buck
    t, p = pcb(470, 110, 170, 130, "Step-Down 5 V", "#1d4ed8", ["IN+", "IN−"], "left", pin_y0=170, pin_gap=40)
    s += t; P["B_IN+"], P["B_IN-"] = p["IN+"], p["IN−"]
    t, p = pcb(470, 110, 170, 130, "", "none", ["OUT+", "OUT−"], "right", pin_y0=170, pin_gap=40)
    s += t; P["B_OUT+"], P["B_OUT-"] = p["OUT+"], p["OUT−"]
    # ESP
    t, p = pcb(760, 90, 190, 40 + 30 * len(ESP_PINS), "ESP32-S3\nWaveshare", "#111", ESP_PINS, "left", pin_y0=150, pin_gap=30)
    s += t; P.update({"E_" + k: v for k, v in p.items()})
    # TMC
    tmc_l = ["VIO", "GND", "EN", "STEP", "DIR", "PDN_UART", "DIAG", "MS1", "MS2"]
    t, p = pcb(1080, 110, 170, 60 + 30 * len(tmc_l), "TMC2209", "#b3122e", tmc_l, "left", pin_y0=160, pin_gap=30)
    s += t; P.update({"T_" + k: v for k, v in p.items()})
    tmc_r = ["VM", "GND ", "1A", "1B", "2A", "2B"]
    t, p = pcb(1080, 110, 170, 60 + 30 * len(tmc_l), "", "none", tmc_r, "right", pin_y0=160, pin_gap=30)
    s += t; P.update({"T_" + k.strip() + ("P" if k == "GND " else ""): v for k, v in p.items()})
    # motor
    s += box(1360, 250, 120, 120, "#333", "#000", 8) + "<circle cx='1420' cy='310' r='22' fill='#888'/>" + text(1420, 395, "NEMA17", 15, "#222", "middle", "bold")
    P["MO_A1"], P["MO_A2"], P["MO_B1"], P["MO_B2"] = (1360, 262), (1360, 286), (1360, 334), (1360, 358)
    # grinder harness
    t, p = pcb(30, 420, 230, 180, "Mühlenstecker 4-pol.\nGrinder harness", "#495057", ["Pin 1 (+5 V)", "Pin 2 (Taste)", "Pin 3 (Relais)", "Pin 4 (GND)"], "right", pin_y0=480, pin_gap=32)
    s += t; P.update({"H1": p["Pin 1 (+5 V)"], "H2": p["Pin 2 (Taste)"], "H3": p["Pin 3 (Relais)"], "H4": p["Pin 4 (GND)"]})
    # HX711 + load cell
    t, p = pcb(400, 520, 170, 150, "HX711", "#2a7a2a", ["VCC", "GND", "DT", "SCK"], "right", pin_y0=560, pin_gap=28)
    s += t; P.update({"X_" + k: v for k, v in p.items()})
    s += box(300, 690, 270, 36, "#c0c4c8", "#666", 4) + text(435, 714, "Wägezelle / load cell", 14, "#222", "middle")
    s += "<path d='M400,660 q-40,20 -20,30' stroke='#d62828' stroke-width='3' fill='none'/><path d='M410,665 q-30,20 0,25' stroke='#222' stroke-width='3' fill='none'/>"
    # capacitors
    s += box(650, 300, 70, 46, "#222", "#000", 8) + text(685, 328, "1000µF", 12, "#fff", "middle")
    P["C1+"], P["C1-"] = (650, 312), (650, 334)
    s += box(1290, 120, 60, 40, "#1d3557", "#000", 8) + text(1320, 145, "100µF", 11, "#fff", "middle")
    P["C2"] = (1290, 140)
    # optional AS5600, endstop
    s += box(1080, 520, 170, 80, "#6a4c93", "#111", 8, 2, "6,4") + text(1165, 548, "AS5600 (opt.)", 15, "#fff", "middle", "bold") + text(1165, 572, "SDA / SCL / 3V3 / GND", 12, "#fff", "middle")
    P["AS_SDA"], P["AS_SCL"] = (1080, 560), (1080, 584)
    s += box(1080, 640, 170, 60, "#adb5bd", "#111", 8, 2, "6,4") + text(1165, 676, "Endschalter (opt.)", 14, "#222", "middle", "bold")
    P["ES"] = (1080, 670)
    return s, P


def wire_set(P):
    """All connections grouped per step: step -> list of (a, b, color, label, dash)."""
    return {
        "a": [("M12", "F_in", COL["red"], "+12 V", None), ("F_out", "B_IN+", COL["red"], None, None), ("MGND", "B_IN-", COL["black"], "GND", None)],
        "b": [("B_OUT+", "E_5V", COL["orange"], "5,0 V!", None), ("B_OUT-", "E_GND", COL["black"], None, None),
              ("C1+", "E_5V", COL["orange"], None, None), ("C1-", "E_GND", COL["black"], None, None)],
        "c": [("E_GPIO7", "T_EN", COL["blue"], None, None), ("E_GPIO5", "T_STEP", COL["blue"], None, None), ("E_GPIO6", "T_DIR", COL["blue"], None, None),
              ("E_GPIO8", "T_PDN_UART", COL["teal"], "1 kΩ", None), ("E_GPIO1", "T_DIAG", COL["blue"], None, None),
              ("E_3V3", "T_VIO", COL["orange"], None, None), ("E_GND", "T_GND", COL["black"], None, None),
              ("F_out", "T_VM", COL["red"], "+12 V → VM", None), ("MGND", "T_GNDP", COL["black"], None, None), ("C2", "T_VM", COL["red"], None, None)],
        "d": [("T_1A", "MO_A1", COL["green"], None, None), ("T_1B", "MO_A2", COL["green"], None, None), ("T_2A", "MO_B1", COL["green"], None, None), ("T_2B", "MO_B2", COL["green"], None, None)],
        "e": [("H2", "E_GPIO17", COL["blue"], "Taste", None), ("H3", "E_GPIO18", COL["blue"], "Relais", None), ("H4", "E_GND", COL["black"], "GND", None)],
        "f": [("X_SCK", "E_GPIO2", COL["purple"], None, None), ("X_DT", "E_GPIO3", COL["purple"], None, None),
              ("X_VCC", "E_3V3", COL["orange"], None, None), ("X_GND", "E_GND", COL["black"], None, None)],
        "g": [("AS_SDA", "E_GPIO15", COL["brown"], None, "8,6"), ("AS_SCL", "E_GPIO16", COL["brown"], None, "8,6"), ("ES", "E_GPIO44", COL["blue"], None, "8,6")],
    }


STEP_TXT = {
    "a": ("5a · 12-V-Abgriff → Sicherung → Step-Down", "12 V tap → fuse → buck converter",
          ["Rote Litze an +12 V (DF10S + / Elko +), schwarze an GND (Elko −).", "In die rote Leitung die 1-A-Sicherung.", "Rot an IN+, schwarz an IN−. Noch NICHTS an OUT!"]),
    "b": ("5b · Step-Down → ESP (5 V)", "buck → ESP (5 V)",
          ["ERST Step-Down auf 5,0 V einstellen (Schritt 6)!", "OUT+ → ESP 5V, OUT− → ESP GND.", "1000-µF-Elko an 5V/GND. Langes Bein = +."]),
    "c": ("5c · ESP → TMC2209", "ESP → TMC2209 driver",
          ["EN=GPIO7, STEP=GPIO5, DIR=GPIO6, DIAG=GPIO1.", "GPIO8 → 1 kΩ → PDN_UART.  3V3 → VIO, GND → GND.", "MS1 + MS2 an GND. VM an +12 V (hinter Sicherung), Elko 100 µF an VM."]),
    "d": ("5d · TMC2209 → Motor", "TMC2209 → stepper motor",
          ["Spulenpaare vorher messen (Schritt 6): ~1,5–2 Ω = ein Paar.", "Paar A → 1A/1B, Paar B → 2A/2B.", "Nie bei eingeschaltetem Strom ab-/anstecken!"]),
    "e": ("5e · ESP → Mühlenstecker", "ESP → grinder harness",
          ["Pin 1 (+5 V): NICHT verbinden, isolieren.", "Pin 2 → GPIO17, Pin 3 → GPIO18, Pin 4 → GND.", "Pin 2 vorher messen: muss ≤ 3,3 V sein."]),
    "f": ("5f · ESP → Waage (HX711)", "ESP → scale (HX711)",
          ["SCK → GPIO2, DT → GPIO3.", "VCC → 3V3, GND → GND.", "Wägezelle wie im SGBW-Original an HX711."]),
    "g": ("5g · Optional: AS5600 + Endschalter", "optional: AS5600 + endstop",
          ["AS5600 SDA → GPIO15, SCL → GPIO16, 3V3/GND, DIR → GND.", "Endschalter: GPIO44 ↔ GND.", "Beides kannst du weglassen."]),
}


def img_wiring():
    order = ["a", "b", "c", "d", "e", "f", "g"]
    for i, st in enumerate(order):
        W, H = 1520, 900
        base, P = wiring_base(st)
        ws = wire_set(P)
        w = ""
        for prev in order[:i]:
            for a, b_, c, lab, dash in ws[prev]:
                w += wire(P[a], P[b_], c, False, dash=dash)
        for a, b_, c, lab, dash in ws[st]:
            w += wire(P[a], P[b_], c, True, lab, dash)
        if st == "c":
            w += "<path d='M1080,370 C1050,370 1050,420 1080,440' stroke='#222' stroke-width='4' fill='none'/><path d='M1080,400 C1060,400 1060,420 1080,440' stroke='#222' stroke-width='4' fill='none'/>" + text(1090, 452, "MS1 + MS2 → GND", 14, "#222", "start", "bold")
        if st == "e":
            w += f"<line x1='{P['H1'][0]}' y1='{P['H1'][1]}' x2='{P['H1'][0]+40}' y2='{P['H1'][1]}' stroke='#999' stroke-width='4'/>" + text(P['H1'][0] + 48, P['H1'][1] + 6, "✖ isolieren / insulate", 14, "#d00000", weight="bold")
        de, en, lines = STEP_TXT[st]
        b = title(W, "Schritt " + de, "Step " + en + "   ·   farbig = neu in diesem Schritt, grau = schon fertig / coloured = new, grey = done")
        info = box(30, 760, 1460, 120, "#f4f7fb", "#9aa", 10, 1.5)
        for k, ln in enumerate(lines):
            info += text(50, 795 + k * 30, f"{k+1}. {ln}", 19, "#222", weight="bold" if k == 0 else "normal")
        save(f"05{st}_{['abgriff_buck','buck_esp','esp_tmc','tmc_motor','esp_muehle','waage','optional'][i]}", svg(W, H, base + w + b + info))


# ---------------------------------------------------------------- 06 multimeter
def img_multimeter():
    W = 1400
    b = title(W, "Schritt 6 · Multimeter-Checks", "Vor dem Anstecken des ESP / before plugging in the ESP")
    cards = [
        ("A  Step-Down einstellen", "Buck set to 5 V", "5.05", "V⎓", "IN an 12 V, OUT offen.\nPoti drehen bis 5,00–5,10 V.", "buck"),
        ("B  Motorspulen finden", "find motor coil pairs", "1.8", "Ω", "~1,5–2 Ω = ein Paar.\nOL (offen) = andere Spule.", "motor"),
        ("C  Durchgang GND", "continuity GND", "0.2", "Ω ))", "Stecker gezogen!\nElko − ↔ Mühlenstecker Pin 4.", "cont"),
        ("D  12-V-Abgriff messen", "measure 12 V tap", "12.4", "V⎓", "Eingesteckt: NICHTS anfassen!\nErwartet 11–14 V.", "volt"),
        ("E  Pin 2 (Taste) messen", "measure pin 2", "3.2", "V⎓", "Muss ≤ 3,3 V sein.\nSonst Spannungsteiler 10k/20k.", "volt"),
    ]
    for i, (t1, t2, val, unit, desc, kind) in enumerate(cards):
        cx = 15 + i * 276
        b += box(cx, 85, 266, 520, "#f8f9fb", "#9aa", 12, 1.5)
        b += text(cx + 133, 118, t1, 17, "#222", "middle", "bold") + text(cx + 133, 142, t2, 13, "#666", "middle", style="italic")
        b += multimeter(cx + 133, 270, val, 0.95, unit)
        b += f"<path d='M{cx+112},{352} C{cx+90},420 {cx+60},430 {cx+60},470' stroke='#222' stroke-width='4' fill='none'/>"
        b += f"<path d='M{cx+154},{352} C{cx+176},420 {cx+206},430 {cx+206},470' stroke='#d62828' stroke-width='4' fill='none'/>"
        b += f"<rect x='{cx+55}' y='470' width='10' height='30' fill='#222'/><rect x='{cx+201}' y='470' width='10' height='30' fill='#d62828'/>"
        b += text(cx + 133, 540, desc, 14, "#222", "middle")
    b += warn(15, 625, 1370, "Messen bei eingesteckter Mühle (D, E): nur isolierte Messspitzen, nur 12-V/5-V-Punkte, eine Hand in der Hosentasche.\nMeasuring while plugged in (D, E): insulated probes only, only 12 V/5 V points.")
    save("06_multimeter", svg(W, 720, b))


# ---------------------------------------------------------------- 08 flasher
def img_flasher():
    W = 1200
    # 08a requirements
    b = title(W, "Schritt 8a · Was du zum Flashen brauchst / What you need to flash")
    b += box(40, 90, 340, 300, "#e9f5ff", "#1d6fd8", 14, 3) + tool_icon("pc", 210, 210) + text(210, 330, "PC / Mac / Linux", 20, "#123", "middle", "bold") + text(210, 358, "Chrome oder Edge (Desktop)", 16, "#123", "middle")
    b += box(430, 90, 340, 300, "#eaffea", "#2a9d3c", 14, 3)
    b += "<path d='M470,240 C540,140 660,340 730,240' stroke='#333' stroke-width='12' fill='none'/>" + box(450, 226, 40, 28, "#bbb", "#555", 6) + box(710, 226, 40, 28, "#bbb", "#555", 6)
    b += text(600, 330, "USB-C-DATENkabel", 20, "#174", "middle", "bold") + text(600, 358, "kein reines Ladekabel! / data cable", 15, "#174", "middle")
    b += box(820, 90, 340, 300, "#ffecec", "#d62828", 14, 3)
    for k, n in enumerate(["Firefox", "Safari", "iPhone / iPad"]):
        b += text(990, 170 + k * 55, "✖ " + n, 24, "#b00", "middle", "bold")
    b += text(990, 358, "funktioniert nicht / not supported", 15, "#b00", "middle")
    b += warn(40, 420, 1120, "Board zum Flashen NICHT in der Mühle angeschlossen lassen: nur USB, Mühle ausgesteckt.\nFlash on the desk: USB only, grinder unplugged.")
    save("08a_voraussetzungen", svg(W, 520, b))

    # 08b annotated screenshot
    shot = os.path.join(OUT, "08_flasher_seite.png")
    if os.path.exists(shot):
        data = base64.b64encode(open(shot, "rb").read()).decode()
        sw, sh = 1100, 1250  # screenshot size (headless Chrome --window-size=1100,1250)
        k = sw / 901.0          # boxes below were measured on a 901 px wide preview
        W2 = sw + 380
        b = f"<image x='0' y='0' width='{sw}' height='{sh}' xlink:href='data:image/png;base64,{data}'/>"
        anns = [(122, 143, 656, 203, "#ff9500", "①", "Warnhinweis lesen\nread the warning"),
                (124, 510, 220, 36, "#1d6fd8", "②", "Reiter „1. Installieren (USB)“\ntab install (USB)"),
                (122, 764, 656, 40, "#2a9d3c", "③", "Version: v0.1.0-beta\npick version"),
                (122, 818, 656, 38, "#d62828", "④", "„Installieren“ klicken\nclick Install")]
        anns = [(x * k, y * k, w * k, h * k, c, n, t) for x, y, w, h, c, n, t in anns]
        for x, y, w, h, c, n, t in anns:
            b += f"<rect x='{x-5}' y='{y-5}' width='{w+10}' height='{h+10}' rx='10' fill='none' stroke='{c}' stroke-width='5'/>"
            b += f"<circle cx='{x-5}' cy='{y-5}' r='18' fill='{c}'/>" + text(x - 5, y + 2, n, 20, "#fff", "middle", "bold")
            b += f"<line x1='{x+w+5}' y1='{y+h/2}' x2='{sw+20}' y2='{y+h/2}' stroke='{c}' stroke-width='3' stroke-dasharray='6,4'/>"
            b += box(sw + 20, y + h / 2 - 32, 340, 64, "#fff", c, 10, 3) + text(sw + 36, y + h / 2 - 6, n + " " + t.split("\n")[0], 17, "#222", weight="bold") + text(sw + 60, y + h / 2 + 18, t.split("\n")[1], 14, "#555", style="italic")
        save("08b_flasher_markiert", svg(W2, sh, b, "#eef"))

    # 08c port dialog (browser)
    b = title(W, "Schritt 8c · Port wählen / choose serial port", "Illustration (vereinfacht) – so sieht der Browser-Dialog ungefähr aus")
    b += box(250, 100, 700, 380, "#fff", "#888", 12, 2)
    b += text(275, 140, "dobbynator94.github.io möchte eine Verbindung", 18, "#222", weight="bold") + text(275, 164, "zu einem seriellen Port herstellen", 18, "#222", weight="bold")
    rows = [("USB JTAG/serial debug unit (COM5)  –  gekoppelt", True), ("Bluetooth-Verbindung (COM3)", False), ("Kommunikationsanschluss (COM1)", False)]
    for k, (r, sel) in enumerate(rows):
        b += box(275, 190 + k * 56, 650, 48, "#dbeafe" if sel else "#f6f6f6", "#1d6fd8" if sel else "#ddd", 6, 3 if sel else 1) + text(295, 220 + k * 56, r, 16, "#222", weight="bold" if sel else "normal")
    b += box(660, 410, 120, 44, "#1a73e8", "#1a73e8", 6) + text(720, 438, "Verbinden", 16, "#fff", "middle", "bold")
    b += box(800, 410, 120, 44, "#fff", "#999", 6) + text(860, 438, "Abbrechen", 16, "#333", "middle")
    b += text(160, 220, "①", 40, "#1d6fd8", "middle", "bold") + text(160, 252, "wählen", 14, "#1d6fd8", "middle")
    b += text(720, 510, "② „Verbinden“ / Connect", 18, "#1a73e8", "middle", "bold")
    b += text(40, 560, "Windows: „USB JTAG/serial debug unit“ oder „USB-Serial (COMx)“.  macOS: „cu.usbmodem…“.  Linux: „ttyACM0“.", 15, "#333")
    b += text(40, 588, "Kein Eintrag? → Datenkabel prüfen, anderen USB-Port, dann BOOT-Modus (Schritt 8g).", 15, "#b00", weight="bold")
    save("08c_port_dialog", svg(W, 610, b))

    # 08d install / erase dialog (ESP Web Tools)
    b = title(W, "Schritt 8d · Installieren + Löschen / Install + erase", "Illustration (vereinfacht) des ESP-Web-Tools-Dialogs")
    b += box(120, 100, 440, 400, "#fff", "#888", 14, 2) + text(340, 145, "EurekaMate", 22, "#222", "middle", "bold")
    b += box(170, 190, 340, 52, "#03a9f4", "#03a9f4", 8) + text(340, 223, "INSTALL EUREKAMATE", 17, "#fff", "middle", "bold")
    b += box(170, 260, 340, 44, "#fff", "#ccc", 8) + text(340, 288, "LOGS & CONSOLE", 15, "#555", "middle")
    b += text(340, 360, "①  hier klicken / click", 18, "#0277bd", "middle", "bold")
    b += box(640, 100, 440, 400, "#fff", "#888", 14, 2) + text(860, 145, "Install EurekaMate", 20, "#222", "middle", "bold")
    b += box(690, 190, 26, 26, "#03a9f4", "#0277bd", 4) + text(703, 210, "✔", 18, "#fff", "middle", "bold") + text(730, 210, "Erase device / Gerät löschen", 17, "#222")
    b += text(690, 260, "Do you want to install EurekaMate", 15, "#444") + text(690, 282, "0.1.0-beta? All data will be erased.", 15, "#444")
    b += box(940, 420, 110, 44, "#03a9f4", "#03a9f4", 8) + text(995, 448, "INSTALL", 15, "#fff", "middle", "bold")
    b += box(820, 420, 100, 44, "#fff", "#ccc", 8) + text(870, 448, "NEXT", 15, "#0277bd", "middle", "bold")
    b += text(860, 360, "② Haken setzen → Next → Install", 17, "#0277bd", "middle", "bold")
    b += text(40, 545, "Beim ersten Mal: „Erase device“ anhaken (alte Firmware weg). Bei Updates per USB: Haken weg = Einstellungen bleiben.", 15, "#333")
    b += text(40, 572, "First time: tick “Erase device”. Re-flash via USB: untick to keep your settings.", 14, "#555", style="italic")
    save("08d_installieren_loeschen", svg(W, 600, b))

    # 08e progress, 08f done
    b = title(W, "Schritt 8e · Warten / wait (ca. 1–2 Minuten)", "Illustration (vereinfacht)")
    b += box(300, 100, 600, 300, "#fff", "#888", 14, 2) + text(600, 150, "Installing", 22, "#222", "middle", "bold")
    b += box(360, 200, 480, 26, "#e0e0e0", "#e0e0e0", 13) + box(360, 200, 300, 26, "#03a9f4", "#03a9f4", 13)
    b += text(600, 270, "Writing progress: 62 %", 18, "#333", "middle") + text(600, 300, "This will take a minute.", 15, "#777", "middle")
    b += text(600, 360, "Kabel NICHT abziehen · Tab offen lassen", 18, "#d00000", "middle", "bold")
    b += text(600, 450, "Zuerst „Preparing / Erasing“, dann „Writing“. Don't unplug, keep the tab open.", 15, "#333", "middle")
    save("08e_fortschritt", svg(W, 480, b))
    b = title(W, "Schritt 8f · Fertig! / Done!", "Illustration (vereinfacht)")
    b += box(120, 100, 480, 320, "#fff", "#888", 14, 2) + text(360, 150, "Installation complete!", 22, "#222", "middle", "bold")
    b += "<circle cx='360' cy='250' r='55' fill='#2a9d3c'/>" + text(360, 272, "✔", 64, "#fff", "middle", "bold")
    b += box(300, 340, 120, 44, "#03a9f4", "#03a9f4", 8) + text(360, 368, "NEXT", 15, "#fff", "middle", "bold")
    b += icon("esp", 900, 250)
    b += f"<rect x='842' y='212' width='116' height='70' fill='#000'/>" + text(900, 245, "EurekaMate", 18, "#ff9500", "middle", "bold") + text(900, 268, "v0.1.0-beta", 12, "#ccc", "middle")
    b += text(900, 345, "Display zeigt kurz das", 16, "#222", "middle") + text(900, 368, "EurekaMate-Logo → Waage", 16, "#222", "middle", "bold")
    b += text(40, 455, "Startet es nicht: einmal RESET drücken oder USB kurz ab-/anstecken. If it doesn't start: press RESET once.", 15, "#333")
    save("08f_fertig", svg(W, 480, b))

    # 08g BOOT/RESET
    b = title(W, "Schritt 8g · Board wird nicht erkannt? → BOOT-Modus", "Board not detected? → enter download (boot) mode")
    for k, (lab, boot, rst) in enumerate([("1. BOOT gedrückt HALTEN", True, False), ("2. RESET kurz drücken", True, True), ("3. BOOT loslassen", False, False)]):
        cx = 60 + k * 380
        b += box(cx, 90, 340, 330, "#f8f9fb", "#9aa", 12, 1.5) + text(cx + 170, 125, lab, 19, "#222", "middle", "bold")
        b += box(cx + 70, 150, 200, 200, "#111", "#000", 18) + box(cx + 85, 165, 170, 130, "#0b3d91", "#000", 6) + box(cx + 150, 345, 40, 16, "#999", "#555", 3)
        b += box(cx + 35, 190, 35, 40, "#ffd60a" if boot else "#777", "#222", 6) + text(cx + 30, 185, "BOOT", 13, "#222", "end", "bold")
        b += box(cx + 35, 260, 35, 40, "#d62828" if rst else "#777", "#222", 6) + text(cx + 30, 255, "RESET", 13, "#222", "end", "bold")
        if boot:
            b += text(cx + 52, 218, "👇", 22, "#000", "middle")
        if rst:
            b += text(cx + 52, 288, "👆", 22, "#000", "middle")
        b += text(cx + 170, 395, ["(Taste gedrückt lassen)", "(BOOT weiter halten!)", "→ Installieren nochmal klicken"][k], 15, "#555", "middle")
    b += text(60, 460, "Die Tasten sitzen seitlich bzw. hinten am Waveshare-Board (Beschriftung BOOT / RST auf der Platine prüfen).", 15, "#333")
    b += text(60, 488, "Nach dem Flashen im BOOT-Modus einmal RESET drücken, damit die neue Firmware startet.", 15, "#333", weight="bold")
    b += text(60, 516, "EN: hold BOOT, tap RESET, release BOOT, click Install again. Press RESET after flashing.", 14, "#555", style="italic")
    save("08g_boot_reset", svg(W, 540, b))


# ---------------------------------------------------------------- 09 hotspot
def img_hotspot():
    W = 1200
    b = title(W, "Schritt 9a · Mit dem Hotspot verbinden / connect to the hotspot")
    # phone
    b += box(90, 90, 280, 520, "#111", "#000", 36) + box(105, 130, 250, 440, "#fff", "#000", 8)
    b += text(125, 165, "WLAN / Wi-Fi", 20, "#222", weight="bold")
    for k, (n, sel) in enumerate([("EurekaMate-Setup", True), ("FritzBox-1234", False), ("Nachbar_5G", False)]):
        b += box(115, 185 + k * 60, 230, 50, "#dbeafe" if sel else "#f4f4f4", "#1d6fd8" if sel else "#ddd", 8, 3 if sel else 1) + text(130, 216 + k * 60, ("📶 " if True else "") + n, 15, "#222", weight="bold" if sel else "normal")
    b += box(115, 390, 230, 46, "#f4f4f4", "#999", 6) + text(130, 419, "Passwort: ••••••••", 15, "#444")
    b += text(230, 480, "Seite öffnet sich", 15, "#222", "middle", "bold") + text(230, 502, "sonst: 192.168.4.1", 15, "#1d6fd8", "middle", "bold")
    # arrow
    b += "<path d='M400,350 L520,350' stroke='#2a9d3c' stroke-width='8'/><polygon points='520,335 550,350 520,365' fill='#2a9d3c'/>"
    # display
    b += icon("esp", 700, 230)
    b += "<rect x='642' y='192' width='116' height='70' fill='#000'/>" + text(700, 212, "Network", 12, "#ff9500", "middle", "bold") + text(700, 232, "EurekaMate-Setup", 10, "#fff", "middle") + text(700, 250, "PW: 8 Zeichen", 10, "#ccc", "middle")
    b += text(700, 330, "Display: Menü → Grind Sync → Network", 15, "#222", "middle", "bold") + text(700, 352, "zeigt Hotspot-Name + 8-stelliges Passwort", 14, "#555", "middle")
    b += box(850, 120, 320, 300, "#f4f7fb", "#9aa", 10, 1.5)
    for k, t in enumerate(["1. Mühle einschalten", "2. Handy: WLAN", "   „EurekaMate-Setup“", "3. Passwort vom Display", "4. Reiter „WLAN & GaggiMate“", "5. Heim-WLAN wählen,", "   speichern", "6. Danach: eurekamate.local"]):
        b += text(870, 155 + k * 32, t, 16, "#222", weight="bold" if t[0].isdigit() else "normal")
    b += text(40, 650, "Nur 2,4-GHz-WLAN. / 2.4 GHz Wi-Fi only.  Hotspot erscheint nur, wenn noch kein WLAN gespeichert ist.", 15, "#333")
    save("09a_hotspot", svg(W, 680, b))


# ---------------------------------------------------------------- 10 gaggimate
def img_gaggimate():
    W = 1200
    b = title(W, "Schritt 10 · GaggiMate verbinden / connect GaggiMate", "Illustration (vereinfacht, nicht die echte GaggiMate-Oberfläche) · GaggiMate ≥ v1.10.0")
    b += box(40, 100, 540, 440, "#1e1e2e", "#000", 14) + text(70, 140, "GaggiMate · Settings → Plugins", 18, "#fff", weight="bold")
    b += box(70, 165, 480, 60, "#2b2d42", "#444", 8) + box(90, 182, 50, 26, "#2a9d3c", "#2a9d3c", 13) + "<circle cx='127' cy='195' r='10' fill='#fff'/>"
    b += text(155, 201, "Mahlkönig E64 WS Grind-by-Sync", 16, "#fff")
    for k in range(6):
        b += box(70, 245 + k * 42, 480, 34, "#2b2d42", "#444", 6) + text(85, 268 + k * 42, f"Rezept {k+1}", 14, "#ccc") + text(530, 268 + k * 42, ["Espresso 18 g", "Double", "Decaf", "Filter", "–", "–"][k] + "  ▾", 14, "#fff", "end")
    b += text(310, 520, "① aktivieren  ② Profile zuordnen  ③ speichern + neu starten", 14, "#ffd60a", "middle", "bold")
    b += "<path d='M600,320 L660,320' stroke='#2a9d3c' stroke-width='8'/><polygon points='660,305 690,320 660,335' fill='#2a9d3c'/>"
    b += box(710, 100, 450, 440, "#fff", "#888", 14, 2) + text(735, 140, "EurekaMate · WLAN & GaggiMate", 18, "#222", weight="bold")
    b += box(735, 170, 400, 40, "#f4f4f4", "#bbb", 6) + text(750, 197, "GaggiMate: 192.168.178.50", 15, "#333")
    b += box(735, 225, 190, 42, "#667eea", "#667eea", 6) + text(830, 252, "Suchen (mDNS)", 15, "#fff", "middle", "bold")
    b += box(945, 225, 190, 42, "#667eea", "#667eea", 6) + text(1040, 252, "Verbindung testen", 15, "#fff", "middle", "bold")
    b += box(735, 290, 400, 50, "#eaffea", "#2a9d3c", 8, 2) + text(935, 322, "✔ GaggiMate Grind-by-Sync OK", 17, "#174", "middle", "bold")
    b += text(735, 390, "Tipp: Im Router für GaggiMate eine", 15, "#333") + text(735, 412, "feste IP reservieren.", 15, "#333", weight="bold")
    b += text(735, 460, "Beide Geräte im gleichen 2,4-GHz-WLAN.", 15, "#333")
    save("10_gaggimate", svg(W, 570, b))


# ---------------------------------------------------------------- 11 first shot + troubleshooting
def img_first_shot():
    W = 1400
    b = title(W, "Schritt 11 · Erster Shot / first shot")
    steps = [("Rezept wählen", "pick recipe", "#667eea"), ("Motor fährt\n(Balken)", "dial moves", "#1d6fd8"), ("Mahlen\n(nach Gewicht)", "grind by weight", "#f77f00"),
             ("Shot an GaggiMate", "pull shot", "#7b2cbf"), ("Zeit auswerten", "shot time check", "#2a9d3c"), ("Mahlgrad\nautomatisch ±", "auto adjust", "#d62828")]
    for k, (de, en, c) in enumerate(steps):
        cx = 30 + k * 228
        b += box(cx, 110, 200, 150, c, c, 16) + text(cx + 100, 160, de, 18, "#fff", "middle", "bold") + text(cx + 100, 230, en, 13, "#eee", "middle", style="italic")
        b += text(cx + 20, 135, str(k + 1), 18, "#fff", weight="bold")
        if k < 5:
            b += f"<polygon points='{cx+204},175 {cx+224},185 {cx+204},195' fill='#555'/>"
    b += "<path d='M1260,270 C1260,340 130,340 130,270' stroke='#999' stroke-width='4' fill='none' stroke-dasharray='8,6'/>" + text(700, 355, "nächster Shot / next shot", 15, "#777", "middle")
    b += box(30, 390, 660, 170, "#fff4e5", "#ff9500", 10, 2)
    for k, t in enumerate(["Während der Motor fährt, startet die Mühle NICHT.", "Während die Mühle mahlt, bewegt sich das Rad NIE.", "Bezug zu lang → gröber · zu kurz → feiner.", "Korrektur nur bei gültigem Shot (15 s … Ziel+15 s)."]):
        b += text(50, 425 + k * 34, "• " + t, 16, "#7a4100", weight="bold" if k < 2 else "normal")
    b += box(710, 390, 660, 170, "#f4f7fb", "#9aa", 10, 2)
    for k, t in enumerate(["Kein Hotspot → Display: Network → Setup hotspot", "„GaggiMate not found“ → Plugin an? IP eintragen", "Motor brummt → Spulenpaare tauschen/messen", "Falsche Richtung → Motor → Richtung umkehren"]):
        b += text(730, 425 + k * 34, "🔧 " + t, 16, "#222")
    save("11_erster_shot", svg(W, 590, b))


if __name__ == "__main__":
    img_shopping(); img_tools(); img_unplug(); img_wiring(); img_multimeter(); img_flasher(); img_hotspot(); img_gaggimate(); img_first_shot()
