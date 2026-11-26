#!/usr/bin/env python3
"""Regenerate src/sync/sync_web_page.h from src/sync/sync_web_page.html."""
from pathlib import Path
root = Path(__file__).resolve().parents[1] / "src" / "sync"
html = (root / "sync_web_page.html").read_text(encoding="utf-8")
(root / "sync_web_page.h").write_text(
    "#pragma once\n// Generated from sync_web_page.html (tools/gen_web_page.py). Do not edit by hand.\n"
    "#include <pgmspace.h>\nstatic const char SYNC_WEB_PAGE[] PROGMEM = R\"SGSHTML(" + html + ")SGSHTML\";\n",
    encoding="utf-8")
print("ok")
