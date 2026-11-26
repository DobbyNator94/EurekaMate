#!/usr/bin/env python3
"""Serve the EurekaMate web UI with fake data (for screenshots / UI work without hardware).

    python3 tools/webui_preview.py --port 8090   ->  http://localhost:8090/#status
"""
import argparse, json, os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PAGE = open(os.path.join(ROOT, "src", "sync", "sync_web_page.html"), encoding="utf-8").read()
NAMES = ["Single", "Double", "Custom", "Filter", "Decaf", "Preset 6"]
recipes = [{"name": n, "gbs": i in (0, 1, 2, 4), "target_time_s": 28.0, "brew_weight_g": [36, 40, 0, 0, 36, 0][i],
            "start_setting": [14.0, 15.5, -1, 60.0, 12.0, -1][i], "hopper_slot": -1, "portions": 3 if i == 3 else 1}
           for i, n in enumerate(NAMES)]
STATUS = {
    "build": 0, "uptime_s": 5321,
    "wifi": {"connected": True, "ip": "192.168.1.57", "ssid": "Kaffeenetz", "rssi": -58, "ap": False, "ap_ssid": ""},
    "machine": {"seen": True, "identity_ok": True, "ip": "192.168.1.40", "state": "ready", "extraction_ms": 0,
                "waiting_for_shot": False, "poll_errors": 0},
    "last_shot": {"valid": True, "time_s": 30.4, "weight_g": 36.2, "recipe": 2, "verdict": "adjust", "delta": 0.3},
    "shots_seen": 42, "adjustments": 17, "recipe_setting": [14.2, 15.8, -1, 60.0, 12.4, -1],
    "dial": {"calibrated": True, "trusted": True, "moving": True, "calibrating": False, "setting": 31.6, "min_setting": 5.0,
             "steps": 5560, "zero_steps": 0, "coarse_steps": 17600, "encoder": True, "slip": False, "stalled": False,
             "brownout": False, "home_ref": True, "error": "", "progress": 62, "eta_ms": 1400, "dose_allowed": False,
             "grinder_start_allowed": False, "target": 15.8},
    "active_recipe": 2, "portion_done": 0, "portion_total": 1, "hopper_slot": -1,
    "motor": {"microsteps": 16, "run_current_ma": 400, "boost_current_ma": 600, "stall_threshold": 0, "hold_current_pct": 0,
              "max_speed_sps": 800, "fast_speed_sps": 4000, "fast_accel_sps2": 6000, "full_range_s": 5.07, "accel_sps2": 1500,
              "backlash_steps": 0, "invert_dir": False, "use_as5600": True, "use_endstop": False, "home_on_boot": False},
    "config": {"enabled": True, "auto_adjust": True, "send_recipe": True, "start_brew_after_grind": False, "follow_profile": True,
               "profile_slots": 3, "ratio": 2.0, "host": "gaggimate.local", "deadband_s": 1.5, "gain_per_s": 0.3, "max_step": 2.0,
               "yield_tolerance": 0.25, "recipes": recipes, "hopper_enabled": False, "hopper_slots": 4},
}
HIST = {"shots": [{"seq": 42 - i, "recipe": 2, "time_s": t, "weight_g": 36.0 + i * 0.1, "target_s": 28.0, "delta": d,
                   "setting_after": 15.8 - i * 0.3, "verdict": v, "manual": False}
                  for i, (t, d, v) in enumerate([(30.4, 0.3, "adjust"), (28.9, 0, "in_deadband"), (25.1, -0.4, "adjust"),
                                                 (12.0, 0, "too_short"), (31.8, 0.7, "adjust")])]}

class H(BaseHTTPRequestHandler):
    def _send(self, body, ctype="application/json"):
        b = body.encode() if isinstance(body, str) else body
        self.send_response(200); self.send_header("Content-Type", ctype); self.send_header("Content-Length", str(len(b)))
        self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        p = self.path.split("?")[0]
        if p in ("/", "/index.html"): return self._send(PAGE, "text/html; charset=utf-8")
        if p == "/api/status": return self._send(json.dumps(STATUS))
        if p == "/api/history": return self._send(json.dumps(HIST))
        if p == "/api/scan": return self._send(json.dumps({"networks": [{"ssid": "Kaffeenetz", "rssi": -58}]}))
        if p == "/api/discover": return self._send(json.dumps({"devices": [{"host": "gaggimate.local", "ip": "192.168.1.40"}]}))
        self._send(json.dumps({"ok": True}))
    def do_POST(self):
        self._send(json.dumps({"ok": True, "msg": "preview"}))
    def log_message(self, *a): pass

ap = argparse.ArgumentParser(); ap.add_argument("--port", type=int, default=8090); a = ap.parse_args()
print(f"preview on http://localhost:{a.port}/")
ThreadingHTTPServer(("127.0.0.1", a.port), H).serve_forever()
