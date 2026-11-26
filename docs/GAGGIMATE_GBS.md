# EurekaMate – Grind-by-Sync with GaggiMate + motorized grind dial

EurekaMate extends [smart-grind-by-weight](https://github.com/jaapp/smart-grind-by-weight)
by **jaapp** (GPLv3 / CERN-OHL-S v2). It adds two things:

1. A **Grind-by-Sync (GbS) client**. The grinder presents itself to
   [GaggiMate](https://github.com/jniebuhr/gaggimate) (≥ v1.10.0) **as a Mahlkönig E64 WS
   grinder**. GaggiMate plays the "Xenia" espresso machine the E64 WS talks to, so GaggiMate
   needs no changes; you only enable its plugin.
2. A **motorized grind dial** (NEMA17 + TMC2209 on the gear-knob kit of the Eureka Mignon
   Specialità). It travels between recipes in seconds, and after each shot it corrects the
   grind automatically from the shot time, like the E64 WS does.

> No GaggiMate source code is used. The client was written from the observable HTTP
> behaviour (endpoints, JSON fields and timing). Mahlkönig, E64 WS, Grind-by-Sync and Xenia
> are trademarks of their owners; this project is not affiliated with them.

German step-by-step guide for beginners: **[ANLEITUNG_DE.md](ANLEITUNG_DE.md)**.
Wiring schematic: **[img/schaltplan.png](img/schaltplan.png)** ([SVG](img/schaltplan.svg)).
Mechanics: **[../hardware/stepper-mount/](../hardware/stepper-mount/README.md)**.

---

## 1. Setup in short

1. GaggiMate web UI → **Settings → Plugins → "Enable Mahlkönig E64 WS Grind-by-Sync"**. In the
   same card, map a profile to each recipe slot. Save and restart. Reserve a fixed IP for
   GaggiMate in your router (2.4 GHz WiFi).
2. Flash EurekaMate (USB the first time, then OTA via the web UI).
3. On first boot, with no WiFi stored, the grinder opens the access point **`EurekaMate-Setup`**.
   Its 8-digit password is shown on the display under *Grind Sync → Network*. Connect to it; the
   captive portal opens (otherwise go to `http://192.168.4.1`).
4. **WLAN & GaggiMate** tab: scan, pick your network, save. After that the device is at
   **http://eurekamate.local**. Press *Suchen (mDNS)* to find GaggiMate (`_gaggimate._tcp`),
   or enter its IP. Then press *Verbindung testen*: it must report "GaggiMate Grind-by-Sync OK".
5. **Mahlgrad** tab: run the calibration wizard (section 5).
6. **Rezepte** tab: name, GbS on/off, target time, beverage weight, start setting and
   portions for each recipe.

## 2. Protocol (as implemented)

The grinder is the HTTP **client**. GaggiMate answers on port 80.

| Request | Purpose | Notes |
|---|---|---|
| `GET /api/v2/machine` | Identity | `MA_TYPE` must be `1` (Xenia). Checked after connecting and after errors |
| `GET /api/v2/mako` | Status, polled every 1 s | `MA_STATUS`: 0 standby, 1 ready, 3 brewing, 4 shot done, 5 service, 6 error. Also `MA_EXTRACTION_STATUS`, `REAL_EXTRACTION_TIME_MS`, `PU_SENS_FLOW_METER_VOLUME`, `BG_SET_TEMP`, `BG_SENS_TEMP`, `TANK_LEVEL` |
| `POST /api/v2/brewratio` | "Grind done" | `{"SYNC_BEVERAGE_WEIGHT": 36.0, "SYNC_FILTER": <recipe 1..6>}`. GaggiMate selects the mapped profile and uses this weight as the target |
| `POST /api/v2/scripts/execute` | Start a brew | `{"ID": 9}`. Sent by the brew button, the display/web button, or automatically after the grind (option) |

Shot detection (`gbs::ShotDetector`):

- `brewing → shot done` = finished shot. GaggiMate keeps "brewing" while the scale settles,
  then shows "shot done" for about 5 s, so a 1 s poll interval is enough.
- `brewing → ready/standby` without "shot done" = aborted shot (ignored for regulation).
- Connection lost while brewing = discarded.
- A shot is only linked to a grind if it starts within **5 min** after that grind
  (`gbs::GrindShotLink`, one grind → at most one shot).

Bench test without a machine: `python3 tools/mock_gaggimate.py --port 8080 --shot 31 --auto`
on a PC, then use `<PC-IP>:8080` as the GaggiMate host. The `-mock` build env simulates
GaggiMate inside the firmware (`MOCK_BUILD`).

## 3. Regulation (GbS-like)

After each valid shot of a recipe with **GbS on**:

```
error  = shot_time - target_time
valid  = 15 s < shot_time <= target_time + 15 s,  not aborted,
         |weight - target_weight| / target_weight <= yield tolerance (25 %)
if |error| <= deadband (1.5 s)              -> no change
effective = error -/+ deadband               (only the part outside the deadband)
delta = clamp(gain (0.3 /s) * effective, ±max_step (2.0)), rounded to 0.1, never 0
new setting = clamp(setting + delta, safety margin, 100)
```

A shot that is too slow gives a positive delta (coarser); too fast gives a negative delta
(finer). The setting is stored **per recipe** in NVS. If the recipe is active, the dial moves
right away with the fine profile, only while the grinder is idle. Verdicts are shown in the
history (display + web): `adjust`, `in_deadband`, `aborted`, `too_short`, `too_long`,
`yield_off`, `disabled`. All parameters are in the **Regelung** tab.

Manual brew-time entry (like the E64 WS "Brew Time" input) is on the Status tab: it feeds the
same regulator without GaggiMate.

## 4. Feature parity with E64 WS + GaggiMate GbS

| E64 WS / GbS feature | EurekaMate |
|---|---|
| Recipe slots synced to machine profiles (`SYNC_FILTER`) | 6 recipes: 1–3 follow the SGBW profiles (Single/Double/Custom), 4–6 are presets (default Filter = 60, Decaf) |
| Beverage weight per recipe (`SYNC_BEVERAGE_WEIGHT`) | Per recipe, or dose × ratio (default 2.0). The dose comes from the scale (grind-by-weight) |
| Start brew from the grinder (script 9) | Button on GPIO17 (original front button possible), display button, web button, or auto after grind |
| GbS on/off and target time per recipe | Yes |
| "Auto Adjust to Recipe DD" | `follow_profile`: the dial travels to the recipe's stored setting when the recipe changes |
| "GbS start DD" | `start_setting` per recipe + "Reset to start" |
| Shot history, connection status | Last 20 shots (persisted), live state on display + web |
| Manual brew time entry | Yes |
| User password | Optional web password (HTTP basic auth, user `admin`) |
| Multiple dosing turns (filter) | `portions` per recipe (1–10). GaggiMate is notified after the last portion |
| (future) 4-bean hopper | Reserved: `hopper_enabled`, `hopper_slot` per recipe, `SYNC_HOPPER_SENSOR_PINS`, hook `GrindSync::on_hopper_slot_changed()`. Any dosing gate must check `dose_allowed` |

## 5. Dial: calibration, soft limits, safety

- **Wizard** (display: *Grind Sync → Dial Calibration*; web: *Mahlgrad*):
  1. Jog finer until the burrs **just touch** (listen while turning by hand; the grinder must be **off**). Press *Set 0*.
  2. Jog to the coarsest setting you will ever use. Press *Set 100*.
  3. Choose the safety margin (default 5): settings below it are never driven. Press *Finish*.
- Step counts are stored in NVS (`dial` namespace) and mapped linearly to 0–100. A minimum span
  of 200 microsteps is required.
- Soft limits: every target is clamped to `[margin, 100]`. Uncalibrated jogs are limited to
  20 000 microsteps.
- **Moves only while the grinder is idle**. The motor task checks a busy predicate before
  every step. A pre-start hook in the relay code stops the motor (≤ 50 ms) before the grinder
  motor may start.
- Position is persisted after every move. A "moving" flag catches resets mid-move
  (brown-out, see below). After such a reset the position counts as **untrusted** until you
  home, confirm, or the AS5600 recovers it.
- Optional homing to the coarse end after boot (StallGuard or endstop on GPIO44).
- Optional AS5600 on the motor axis: multi-turn tracking, slip check, boot recovery (assumes
  less than half a motor turn moved while powered off).

### Interlocks

| Interlock | Rule | Implementation |
|---|---|---|
| Grinder ↔ dial (soft) | A grind is **not started** while the dial moves or has a move queued | `GrindController::set_start_gate()` → `DialMotor::grinder_start_allowed()` |
| Grinder ↔ dial (hard) | Before **every** relay start (grind, pulse, purge) a still-moving dial is stopped first | `Grinder::pre_start_hook` → `stop_and_wait(50)` |
| Dial ↔ grinder | The dial never steps while the grinder runs | busy predicate checked per step |
| **Dosing gate (future hopper)** | Beans may only be released to the burrs once the setting move is **complete and confirmed** | `dose_allowed` = no move running/queued, calibrated, position trusted, not calibrating, no stall. `DialMotor::dose_allowed()`, `GrindSync::dose_allowed()`, web `/api/status → dial.dose_allowed` |

The logic is pure and unit-tested (`gbs::grinder_start_allowed`, `gbs::dose_allowed`). The motor
task peeks the command queue before taking a command, so there is no window where a command
is neither queued nor running.

### Travel profiles (fast recipe switching)

| Profile | Default | Used for |
|---|---|---|
| Fast | 4000 µsteps/s, 6000 µsteps/s², boost current kept | Recipe changes (Filter 60 → Decaf) |
| Fine | 800 µsteps/s, 1500 µsteps/s² | GbS corrections, the last `DIAL_SLOW_ZONE` (5 units) before low targets, jogs |

Targets are always approached **from the coarse side** (backlash compensation, optional extra
steps). Progress and ETA are shown on display and web. The web status shows the computed
full-range travel time (`full_range_s`).

**Travel-time example** (defaults, 16 µsteps, reduction i = 5.5, assuming the dial ring turns
1 full revolution from 0 to 100):
steps for 0–100 = 200 × 16 × 5.5 × 1 = 17 600 µsteps. Fast: ramp ≈ 0.67 s, then 4000 µsteps/s
→ full range ≈ **4.8 s**. Filter 60 → Decaf 12 = 48 units ≈ 8450 µsteps; the last 5 units run
fine → ≈ **3.5 s**. The exact numbers come from your calibration.

## 6. Power

**Primary: tap the grinder's internal ~12 V DC rail** (after the DF10S bridge rectifier, before
the 78L05; e.g. on the + of the reservoir capacitor). **Never touch the 230 V side.**

| Rail | Feeds | Notes |
|---|---|---|
| ~12 V (tap) → fuse 1 A → | TMC2209 **VM** + 100–470 µF/25 V bulk cap at VM | Motor power |
| ~12 V → MP1584/Mini560 **set to 5.0 V** → | ESP32-S3 **5V** pin (+1000 µF as in SGBW) | Logic |
| Grinder harness pin 1 (+5 V from the 78L05) | **Not connected** | The 78L05 has < 100 mA spare |
| GND | Common for everything | |

The harness then only provides GND, the relay signal (pin 3) and the button (pin 2).

Load budget (rough): ESP32-S3 + AMOLED + WiFi 150–300 mA at 5 V ≈ 0.1–0.15 A at 12 V.
Stepper at 400–600 mA RMS ≈ 0.15–0.3 A at 12 V while moving. The motor is **disabled while
idle** (hold 0 %) and **never moves while the grinder runs**, so the relay coil and the
stepper never draw at the same time.

**You must measure** (multimeter on the tap, DC volts):

1. Idle, EurekaMate running: expect 11–14 V.
2. During a fast recipe change with boost current: **must stay ≥ 10 V**.
3. While grinding (relay on): must stay ≥ 10 V and the ESP must not reset.

If it sags below 10 V, the ESP resets ("Position unsicher" after a move), or the relay
chatters → use the **fallback: an external 12 V / 2 A power supply** for VM and the buck. Keep
GND common with the grinder (relay signal reference). Firmware help: low default run current
(400 mA), boost only for 400 ms, slow ramps, driver off when idle, and brown-out detection
through the persisted "moving" flag.

## 7. Torque estimate

| Quantity | Default | Note |
|---|---|---|
| Motor | 17HS4401, ~0.40–0.45 Nm holding at 1.7 A | Normal length is recommended; pancakes (~0.1–0.2 Nm) are too weak for a stiff dial |
| Torque at current I | T ≈ 0.42 Nm × I / 1.7 A | 400 mA → 0.10 Nm, 600 mA → 0.15 Nm; dynamic at low speed ≈ 70 % |
| Reduction | i = (20/12) × (66/20) = 5.5 | pinion 12 T → knob gear 20 T → ring 66 T (**count your kit!**) |
| Efficiency | η ≈ 0.85² ≈ 0.72 | two printed/alu spur stages |
| Dial torque | T_dial ≈ T_motor × 0.7 × i × η | 400 mA: ≈ 0.28 Nm · 600 mA boost: ≈ 0.42 Nm |

**Measure your dial**: wrap a string around the knob (radius r) and pull with a luggage or
kitchen scale until it turns. T = m[kg] × 9.81 × r[m]. Example: 1.5 kg at 25 mm = 0.37 Nm.
Want at least 1.5× margin.

If it is not enough, in this order:
1. Raise boost current (`boost_current_ma`, up to 1000 mA; check the 12 V rail).
2. Smaller pinion (`pinion_teeth = 10`) → i = 6.6 (slower travel).
3. Raise run current (heat: TMC2209 with heatsink, vents).
4. NEMA17 with **5:1 planetary gearbox** (e.g. 17HS15-1684S-PG5): ≈ 5× torque, travel ≈ 5× slower (raise `fast_speed_sps`).
5. External 12 V 2 A supply.

### StallGuard (stall detection)

TMC2209 StallGuard4 via DIAG → GPIO1. Default **off** (`stall_threshold = 0`). A stall stops
the motor, marks the position untrusted and shows an error on display + web. Tuning: in the
**Motor** tab, raise `stall_threshold` step by step (e.g. 20, 40, 60 …) while doing fast moves.
Stop just below the value that gives false stalls in free travel. Then test by holding the
dial: it must stop. StallGuard needs the UART connection and is only armed 250 ms after the
move starts.

## 8. Pins (Waveshare ESP32-S3-Touch-AMOLED-1.64)

| Function | GPIO | Notes |
|---|---|---|
| HX711 SCK / DOUT | 2 / 3 | unchanged from SGBW |
| Grinder relay (harness pin 3) | 18 | unchanged, active HIGH |
| Brew button (harness pin 2 or extra button) | 17 | active LOW, internal pull-up. Measure pin 2 open: > 3.3 V → divider |
| TMC2209 STEP / DIR / EN | 5 / 6 / 7 | EN active LOW |
| TMC2209 PDN_UART | 8 | via **1 kΩ** series resistor (write-only UART, address 0: MS1=MS2=GND) |
| TMC2209 DIAG | 1 | StallGuard |
| AS5600 SDA / SCL | 15 / 16 | optional, I²C port 1, 3V3 |
| Coarse endstop | 44 | optional, switch to GND (U0RXD, not needed after flashing) |

**Verify against the Waveshare schematic before soldering.** All pins are in `src/config/grind_sync.h`.

## 9. Web UI and OTA

`http://eurekamate.local` (or the IP shown on the display). Tabs: Status (gauge, progress/ETA,
recipe buttons, start brew, manual shot), Rezepte, Regelung, Mahlgrad (jog + wizard), Motor
(currents, microsteps, speeds, StallGuard, AS5600/endstop/homing), WLAN & GaggiMate (scan,
mDNS discovery, test), Verlauf, Update (OTA `.bin` upload, `firmware.bin` from
`.pio/build/<env>/`). An optional password protects all pages and the OTA upload. WiFi
credentials and the password live only in NVS, never in the repository.

## 10. Known limitations / not tested on hardware

- **Not yet tested on real hardware.** The firmware compiles; the protocol and regulator logic are covered by native unit tests (`pio test -e native`).
- TMC UART is write-only (no register readback or driver diagnostics).
- StallGuard thresholds depend on the grinder; default off.
- AS5600 boot recovery assumes less than half a motor turn of movement while off.
- Live status in the web UI is 1 s polling.
- The 4-bean hopper is a data model + hooks only.
