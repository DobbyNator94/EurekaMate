# EurekaMate – Bauanleitung Schritt für Schritt 🛠️

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

🇬🇧 English version: **[BUILD_GUIDE_EN.md](BUILD_GUIDE_EN.md)**

Diese Anleitung ist für Einsteiger. Kurze Sätze, ein Bild pro Schritt. Arbeite die Schritte **der Reihe nach** ab.

> ⚠️ **BETA:** Die Firmware **v0.1.0-beta** ist noch **nicht an einer echten Mühle getestet**.
> Nachbau auf eigene Gefahr. Miss alles nach.

> 🔴 **230 V – Lebensgefahr!**
> In der Mühle liegen 230 V. **Vor jedem Öffnen: Netzstecker ziehen.** Ausschalten reicht nicht.
> Wir arbeiten **nur** im Niederspannungsteil (12 V / 5 V). An 230-V-Teilen wird nichts gelötet.

**Links:** [⚡ Web-Flasher](https://dobbynator94.github.io/EurekaMate/) ·
[📦 Release v0.1.0-beta](https://github.com/DobbyNator94/EurekaMate/releases/tag/v0.1.0-beta) ·
[📘 Ausführliche Anleitung](ANLEITUNG_DE.md) · [🛒 Einkaufsliste](EINKAUFSLISTE_DE.md) · [🔌 Schaltplan](img/schaltplan.png)

## Übersicht

| Schritt | Was | Dauer |
|---|---|---|
| [1](#1-was-du-brauchst) | Was du brauchst | – |
| [2](#2-was-du-druckst) | Was du druckst | ~10 h Drucker |
| [3](#3-werkzeug) | Werkzeug | – |
| [4](#4-mühle-sicher-öffnen) | Mühle sicher öffnen | 15 min |
| [5](#5-kabel-verbinden--eins-nach-dem-anderen) | Kabel verbinden | 1–2 h |
| [6](#6-multimeter-checks) | Multimeter-Checks | 20 min |
| [7](#7-halter-montieren) | Halter montieren | 30 min |
| [8](#8-firmware-flashen-per-web-flasher) | Firmware flashen (Web-Flasher) | 5 min |
| [9](#9-erste-einrichtung) | Erste Einrichtung | 15 min |
| [10](#10-gaggimate-verbinden) | GaggiMate verbinden | 10 min |
| [11](#11-erster-shot--fehlersuche) | Erster Shot + Fehlersuche | – |

> 💡 **Tipp:** Mach Schritt 8 (Flashen) ruhig **zuerst** am Schreibtisch. Dann weißt du, dass das Board funktioniert, bevor du lötest.

---

## 1. Was du brauchst

![Einkaufsliste](img/bauanleitung/01_einkaufsliste.png)

| # | Teil | Menge | Preis (Stand 26.11.2026) |
|---|---|---|---|
| 1 | Waveshare **ESP32-S3-Touch-AMOLED-1.64** | 1 | 33,99 € |
| 2 | Wägezelle **T70 1 kg** + **HX711**-Modul | 1 | 28,53 € + 5,49 € |
| 3 | **NEMA17** Schrittmotor 17HS4417P1 / 17HS4401, 40 mm (für AS5600: Doppelwelle 17HS8401B) | 1 | 6,99 € (14,58 €) |
| 4 | **TMC2209** Treiber mit Kühlkörper | 1 | 8,49 € |
| 5 | **Step-Down 5 V** MP1584 (auf 5,0 V einstellen) | 1 | 5,99 € (5er) |
| 6 | Elko **100–470 µF / 25 V** (für den Motortreiber, z. B. 220 µF) | 1 | 3,45 € (24er) |
| 7 | Elko **1000 µF / ≥10 V** (für den ESP) | 1 | 4,59 € (10er) |
| 8 | Widerstand **1 kΩ** | 1 | 5,99 € (Sortiment) |
| 9 | **Sicherung 1 A** flink + Inline-Halter | 1 | 3,89 € + 6,49 € |
| 10 | Zahnrad-Einstellrad-Kit für Eureka Mignon (ARO) | 1 | 42,00 € |
| 11 | Litze 22 AWG, JST-PH-4-pol./Dupont, Schrumpfschlauch, Kabelbinder | – | 37,24 € (Sets) |
| 12 | Schrauben M3 (4× 6 mm Motor, 4× 8 mm Deckel, 2× 10 mm + Scheiben, 1× 30 mm + Mutter, Madenschraube M3×4 + Mutter) + 6× Einschmelzmutter M3×5,7 | – | 30,18 € (falls nicht vorhanden) |
| 13 | Filament **PETG** + **TPU 95A** | – | – |
| opt. | AS5600 + Magnet 6×2,5 mm · Endschalter · Netzteil 12 V/2 A | – | 51,13 € (alles) |

🛒 **Komplette Einkaufsliste mit geprüften Amazon-Links, Versandkosten und Summen: [EINKAUFSLISTE_DE.md](EINKAUFSLISTE_DE.md)**
(Pflicht ≈ 193 € ohne / ≈ 223 € mit Schrauben & Inserts, Stand 26.11.2026)

Zusätzlich brauchst du die **Waagen- und Display-Teile von smart-grind-by-weight** (siehe [DOC.md](DOC.md)).

---

## 2. Was du druckst

> ⚠️ **Erst messen, dann drucken!** Werte mit `TODO-MEASURE` in
> [`params.scad`](../hardware/stepper-mount/scad/params.scad) an deine Mühle und dein Kit anpassen.
> Dann `./export.sh` im Ordner `hardware/stepper-mount` laufen lassen.

Alle Teile liegen in [`hardware/stepper-mount/stl/`](../hardware/stepper-mount/stl/). Die Bilder zeigen das Teil **so, wie es aufs Druckbett kommt**.

| Teil | Bild (so aufs Bett) | Menge | Material | Einstellungen |
|---|---|---|---|---|
| **Ritzel** `pinion.stl` | ![](img/bauanleitung/02_druck_pinion.png) | 1 | PETG | 0,12–0,16 mm Schicht, 100 % Füllung, **Zähne nach unten**. **Zuerst drucken** und Passung testen! |
| **Motorgehäuse** `motor_housing.stl` | ![](img/bauanleitung/02_druck_motor_housing.png) | 1 | PETG | 0,2 mm, 4 Wände, 30–40 % Gyroid, **keine Stützen** |
| **Deckel** `housing_lid.stl` | ![](img/bauanleitung/02_druck_housing_lid.png) | 1 | PETG | 0,2 mm, flache Seite nach unten (STL ist schon gedreht), keine Stützen |
| **Seitenhalter** `side_brace.stl` | ![](img/bauanleitung/02_druck_side_brace.png) | 1 | PETG | 0,2 mm, 4 Wände, 40 %, stehend wie im Bild, **Tree-Stützen unter dem oberen Arm und der Ablage** |
| **Spannband-Glied** `strap_segment.stl` | ![](img/bauanleitung/02_druck_strap_segment.png) | **4** | TPU 95A (oder PETG mit `strap_t = 1.2`) | flach, 0,2 mm, langsam (20–30 mm/s bei TPU) |
| **Spannschieber** `strap_tension_slider.stl` | ![](img/bauanleitung/02_druck_strap_tension_slider.png) | 1 | PETG | 0,2 mm, 100 % |
| **Magnetkappe** `magnet_cap.stl` (optional) | ![](img/bauanleitung/02_druck_magnet_cap.png) | 1 | PETG | nur mit AS5600 und Doppelwellen-Motor |

Reihenfolge: **Ritzel → Seitenhalter → Gehäuse → Deckel → Band.**

---

## 3. Werkzeug

![Werkzeug](img/bauanleitung/03_werkzeug.png)

- Lötkolben (≥ 60 W), Lötzinn
- **Multimeter** (V⎓, Ω, Durchgang)
- Seitenschneider, Abisolierzange
- Kreuzschraubendreher, Innensechskant, kleiner Schlitzschraubendreher (für das Poti am Step-Down)
- Messschieber
- Schrumpfschlauch + Heißluft oder Feuerzeug, Kabelbinder
- 3D-Drucker
- PC/Mac mit **Chrome oder Edge** und einem **USB-C-Datenkabel**

---

## 4. Mühle sicher öffnen

![Netzstecker ziehen](img/bauanleitung/04_stecker_ziehen.png)

> 🔴 **230 V!** Netzstecker ziehen. **2 Minuten warten** (der Motorkondensator kann noch geladen sein).

1. **Netzstecker ziehen.** Wirklich ziehen, nicht nur ausschalten.
2. Bohnenbehälter abnehmen. Mühle auf ein Handtuch legen.
3. Mühle von unten öffnen, wie im SGBW-Original beschrieben ([DOC.md](DOC.md)).
4. Powerboard suchen: kleiner **Trafo**, schwarzer **DF10S** (4 Beinchen, „+“ und „−“), **Elko** daneben, **78L05** (3 Beinchen).
5. Mach ein **Foto**, bevor du etwas absteckst. So weißt du später, wie es war.
6. Fass nichts an, was mit dem Netzkabel, dem Motor oder dem Relais verbunden ist.

---

## 5. Kabel verbinden – eins nach dem anderen

> 🔴 **Mühle bleibt ausgesteckt**, solange du lötest.
> Farbig = **neu** in diesem Schritt. Grau = schon fertig.

Gesamtplan: [Schaltplan](img/schaltplan.png) · Tabelle: [ANLEITUNG_DE.md § 3](ANLEITUNG_DE.md#verdrahtungstabelle)

### 5a. 12-V-Abgriff → Sicherung → Step-Down

![5a](img/bauanleitung/05a_abgriff_buck.png)

1. Rote Litze an **+12 V** (DF10S „+“ oder Elko „+“) löten.
2. Schwarze Litze an **GND** (Elko „−“).
3. **1-A-Sicherung** in die rote Leitung.
4. Rot an **IN+**, schwarz an **IN−** vom Step-Down.
5. An **OUT** noch nichts anschließen!

### 5b. Step-Down → ESP (5 V)

![5b](img/bauanleitung/05b_buck_esp.png)

> ⚠️ **Erst Schritt 6 A machen:** Step-Down auf **5,0 V** einstellen. Zu viel Spannung zerstört den ESP.

1. **OUT+** → ESP **5V**.
2. **OUT−** → ESP **GND**.
3. **1000-µF-Elko** an 5V und GND. Langes Bein = **+**. Der Streifen am Elko = **−**.

### 5c. ESP → TMC2209

![5c](img/bauanleitung/05c_esp_tmc.png)

1. GPIO7 → **EN**, GPIO5 → **STEP**, GPIO6 → **DIR**, GPIO1 → **DIAG**.
2. GPIO8 → **1-kΩ-Widerstand** → **PDN_UART**.
3. ESP 3V3 → **VIO**, ESP GND → **GND**.
4. **MS1** und **MS2** an GND.
5. **VM** an +12 V (hinter der Sicherung), GND (Power) an GND.
6. **100-µF-Elko** direkt an VM/GND. Polung beachten!
7. Kühlkörper auf den TMC2209 kleben.

### 5d. TMC2209 → Motor

![5d](img/bauanleitung/05d_tmc_motor.png)

1. Erst die **Spulenpaare messen** (Schritt 6 B).
2. Paar A → **1A / 1B**. Paar B → **2A / 2B**.
3. Motor **nie** unter Strom an- oder abstecken. Das zerstört den Treiber.

### 5e. ESP → Mühlenstecker

![5e](img/bauanleitung/05e_esp_muehle.png)

1. **Pin 1 (+5 V): nicht verbinden!** Abschneiden und isolieren.
2. **Pin 2** (Taste) → GPIO17. Vorher messen (Schritt 6 E)!
3. **Pin 3** (Relais) → GPIO18.
4. **Pin 4** (GND) → ESP GND.

### 5f. ESP → Waage (HX711)

![5f](img/bauanleitung/05f_waage.png)

1. HX711 **SCK** → GPIO2, **DT** → GPIO3.
2. **VCC** → 3V3, **GND** → GND.
3. Wägezelle wie im SGBW-Original an den HX711.

### 5g. Optional: AS5600 und Endschalter

![5g](img/bauanleitung/05g_optional.png)

1. AS5600: SDA → GPIO15, SCL → GPIO16, VCC → 3V3, GND → GND, **DIR → GND**.
2. Endschalter: GPIO44 ↔ GND.
3. Beides darfst du weglassen.

> 💡 Alle Lötstellen mit **Schrumpfschlauch** isolieren. Kabel mit **Kabelbinder** sichern – weg von 230-V-Teilen und vom Mühlenmotor.

---

## 6. Multimeter-Checks

![Multimeter](img/bauanleitung/06_multimeter.png)

| Check | Wie | Soll | Wenn nicht |
|---|---|---|---|
| **A** Step-Down | IN an 12 V, OUT offen, Poti drehen | **5,00–5,10 V** | weiterdrehen; Mini560 tauschen |
| **B** Motorspulen | Ω-Messung zwischen Motordrähten | **~1,5–2 Ω** = ein Paar; OL = andere Spule | – |
| **C** Durchgang GND | Stecker gezogen: Elko − ↔ Mühlenstecker Pin 4 | piept | anderen GND-Punkt suchen |
| **D** 12-V-Abgriff | eingesteckt, nichts anfassen | **11–14 V⎓** | Foto in ein Issue stellen |
| **E** Pin 2 (Taste) | eingesteckt, Taste nicht gedrückt | **≤ 3,3 V** | Spannungsteiler 10 kΩ/20 kΩ oder Taste weglassen |

> 🔴 **D und E** misst du mit eingesteckter Mühle. **Nur** isolierte Messspitzen, **nur** an den 12-V/5-V-Punkten. Danach **wieder ausstecken**.

---

## 7. Halter montieren

<table><tr>
<td><img src="img/bauanleitung/07_montage_front.png" alt="vorne"></td>
<td><img src="img/bauanleitung/07_montage_rechts.png" alt="rechts"></td>
</tr><tr>
<td><img src="img/bauanleitung/07_montage_hinten.png" alt="hinten"></td>
<td><img src="img/bauanleitung/07_montage_oben.png" alt="oben"></td>
</tr><tr><td colspan="2" align="center"><sub>Fertig montiert: vorne, rechts, hinten, oben (OpenSCAD-Render)</sub></td></tr></table>

**Explosionszeichnung** – so gehört alles zusammen:

![Explosion](img/bauanleitung/07_montage_explosion.png)

| | Schritt |
|---|---|
| ![](img/bauanleitung/07a_halter.png) | **7a.** Seitenhalter von **unten** an die **rechte** Seite schieben, bis der obere Arm auf der Kante liegt. |
| ![](img/bauanleitung/07b_band.png) | **7b.** Spannband (4 Glieder) einmal um die Mühle. Vorne einhängen, hinten in den Spannschieber. M3×30 festziehen – fest, aber nicht brutal. |
| ![](img/bauanleitung/07c_gehaeuse.png) | **7c.** Motor mit 4× **M3×6** ins Gehäuse (nicht länger – das Motorgewinde ist nur ~4,5 mm tief) (Welle nach unten). Ritzel aufstecken, Madenschraube auf die flache Seite. Elektronik einsetzen. Gehäuse mit 2× M3×10 + Scheiben auf die Säulen. |
| ![](img/bauanleitung/07d_deckel.png) | **7d.** **Zahnspiel einstellen:** Gehäuse im Langloch zum Knopf schieben, bis die Zahnräder greifen. Minimal zurück (ein Blatt Papier). Festziehen. Deckel mit 4× **M3×8** drauf (in die Einschmelzmuttern). |

✅ **Test:** Mühle aus. Rad einmal von Hand über den ganzen Bereich drehen. Es darf **nirgends klemmen**.

---

## 8. Firmware flashen per Web-Flasher

Kein Programmieren nötig. Alles passiert im Browser.

👉 **Web-Flasher: https://dobbynator94.github.io/EurekaMate/**

> ⚠️ **Zum Flashen:** Board am Schreibtisch **nur per USB** anschließen. Die Mühle ist **ausgesteckt**.

### 8a. Was du brauchst

![Voraussetzungen](img/bauanleitung/08a_voraussetzungen.png)

- Einen **PC, Mac oder Linux-Rechner**.
- **Google Chrome** oder **Microsoft Edge** (Desktop-Version).
  Firefox, Safari, iPhone und iPad gehen **nicht** (kein Web Serial).
- Ein **USB-C-Datenkabel**. Viele billige Kabel können nur laden! Wenn kein Port erscheint, zuerst das Kabel tauschen.

### 8b. Flasher öffnen und „Installieren“ klicken

![Flasher-Seite](img/bauanleitung/08b_flasher_markiert.png)

1. Board per USB-C an den PC stecken.
2. **https://dobbynator94.github.io/EurekaMate/** in Chrome/Edge öffnen.
3. ① Warnhinweis lesen.
4. ② Reiter **„1. Installieren (USB)“** ist schon ausgewählt.
5. ③ Version **v0.1.0-beta** wählen (Haken „Beta-Versionen anzeigen“ ist gesetzt).
6. ④ Auf **„⚡ Installieren / Install (USB)“** klicken.

### 8c. Port im Browser-Dialog wählen

![Port-Dialog](img/bauanleitung/08c_port_dialog.png)

1. Der Browser fragt nach einem **seriellen Port**.
2. Wähle das Board:
   - Windows: „USB JTAG/serial debug unit (COMx)“ oder „USB-Serial (COMx)“
   - macOS: „cu.usbmodem…“
   - Linux: „ttyACM0“
3. Auf **„Verbinden“** klicken.

> 💡 Nicht sicher, welcher Port? Board abstecken – der Eintrag, der verschwindet, ist es.

### 8d. Installieren und Löschen

![Installieren](img/bauanleitung/08d_installieren_loeschen.png)

1. Im EurekaMate-Fenster auf **„INSTALL EUREKAMATE“** klicken.
2. **Erstes Mal:** Haken bei **„Erase device“** (Gerät löschen) setzen. Damit ist die alte Firmware weg.
3. **„Next“**, dann **„Install“** klicken.

> ℹ️ Später nochmal per USB flashen und Einstellungen behalten? Dann den Haken **weglassen**.

### 8e. Warten

![Fortschritt](img/bauanleitung/08e_fortschritt.png)

1. Erst „Preparing / Erasing“, dann „Writing“ mit Prozent.
2. Dauert **1–2 Minuten**.
3. **Kabel nicht abziehen. Tab nicht schließen.**

### 8f. Fertig!

![Fertig](img/bauanleitung/08f_fertig.png)

1. „Installation complete!“ erscheint. Auf **„Next“** klicken.
2. Das Display zeigt kurz das **EurekaMate-Logo**, dann die Waage.
3. Startet nichts? Einmal **RESET** drücken oder USB kurz ab- und anstecken.

### 8g. Board wird nicht erkannt? → BOOT-Modus

![BOOT/RESET](img/bauanleitung/08g_boot_reset.png)

1. **BOOT** gedrückt **halten**.
2. **RESET** kurz drücken.
3. **BOOT** loslassen.
4. Im Flasher nochmal **„Installieren“** klicken und den Port wählen.
5. Nach dem Flashen einmal **RESET** drücken.

### 8h. Treiber und Probleme

| Problem | Lösung |
|---|---|
| Kein Port im Dialog | **Anderes USB-C-Kabel** (Datenkabel!), anderer USB-Port, kein USB-Hub, dann BOOT-Modus (8g) |
| Windows: unbekanntes Gerät | Der ESP32-S3 hat USB eingebaut. Windows 10/11 braucht **keinen Treiber**. Sonst: Windows-Update laufen lassen, Gerät im Geräte-Manager entfernen, neu einstecken |
| Linux: „Permission denied“ | Benutzer in die Gruppe `dialout` (bzw. `uucp`) aufnehmen: `sudo usermod -aG dialout $USER`, dann ab- und wieder anmelden |
| macOS fragt nach Zugriff | Erlauben. Keine Treiber nötig |
| „Browser wird nicht unterstützt“ | Chrome oder Edge am PC nehmen (nicht Handy, nicht Firefox/Safari) |
| „Failed to connect“ / Timeout | Andere Programme schließen, die den Port nutzen (Arduino, PlatformIO-Monitor, Cura). BOOT-Modus (8g) |
| Bricht mittendrin ab | Kabel/Port tauschen, nochmal mit „Erase device“ |
| Display bleibt schwarz | RESET drücken. Nochmal flashen mit „Erase device“ |

**Updates später:** ohne Kabel über die Weboberfläche (Reiter *Update*) oder im Flasher im Reiter **„Update (Bluetooth)“**.

<details><summary>Für Profis: ohne Browser mit esptool</summary>

Dateien aus dem [Release](https://github.com/DobbyNator94/EurekaMate/releases/tag/v0.1.0-beta):

```bash
pip install esptool
esptool --chip esp32s3 write-flash 0x0 eurekamate-v0.1.0-beta-full.bin
```
</details>

---

## 9. Erste Einrichtung

### 9a. Hotspot

![Hotspot](img/bauanleitung/09a_hotspot.png)

1. Mühle einschalten. Ohne WLAN startet der Hotspot **`EurekaMate-Setup`**.
2. Das **8-stellige Passwort** steht am Display: *Menü → Grind Sync → Network*.
3. Mit dem Handy verbinden. Die Seite öffnet sich von selbst (sonst **http://192.168.4.1**).
4. Reiter **„WLAN & GaggiMate“** → *Netzwerke suchen* → dein WLAN (**2,4 GHz**) → Passwort → *Speichern*.
5. Ab jetzt: **http://eurekamate.local** (oder die IP vom Display).

![Weboberfläche Netzwerk](img/ui/web_net.png)

### 9b. Motor prüfen

![Motor-Reiter](img/ui/web_motor.png)

1. Reiter **Motor**. Startwerte: Laufstrom 400 mA, Boost 600 mA, Haltestrom 0 %.
2. Ein paar Schritte fahren. Dreht er falsch herum? → **Richtung umkehren**.

### 9c. Kalibrierung 0 / 100

> ⚠️ Die Mühle bleibt dabei **AUS**. Nicht mahlen!

<table><tr>
<td width="40%"><img src="img/ui/display_calibration.png" alt="Display Kalibrierung"></td>
<td width="60%"><img src="img/ui/web_dial.png" alt="Web Kalibrierung"></td>
</tr></table>

1. Reiter **Mahlgrad** → *Kalibrierung starten* (oder am Display *Dial Calibration*).
2. Mit den Tasten **feiner** fahren, bis die Mahlscheiben **gerade so berühren** (leises Schaben) → **„0 setzen“**.
3. Zum **gröbsten** Punkt fahren, den du brauchst → **„100 setzen“**.
4. Sicherheitsabstand lassen (5). Darunter fährt der Motor nie.
5. **Fertig.** Jetzt zeigt das Display 0–100.

### 9d. Rezepte anlegen

![Rezepte](img/ui/web_rec.png)

Beispiel: *Double* – GbS an, 28 s, 36 g, Start-Mahlgrad 15. *Filter* – GbS aus, Mahlgrad 60.

---

## 10. GaggiMate verbinden

![GaggiMate](img/bauanleitung/10_gaggimate.png)

1. GaggiMate muss **v1.10.0 oder neuer** sein.
2. In GaggiMate: *Einstellungen → Plugins → „Mahlkönig E64 WS Grind-by-Sync“* **aktivieren**.
3. Jedem Rezeptplatz (1–6) ein Profil zuordnen. Speichern. **Neu starten.**
4. Im Router für GaggiMate eine **feste IP** einstellen.
5. In EurekaMate: Reiter **WLAN & GaggiMate** → *Suchen (mDNS)* oder IP eintragen → **Verbindung testen**.
6. Es muss **„GaggiMate Grind-by-Sync OK“** kommen.

(Das Bild ist eine vereinfachte Illustration, nicht die echte GaggiMate-Oberfläche.)

---

## 11. Erster Shot + Fehlersuche

![Erster Shot](img/bauanleitung/11_erster_shot.png)

1. Rezept antippen. Der Motor fährt (Balken). **Warten, bis der Balken weg ist.**
2. Mahlen. Die Waage stoppt bei der Zieldosis.
3. Shot an GaggiMate ziehen.
4. EurekaMate wertet die Bezugszeit aus: **zu lang → gröber**, **zu kurz → feiner**.
5. Nächster Shot. Nach 2–3 Shots sollte die Zeit passen.
6. Im Reiter **Verlauf** siehst du jede Korrektur und den Grund.

![Status](img/ui/web_status.png)

> 🛡️ **Sicherheit:** Während der Motor fährt, startet die Mühle **nicht**. Während die Mühle mahlt, bewegt sich das Rad **nie**.

### 12-V-Belastungstest (wichtig!)

Multimeter am 12-V-Abgriff lassen. Mehrmals ganz fein ↔ ganz grob fahren und einmal mahlen. Die Spannung muss **≥ 10 V** bleiben, das Display darf nicht neu starten. Sonst: externes **12-V/2-A-Netzteil** (siehe [ANLEITUNG_DE.md § 9](ANLEITUNG_DE.md#9-belastungstest-der-12-v-schiene-wichtig)).

### Fehlersuche

| Problem | Lösung |
|---|---|
| Kein Hotspot | Schon ein WLAN gespeichert → Display *Network → Setup hotspot* |
| „GaggiMate not found“ | Plugin an? Gleiches 2,4-GHz-WLAN? IP statt `gaggimate.local` eintragen |
| „no Grind-by-Sync endpoint“ | GaggiMate < v1.10.0 oder Plugin aus |
| Motor brummt, dreht nicht | Spulenpaare falsch → nachmessen. Boost-Strom erhöhen |
| Motor dreht falsch herum | *Motor → Richtung umkehren*, neu kalibrieren |
| Motor stoppt mit „Stall“ | StallGuard-Wert senken oder 0 |
| Rad zu schwer | Boost erhöhen, kleineres Ritzel (10 Z), Getriebemotor 5:1 |
| Mahlgrad wird nicht korrigiert | GbS im Rezept aus? Shot < 15 s oder abgebrochen? → Reiter Verlauf |
| TMC wird heiß | Kühlkörper, Laufstrom senken, Haltestrom 0 % |
| Display startet neu beim Fahren | 12-V-Schiene zu schwach → Netzteil-Fallback |

Mehr: [ANLEITUNG_DE.md](ANLEITUNG_DE.md) · [TROUBLESHOOTING.md](TROUBLESHOOTING.md) · [Issue erstellen](https://github.com/DobbyNator94/EurekaMate/issues)

---

<sub>Bilder erzeugt mit `tools/gen_bauanleitung_images.py` (Illustrationen), OpenSCAD (Renders) und Headless-Chrome (Flasher-Screenshot).</sub>
