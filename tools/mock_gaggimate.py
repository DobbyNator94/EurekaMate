#!/usr/bin/env python3
"""Tiny fake GaggiMate (Xenia-style Grind-by-Sync endpoints) for bench tests.

Written from the observable protocol only (no GaggiMate code). Run it on a PC in the
same WiFi and enter the PC's IP as "GaggiMate host" in the EurekaMate web UI.

    python3 tools/mock_gaggimate.py --port 8080 --shot 31.5

Endpoints:
  GET  /api/v2/machine           -> {"MA_TYPE": 1, ...}
  GET  /api/v2/mako              -> status (MA_STATUS 1 ready, 3 brewing, 4 shot done)
  POST /api/v2/brewratio         -> stores SYNC_BEVERAGE_WEIGHT / SYNC_FILTER, prints them
  POST /api/v2/scripts/execute   -> {"ID": 9} starts a simulated shot
  GET  /shot?t=27.5              -> start a simulated shot of t seconds (browser/curl)
Note: the firmware talks to port 80; use --port 80 (root/admin) or a port-forward.
"""
import argparse, json, threading, time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

state = {"status": 1, "t0": 0.0, "dur": 30.0, "done_at": 0.0, "weight": 36.0, "filter": 1}
lock = threading.Lock()

def tick():
    now = time.time()
    with lock:
        if state["status"] == 3 and now - state["t0"] >= state["dur"]:
            state["status"], state["done_at"] = 4, now
        elif state["status"] == 4 and now - state["done_at"] >= 5:
            state["status"] = 1

def start_shot(dur):
    with lock:
        state.update(status=3, t0=time.time(), dur=dur)
    print(f"[mock] shot started ({dur:.1f} s)")

class H(BaseHTTPRequestHandler):
    def _json(self, obj, code=200):
        b = json.dumps(obj).encode()
        self.send_response(code); self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(b))); self.end_headers(); self.wfile.write(b)

    def do_GET(self):
        tick(); u = urlparse(self.path)
        if u.path == "/api/v2/machine":
            return self._json({"MA_TYPE": 1, "MA_NAME": "mock-gaggimate"})
        if u.path == "/api/v2/mako":
            with lock:
                st = dict(state)
            el = (time.time() - st["t0"]) if st["status"] == 3 else (st["dur"] if st["status"] == 4 else 0)
            return self._json({"MA_STATUS": st["status"], "MA_EXTRACTION_STATUS": 1 if st["status"] == 3 else 0,
                               "REAL_EXTRACTION_TIME_MS": int(el * 1000), "PU_SENS_FLOW_METER_VOLUME": round(el * 1.2, 1),
                               "BG_SET_TEMP": 93.0, "BG_SENS_TEMP": 92.8, "TANK_LEVEL": 1})
        if u.path == "/shot":
            start_shot(float(parse_qs(u.query).get("t", [args.shot])[0])); return self._json({"ok": True})
        self._json({"error": "not found"}, 404)

    def do_POST(self):
        n = int(self.headers.get("Content-Length", 0)); body = self.rfile.read(n) if n else b"{}"
        try: data = json.loads(body or b"{}")
        except ValueError: return self._json({"error": "bad json"}, 400)
        if self.path == "/api/v2/brewratio":
            with lock:
                state["weight"] = data.get("SYNC_BEVERAGE_WEIGHT", state["weight"])
                state["filter"] = data.get("SYNC_FILTER", state["filter"])
            print(f"[mock] grind done -> beverage weight {state['weight']} g, recipe/filter {state['filter']}")
            if args.auto: threading.Timer(3, start_shot, [args.shot]).start()
            return self._json({"ok": True})
        if self.path == "/api/v2/scripts/execute":
            if data.get("ID") == 9: start_shot(args.shot)
            return self._json({"ok": True})
        self._json({"error": "not found"}, 404)

    def log_message(self, *a): pass

ap = argparse.ArgumentParser()
ap.add_argument("--port", type=int, default=80)
ap.add_argument("--shot", type=float, default=30.0, help="simulated shot time in s")
ap.add_argument("--auto", action="store_true", help="auto-start a shot 3 s after grind done")
args = ap.parse_args()
print(f"[mock] fake GaggiMate on :{args.port} (shot {args.shot} s)")
ThreadingHTTPServer(("", args.port), H).serve_forever()
