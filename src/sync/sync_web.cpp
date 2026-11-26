#include "sync_web.h"

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <math.h>
#include "grind_sync.h"
#include "sync_web_page.h"
#include "../config/build_info.h"
#include "../config/logging.h"
#include "../motor/dial_motor.h"

SyncWeb sync_web;

namespace {
WebServer* server = nullptr;
DNSServer* dns = nullptr;
Preferences web_prefs;
UpdateClass web_update;  // own instance: the project builds with NO_GLOBAL_UPDATE (esp32-flashz)
String web_password;  // empty = no login (home network); the setup AP has its own password

float num_or_nan(float v) { return isnan(v) ? -1.0f : v; }

bool authorized() {
    if (web_password.isEmpty()) return true;
    if (server->authenticate("admin", web_password.c_str())) return true;
    server->requestAuthentication(BASIC_AUTH, "EurekaMate");
    return false;
}

void send_json(JsonDocument& doc, int code = 200) {
    String out;
    serializeJson(doc, out);
    server->sendHeader("Cache-Control", "no-store");
    server->send(code, "application/json", out);
}

void send_ok(bool ok, const char* msg = nullptr) {
    JsonDocument doc;
    doc["ok"] = ok;
    if (msg) doc["msg"] = msg;
    send_json(doc, ok ? 200 : 400);
}

bool parse_body(JsonDocument& doc) {
    if (!server->hasArg("plain")) return false;
    return deserializeJson(doc, server->arg("plain")) == DeserializationError::Ok;
}

void fill_config(JsonObject o, const GrindSync::Config& c) {
    o["enabled"] = c.enabled;
    o["auto_adjust"] = c.auto_adjust;
    o["send_recipe"] = c.send_recipe;
    o["start_brew_after_grind"] = c.start_brew_after_grind;
    o["follow_profile"] = c.follow_profile;
    o["profile_slots"] = SYNC_PROFILE_SLOTS;
    o["ratio"] = c.ratio;
    o["host"] = c.host;
    o["deadband_s"] = c.reg.deadband_s;
    o["gain_per_s"] = c.reg.gain_per_s;
    o["max_step"] = c.reg.max_step;
    o["yield_tolerance"] = c.reg.yield_tolerance;
    JsonArray rs = o["recipes"].to<JsonArray>();
    for (int i = 0; i < SYNC_RECIPE_SLOTS; i++) {
        JsonObject r = rs.add<JsonObject>();
        r["name"] = c.recipe[i].name;
        r["gbs"] = c.recipe[i].gbs;
        r["target_time_s"] = c.recipe[i].target_time_s;
        r["brew_weight_g"] = c.recipe[i].brew_weight_g;
        r["start_setting"] = num_or_nan(c.recipe[i].start_setting);
        r["hopper_slot"] = c.recipe[i].hopper_slot;   // reserved: future 4-bean hopper (-1 = none)
        r["portions"] = c.recipe[i].portions;
    }
    o["hopper_enabled"] = c.hopper_enabled;
    o["hopper_slots"] = SYNC_HOPPER_SLOTS;
}

void handle_status() {
    if (!authorized()) return;
    const GrindSync::Status s = grind_sync.status();
    const DialMotor::Status d = dial_motor.status();
    JsonDocument doc;
    doc["build"] = BUILD_NUMBER;
    doc["uptime_s"] = millis() / 1000;
    JsonObject w = doc["wifi"].to<JsonObject>();
    w["connected"] = s.wifi_connected;
    w["ip"] = s.ip;
    w["ssid"] = grind_sync.wifi_ssid();
    w["rssi"] = s.wifi_connected ? WiFi.RSSI() : 0;
    w["ap"] = s.portal_active;
    w["ap_ssid"] = s.portal_ssid;
    JsonObject m = doc["machine"].to<JsonObject>();
    m["seen"] = s.machine_seen;
    m["identity_ok"] = s.identity_ok;
    m["ip"] = s.machine_ip;
    m["state"] = gbs::state_name(s.machine_state);
    m["extraction_ms"] = s.live_extraction_ms;
    m["waiting_for_shot"] = s.shot_pending_grind;
    m["poll_errors"] = s.poll_errors;
    JsonObject l = doc["last_shot"].to<JsonObject>();
    l["valid"] = s.has_last_shot;
    l["time_s"] = s.last_time_s;
    l["weight_g"] = s.last_weight_g;
    l["recipe"] = s.last_profile + 1;
    l["verdict"] = gbs::verdict_name(s.last_verdict);
    l["delta"] = s.last_delta;
    doc["shots_seen"] = s.shots_seen;
    doc["adjustments"] = s.adjustments;
    JsonArray ps = doc["recipe_setting"].to<JsonArray>();
    for (int i = 0; i < SYNC_RECIPE_SLOTS; i++) ps.add(num_or_nan(s.profile_setting[i]));
    JsonObject dj = doc["dial"].to<JsonObject>();
    dj["calibrated"] = d.calibrated;
    dj["trusted"] = d.position_trusted;
    dj["moving"] = d.moving;
    dj["calibrating"] = d.calibrating;
    dj["setting"] = num_or_nan(d.setting);
    dj["min_setting"] = d.min_setting;
    dj["steps"] = d.position_steps;
    dj["zero_steps"] = d.zero_steps;
    dj["coarse_steps"] = d.coarse_steps;
    dj["encoder"] = d.encoder_present;
    dj["slip"] = d.slip_detected;
    dj["stalled"] = d.stalled;
    dj["brownout"] = d.brownout_during_move;
    dj["home_ref"] = d.homed_reference;
    dj["error"] = d.last_error;
    dj["progress"] = d.progress_pct;
    dj["eta_ms"] = d.eta_ms;
    dj["dose_allowed"] = d.dose_allowed;                 // interlock for future dosing gate / hopper
    dj["grinder_start_allowed"] = d.grinder_start_allowed;
    dj["target"] = num_or_nan(d.target_setting);
    doc["active_recipe"] = s.active_recipe + 1;
    doc["portion_done"] = s.portion_done;
    doc["portion_total"] = s.portion_total;
    doc["hopper_slot"] = s.hopper_slot;
    const DialMotor::Settings ms = dial_motor.settings();
    JsonObject mo = doc["motor"].to<JsonObject>();
    mo["microsteps"] = ms.microsteps;
    mo["run_current_ma"] = ms.run_current_ma;
    mo["boost_current_ma"] = ms.boost_current_ma;
    mo["stall_threshold"] = ms.stall_threshold;
    mo["hold_current_pct"] = ms.hold_current_pct;
    mo["max_speed_sps"] = ms.max_speed_sps;
    mo["fast_speed_sps"] = ms.fast_speed_sps;
    mo["fast_accel_sps2"] = ms.fast_accel_sps2;
    // full-range travel estimate from the calibrated span (fast profile)
    if (d.calibrated) {
        const float span = fabsf(static_cast<float>(d.coarse_steps - d.zero_steps));
        const float v = ms.fast_speed_sps, a = ms.fast_accel_sps2;
        const float ramp = v * v / (2 * a);
        const float t = span >= 2 * ramp ? 2 * v / a + (span - 2 * ramp) / v : 2 * sqrtf(span / a);
        mo["full_range_s"] = t;
    }
    mo["accel_sps2"] = ms.accel_sps2;
    mo["backlash_steps"] = ms.backlash_steps;
    mo["invert_dir"] = ms.invert_dir;
    mo["use_as5600"] = ms.use_as5600;
    mo["use_endstop"] = ms.use_endstop;
    mo["home_on_boot"] = ms.home_on_boot;
    fill_config(doc["config"].to<JsonObject>(), grind_sync.config());
    doc["web_password_set"] = !web_password.isEmpty();
    send_json(doc);
}

void handle_config_post() {
    if (!authorized()) return;
    JsonDocument in;
    if (!parse_body(in)) return send_ok(false, "bad json");
    GrindSync::Config c = grind_sync.config();
    if (!in["enabled"].isNull()) c.enabled = in["enabled"];
    if (!in["auto_adjust"].isNull()) c.auto_adjust = in["auto_adjust"];
    if (!in["send_recipe"].isNull()) c.send_recipe = in["send_recipe"];
    if (!in["start_brew_after_grind"].isNull()) c.start_brew_after_grind = in["start_brew_after_grind"];
    if (!in["follow_profile"].isNull()) c.follow_profile = in["follow_profile"];
    if (!in["ratio"].isNull()) c.ratio = in["ratio"];
    if (!in["deadband_s"].isNull()) c.reg.deadband_s = in["deadband_s"];
    if (!in["gain_per_s"].isNull()) c.reg.gain_per_s = in["gain_per_s"];
    if (!in["max_step"].isNull()) c.reg.max_step = in["max_step"];
    if (!in["yield_tolerance"].isNull()) c.reg.yield_tolerance = in["yield_tolerance"];
    if (!in["hopper_enabled"].isNull()) c.hopper_enabled = in["hopper_enabled"];
    if (in["host"].is<const char*>()) strlcpy(c.host, in["host"].as<const char*>(), sizeof(c.host));
    JsonArrayConst rs = in["recipes"].as<JsonArrayConst>();
    int i = 0;
    for (JsonObjectConst r : rs) {
        if (i >= SYNC_RECIPE_SLOTS) break;
        if (r["name"].is<const char*>()) strlcpy(c.recipe[i].name, r["name"].as<const char*>(), sizeof(c.recipe[i].name));
        if (!r["gbs"].isNull()) c.recipe[i].gbs = r["gbs"];
        if (!r["target_time_s"].isNull()) c.recipe[i].target_time_s = r["target_time_s"];
        if (!r["brew_weight_g"].isNull()) c.recipe[i].brew_weight_g = r["brew_weight_g"];
        if (!r["start_setting"].isNull()) {
            const float v = r["start_setting"];
            c.recipe[i].start_setting = v < 0 ? NAN : v;
        }
        if (!r["hopper_slot"].isNull()) c.recipe[i].hopper_slot = static_cast<int8_t>(r["hopper_slot"].as<int>());
        if (!r["portions"].isNull()) c.recipe[i].portions = static_cast<uint8_t>(constrain(r["portions"].as<int>(), 1, SYNC_MAX_PORTIONS));
        i++;
    }
    grind_sync.set_config(c);
    send_ok(true);
}

void handle_wifi_post() {
    if (!authorized()) return;
    JsonDocument in;
    if (!parse_body(in)) return send_ok(false, "bad json");
    const char* ssid = in["ssid"] | "";
    const char* pass = in["pass"] | "";
    if (strlen(ssid) == 0) return send_ok(false, "SSID missing");
    grind_sync.set_wifi_credentials(ssid, pass);
    GrindSync::Config c = grind_sync.config();
    if (!c.enabled) { c.enabled = true; grind_sync.set_config(c); }
    send_ok(true, "saved - connecting");
}

void handle_scan() {
    if (!authorized()) return;
    int n = WiFi.scanComplete();
    JsonDocument doc;
    if (n == WIFI_SCAN_FAILED || n == -2) {
        WiFi.scanNetworks(true);
        doc["scanning"] = true;
        return send_json(doc);
    }
    if (n == WIFI_SCAN_RUNNING) { doc["scanning"] = true; return send_json(doc); }
    JsonArray arr = doc["networks"].to<JsonArray>();
    for (int i = 0; i < n; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = WiFi.SSID(i);
        o["rssi"] = WiFi.RSSI(i);
        o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    send_json(doc);
}

void handle_discover() {
    if (!authorized()) return;
    JsonDocument doc;
    JsonArray arr = doc["devices"].to<JsonArray>();
    if (WiFi.status() == WL_CONNECTED) {
        const int n = MDNS.queryService("gaggimate", "tcp");
        for (int i = 0; i < n; i++) {
            JsonObject o = arr.add<JsonObject>();
            o["host"] = MDNS.hostname(i) + ".local";
            o["ip"] = MDNS.address(i).toString();
        }
    }
    send_json(doc);
}

void handle_test() {
    if (!authorized()) return;
    JsonDocument in;
    parse_body(in);
    String host = in["host"] | grind_sync.config().host;
    JsonDocument doc;
    doc["host"] = host;
    if (WiFi.status() != WL_CONNECTED) { doc["ok"] = false; doc["msg"] = "WiFi not connected"; return send_json(doc); }
    uint16_t port = 80;
    const int colon = host.lastIndexOf(':');
    if (colon > 0) {
        const long p = host.substring(colon + 1).toInt();
        if (p > 0 && p < 65536) port = static_cast<uint16_t>(p);
        host = host.substring(0, colon);
    }
    IPAddress ip;
    bool resolved = ip.fromString(host);
    if (!resolved) {
        String name = host;
        if (name.endsWith(".local")) {
            name.remove(name.length() - 6);
            ip = MDNS.queryHost(name.c_str(), 2000);
            resolved = ip != IPAddress();
        } else {
            resolved = WiFi.hostByName(host.c_str(), ip) == 1;
        }
    }
    if (!resolved) { doc["ok"] = false; doc["msg"] = "host not found"; return send_json(doc); }
    doc["ip"] = ip.toString();
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(2000);
    http.begin(client, ip.toString(), port, gbs::PATH_IDENTITY);
    const int code = http.GET();
    String body = code == 200 ? http.getString() : String();
    http.end();
    int type = -1;
    const bool parsed = code == 200 && gbs::parse_identity(body.c_str(), body.length(), type);
    doc["http"] = code;
    doc["ma_type"] = type;
    doc["ok"] = parsed && type == gbs::MACHINE_TYPE_XENIA;
    doc["msg"] = !parsed ? "no Grind-by-Sync endpoint - enable the Mahlkoenig E64 WS plugin in GaggiMate"
                         : (type == gbs::MACHINE_TYPE_XENIA ? "GaggiMate Grind-by-Sync OK" : "unexpected machine type");
    if (doc["ok"]) grind_sync.request_reconnect();
    send_json(doc);
}

void handle_dial_post() {
    if (!authorized()) return;
    JsonDocument in;
    if (!parse_body(in)) return send_ok(false, "bad json");
    const String a = in["action"] | "";
    bool ok = false;
    if (a == "jog_raw") ok = dial_motor.jog_raw(in["steps"] | 0);
    else if (a == "jog") ok = dial_motor.jog_setting(in["delta"] | 0.0f);
    else if (a == "goto") ok = dial_motor.move_to_setting(in["setting"] | 50.0f, true);
    else if (a == "select_recipe") { grind_sync.select_recipe((in["recipe"] | 1) - 1); ok = true; }
    else if (a == "stop") { dial_motor.stop(); ok = true; }
    else if (a == "home") ok = dial_motor.request_home();
    else if (a == "cal_begin") { dial_motor.begin_calibration(); ok = true; }
    else if (a == "cal_zero") ok = dial_motor.set_zero_here();
    else if (a == "cal_coarse") ok = dial_motor.set_coarse_here();
    else if (a == "cal_finish") ok = dial_motor.finish_calibration(in["margin"] | DIAL_DEFAULT_SAFETY_MARGIN_F);
    else if (a == "cal_cancel") { dial_motor.cancel_calibration(); ok = true; }
    else if (a == "margin") ok = dial_motor.set_safety_margin(in["margin"] | 5.0f);
    else if (a == "trust") { dial_motor.trust_position(); ok = true; }
    else if (a == "save_recipe") { grind_sync.remember_current_setting_for_profile((in["recipe"] | 1) - 1); ok = true; }
    else if (a == "reset_recipe") { grind_sync.reset_profile_to_start((in["recipe"] | 1) - 1); ok = true; }
    const DialMotor::Status d = dial_motor.status();
    send_ok(ok, ok ? nullptr : d.last_error);
}

void handle_motor_post() {
    if (!authorized()) return;
    JsonDocument in;
    if (!parse_body(in)) return send_ok(false, "bad json");
    DialMotor::Settings s = dial_motor.settings();
    if (!in["microsteps"].isNull()) s.microsteps = in["microsteps"];
    if (!in["run_current_ma"].isNull()) s.run_current_ma = in["run_current_ma"];
    if (!in["boost_current_ma"].isNull()) s.boost_current_ma = in["boost_current_ma"];
    if (!in["stall_threshold"].isNull()) s.stall_threshold = in["stall_threshold"];
    if (!in["hold_current_pct"].isNull()) s.hold_current_pct = in["hold_current_pct"];
    if (!in["max_speed_sps"].isNull()) s.max_speed_sps = in["max_speed_sps"];
    if (!in["fast_speed_sps"].isNull()) s.fast_speed_sps = in["fast_speed_sps"];
    if (!in["fast_accel_sps2"].isNull()) s.fast_accel_sps2 = in["fast_accel_sps2"];
    if (!in["accel_sps2"].isNull()) s.accel_sps2 = in["accel_sps2"];
    if (!in["backlash_steps"].isNull()) s.backlash_steps = in["backlash_steps"];
    if (!in["invert_dir"].isNull()) s.invert_dir = in["invert_dir"];
    if (!in["use_as5600"].isNull()) s.use_as5600 = in["use_as5600"];
    if (!in["use_endstop"].isNull()) s.use_endstop = in["use_endstop"];
    if (!in["home_on_boot"].isNull()) s.home_on_boot = in["home_on_boot"];
    send_ok(dial_motor.apply_settings(s), "motor busy");
}

void handle_history() {
    if (!authorized()) return;
    GrindSync::ShotRecord recs[GrindSync::HISTORY_SIZE];
    const int n = grind_sync.history(recs, GrindSync::HISTORY_SIZE);
    JsonDocument doc;
    JsonArray arr = doc["shots"].to<JsonArray>();
    for (int i = 0; i < n; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["seq"] = recs[i].seq;
        o["uptime_s"] = recs[i].uptime_s;
        o["time_s"] = recs[i].time_s;
        o["weight_g"] = recs[i].weight_g;
        o["target_s"] = recs[i].target_s;
        o["delta"] = recs[i].delta;
        o["setting"] = num_or_nan(recs[i].setting_after);
        o["recipe"] = recs[i].profile + 1;
        o["verdict"] = gbs::verdict_name(static_cast<gbs::Verdict>(recs[i].verdict));
        o["manual"] = recs[i].manual != 0;
    }
    send_json(doc);
}

void handle_shot_post() {
    if (!authorized()) return;
    JsonDocument in;
    if (!parse_body(in)) return send_ok(false, "bad json");
    const float t = in["time_s"] | 0.0f;
    if (t <= 0 || t > 120) return send_ok(false, "time out of range");
    grind_sync.submit_manual_shot(t, in["weight_g"] | 0.0f);
    send_ok(true);
}

void handle_password_post() {
    if (!authorized()) return;
    JsonDocument in;
    if (!parse_body(in)) return send_ok(false, "bad json");
    const String pw = in["password"] | "";
    if (pw.length() > 0 && pw.length() < 8) return send_ok(false, "min. 8 characters");
    web_password = pw;
    web_prefs.putString("webpw", pw);
    send_ok(true);
}

void handle_update_done() {
    if (!authorized()) return;
    const bool ok = !web_update.hasError();
    server->sendHeader("Connection", "close");
    server->send(ok ? 200 : 500, "text/plain", ok ? "Update OK - rebooting" : web_update.errorString());
    if (ok) {
        delay(500);
        ESP.restart();
    }
}

void handle_update_upload() {
    if (!web_password.isEmpty() && !server->authenticate("admin", web_password.c_str())) return;
    HTTPUpload& up = server->upload();
    if (up.status == UPLOAD_FILE_START) {
        dial_motor.stop();
        LOG_BLE("[WEB] OTA start %s\n", up.filename.c_str());
        if (!web_update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) LOG_BLE("[WEB] OTA begin failed: %s\n", web_update.errorString());
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (web_update.write(up.buf, up.currentSize) != up.currentSize) LOG_BLE("[WEB] OTA write failed\n");
    } else if (up.status == UPLOAD_FILE_END) {
        if (web_update.end(true)) LOG_BLE("[WEB] OTA done %u bytes\n", up.totalSize);
        else LOG_BLE("[WEB] OTA end failed: %s\n", web_update.errorString());
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        web_update.abort();
    }
}

}  // namespace

void SyncWeb::init() {
    xTaskCreatePinnedToCore(&SyncWeb::task_entry, "sync_web", 8192, this, 1, nullptr, 0);
}

void SyncWeb::task_entry(void* arg) { static_cast<SyncWeb*>(arg)->task_loop(); }

void SyncWeb::task_loop() {
    web_prefs.begin("gbsweb", false);
    web_password = web_prefs.getString("webpw", "");
    bool running = false;
    for (;;) {
        const GrindSync::Status s = grind_sync.status();
        const bool want = s.wifi_connected || s.portal_active;
        if (want && !running) {
            server = new WebServer(80);
            server->on("/", HTTP_GET, []() {
                if (!authorized()) return;
                server->sendHeader("Cache-Control", "no-store");
                server->send_P(200, "text/html", SYNC_WEB_PAGE);
            });
            server->on("/api/status", HTTP_GET, handle_status);
            server->on("/api/config", HTTP_POST, handle_config_post);
            server->on("/api/wifi", HTTP_POST, handle_wifi_post);
            server->on("/api/scan", HTTP_GET, handle_scan);
            server->on("/api/discover", HTTP_GET, handle_discover);
            server->on("/api/test", HTTP_POST, handle_test);
            server->on("/api/dial", HTTP_POST, handle_dial_post);
            server->on("/api/motor", HTTP_POST, handle_motor_post);
            server->on("/api/history", HTTP_GET, handle_history);
            server->on("/api/shot", HTTP_POST, handle_shot_post);
            server->on("/api/brew", HTTP_POST, []() {
                if (!authorized()) return;
                grind_sync.request_brew_start();
                send_ok(true);
            });
            server->on("/api/password", HTTP_POST, handle_password_post);
            server->on("/api/ap", HTTP_POST, []() {
                if (!authorized()) return;
                JsonDocument in;
                parse_body(in);
                if (in["on"] | false) grind_sync.start_setup_portal(); else grind_sync.stop_setup_portal();
                send_ok(true);
            });
            server->on("/update", HTTP_POST, handle_update_done, handle_update_upload);
            server->onNotFound([]() {
                // captive portal: send every unknown URL (generate_204, hotspot-detect, ...) to the UI
                server->sendHeader("Location", String("http://") + server->client().localIP().toString() + "/", true);
                server->send(302, "text/plain", "");
            });
            server->begin();
            MDNS.addService("http", "tcp", 80);
            running = true;
            LOG_BLE("[WEB] web UI running\n");
        } else if (!want && running) {
            server->stop();
            delete server;
            server = nullptr;
            running = false;
        }
        if (s.portal_active && !dns) {
            dns = new DNSServer();
            dns->start(53, "*", WiFi.softAPIP());
        } else if (!s.portal_active && dns) {
            dns->stop();
            delete dns;
            dns = nullptr;
        }
        if (dns) dns->processNextRequest();
        if (running) server->handleClient();
        vTaskDelay(pdMS_TO_TICKS(running ? 5 : 250));
    }
}
