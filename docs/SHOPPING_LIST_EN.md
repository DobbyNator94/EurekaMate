# EurekaMate – Shopping List (Germany / EU, cheapest sources)

🇩🇪 Deutsche Version: [EINKAUFSLISTE_DE.md](EINKAUFSLISTE_DE.md)

> [!WARNING]
> **⚠️ Vibe-coded & untested.** This project was created with AI and is **completely untested**. No guarantee, no warranty – everyone acts at their own risk. **230 V can kill you.** If you don't know what you're doing, better don't do anything.

> [!NOTE]
> **Prices and availability as of 26 Nov 2026** (Amazon.de, logged out, German delivery address). Prices change daily. For variant listings, make sure the **right variant** (e.g. "T70 1KG") is selected before buying.
> "Prime" = fulfilled by Amazon (free shipping with Prime). Every non-Prime item in this list also ships **free**.

**Strategy:** almost everything from **Amazon.de Prime** (no shipping cost, easy returns). eBay.de was checked for every part but was never cheaper: either more expensive incl. shipping (e.g. Waveshare display from China ≈ €28.55 + ~€10 shipping) or the listing had ended. Other shops (BerryBase, Reichelt) are only a fallback if Amazon can't deliver. AliExpress is deliberately not used.

**You print the 3D parts yourself** – no print service in this list, see [Printed parts](#printed-parts-print-yourself).

---

## Total cost (overview)

| Block | Total |
|---|---:|
| **A** Base electronics (display, scale) | €68.01 |
| **B** Motor upgrade (dial kit, motor, driver, electronics) | €87.88 |
| **C** Cables & consumables | €37.24 |
| **Required, without screws/inserts (if you have them)** | **€193.13** |
| **D** Screws & heat-set inserts (checklist) | €30.18 |
| **Required, complete incl. screws/inserts** | **€223.31** |
| **E** Optional (everything) | €51.13 |

Saving tips:
- If you already have wire, jumpers, heat shrink and cable ties, you save up to **€29.25** of block C.
- If you already own the dial kit (or only build scale/display without a motor), block B goes away entirely.
- Shipping: €0 for every item (Prime or free shipping).

---

## A – Base electronics

| ✔ | Part | Qty | Spec | Shop | Price | Shipping | Link |
|---|---|---|---|---|---:|---:|---|
| ☐ | Waveshare ESP32-S3-Touch-AMOLED-1.64 | 1 | ESP32-S3R8, 1.64" AMOLED 280×456, touch | Amazon.de (sold/shipped by Waveshare, **not Prime**) | €33.99 | free | https://www.amazon.de/dp/B0DT9NGTLW |
| ☐ | Load cell **T70 1 kg** (single point, 70×22×15 mm, 4 holes in a ": :" pattern) | 1 | fits the SGBW back plate, 1 kg rated | Amazon.de (marketplace, **not Prime**, delivery approx. 3–4 weeks) | €28.53 | free | https://www.amazon.de/dp/B0FJF89YX1 |
| ☐ | HX711 module (kit with cell) | 1 | 24-bit ADC; **the bundled 5 kg cell is NOT used** | Amazon.de Prime | €5.49 | €0 | https://www.amazon.de/dp/B07MY35F92 |
| | **Total A** | | | | **€68.01** | | |

Notes:
- **No relay/SSR needed:** the grinder motor is switched by the grinder's own relay (harness pin 3 → GPIO18).
- **T70 alternatives** (all variant "T70 1KG", free shipping, not Prime): https://www.amazon.de/dp/B0H17WP5SK (€28.15, but delivery only approx. 30 Dec – 6 Jan) and https://www.amazon.de/dp/B0GLPQQ71V (€32.81, faster: approx. 8–14 Dec). The cheap bar cells from HX711 kits (2 holes) do **not** fit the SGBW mount. No T70 1 kg was found on eBay.de.
- **Waveshare fallback:** BerryBase ≈ €29.90 + shipping – about the same as Amazon incl. shipping.
- **Watch out, Waveshare V1.1:** per the Waveshare docs, board revision V1.1 uses **GPIO18 for TP_INT** and **GPIO17 for IMU_INT2**. Our harness (and upstream SGBW) uses GPIO17 (button) and GPIO18 (relay). Check your revision before wiring – untested!
- A USB-C data cable (for flashing) is assumed to be on hand.

## B – Motor upgrade (stepper retrofit)

| ✔ | Part | Qty | Spec | Shop | Price | Shipping | Link |
|---|---|---|---|---|---:|---:|---|
| ☐ | Eureka grind-adjustment dial (gear kit) | 1 | ARO Espresso aluminium dial for Eureka Mignon | Amazon.de Prime (sold by ARO Espresso) | €42.00 | €0 | https://www.amazon.de/dp/B0CLCB8VQ6 |
| ☐ | NEMA17 stepper 17HS4417P1 | 1 | 1.7 A, 0.4 Nm, 40 mm body, 5 mm D-shaft | Amazon.de Prime (ACT Motor) | €6.99 | €0 | https://www.amazon.de/dp/B0723H9KLZ |
| ☐ | TMC2209 driver (BIGTREETECH V1.3) | 1 | UART, StealthChop | Amazon.de Prime | €8.49 | €0 | https://www.amazon.de/dp/B0893DPSJL |
| ☐ | MP1584EN buck converter, 5-pack | 1 (pack) | 22×17 mm, **set to 5.0 V before installing!** | Amazon.de Prime (DollaTek) | €5.99 | €0 | https://www.amazon.de/dp/B07DJ5HZ7G |
| ☐ | Electrolytic cap 1000 µF / 25 V, 10-pack | 1 (pack) | 10×17 mm, 5 V rail (ESP/display) after the buck | Amazon.de Prime (QUARKZMAN) | €4.59 | €0 | https://www.amazon.de/dp/B0G1MYYMT8 |
| ☐ | Electrolytic cap 220 µF / 25 V, 24-pack | 1 (pack) | on the TMC2209 VM (12 V) | Amazon.de Prime (QUARKZMAN) | €3.45 | €0 | https://www.amazon.de/dp/B0G1N1RYZ9 |
| ☐ | Resistor assortment 600 pcs (30 values, 1/4 W, 1 %) | 1 (set) | includes **1 kΩ** (TMC2209 UART resistor) and 10k/20k (divider) | Amazon.de Prime (ARCELI) | €5.99 | €0 | https://www.amazon.de/dp/B07PXYY4HV |
| ☐ | Inline fuse holder 5×20, 5-pack | 1 (pack) | 22 AWG leads, for the 12 V feed | Amazon.de Prime (sourcing map) | €6.49 | €0 | https://www.amazon.de/dp/B00H3CVSXQ |
| ☐ | Fuse 1 A fast-blow 5×20, 10-pack | 1 (pack) | 1 A F | Amazon.de | €3.89 | €0 | https://www.amazon.de/dp/B09JZFNJ71 |
| | **Total B** | | | | **€87.88** | | |

Notes:
- **Set instead of single part:** the resistor assortment (€5.99) costs the same as a 100-pack of only 1 kΩ (€5.99, https://www.amazon.de/dp/B0CL6XM7RD) but also covers the voltage divider (10k/20k).
- **Don't buy:** Mini560 listing B0D9TR6L3X – default variant is 3.3 V and at 29×18 mm it's too large for the buck tray (22.5×17.5 mm).
- Alternative buck (larger, size unchecked): Aideepen 12→5 V 3 A, €6.79, https://www.amazon.de/dp/B0CLRSFL13
- Alternative fuse (Prime): https://www.amazon.de/dp/B01CUAF7DW (€3.99)
- **Dual-shaft motor** for the AS5600 encoder: see block E.

## C – Cables & consumables

| ✔ | Part | Qty | Spec | Shop | Price | Shipping | Link |
|---|---|---|---|---|---:|---:|---|
| ☐ | JST-PH 2.0 4-pin male+female with leads, 15 pairs | 1 (set) | 150 mm leads, for display harness/HX711 | Amazon.de Prime (Lyeteung) | €7.99 | €0 | https://www.amazon.de/dp/B0CBWXJ6ZC |
| ☐ | Silicone wire 22 AWG, 6 colours × 4 m | 1 (set) | heat-resistant, flexible | Amazon.de Prime (SCHDRA) | €11.99 | €0 | https://www.amazon.de/dp/B0C7T9KYND |
| ☐ | Jumper wires 3×40 pcs, 20 cm | 1 (set) | M-M / M-F / F-F, for testing/wiring | Amazon.de Prime (ELEGOO) | €6.99 | €0 | https://www.amazon.de/dp/B01EV70C78 |
| ☐ | Heat-shrink assortment 900 pcs | 1 (set) | 2:1 | Amazon.de Prime (Lectorias) | €6.28 | €0 | https://www.amazon.de/dp/B0CPBKQ4RW |
| ☐ | Cable ties 100×2.5 mm, 200 pcs | 1 (pack) | | Amazon.de Prime | €3.99 | €0 | https://www.amazon.de/dp/B0FF9BZ6KV |
| | **Total C** | | | | **€37.24** | | |
| | of which "often already on hand" (wire, jumpers, heat shrink, ties) | | | | €29.25 | | |

Tools (soldering iron, solder, multimeter, wire stripper, heat gun/lighter for heat shrink) are assumed to be on hand.

## D – Screws & heat-set inserts (tick-off checklist)

You probably have these already. Tick off what you have – only re-buy what's missing. Sizes come from `hardware/stepper-mount/scad/*.scad` and the SGBW requirements (`docs/DOC.md`).

### Exactly required

| ✔ | Size | Pcs | Used for | Why this length |
|---|---|---:|---|---|
| ☐ | **M3×6** socket head (DIN 912 / ISO 4762) | 4 | NEMA17 to the motor-housing floor | floor 3 mm, counterbore 1.6 mm → 2.4 mm of material; M3×6 engages 3.6 mm in the motor thread. M3×8 would bottom out in the ~4.5 mm deep NEMA17 thread. |
| ☐ | **M3×8** socket head | 4 | lid onto the housing bosses (into inserts) | lid 2.4 mm → 5.6 mm in the 5.7 mm insert |
| ☐ | **M3×10** socket head | 8 | 2× housing feet to the side brace (inserts) + 6× SGBW (back plate/cover/load cell) | |
| ☐ | **M3×30** socket head | 1 | tensioning screw of the strap slider | passes through the 3.4 mm hole into the nut trap |
| ☐ | **M3 nut** (DIN 934) | 2 | 1× tension slider, 1× pinion (nut trap) | |
| ☐ | **M3×4 grub screw** (set screw, DIN 913) | 1 | pinion on the D-shaft | |
| ☐ | **M3 washer** (DIN 125) | 2 | under the M3×10 at the housing feet | |
| ☐ | **M3 heat-set insert** (ruthex RX-M3x5.7 or similar: M3, L 5.7 mm, OD 4.6 mm) | 6 | 4× lid bosses in the housing, 2× side-brace posts | SCAD hole 4.0 mm, ≥ 6.5 mm deep (`heat_inserts = true` in `params.scad`) |

> Without inserts: set `heat_inserts = false` in `hardware/stepper-mount/scad/params.scad` → 2.6 mm holes, the M3 screw cuts directly into PETG (less durable).

### Re-buy links (if a size is missing)

| ✔ | Part | Contents | Shop | Price | Shipping | Link |
|---|---|---|---|---:|---:|---|
| ☐ | M3 screw set 400 pcs (WZHUIDA) | 40× each M3×6/M3×8, 30× each M3×10/12/16/20, 200 nuts | Amazon.de Prime | €7.99 | €0 | https://www.amazon.de/dp/B0CQ24K842 |
| ☐ | M3×30 DIN 912 A2, 30 pcs (TXXATX) | | Amazon.de Prime | €5.49 | €0 | https://www.amazon.de/dp/B0GQLGMRQL |
| ☐ | Grub screw M3×4, 20 pcs (German Fastener Parts) | | Amazon.de Prime | €4.99 | €0 | https://www.amazon.de/dp/B093MWG6QJ |
| ☐ | Washers M3 DIN 125 A2 | | Amazon.de | €4.07 | €0 | https://www.amazon.de/dp/B0CJX2HF7P |
| ☐ | ruthex threaded insert M3 RX-M3x5.7, 100 pcs | | Amazon.de Prime (sold by ruthex) | €7.64 | €0 | https://www.amazon.de/dp/B08BCRZZS3 |
| | **Total D (re-buy everything)** | | | **€30.18** | | |

Notes:
- **The set is cheaper:** the 400-piece set (€7.99) covers M3×6/8/10 and all nuts – single packs would be three purchases.
- Cheaper insert alternative: M3×5.7×4.6, 80 pcs, €5.49: https://www.amazon.de/dp/B0D3DZXT4R (check the hole size in its data sheet, adjust `insert_hole_d` if needed).
- Nuts + washers combo (Prime): https://www.amazon.de/dp/B0GFC9H67Y (€6.49, 100 each).

## E – Optional

| ✔ | Part | Qty | Spec | Shop | Price | Shipping | Link |
|---|---|---|---|---|---:|---:|---|
| ☐ | AS5600 encoder, 4 pcs incl. magnet | 1 (pack) | magnetic angle sensor, slides into the lid holder | Amazon.de Prime | €9.99 | €0 | https://www.amazon.de/dp/B0D4YW7NVB |
| ☐ | NEMA17 **dual-shaft** 17HS8401B (RATTMMOTOR) | 1 | 48 mm long, rear shaft 10 mm – required for the AS5600 | Amazon.de Prime | €14.58 | €0 | https://www.amazon.de/dp/B09DG1B48W |
| ☐ | Mechanical endstop, 4 pcs | 1 (pack) | homing | Amazon.de Prime | €5.99 | €0 | https://www.amazon.de/dp/B0FHB5SR6N |
| ☐ | Power supply 12 V / 2 A, 5.5×2.1 mm | 1 | external motor supply | Amazon.de Prime (Velainwom) | €8.49 | €0 | https://www.amazon.de/dp/B07JLGHSFJ |
| ☐ | DC jack 5.5×2.1 → screw terminal | 1 (pack) | PSU connection | Amazon.de Prime (SatChef) | €3.49 | €0 | https://www.amazon.de/dp/B07D62QFWF |
| ☐ | Dosing cup 54 mm (HAPPOW) | 1 | for the scale | Amazon.de Prime | €8.59 | €0 | https://www.amazon.de/dp/B0GSW3HB9K |
| | **Total E** | | | | **€51.13** | | |

Notes:
- The dual-shaft motor **replaces** the €6.99 motor from block B (effective extra cost +€7.59). Then set `motor_len = 48` and `rear_shaft_len = 10` in `params.scad` and re-export the STLs.
- The default SCAD has `as5600 = true` (holder in the lid). Without the encoder you can set `as5600 = false` → shorter housing. Magnet: 6×2.5 mm diametric (for `magnet_cap`) – check the size of the bundled magnet.
- Cheaper AS5600 alternative: 2 pcs DollaTek, €6.99: https://www.amazon.de/dp/B0BGPF3WT3 (check the magnet).

## Printed parts (print yourself)

No print service – material recommendation only. STLs: [`hardware/stepper-mount/stl/`](../hardware/stepper-mount/stl/), details: [3D_PRINTS.md](3D_PRINTS.md).

| Part | Qty | Material |
|---|---:|---|
| Motor housing (`motor_housing`) | 1 | PETG (or ASA, more heat-resistant) |
| Housing lid (`housing_lid`) | 1 | PETG/ASA |
| Side brace (`side_brace`) | 1 | PETG/ASA |
| Pinion (`pinion`) | 1 | PETG (100 % infill recommended) |
| Tension slider (`strap_tension_slider`) | 1 | PETG |
| Magnet cap (`magnet_cap`, AS5600 only) | 1 | PETG |
| Strap segment (`strap_segment`) | 4 | **TPU 95A** (PETG works with `strap_t = 1.2`) |
| SGBW parts: back plate, cover, screen adapter, cup holder, covers | 1 each | PETG |

Filament is assumed to be on hand (an opened spool of PETG plus a bit of TPU is enough).
