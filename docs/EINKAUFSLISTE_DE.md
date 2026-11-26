# EurekaMate – Einkaufsliste Deutschland (günstigste Quellen)

🇬🇧 English version: [SHOPPING_LIST_EN.md](SHOPPING_LIST_EN.md)

> [!WARNING]
> **⚠️ Vibe-coded & ungetestet.** Dieses Projekt wurde mit KI erstellt und ist **komplett ungetestet**. Keine Garantie, keine Gewährleistung – jede:r handelt auf eigene Verantwortung. **230 V ist lebensgefährlich.** Wer nicht weiß, was er tut, lässt es besser.

> [!NOTE]
> **Preise und Verfügbarkeit: Stand 26.11.2026** (Amazon.de, abgerufen ohne Login, deutsche Lieferadresse). Preise ändern sich täglich. Bei Varianten-Angeboten vor dem Kauf prüfen, ob die **richtige Variante** (z. B. „T70 1KG“) ausgewählt ist.
> „Prime“ = Versand durch Amazon (kostenlos mit Prime). Alle Artikel ohne Prime in dieser Liste sind ebenfalls **versandkostenfrei**.

**Strategie:** Fast alles über **Amazon.de Prime** (keine Versandkosten, schnelle Retoure). eBay.de wurde für jedes Teil geprüft, war aber nirgends günstiger – entweder teurer inkl. Versand (z. B. Waveshare-Display aus China ≈ 28,55 € + ~10 € Versand) oder das Angebot war beendet. Andere Shops (BerryBase, Reichelt) nur als Fallback, falls Amazon nicht liefert. AliExpress wird bewusst nicht verwendet.

