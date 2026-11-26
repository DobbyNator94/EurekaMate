# EurekaMate – Step-by-step build guide 🛠️

> [!CAUTION]
> **🇩🇪 ACHTUNG – BITTE ZUERST LESEN:** Dieses Projekt ist **komplett „vibe-coded“ (KI-generiert)** und
> **komplett ungetestet** – weder die Firmware noch die Hardware, die Pläne oder die Anleitungen wurden an einer
> echten Mühle ausprobiert. Es gibt **keinerlei Garantie, Gewährleistung oder Haftung für irgendetwas**.
> **Jeder ist allein für sich selbst verantwortlich.** Arbeiten an **230-V-Geräten ist gefährlich** und kann
> tödlich sein. **Wenn du nicht genau weißt, was du tust, lass es lieber ganz bleiben.**
>
> **🇬🇧 WARNING – READ THIS FIRST:** This project is **entirely vibe-coded (AI-generated)** and **completely
> untested** – none of the firmware, hardware, schematics or guides have been tried on a real grinder. There is
> **no guarantee, warranty or liability for anything whatsoever**. **Everyone is solely responsible for
> themselves.** Working on **230 V devices is dangerous** and can kill you. **If you don't know exactly what
> you're doing, better don't do anything at all.**

🇩🇪 Deutsche Version: **[BAUANLEITUNG.md](BAUANLEITUNG.md)**

This guide is for beginners. Short sentences, one image per step. Do the steps **in order**.
(The images are shared with the German guide and carry German + English labels.)

> ⚠️ **BETA:** Firmware **v0.1.0-beta** has **not yet been tested on a real grinder**.
> Build at your own risk. Measure everything.

> 🔴 **230 V – danger to life!**
> The grinder contains 230 V. **Unplug it every time before opening it.** Switching off is not enough.
> We work **only** on the low-voltage side (12 V / 5 V). Never solder to 230 V parts.

