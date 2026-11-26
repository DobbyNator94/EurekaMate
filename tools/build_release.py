#!/usr/bin/env python3
"""Package a EurekaMate release + the static web flasher site (no GitHub Actions needed).

    pio run -e waveshare-esp32s3-touch-amoled-164
    python3 tools/build_release.py v0.1.0-beta --out ../eurekamate-release --site ../eurekamate-site

Creates in --out:
  eurekamate-<tag>.bin               application (OTA / web UI update)
  eurekamate-<tag>-bootloader.bin    bootloader   @ 0x0
  eurekamate-<tag>-partitions.bin    partitions   @ 0x8000
  blank_8KB.bin                      erases otadata @ 0xE000 -> boots the factory slot
  eurekamate-<tag>-web-ota.bin       detools/heatshrink patch for the Bluetooth web OTA
  eurekamate-<tag>-full.bin          merged image, flash with: esptool.py --chip esp32s3 write_flash 0x0 <file>
  eurekamate-<tag>.manifest.json     ESP Web Tools manifest (relative paths)
  SHA256SUMS.txt
and in --site the flasher (tools/web-flasher) with firmware/<tag>/... and firmware/index.json,
ready to be pushed to the gh-pages branch (same origin -> no CORS problems).

Layout: the app goes to the FACTORY slot (0x20000, same place PlatformIO uploads to) and
otadata is blanked, so the freshly flashed image always boots - even if an older image sits
in ota_0/ota_1. OTA updates later go to ota_0/ota_1 as usual.
"""
import argparse, glob, hashlib, json, os, shutil, subprocess, sys

ENV = "waveshare-esp32s3-touch-amoled-164"
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
OFF_BOOT, OFF_PART, OFF_OTADATA, OFF_APP = 0x0, 0x8000, 0xE000, 0x20000

def sha256(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 16), b""): h.update(b)
    return h.hexdigest()

def find_esptool():
    try:
        import esptool  # noqa: F401  (pip install esptool)
        return [sys.executable, "-m", "esptool"]
    except ImportError:
        pass
    for c in glob.glob(os.path.expanduser("~/.platformio/packages/tool-esptoolpy*/esptool.py")):
        return [sys.executable, c]
    if shutil.which("esptool.py"): return ["esptool.py"]
    return None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("tag")
    ap.add_argument("--out", required=True)
    ap.add_argument("--site")
    a = ap.parse_args()
    tag, base = a.tag, f"eurekamate-{a.tag}"
    build = os.path.join(ROOT, ".pio", "build", ENV)
    os.makedirs(a.out, exist_ok=True)
    o = lambda n: os.path.join(a.out, n)
    shutil.copy(os.path.join(build, "firmware.bin"), o(f"{base}.bin"))
    shutil.copy(os.path.join(build, "bootloader.bin"), o(f"{base}-bootloader.bin"))
    shutil.copy(os.path.join(build, "partitions.bin"), o(f"{base}-partitions.bin"))
    with open(o("blank_8KB.bin"), "wb") as f: f.write(b"\xff" * 8192)   # erased flash state

    # Bluetooth web OTA expects a detools heatshrink patch (from an empty image = full image)
    try:
        import detools
        empty = o(".empty.bin"); open(empty, "wb").close()
        subprocess.run([sys.executable, "-m", "detools", "create_patch", "-c", "heatshrink", empty,
                        o(f"{base}.bin"), o(f"{base}-web-ota.bin")], check=True)
        os.remove(empty)
    except ImportError:
        print("WARNING: detools not installed - no -web-ota.bin (Bluetooth web update)", file=sys.stderr)

    est = find_esptool()
    if est:
        subprocess.run(est + ["--chip", "esp32s3", "merge_bin", "-o", o(f"{base}-full.bin"),
                              "--flash_mode", "keep", "--flash_freq", "keep", "--flash_size", "keep",
                              hex(OFF_BOOT), o(f"{base}-bootloader.bin"), hex(OFF_PART), o(f"{base}-partitions.bin"),
                              hex(OFF_OTADATA), o("blank_8KB.bin"), hex(OFF_APP), o(f"{base}.bin")], check=True)
    version = tag.lstrip("v")
    manifest = {
        "name": "EurekaMate",
        "version": version,
        "new_install_prompt_erase": True,
        "builds": [{"chipFamily": "ESP32-S3", "parts": [
            {"path": f"{base}-bootloader.bin", "offset": OFF_BOOT},
            {"path": f"{base}-partitions.bin", "offset": OFF_PART},
            {"path": "blank_8KB.bin", "offset": OFF_OTADATA},
            {"path": f"{base}.bin", "offset": OFF_APP}]}],
    }
    with open(o(f"{base}.manifest.json"), "w") as f: json.dump(manifest, f, indent=2); f.write("\n")
    files = sorted(n for n in os.listdir(a.out) if not n.startswith(".") and n != "SHA256SUMS.txt")
    with open(o("SHA256SUMS.txt"), "w") as f:
        for n in files: f.write(f"{sha256(o(n))}  {n}\n")
    print("release files:", *files, sep="\n  ")

    if a.site:
        if os.path.exists(a.site):
            for n in os.listdir(a.site):
                if n == ".git": continue
                p = os.path.join(a.site, n)
                shutil.rmtree(p) if os.path.isdir(p) else os.remove(p)
        else:
            os.makedirs(a.site)
        src = os.path.join(ROOT, "tools", "web-flasher")
        for n in ("index.html", "flasher.js", "README.md"):
            shutil.copy(os.path.join(src, n), os.path.join(a.site, n))
        fw = os.path.join(a.site, "firmware", tag); os.makedirs(fw)
        for n in files:
            if n.endswith("-full.bin"): continue   # keep the site small; full image is on the release
            shutil.copy(o(n), os.path.join(fw, n))
        entry = {"tag": tag, "version": version, "display": tag, "prerelease": any(k in tag for k in ("-rc", "-beta", "-alpha")),
                 "manifest": f"firmware/{tag}/{base}.manifest.json"}
        if os.path.exists(o(f"{base}-web-ota.bin")): entry["ota"] = f"firmware/{tag}/{base}-web-ota.bin"
        with open(os.path.join(a.site, "firmware", "index.json"), "w") as f: json.dump([entry], f, indent=2); f.write("\n")
        open(os.path.join(a.site, ".nojekyll"), "w").close()
        print("site ready:", a.site)

if __name__ == "__main__":
    main()