**3D-Druck machst du selbst** – kein Druckservice in der Liste, siehe [Druckteile](#druckteile-selbst-drucken).

---

## Gesamtkosten (Überblick)

| Block | Summe |
|---|---:|
| **A** Basis-Elektronik (Display, Waage) | 68,01 € |
| **B** Motor-Upgrade (Einstellrad-Kit, Motor, Treiber, Elektronik) | 87,88 € |
| **C** Kabel & Verbrauchsmaterial | 37,24 € |
| **Pflicht ohne Schrauben/Inserts (falls vorhanden)** | **193,13 €** |
| **D** Schrauben & Einschmelzmuttern (Checkliste) | 30,18 € |
| **Pflicht komplett inkl. Schrauben/Inserts** | **223,31 €** |
| **E** Optional (alles) | 51,13 € |

Spartipps:
- Hast du Kabel, Jumper, Schrumpfschlauch und Kabelbinder schon da, sparst du bis zu **29,25 €** aus Block C.
- Hast du das Einstellrad-Kit schon (oder baust du nur Waage/Display ohne Motor), entfällt Block B ganz.
- Versand: 0 € bei allen Positionen (Prime bzw. versandkostenfrei).

---

## A – Basis-Elektronik

| ✔ | Teil | Menge | Spezifikation | Shop | Preis | Versand | Link |
|---|---|---|---|---|---:|---:|---|
| ☐ | Waveshare ESP32-S3-Touch-AMOLED-1.64 | 1 | ESP32-S3R8, 1,64" AMOLED 280×456, Touch | Amazon.de (Verkauf/Versand Waveshare, **kein Prime**) | 33,99 € | gratis | https://www.amazon.de/dp/B0DT9NGTLW |
| ☐ | Wägezelle **T70 1 kg** (Single-Point, 70×22×15 mm, 4 Löcher im „: :“-Muster) | 1 | passt in die SGBW-Rückplatte, 1 kg Nennlast | Amazon.de (Marktplatz, **kein Prime**, Lieferung ca. 3–4 Wochen) | 28,53 € | gratis | https://www.amazon.de/dp/B0FJF89YX1 |
| ☐ | HX711-Modul (Kit mit Zelle) | 1 | 24-bit ADC; **die beiliegende 5-kg-Zelle wird NICHT verwendet** | Amazon.de Prime | 5,49 € | 0 € | https://www.amazon.de/dp/B07MY35F92 |
| | **Summe A** | | | | **68,01 €** | | |

Hinweise:
- **Keine Relais/SSR nötig:** Der Motor der Mühle wird über das mühleneigene Relais geschaltet (Kabelbaum Pin 3 → GPIO18).
- **T70-Alternativen** (alle Variante „T70 1KG“, gratis Versand, kein Prime): https://www.amazon.de/dp/B0H17WP5SK (28,15 €, aber Lieferung erst ca. 30.12.–06.01.) und https://www.amazon.de/dp/B0GLPQQ71V (32,81 €, schneller: ca. 08.–14.12.). Die billigen Waagen-Zellen aus HX711-Kits (Balken mit 2 Löchern) passen **nicht** in die SGBW-Halterung. Auf eBay.de wurde keine T70 1 kg gefunden.
- **Waveshare-Fallback:** BerryBase ≈ 29,90 € + Versand – inkl. Versand etwa gleich teuer wie Amazon.
- **Achtung Waveshare V1.1:** Laut Waveshare-Doku nutzt die Board-Revision V1.1 **GPIO18 für TP_INT** und **GPIO17 für IMU_INT2**. Unser Kabelbaum (und die Upstream-SGBW) nutzt GPIO17 (Taster) und GPIO18 (Relais). Vor dem Anschließen die Revision prüfen – ungetestet!
- Ein USB-C-Datenkabel (zum Flashen) wird als vorhanden vorausgesetzt.

## B – Motor-Upgrade (Stepper-Nachrüstung)

| ✔ | Teil | Menge | Spezifikation | Shop | Preis | Versand | Link |
|---|---|---|---|---|---:|---:|---|
| ☐ | Eureka-Mahlgrad-Einstellrad (Zahnrad-Kit) | 1 | ARO Espresso Aluminium-Einstellrad für Eureka Mignon | Amazon.de Prime (Verkauf ARO Espresso) | 42,00 € | 0 € | https://www.amazon.de/dp/B0CLCB8VQ6 |
| ☐ | NEMA17-Schrittmotor 17HS4417P1 | 1 | 1,7 A, 0,4 Nm, 40 mm Baulänge, D-Welle 5 mm | Amazon.de Prime (ACT Motor) | 6,99 € | 0 € | https://www.amazon.de/dp/B0723H9KLZ |
| ☐ | TMC2209-Treiber (BIGTREETECH V1.3) | 1 | UART, StealthChop | Amazon.de Prime | 8,49 € | 0 € | https://www.amazon.de/dp/B0893DPSJL |
| ☐ | Step-down MP1584EN, 5er-Pack | 1 (Pack) | 22×17 mm, **vor dem Einbau auf 5,0 V einstellen!** | Amazon.de Prime (DollaTek) | 5,99 € | 0 € | https://www.amazon.de/dp/B07DJ5HZ7G |
| ☐ | Elko 1000 µF / 25 V, 10er | 1 (Pack) | 10×17 mm, 5-V-Schiene (ESP/Display) hinter dem Buck | Amazon.de Prime (QUARKZMAN) | 4,59 € | 0 € | https://www.amazon.de/dp/B0G1MYYMT8 |
| ☐ | Elko 220 µF / 25 V, 24er | 1 (Pack) | an VM des TMC2209 (12 V) | Amazon.de Prime (QUARKZMAN) | 3,45 € | 0 € | https://www.amazon.de/dp/B0G1N1RYZ9 |
| ☐ | Widerstands-Sortiment 600 Stk (30 Werte, 1/4 W, 1 %) | 1 (Set) | enthält **1 kΩ** (UART-Widerstand TMC2209) und 10k/20k (Spannungsteiler) | Amazon.de Prime (ARCELI) | 5,99 € | 0 € | https://www.amazon.de/dp/B07PXYY4HV |
| ☐ | Inline-Sicherungshalter 5×20, 5er | 1 (Pack) | 22 AWG Litze, für 12-V-Zuleitung | Amazon.de Prime (sourcing map) | 6,49 € | 0 € | https://www.amazon.de/dp/B00H3CVSXQ |
| ☐ | Feinsicherung 1 A flink 5×20, 10er | 1 (Pack) | 1 A F | Amazon.de | 3,89 € | 0 € | https://www.amazon.de/dp/B09JZFNJ71 |
| | **Summe B** | | | | **87,88 €** | | |

Hinweise:
- **Set statt Einzelteil:** Das Widerstands-Sortiment (5,99 €) kostet genauso viel wie ein 100er-Beutel nur 1 kΩ (5,99 €, https://www.amazon.de/dp/B0CL6XM7RD), deckt aber zusätzlich den Spannungsteiler (10k/20k) ab.
- **Nicht nehmen:** Mini560-Angebot B0D9TR6L3X – Standardvariante ist 3,3 V und mit 29×18 mm zu groß für die Buck-Schale (22,5×17,5 mm).
- Alternative Step-down (größer, Maße unbekannt): Aideepen 12→5 V 3 A, 6,79 €, https://www.amazon.de/dp/B0CLRSFL13
- Alternative Sicherung (Prime): https://www.amazon.de/dp/B01CUAF7DW (3,99 €)
- **Doppelwellen-Motor** für den AS5600-Encoder siehe Block E.

## C – Kabel & Verbrauchsmaterial

| ✔ | Teil | Menge | Spezifikation | Shop | Preis | Versand | Link |
|---|---|---|---|---|---:|---:|---|
| ☐ | JST-PH 2.0 4-pin Stecker+Buchse mit Litze, 15 Paar | 1 (Set) | 150 mm Litze, für Display-Kabelbaum/HX711 | Amazon.de Prime (Lyeteung) | 7,99 € | 0 € | https://www.amazon.de/dp/B0CBWXJ6ZC |
| ☐ | Silikonlitze 22 AWG, 6 Farben × 4 m | 1 (Set) | hitzefest, flexibel | Amazon.de Prime (SCHDRA) | 11,99 € | 0 € | https://www.amazon.de/dp/B0C7T9KYND |
| ☐ | Jumper-Kabel 3×40 Stk, 20 cm | 1 (Set) | M-M / M-F / F-F, zum Testen/Verdrahten | Amazon.de Prime (ELEGOO) | 6,99 € | 0 € | https://www.amazon.de/dp/B01EV70C78 |
| ☐ | Schrumpfschlauch-Sortiment 900 Stk | 1 (Set) | 2:1 | Amazon.de Prime (Lectorias) | 6,28 € | 0 € | https://www.amazon.de/dp/B0CPBKQ4RW |
| ☐ | Kabelbinder 100×2,5 mm, 200 Stk | 1 (Pack) | | Amazon.de Prime | 3,99 € | 0 € | https://www.amazon.de/dp/B0FF9BZ6KV |
| | **Summe C** | | | | **37,24 €** | | |
| | davon „oft schon vorhanden“ (Litze, Jumper, Schrumpf, Kabelbinder) | | | | 29,25 € | | |

Werkzeug (Lötkolben, Lötzinn, Multimeter, Abisolierzange, Heißluft/Feuerzeug für Schrumpfschlauch) wird als vorhanden vorausgesetzt.

## D – Schrauben & Einschmelzmuttern (Checkliste zum Abhaken)

Wahrscheinlich hast du das schon. Hake ab, was da ist – kaufe nur nach, was fehlt. Die Maße stammen aus `hardware/stepper-mount/scad/*.scad` und den SGBW-Anforderungen (`docs/DOC.md`).

### Exakt benötigt

| ✔ | Größe | Stück | Wofür | Warum genau diese Länge |
|---|---|---:|---|---|
| ☐ | **M3×6** Zylinderkopf (DIN 912 / ISO 4762) | 4 | NEMA17 an den Motor-Gehäuseboden | Boden 3 mm, Senkung 1,6 mm → 2,4 mm Material; M3×6 greift 3,6 mm ins Motorgewinde. M3×8 würde im ~4,5 mm tiefen NEMA17-Gewinde aufsetzen. |
| ☐ | **M3×8** Zylinderkopf | 4 | Deckel auf die Gehäuse-Dome (in Inserts) | Deckel 2,4 mm → 5,6 mm im 5,7-mm-Insert |
| ☐ | **M3×10** Zylinderkopf | 8 | 2× Gehäusefüße an die Seitenstütze (Inserts) + 6× SGBW (Rückplatte/Abdeckung/Wägezelle) | |
| ☐ | **M3×30** Zylinderkopf | 1 | Spannschraube des Spannband-Schlittens | geht durch die 3,4-mm-Bohrung in die Mutterfalle |
| ☐ | **M3-Mutter** (DIN 934) | 2 | 1× Spannschlitten, 1× Ritzel (Mutterfalle) | |
| ☐ | **M3×4 Madenschraube** (Gewindestift, DIN 913) | 1 | Ritzel auf der D-Welle | |
| ☐ | **M3-Unterlegscheibe** (DIN 125) | 2 | unter den M3×10 an den Gehäusefüßen | |
| ☐ | **Einschmelzmutter M3** (ruthex RX-M3x5.7 o. ä.: M3, L 5,7 mm, AD 4,6 mm) | 6 | 4× Deckel-Dome im Gehäuse, 2× Pfosten der Seitenstütze | SCAD-Bohrung 4,0 mm, ≥ 6,5 mm tief (`heat_inserts = true` in `params.scad`) |

> Ohne Inserts: In `hardware/stepper-mount/scad/params.scad` `heat_inserts = false` setzen → 2,6-mm-Löcher, M3 schneidet direkt ins PETG (weniger haltbar).

### Nachkauf-Links (falls eine Größe fehlt)

| ✔ | Teil | Inhalt | Shop | Preis | Versand | Link |
|---|---|---|---|---:|---:|---|
| ☐ | M3-Schraubenset 400 Stk (WZHUIDA) | je 40× M3×6/M3×8, je 30× M3×10/12/16/20, 200 Muttern | Amazon.de Prime | 7,99 € | 0 € | https://www.amazon.de/dp/B0CQ24K842 |
| ☐ | M3×30 DIN 912 A2, 30 Stk (TXXATX) | | Amazon.de Prime | 5,49 € | 0 € | https://www.amazon.de/dp/B0GQLGMRQL |
| ☐ | Gewindestift M3×4, 20 Stk (German Fastener Parts) | | Amazon.de Prime | 4,99 € | 0 € | https://www.amazon.de/dp/B093MWG6QJ |
| ☐ | Unterlegscheiben M3 DIN 125 A2 | | Amazon.de | 4,07 € | 0 € | https://www.amazon.de/dp/B0CJX2HF7P |
| ☐ | ruthex Gewindeeinsatz M3 RX-M3x5.7, 100 Stk | | Amazon.de Prime (Verkauf ruthex) | 7,64 € | 0 € | https://www.amazon.de/dp/B08BCRZZS3 |
| | **Summe D (alles nachkaufen)** | | | **30,18 €** | | |

Hinweise:
- **Set ist günstiger:** Das 400er-Set (7,99 €) deckt M3×6/8/10 und alle Muttern ab – einzeln wären das drei Packungen.
- Billigere Insert-Alternative: M3×5,7×4,6, 80 Stk, 5,49 €: https://www.amazon.de/dp/B0D3DZXT4R (Bohrung laut Datenblatt prüfen, ggf. `insert_hole_d` anpassen).
- Muttern + Scheiben als Kombi (Prime): https://www.amazon.de/dp/B0GFC9H67Y (6,49 €, je 100 Stk).

## E – Optional

| ✔ | Teil | Menge | Spezifikation | Shop | Preis | Versand | Link |
|---|---|---|---|---|---:|---:|---|
| ☐ | AS5600-Encoder, 4 Stk inkl. Magnet | 1 (Pack) | Magnetischer Winkelsensor, schiebt sich in den Deckelhalter | Amazon.de Prime | 9,99 € | 0 € | https://www.amazon.de/dp/B0D4YW7NVB |
| ☐ | NEMA17 **mit Doppelwelle** 17HS8401B (RATTMMOTOR) | 1 | 48 mm lang, hintere Welle 10 mm – nötig für AS5600 | Amazon.de Prime | 14,58 € | 0 € | https://www.amazon.de/dp/B09DG1B48W |
| ☐ | Endschalter mechanisch, 4 Stk | 1 (Pack) | Referenzfahrt | Amazon.de Prime | 5,99 € | 0 € | https://www.amazon.de/dp/B0FHB5SR6N |
| ☐ | Netzteil 12 V / 2 A, 5,5×2,1 mm | 1 | externe Motorversorgung | Amazon.de Prime (Velainwom) | 8,49 € | 0 € | https://www.amazon.de/dp/B07JLGHSFJ |
| ☐ | DC-Buchse 5,5×2,1 → Schraubklemme | 1 (Pack) | Anschluss Netzteil | Amazon.de Prime (SatChef) | 3,49 € | 0 € | https://www.amazon.de/dp/B07D62QFWF |
| ☐ | Dosierbecher 54 mm (HAPPOW) | 1 | für die Waage | Amazon.de Prime | 8,59 € | 0 € | https://www.amazon.de/dp/B0GSW3HB9K |
| | **Summe E** | | | | **51,13 €** | | |

Hinweise:
- Der Doppelwellen-Motor **ersetzt** den 6,99-€-Motor aus Block B (Mehrkosten effektiv +7,59 €). In `params.scad` dann `motor_len = 48` und `rear_shaft_len = 10` setzen und STLs neu exportieren.
- Das Standard-SCAD hat `as5600 = true` (Halter im Deckel). Ohne Encoder kannst du `as5600 = false` setzen → flacheres Gehäuse. Magnet: 6×2,5 mm diametral (für `magnet_cap`) – Maß des beiliegenden Magneten prüfen.
- Billigere AS5600-Alternative: 2 Stk DollaTek, 6,99 €: https://www.amazon.de/dp/B0BGPF3WT3 (Magnet prüfen).

## Druckteile (selbst drucken)

Kein Druckservice – nur Material-Empfehlung. STLs: [`hardware/stepper-mount/stl/`](../hardware/stepper-mount/stl/), Details: [3D_PRINTS.md](3D_PRINTS.md).

| Teil | Menge | Material |
|---|---:|---|
| Motor-Gehäuse (`motor_housing`) | 1 | PETG (oder ASA, wärmebeständiger) |
| Gehäusedeckel (`housing_lid`) | 1 | PETG/ASA |
| Seitenstütze (`side_brace`) | 1 | PETG/ASA |
| Ritzel (`pinion`) | 1 | PETG (100 % Infill empfohlen) |
| Spannschlitten (`strap_tension_slider`) | 1 | PETG |
| Magnetkappe (`magnet_cap`, nur mit AS5600) | 1 | PETG |
| Spannband-Glied (`strap_segment`) | 4 | **TPU 95A** (PETG geht mit `strap_t = 1.2`) |
| SGBW-Teile: Rückplatte, Abdeckung, Display-Adapter, Becherhalter, Abdeckungen | je 1 | PETG |

Filament wird als vorhanden vorausgesetzt (eine angebrochene Rolle PETG + etwas TPU reicht).