**Links:** [⚡ Web flasher](https://dobbynator94.github.io/EurekaMate/) ·
[📦 Release v0.1.0-beta](https://github.com/DobbyNator94/EurekaMate/releases/tag/v0.1.0-beta) ·
[📘 Detailed guide](ANLEITUNG_EN.md) · [🛒 Shopping list](SHOPPING_LIST_EN.md) · [🔌 Schematic](img/schaltplan.png)

## Overview

| Step | What | Time |
|---|---|---|
| [1](#1-what-you-need) | What you need | – |
| [2](#2-what-you-print) | What you print | ~10 h printer |
| [3](#3-tools) | Tools | – |
| [4](#4-open-the-grinder-safely) | Open the grinder safely | 15 min |
| [5](#5-connect-the-wires--one-at-a-time) | Connect the wires | 1–2 h |
| [6](#6-multimeter-checks) | Multimeter checks | 20 min |
| [7](#7-mount-the-bracket) | Mount the bracket | 30 min |
| [8](#8-flash-the-firmware-with-the-web-flasher) | Flash the firmware (web flasher) | 5 min |
| [9](#9-first-setup) | First setup | 15 min |
| [10](#10-connect-gaggimate) | Connect GaggiMate | 10 min |
| [11](#11-first-shot--troubleshooting) | First shot + troubleshooting | – |

> 💡 **Tip:** Do step 8 (flashing) **first**, on your desk. Then you know the board works before you solder.

---

## 1. What you need

![Shopping list](img/bauanleitung/01_einkaufsliste.png)

| # | Part | Qty | Price (as of 26 Nov 2026) |
|---|---|---|---|
| 1 | Waveshare **ESP32-S3-Touch-AMOLED-1.64** | 1 | €33.99 |
| 2 | Load cell **T70 1 kg** + **HX711** module | 1 | €28.53 + €5.49 |
| 3 | **NEMA17** stepper 17HS4417P1 / 17HS4401, 40 mm (for AS5600: dual shaft 17HS8401B) | 1 | €6.99 (€14.58) |
| 4 | **TMC2209** driver with heatsink | 1 | €8.49 |
| 5 | **5 V buck converter** MP1584 (set to 5.0 V) | 1 | €5.99 (5-pack) |
| 6 | Capacitor **100–470 µF / 25 V** (motor driver, e.g. 220 µF) | 1 | €3.45 (24-pack) |
| 7 | Capacitor **1000 µF / ≥10 V** (ESP) | 1 | €4.59 (10-pack) |
| 8 | Resistor **1 kΩ** | 1 | €5.99 (assortment) |
| 9 | **1 A fast fuse** + inline holder | 1 | €3.89 + €6.49 |
| 10 | Gear dial kit for Eureka Mignon (ARO) | 1 | €42.00 |
| 11 | 22 AWG wire, 4-pin JST-PH/Dupont, heat shrink, cable ties | – | €37.24 (sets) |
| 12 | M3 screws (4× 6 mm motor, 4× 8 mm lid, 2× 10 mm + washers, 1× 30 mm + nut, M3×4 grub screw + nut) + 6× M3×5.7 heat-set inserts | – | €30.18 (if not on hand) |
| 13 | **PETG** + **TPU 95A** filament | – | – |
| opt. | AS5600 + 6×2.5 mm magnet · endstop · 12 V/2 A power supply | – | €51.13 (all) |

🛒 **Full shopping list with verified Amazon links, shipping and totals: [SHOPPING_LIST_EN.md](SHOPPING_LIST_EN.md)**
(required ≈ €193 without / ≈ €223 with screws & inserts, as of 26 Nov 2026)

You also need the **scale and display parts from smart-grind-by-weight** (see [DOC.md](DOC.md)).

---

## 2. What you print

> ⚠️ **Measure first, then print!** Adjust every `TODO-MEASURE` value in
> [`params.scad`](../hardware/stepper-mount/scad/params.scad) to your grinder and kit.
> Then run `./export.sh` in `hardware/stepper-mount`.

All parts are in [`hardware/stepper-mount/stl/`](../hardware/stepper-mount/stl/). The images show each part **as it goes on the print bed**.

| Part | Image (bed orientation) | Qty | Material | Settings |
|---|---|---|---|---|
| **Pinion** `pinion.stl` | ![](img/bauanleitung/02_druck_pinion.png) | 1 | PETG | 0.12–0.16 mm layers, 100 % infill, **teeth down**. **Print first** and test the fit! |
| **Motor housing** `motor_housing.stl` | ![](img/bauanleitung/02_druck_motor_housing.png) | 1 | PETG | 0.2 mm, 4 walls, 30–40 % gyroid, **no supports** |
| **Lid** `housing_lid.stl` | ![](img/bauanleitung/02_druck_housing_lid.png) | 1 | PETG | 0.2 mm, flat face down (STL already flipped), no supports |
| **Side brace** `side_brace.stl` | ![](img/bauanleitung/02_druck_side_brace.png) | 1 | PETG | 0.2 mm, 4 walls, 40 %, upright as shown, **tree supports under the top arm and the shelf** |
| **Strap segment** `strap_segment.stl` | ![](img/bauanleitung/02_druck_strap_segment.png) | **4** | TPU 95A (or PETG with `strap_t = 1.2`) | flat, 0.2 mm, slow (20–30 mm/s for TPU) |
| **Tension slider** `strap_tension_slider.stl` | ![](img/bauanleitung/02_druck_strap_tension_slider.png) | 1 | PETG | 0.2 mm, 100 % |
| **Magnet cap** `magnet_cap.stl` (optional) | ![](img/bauanleitung/02_druck_magnet_cap.png) | 1 | PETG | only with AS5600 and a dual-shaft motor |

Order: **pinion → side brace → housing → lid → strap.**

---

## 3. Tools

![Tools](img/bauanleitung/03_werkzeug.png)

- Soldering iron (≥ 60 W), solder
- **Multimeter** (V DC, Ω, continuity)
- Side cutters, wire stripper
- Phillips screwdriver, hex keys, small flat screwdriver (for the buck converter's trimmer)
- Calipers
- Heat shrink + heat gun or lighter, zip ties
- 3D printer
- PC/Mac with **Chrome or Edge** and a **USB-C data cable**

---

## 4. Open the grinder safely

![Unplug](img/bauanleitung/04_stecker_ziehen.png)

> 🔴 **230 V!** Unplug the grinder. **Wait 2 minutes** (the motor capacitor may still be charged).

1. **Pull the mains plug.** Really pull it, don't just switch off.
2. Remove the hopper. Lay the grinder on a towel.
3. Open the grinder from below, as described in the SGBW original ([DOC.md](DOC.md)).
4. Find the power board: small **transformer**, black **DF10S** (4 legs, “+” and “−”), **capacitor** next to it, **78L05** (3 legs).
5. Take a **photo** before you unplug anything, so you know how it was.
6. Do not touch anything connected to the mains cable, the motor or the relay.

---

## 5. Connect the wires – one at a time

> 🔴 **The grinder stays unplugged** while you solder.
> Coloured = **new** in this step. Grey = already done.

Full plan: [schematic](img/schaltplan.png) · table: [ANLEITUNG_EN.md § 3](ANLEITUNG_EN.md#wiring-table)

### 5a. 12 V tap → fuse → buck converter

![5a](img/bauanleitung/05a_abgriff_buck.png)

1. Solder a red wire to **+12 V** (DF10S “+” or capacitor “+”).
2. Solder a black wire to **GND** (capacitor “−”).
3. Put the **1 A fuse** in the red wire.
4. Red to the buck's **IN+**, black to **IN−**.
5. Do not connect anything to **OUT** yet!

### 5b. Buck converter → ESP (5 V)

![5b](img/bauanleitung/05b_buck_esp.png)

> ⚠️ **Do check 6 A first:** set the buck converter to **5.0 V**. Too much voltage destroys the ESP.

1. **OUT+** → ESP **5V**.
2. **OUT−** → ESP **GND**.
3. **1000 µF capacitor** across 5V and GND. Long leg = **+**. The stripe = **−**.

### 5c. ESP → TMC2209

![5c](img/bauanleitung/05c_esp_tmc.png)

1. GPIO7 → **EN**, GPIO5 → **STEP**, GPIO6 → **DIR**, GPIO1 → **DIAG**.
2. GPIO8 → **1 kΩ resistor** → **PDN_UART**.
3. ESP 3V3 → **VIO**, ESP GND → **GND**.
4. **MS1** and **MS2** to GND.
5. **VM** to +12 V (after the fuse), power GND to GND.
6. **100 µF capacitor** directly across VM/GND. Mind the polarity!
7. Stick the heatsink on the TMC2209.

### 5d. TMC2209 → motor

![5d](img/bauanleitung/05d_tmc_motor.png)

1. **Measure the coil pairs** first (check 6 B).
2. Pair A → **1A / 1B**. Pair B → **2A / 2B**.
3. **Never** plug or unplug the motor while powered. That kills the driver.

### 5e. ESP → grinder harness

![5e](img/bauanleitung/05e_esp_muehle.png)

1. **Pin 1 (+5 V): do not connect!** Cut it and insulate it.
2. **Pin 2** (button) → GPIO17. Measure it first (check 6 E)!
3. **Pin 3** (relay) → GPIO18.
4. **Pin 4** (GND) → ESP GND.

### 5f. ESP → scale (HX711)

![5f](img/bauanleitung/05f_waage.png)

1. HX711 **SCK** → GPIO2, **DT** → GPIO3.
2. **VCC** → 3V3, **GND** → GND.
3. Wire the load cell to the HX711 as in the SGBW original.

### 5g. Optional: AS5600 and endstop

![5g](img/bauanleitung/05g_optional.png)

1. AS5600: SDA → GPIO15, SCL → GPIO16, VCC → 3V3, GND → GND, **DIR → GND**.
2. Endstop: GPIO44 ↔ GND.
3. You can leave both out.

> 💡 Insulate every joint with **heat shrink**. Secure the wires with **zip ties**, away from 230 V parts and the grinder motor.

---

## 6. Multimeter checks

![Multimeter](img/bauanleitung/06_multimeter.png)

| Check | How | Expected | If not |
|---|---|---|---|
| **A** Buck converter | IN on 12 V, OUT open, turn the trimmer | **5.00–5.10 V** | keep turning; swap for a Mini560 |
| **B** Motor coils | Ω between motor wires | **~1.5–2 Ω** = one pair; OL = other coil | – |
| **C** GND continuity | unplugged: capacitor − ↔ harness pin 4 | beeps | find another GND point |
| **D** 12 V tap | plugged in, touch nothing | **11–14 V DC** | post a photo in an issue |
| **E** Pin 2 (button) | plugged in, button not pressed | **≤ 3.3 V** | 10 kΩ/20 kΩ divider, or leave the button out |

> 🔴 You measure **D and E** with the grinder plugged in. Insulated probes **only**, **only** on the 12 V/5 V points. **Unplug again** afterwards.

---

## 7. Mount the bracket

<table><tr>
<td><img src="img/bauanleitung/07_montage_front.png" alt="front"></td>
<td><img src="img/bauanleitung/07_montage_rechts.png" alt="right"></td>
</tr><tr>
<td><img src="img/bauanleitung/07_montage_hinten.png" alt="rear"></td>
<td><img src="img/bauanleitung/07_montage_oben.png" alt="top"></td>
</tr><tr><td colspan="2" align="center"><sub>Fully assembled: front, right, rear, top (OpenSCAD render)</sub></td></tr></table>

**Exploded view** – how everything fits together:

![Exploded](img/bauanleitung/07_montage_explosion.png)

| | Step |
|---|---|
| ![](img/bauanleitung/07a_halter.png) | **7a.** Slide the side brace on from **below** along the **right** side until the top arm rests on the edge. |
| ![](img/bauanleitung/07b_band.png) | **7b.** Put the strap (4 segments) once around the grinder. Hook it in at the front and into the tension slider at the back. Tighten the M3×30 – firm, not brutal. |
| ![](img/bauanleitung/07c_gehaeuse.png) | **7c.** Motor into the housing with 4× **M3×6** (no longer – the motor thread is only ~4.5 mm deep) (shaft down). Push on the pinion, grub screw on the flat. Fit the electronics. Mount the housing on the posts with 2× M3×10 + washers. |
| ![](img/bauanleitung/07d_deckel.png) | **7d.** **Set the gear mesh:** slide the housing along the slots toward the knob until the gears mesh. Back off a hair (one sheet of paper). Tighten. Close the lid with 4× **M3×8** (into the heat-set inserts). |

✅ **Test:** grinder off. Turn the dial by hand once over the full range. It must **not bind anywhere**.

---

## 8. Flash the firmware with the web flasher

No programming needed. Everything happens in the browser.

👉 **Web flasher: https://dobbynator94.github.io/EurekaMate/**

> ⚠️ **For flashing:** connect the board on your desk **via USB only**. The grinder is **unplugged**.

### 8a. What you need

![Requirements](img/bauanleitung/08a_voraussetzungen.png)

- A **PC, Mac or Linux computer**.
- **Google Chrome** or **Microsoft Edge** (desktop version).
  Firefox, Safari, iPhone and iPad do **not** work (no Web Serial).
- A **USB-C data cable**. Many cheap cables only charge! If no port shows up, swap the cable first.

### 8b. Open the flasher and click “Install”

![Flasher page](img/bauanleitung/08b_flasher_markiert.png)

1. Plug the board into the PC with USB-C.
2. Open **https://dobbynator94.github.io/EurekaMate/** in Chrome/Edge.
3. ① Read the warning.
4. ② The tab **“1. Installieren (USB)”** is already selected.
5. ③ Pick version **v0.1.0-beta** (“show beta releases” is ticked).
6. ④ Click **“⚡ Installieren / Install (USB)”**.

### 8c. Choose the port in the browser dialog

![Port dialog](img/bauanleitung/08c_port_dialog.png)

1. The browser asks for a **serial port**.
2. Pick the board:
   - Windows: “USB JTAG/serial debug unit (COMx)” or “USB-Serial (COMx)”
   - macOS: “cu.usbmodem…”
   - Linux: “ttyACM0”
3. Click **“Connect”**.

> 💡 Not sure which port? Unplug the board – the entry that disappears is the one.

### 8d. Install and erase

![Install](img/bauanleitung/08d_installieren_loeschen.png)

1. In the EurekaMate window click **“INSTALL EUREKAMATE”**.
2. **First time:** tick **“Erase device”**. This removes the old firmware.
3. Click **“Next”**, then **“Install”**.

> ℹ️ Re-flashing via USB later and keeping your settings? Then **leave the box unticked**.

### 8e. Wait

![Progress](img/bauanleitung/08e_fortschritt.png)

1. First “Preparing / Erasing”, then “Writing” with a percentage.
2. Takes **1–2 minutes**.
3. **Don't unplug the cable. Don't close the tab.**

### 8f. Done!

![Done](img/bauanleitung/08f_fertig.png)

1. “Installation complete!” appears. Click **“Next”**.
2. The display briefly shows the **EurekaMate logo**, then the scale.
3. Nothing happens? Press **RESET** once or unplug/replug USB.

### 8g. Board not detected? → boot mode

![BOOT/RESET](img/bauanleitung/08g_boot_reset.png)

1. Press and **hold BOOT**.
2. Tap **RESET**.
3. Release **BOOT**.
4. Click **“Install”** in the flasher again and pick the port.
5. Press **RESET** once after flashing.

### 8h. Drivers and problems

| Problem | Fix |
|---|---|
| No port in the dialog | **Another USB-C cable** (data cable!), another USB port, no hub, then boot mode (8g) |
| Windows: unknown device | The ESP32-S3 has native USB. Windows 10/11 needs **no driver**. Otherwise run Windows Update, remove the device in Device Manager, replug |
| Linux: “Permission denied” | Add your user to `dialout` (or `uucp`): `sudo usermod -aG dialout $USER`, then log out and in |
| macOS asks for access | Allow it. No drivers needed |
| “Browser not supported” | Use Chrome or Edge on a computer (not a phone, not Firefox/Safari) |
| “Failed to connect” / timeout | Close other programs using the port (Arduino, PlatformIO monitor, Cura). Boot mode (8g) |
| Stops halfway | Swap cable/port, retry with “Erase device” |
| Display stays black | Press RESET. Re-flash with “Erase device” |

**Later updates:** without a cable via the web UI (*Update* tab) or in the flasher's **“Update (Bluetooth)”** tab.

<details><summary>For experts: without a browser, using esptool</summary>

Files from the [release](https://github.com/DobbyNator94/EurekaMate/releases/tag/v0.1.0-beta):

```bash
pip install esptool
esptool --chip esp32s3 write-flash 0x0 eurekamate-v0.1.0-beta-full.bin
```
</details>

---

## 9. First setup

### 9a. Hotspot

![Hotspot](img/bauanleitung/09a_hotspot.png)

1. Switch the grinder on. With no WiFi stored, the **`EurekaMate-Setup`** hotspot starts.
2. The **8-character password** is on the display: *Menu → Grind Sync → Network*.
3. Connect your phone. The page opens by itself (otherwise **http://192.168.4.1**).
4. Tab **“WLAN & GaggiMate”** → *scan* → your WiFi (**2.4 GHz**) → password → *save*.
5. From now on: **http://eurekamate.local** (or the IP on the display).

![Web UI network](img/ui/web_net.png)

### 9b. Check the motor

![Motor tab](img/ui/web_motor.png)

1. Tab **Motor**. Start values: run current 400 mA, boost 600 mA, hold current 0 %.
2. Jog a few steps. Wrong direction? → **Reverse direction**.

### 9c. Calibration 0 / 100

> ⚠️ The grinder stays **OFF** during calibration. Do not grind!

<table><tr>
<td width="40%"><img src="img/ui/display_calibration.png" alt="Display calibration"></td>
<td width="60%"><img src="img/ui/web_dial.png" alt="Web calibration"></td>
</tr></table>

1. Tab **Mahlgrad** (dial) → *start calibration* (or *Dial Calibration* on the display).
2. Jog **finer** until the burrs **just touch** (faint scraping) → **“set 0”**.
3. Go to the **coarsest** point you need → **“set 100”**.
4. Keep the safety margin (5). The motor never goes below it.
5. **Done.** The display now shows 0–100.

### 9d. Create recipes

![Recipes](img/ui/web_rec.png)

Example: *Double* – GbS on, 28 s, 36 g, start grind 15. *Filter* – GbS off, grind 60.

---

## 10. Connect GaggiMate

![GaggiMate](img/bauanleitung/10_gaggimate.png)

1. GaggiMate must be **v1.10.0 or newer**.
2. In GaggiMate: *Settings → Plugins → “Mahlkönig E64 WS Grind-by-Sync”* → **enable**.
3. Assign a profile to each recipe slot (1–6). Save. **Restart.**
4. Reserve a **fixed IP** for GaggiMate in your router.
5. In EurekaMate: tab **WLAN & GaggiMate** → *Search (mDNS)* or enter the IP → **test connection**.
6. You must see **“GaggiMate Grind-by-Sync OK”**.

(The image is a simplified illustration, not the real GaggiMate UI.)

---

## 11. First shot + troubleshooting

![First shot](img/bauanleitung/11_erster_shot.png)

1. Tap a recipe. The motor moves (progress bar). **Wait until the bar is gone.**
2. Grind. The scale stops at the target dose.
3. Pull the shot on GaggiMate.
4. EurekaMate checks the shot time: **too long → coarser**, **too short → finer**.
5. Next shot. After 2–3 shots the time should be right.
6. The **history** tab shows every correction and its reason.

![Status](img/ui/web_status.png)

> 🛡️ **Safety:** While the motor moves, the grinder does **not** start. While the grinder runs, the dial **never** moves.

### 12 V load test (important!)

Keep the multimeter on the 12 V tap. Move very fine ↔ very coarse a few times and grind once. The voltage must stay **≥ 10 V** and the display must not reboot. Otherwise use an external **12 V/2 A power supply** (see [ANLEITUNG_EN.md § 9](ANLEITUNG_EN.md#9-12-v-rail-load-test-important)).

### Troubleshooting

| Problem | Fix |
|---|---|
| No hotspot | WiFi already stored → display *Network → Setup hotspot* |
| “GaggiMate not found” | Plugin on? Same 2.4 GHz WiFi? Enter the IP instead of `gaggimate.local` |
| “no Grind-by-Sync endpoint” | GaggiMate < v1.10.0 or plugin off |
| Motor hums, doesn't turn | Wrong coil pairs → measure again. Raise boost current |
| Motor turns the wrong way | *Motor → Reverse direction*, recalibrate |
| Motor stops with “Stall” | Lower the StallGuard value or set 0 |
| Dial too stiff | More boost, smaller pinion (10 T), 5:1 geared motor |
| Grind not corrected | GbS off in the recipe? Shot < 15 s or aborted? → history tab |
| TMC gets hot | Heatsink, lower run current, hold current 0 % |
| Display reboots while moving | 12 V rail too weak → power supply fallback |

More: [ANLEITUNG_EN.md](ANLEITUNG_EN.md) · [TROUBLESHOOTING.md](TROUBLESHOOTING.md) · [open an issue](https://github.com/DobbyNator94/EurekaMate/issues)

---

<sub>Images generated with `tools/gen_bauanleitung_images.py` (illustrations), OpenSCAD (renders) and headless Chrome (flasher screenshot).</sub>
