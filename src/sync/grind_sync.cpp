#include "grind_sync.h"

#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_random.h>
#include <math.h>
#include "../config/logging.h"
#include "../motor/dial_motor.h"

GrindSync grind_sync;

namespace {
constexpr const char* NVS_NS = "gbs";
constexpr const char* DEVICE_HOSTNAME = "eurekamate";
constexpr uint32_t WIFI_RETRY_MS = 15000;
constexpr uint32_t RESOLVE_REFRESH_MS = 5UL * 60UL * 1000UL;

class Lock {
public:
    explicit Lock(SemaphoreHandle_t m) : m_(m) { if (m_) xSemaphoreTake(m_, portMAX_DELAY); }
    ~Lock() { if (m_) xSemaphoreGive(m_); }
private:
    SemaphoreHandle_t m_;
};

[[maybe_unused]] String html_escape(const String& in) {
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); i++) {
        const char c = in[i];
        if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '&') out += "&amp;";
        else if (c == '"') out += "&quot;";
        else out += c;
    }
    return out;
}
}  // namespace

// ============================================================================
void GrindSync::init() {
    mutex_ = xSemaphoreCreateMutex();
    load_config();
    for (int i = 0; i < SYNC_RECIPE_SLOTS; i++) {
        char key[6];
        snprintf(key, sizeof(key), "ps%d", i);
        st_.profile_setting[i] = prefs_.getFloat(key, NAN);
    }
    load_history();
#if SYNC_BREW_BUTTON_PIN >= 0
    pinMode(SYNC_BREW_BUTTON_PIN, INPUT_PULLUP);
#endif
    xTaskCreatePinnedToCore(&GrindSync::task_entry, "grind_sync", 8192, this, 1, nullptr, 0);
}

void GrindSync::load_config() {
    prefs_.begin(NVS_NS, false);
    cfg_.enabled = prefs_.getBool("en", false);
    cfg_.auto_adjust = prefs_.getBool("auto", false);
    cfg_.send_recipe = prefs_.getBool("sendrec", true);
    cfg_.start_brew_after_grind = prefs_.getBool("brewafter", false);
    cfg_.follow_profile = prefs_.getBool("follow", true);
    cfg_.ratio = prefs_.getFloat("ratio", SYNC_DEFAULT_RATIO);
    cfg_.reg.target_time_s = prefs_.getFloat("tgt", SYNC_DEFAULT_TARGET_TIME_S);
    cfg_.reg.deadband_s = prefs_.getFloat("dead", 1.5f);
    cfg_.reg.gain_per_s = prefs_.getFloat("gain", 0.3f);
    cfg_.reg.max_step = prefs_.getFloat("maxstep", 2.0f);
    static const char* kNames[SYNC_RECIPE_SLOTS] = {"Single", "Double", "Custom", "Filter", "Decaf", "Preset 6"};
    for (int i = 0; i < SYNC_RECIPE_SLOTS; i++) {
        char k[8];
        snprintf(k, sizeof(k), "r%dn", i);
        String nm = prefs_.getString(k, kNames[i]);
        strlcpy(cfg_.recipe[i].name, nm.c_str(), sizeof(cfg_.recipe[i].name));
        snprintf(k, sizeof(k), "r%dg", i);  cfg_.recipe[i].gbs = prefs_.getBool(k, i < SYNC_PROFILE_SLOTS || i == 4);
        snprintf(k, sizeof(k), "r%dt", i);  cfg_.recipe[i].target_time_s = prefs_.getFloat(k, SYNC_DEFAULT_TARGET_TIME_S);
        snprintf(k, sizeof(k), "r%dw", i);  cfg_.recipe[i].brew_weight_g = prefs_.getFloat(k, 0.0f);
        snprintf(k, sizeof(k), "r%ds", i);  cfg_.recipe[i].start_setting = prefs_.getFloat(k, i == 3 ? 60.0f : NAN);
        snprintf(k, sizeof(k), "r%dh", i);  cfg_.recipe[i].hopper_slot = static_cast<int8_t>(prefs_.getChar(k, -1));
        snprintf(k, sizeof(k), "r%dp", i);  cfg_.recipe[i].portions = prefs_.getUChar(k, 1);
    }
    String host = prefs_.getString("host", SYNC_DEFAULT_HOST);
    strlcpy(cfg_.host, host.c_str(), sizeof(cfg_.host));
    cfg_.hopper_enabled = prefs_.getBool("hopper", false);
}

void GrindSync::save_config() {
    prefs_.putBool("en", cfg_.enabled);
    prefs_.putBool("auto", cfg_.auto_adjust);
    prefs_.putBool("sendrec", cfg_.send_recipe);
    prefs_.putBool("brewafter", cfg_.start_brew_after_grind);
    prefs_.putBool("follow", cfg_.follow_profile);
    prefs_.putFloat("ratio", cfg_.ratio);
    prefs_.putFloat("tgt", cfg_.reg.target_time_s);
    prefs_.putFloat("dead", cfg_.reg.deadband_s);
    prefs_.putFloat("gain", cfg_.reg.gain_per_s);
    prefs_.putFloat("maxstep", cfg_.reg.max_step);
    prefs_.putString("host", cfg_.host);
    prefs_.putBool("hopper", cfg_.hopper_enabled);
    for (int i = 0; i < SYNC_RECIPE_SLOTS; i++) {
        char k[8];
        snprintf(k, sizeof(k), "r%dn", i);  prefs_.putString(k, cfg_.recipe[i].name);
        snprintf(k, sizeof(k), "r%dg", i);  prefs_.putBool(k, cfg_.recipe[i].gbs);
        snprintf(k, sizeof(k), "r%dt", i);  prefs_.putFloat(k, cfg_.recipe[i].target_time_s);
        snprintf(k, sizeof(k), "r%dw", i);  prefs_.putFloat(k, cfg_.recipe[i].brew_weight_g);
        snprintf(k, sizeof(k), "r%ds", i);  prefs_.putFloat(k, cfg_.recipe[i].start_setting);
        snprintf(k, sizeof(k), "r%dh", i);  prefs_.putChar(k, cfg_.recipe[i].hopper_slot);
        snprintf(k, sizeof(k), "r%dp", i);  prefs_.putUChar(k, cfg_.recipe[i].portions);
    }
}

GrindSync::Config GrindSync::config() {
    Lock l(mutex_);
    return cfg_;
}

void GrindSync::set_config(const Config& c) {
    Lock l(mutex_);
    const bool host_changed = strcmp(c.host, cfg_.host) != 0;
    cfg_ = c;
    if (cfg_.ratio < 1.0f) cfg_.ratio = 1.0f;
    if (cfg_.ratio > 4.0f) cfg_.ratio = 4.0f;
    if (cfg_.reg.max_step < 0.1f) cfg_.reg.max_step = 0.1f;
    if (cfg_.reg.max_step > 10.0f) cfg_.reg.max_step = 10.0f;
    for (int i = 0; i < SYNC_RECIPE_SLOTS; i++) {
        Recipe& r = cfg_.recipe[i];
        if (r.target_time_s < 15.0f) r.target_time_s = 15.0f;
        if (r.target_time_s > 60.0f) r.target_time_s = 60.0f;
        if (r.brew_weight_g < 0.0f || r.brew_weight_g > 120.0f) r.brew_weight_g = 0.0f;
        if (!isnan(r.start_setting) && (r.start_setting < 0.0f || r.start_setting > 100.0f)) r.start_setting = NAN;
        if (r.hopper_slot < -1 || r.hopper_slot >= SYNC_HOPPER_SLOTS) r.hopper_slot = -1;
        if (r.portions < 1) r.portions = 1;
        if (r.portions > SYNC_MAX_PORTIONS) r.portions = SYNC_MAX_PORTIONS;
    }
    save_config();
    if (host_changed) { identity_checked_ = false; last_resolve_ms_ = 0; machine_addr_ = IPAddress(); }
    config_dirty_ = true;
}

GrindSync::Status GrindSync::status() {
    Lock l(mutex_);
    st_.active_recipe = active_recipe_;
    st_.hopper_slot = -1;
    st_.portion_total = 1;
    return st_;
}

bool GrindSync::has_wifi_credentials() {
    Lock l(mutex_);
    return prefs_.isKey("ssid") && prefs_.getString("ssid", "").length() > 0;
}

void GrindSync::notify_grind_completed(int profile, float dose_g) {
    (void)profile;  // the active recipe (profile 1-3 or a preset 4-6) is what was ground
    const int r = active_recipe_;
    float total = dose_g;
    {
        Lock l(mutex_);
        // multi-portion recipes (e.g. filter): only the last portion completes the recipe
        if (portions_recipe_ != r) {
            portions_.start(r >= 0 && r < SYNC_RECIPE_SLOTS ? cfg_.recipe[r].portions : 1);
            portions_recipe_ = r;
        }
        const bool complete = portions_.add(dose_g, &total);
        st_.portion_total = portions_.total;
        st_.portion_done = complete ? portions_.total : portions_.done;
        if (!complete) return;
    }
    grind_profile_ = r;
    grind_dose_ = total;
    grind_done_flag_ = true;
}

bool GrindSync::dose_allowed() { return dial_motor.dose_allowed(); }

void GrindSync::reset_portions() {
    Lock l(mutex_);
    portions_.reset();
    portions_recipe_ = -1;
    st_.portion_done = 0;
}

void GrindSync::on_hopper_slot_changed(int slot) {
    int recipe = -1;
    {
        Lock l(mutex_);
        st_.hopper_slot = static_cast<int8_t>(slot);
        if (!cfg_.hopper_enabled || slot < 0) return;
        int8_t map[SYNC_RECIPE_SLOTS];
        for (int i = 0; i < SYNC_RECIPE_SLOTS; i++) map[i] = cfg_.recipe[i].hopper_slot;
        recipe = gbs::recipe_for_hopper_slot(map, SYNC_RECIPE_SLOTS, slot);
    }
    if (recipe >= 0 && recipe != active_recipe_) select_recipe(recipe);
}

// Reserved for the 4-bean hopper: reads SYNC_HOPPER_SENSOR_PINS (active LOW).
// With all pins = -1 (default) this does nothing.
void GrindSync::poll_hopper_sensors() {
    static const int pins[SYNC_HOPPER_SLOTS] = SYNC_HOPPER_SENSOR_PINS;
    static bool configured = false, any = false;
    if (!configured) {
        configured = true;
        for (int i = 0; i < SYNC_HOPPER_SLOTS; i++)
            if (pins[i] >= 0) { pinMode(pins[i], INPUT_PULLUP); any = true; }
    }
    if (!any) return;
    int8_t slot = -1;
    for (int i = 0; i < SYNC_HOPPER_SLOTS; i++)
        if (pins[i] >= 0 && digitalRead(pins[i]) == LOW) { slot = static_cast<int8_t>(i); break; }
    const uint32_t now = millis();
    if (slot != hopper_raw_) { hopper_raw_ = slot; hopper_change_ms_ = now; return; }
    if (now - hopper_change_ms_ >= SYNC_HOPPER_DEBOUNCE_MS && st_.hopper_slot != slot) on_hopper_slot_changed(slot);
}

void GrindSync::request_brew_start() { brew_req_ = true; }
void GrindSync::start_setup_portal() { portal_req_ = true; }
void GrindSync::stop_setup_portal() { portal_stop_req_ = true; }

void GrindSync::remember_current_setting_for_profile(int profile) {
    if (profile < 0 || profile >= SYNC_RECIPE_SLOTS) return;
    const DialMotor::Status ds = dial_motor.status();
    if (!ds.calibrated || isnan(ds.setting)) return;
    Lock l(mutex_);
    st_.profile_setting[profile] = ds.setting;
    char key[6];
    snprintf(key, sizeof(key), "ps%d", profile);
    prefs_.putFloat(key, ds.setting);
    last_followed_profile_ = profile;
}

// ============================================================================
void GrindSync::task_entry(void* arg) { static_cast<GrindSync*>(arg)->task_loop(); }

void GrindSync::task_loop() {
    uint32_t last_poll = 0;
    bool button_was_down = false;
    uint32_t button_change_ms = 0;
    for (;;) {
        const uint32_t now = millis();

#if SYNC_BREW_BUTTON_PIN >= 0
        const bool down = digitalRead(SYNC_BREW_BUTTON_PIN) == LOW;
        if (down != button_was_down && now - button_change_ms > 50) {
            button_change_ms = now;
            button_was_down = down;
            if (down) request_brew_start();
        }
#endif
        poll_hopper_sensors();  // reserved 4-bean hopper (no-op while no pins are configured)
        if (portal_req_ || portal_stop_req_ || st_.portal_active) portal_loop();

        bool enabled;
        { Lock l(mutex_); enabled = cfg_.enabled; }
        if (!enabled) {
#if !defined(MOCK_BUILD)
            if (!st_.portal_active && WiFi.getMode() != WIFI_OFF) {
                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                Lock l(mutex_);
                st_.wifi_connected = false;
                st_.machine_seen = false;
            }
#endif
            if (grind_done_flag_) {
                grind_done_flag_ = false;
                Lock l(mutex_);
                link_.grind_done(grind_profile_, grind_dose_, millis());
                st_.shot_pending_grind = true;
            }
            brew_req_ = false;
            if (manual_shot_) {
                gbs::ShotResult shot;
                shot.time_s = manual_time_;
                shot.weight_g = manual_weight_;
                {
                    Lock l(mutex_);
                    if (link_.active_profile() < 0) link_.shot_started(millis(), SYNC_SHOT_WINDOW_MS);
                    st_.shot_pending_grind = false;
                }
                handle_shot(shot);
                manual_shot_ = false;
            }
            follow_profile();
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

#if defined(MOCK_BUILD)
        if (now - last_poll >= SYNC_POLL_INTERVAL_MS) {
            last_poll = now;
            mock_tick();
        }
#else
        if (wifi_reconnect_req_) {
            wifi_reconnect_req_ = false;
            WiFi.disconnect(false);
            last_wifi_attempt_ms_ = 0;
        }
        if (!st_.portal_active && !has_wifi_credentials()) portal_req_ = true;  // first setup
        ensure_wifi();
        if (WiFi.status() == WL_CONNECTED) {
            handle_pending_actions();
            if (now - last_poll >= SYNC_POLL_INTERVAL_MS) {
                last_poll = now;
                poll_once();
            }
        }
#endif
        if (manual_shot_) {
            gbs::ShotResult shot;
            shot.time_s = manual_time_;
            shot.weight_g = manual_weight_;
            {
                Lock l(mutex_);
                // A manual entry belongs to the last grind (like the E64 WS brew-time prompt)
                if (link_.active_profile() < 0) link_.shot_started(millis(), SYNC_SHOT_WINDOW_MS);
                st_.shot_pending_grind = false;
            }
            handle_shot(shot);
            manual_shot_ = false;
        }
        follow_profile();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================================
void GrindSync::ensure_wifi() {
    if (WiFi.status() == WL_CONNECTED) {
        Lock l(mutex_);
        if (!st_.wifi_connected) {
            st_.wifi_connected = true;
            strlcpy(st_.ip, WiFi.localIP().toString().c_str(), sizeof(st_.ip));
            MDNS.begin(DEVICE_HOSTNAME);
            LOG_BLE("[SYNC] WiFi connected, ip=%s\n", st_.ip);
        }
        return;
    }
    {
        Lock l(mutex_);
        if (st_.wifi_connected) {
            st_.wifi_connected = false;
            st_.machine_seen = false;
            detector_.connection_lost(millis());
        }
    }
    const uint32_t now = millis();
    if (last_wifi_attempt_ms_ != 0 && now - last_wifi_attempt_ms_ < WIFI_RETRY_MS) return;
    last_wifi_attempt_ms_ = now;
    String ssid, pass;
    {
        Lock l(mutex_);
        ssid = prefs_.getString("ssid", "");
        pass = prefs_.getString("pass", "");
    }
    if (ssid.isEmpty()) return;
    if (!st_.portal_active) WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_HOSTNAME);
    WiFi.setSleep(true);  // required for WiFi/BLE coexistence
    WiFi.begin(ssid.c_str(), pass.c_str());
    LOG_BLE("[SYNC] connecting to WiFi '%s'\n", ssid.c_str());
}

bool GrindSync::resolve_host(IPAddress& out) {
    const uint32_t now = millis();
    if (machine_addr_ != IPAddress() && now - last_resolve_ms_ < RESOLVE_REFRESH_MS) {
        out = machine_addr_;
        return true;
    }
    char host[64];
    { Lock l(mutex_); strlcpy(host, cfg_.host, sizeof(host)); }
    // optional ":port" suffix (bench tests against tools/mock_gaggimate.py)
    machine_port_ = 80;
    if (char* colon = strrchr(host, ':')) {
        const long p = strtol(colon + 1, nullptr, 10);
        if (p > 0 && p < 65536) machine_port_ = static_cast<uint16_t>(p);
        *colon = '\0';
    }
    IPAddress ip;
    bool ok = ip.fromString(host);
    if (!ok) {
        const size_t n = strlen(host);
        if (n > 6 && strcasecmp(host + n - 6, ".local") == 0) {
            String name(host);
            name.remove(n - 6);
            ip = MDNS.queryHost(name.c_str(), 2000);
            ok = ip != IPAddress();
        } else {
            ok = WiFi.hostByName(host, ip) == 1;
        }
    }
    if (!ok) return false;
    machine_addr_ = ip;
    last_resolve_ms_ = now;
    Lock l(mutex_);
    strlcpy(st_.machine_ip, ip.toString().c_str(), sizeof(st_.machine_ip));
    out = ip;
    return true;
}

bool GrindSync::http_get(const char* path, String& body) {
    IPAddress ip;
    if (!resolve_host(ip)) return false;
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(SYNC_HTTP_TIMEOUT_MS);
    http.setConnectTimeout(SYNC_HTTP_TIMEOUT_MS);
    if (!http.begin(client, ip.toString(), machine_port_, path)) return false;
    const int code = http.GET();
    if (code == 200) body = http.getString();
    http.end();
    return code == 200;
}

bool GrindSync::http_post(const char* path, const char* body) {
    IPAddress ip;
    if (!resolve_host(ip)) return false;
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(SYNC_HTTP_TIMEOUT_MS);
    http.setConnectTimeout(SYNC_HTTP_TIMEOUT_MS);
    if (!http.begin(client, ip.toString(), machine_port_, path)) return false;
    http.addHeader("Content-Type", "application/json");
    const int code = http.POST(reinterpret_cast<uint8_t*>(const_cast<char*>(body)), strlen(body));
    http.end();
    LOG_BLE("[SYNC] POST %s %s -> %d\n", path, body, code);
    return code == 200;
}

void GrindSync::poll_once() {
    String body;
    if (!identity_checked_) {
        int type = -1;
        if (http_get(gbs::PATH_IDENTITY, body) && gbs::parse_identity(body.c_str(), body.length(), type)) {
            identity_checked_ = true;
            Lock l(mutex_);
            st_.identity_ok = (type == gbs::MACHINE_TYPE_XENIA);
            LOG_BLE("[SYNC] machine identity MA_TYPE=%d\n", type);
        }
    }
    gbs::MachineStatus ms;
    if (!http_get(gbs::PATH_STATUS, body) || !gbs::parse_status(body.c_str(), body.length(), ms)) {
        if (++consecutive_errors_ >= 3) {
            Lock l(mutex_);
            st_.machine_seen = false;
            detector_.connection_lost(millis());
            machine_addr_ = IPAddress();  // re-resolve (DHCP change)
            identity_checked_ = false;
        }
        Lock l(mutex_);
        st_.poll_errors++;
        return;
    }
    consecutive_errors_ = 0;
    gbs::ShotResult shot;
    gbs::ShotDetector::Event ev;
    {
        Lock l(mutex_);
        st_.machine_seen = true;
        st_.machine_state = ms.state;
        st_.live_extraction_ms = ms.extraction_ms;
        ev = detector_.feed(ms, millis(), shot);
        if (ev == gbs::ShotDetector::Event::ShotStarted) {
            link_.shot_started(millis(), SYNC_SHOT_WINDOW_MS);
            st_.shot_pending_grind = false;
        }
    }
    if (ev == gbs::ShotDetector::Event::ShotFinished) handle_shot(shot);
}

void GrindSync::handle_pending_actions() {
    if (grind_done_flag_) {
        grind_done_flag_ = false;
        const int profile = grind_profile_;
        const float dose = grind_dose_;
        Config c = config();
        {
            Lock l(mutex_);
            link_.grind_done(profile, dose, millis());
            st_.shot_pending_grind = true;
        }
        if (c.send_recipe) {
            char buf[96];
            const float bev = recipe_target_weight(profile, dose);
            if (gbs::build_grind_done(buf, sizeof(buf), bev, profile + 1)) http_post(gbs::PATH_GRIND_DONE, buf);
        }
        if (c.start_brew_after_grind) brew_req_ = true;
    }
    if (brew_req_) {
        brew_req_ = false;
        char buf[32];
        if (gbs::build_start_brew(buf, sizeof(buf))) http_post(gbs::PATH_RUN_SCRIPT, buf);
    }
}

float GrindSync::recipe_target_weight(int profile, float dose_g) {
    Config c = config();
    if (profile >= 0 && profile < SYNC_RECIPE_SLOTS && c.recipe[profile].brew_weight_g > 0.0f)
        return c.recipe[profile].brew_weight_g;
    return dose_g > 0 ? dose_g * c.ratio : 0.0f;
}

void GrindSync::handle_shot(const gbs::ShotResult& shot) {
    Config c = config();
    int profile;
    float dose;
    {
        Lock l(mutex_);
        profile = link_.active_profile();
        dose = link_.active_dose();
        link_.clear();
    }
    const float target_weight = profile >= 0 ? recipe_target_weight(profile, dose) : 0.0f;
    gbs::Decision d;
    gbs::RegulatorConfig reg = c.reg;
    if (profile >= 0 && profile < SYNC_RECIPE_SLOTS) reg.target_time_s = c.recipe[profile].target_time_s;
    if (profile < 0) {
        d.verdict = gbs::Verdict::Disabled;  // shot not preceded by a grind from this grinder
    } else {
        d = gbs::evaluate_shot(reg, shot, target_weight);
        if (!c.auto_adjust || !c.recipe[profile].gbs) {
            if (d.verdict == gbs::Verdict::Adjust) d.verdict = gbs::Verdict::Disabled;
        }
    }

    float applied = 0.0f;
    float after = NAN;
    const DialMotor::Status ds = dial_motor.status();
    if (d.verdict == gbs::Verdict::Adjust && ds.calibrated && ds.position_trusted) {
        float base;
        { Lock l(mutex_); base = st_.profile_setting[profile]; }
        if (isnan(base)) base = ds.setting;
        const float next = gbs::apply_delta(base, d.delta, ds.min_setting);
        applied = next - base;
        after = next;
        {
            Lock l(mutex_);
            st_.profile_setting[profile] = next;
            st_.adjustments++;
            char key[6];
            snprintf(key, sizeof(key), "ps%d", profile);
            prefs_.putFloat(key, next);
        }
        if (profile == active_recipe_) {
            // GbS correction: slow, fine move; on refusal follow_profile() retries later
            const bool moved = dial_motor.move_to_setting(next, false);
            Lock l(mutex_);
            last_followed_profile_ = moved ? profile : -1;
        }
        LOG_BLE("[SYNC] shot %.1fs %.1fg -> recipe %d setting %.1f -> %.1f\n", shot.time_s, shot.weight_g,
                profile + 1, base, next);
    } else if (profile >= 0 && profile < SYNC_RECIPE_SLOTS) {
        Lock l(mutex_);
        after = st_.profile_setting[profile];
    }
    ShotRecord rec{};
    rec.uptime_s = millis() / 1000;
    rec.time_s = shot.time_s;
    rec.weight_g = shot.weight_g;
    rec.target_s = reg.target_time_s;
    rec.delta = applied;
    rec.setting_after = after;
    rec.profile = static_cast<int8_t>(profile);
    rec.verdict = static_cast<uint8_t>(d.verdict);
    rec.manual = manual_shot_ ? 1 : 0;
    add_history(rec);
    Lock l(mutex_);
    st_.has_last_shot = true;
    st_.last_time_s = shot.time_s;
    st_.last_weight_g = shot.weight_g;
    st_.last_profile = profile;
    st_.last_verdict = d.verdict;
    st_.last_delta = applied;
    st_.shots_seen++;
}

void GrindSync::add_history(const ShotRecord& r) {
    {
        Lock l(mutex_);
        ShotRecord rec = r;
        rec.seq = ++history_seq_;
        history_[history_head_] = rec;
        history_head_ = (history_head_ + 1) % HISTORY_SIZE;
        if (history_count_ < HISTORY_SIZE) history_count_++;
    }
    save_history();
}

int GrindSync::history(ShotRecord* out, int max_items) {
    Lock l(mutex_);
    int n = 0;
    for (int i = 0; i < history_count_ && n < max_items; i++) {
        const int idx = (history_head_ - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
        out[n++] = history_[idx];
    }
    return n;
}

void GrindSync::load_history() {
    const size_t len = prefs_.getBytesLength("hist");
    if (len == sizeof(history_)) {
        prefs_.getBytes("hist", history_, sizeof(history_));
        history_count_ = 0;
        uint32_t max_seq = 0;
        int newest = -1;
        for (int i = 0; i < HISTORY_SIZE; i++) {
            if (history_[i].seq) {
                history_count_++;
                if (history_[i].seq > max_seq) { max_seq = history_[i].seq; newest = i; }
            }
        }
        history_seq_ = max_seq;
        history_head_ = newest >= 0 ? (newest + 1) % HISTORY_SIZE : 0;
    }
}

void GrindSync::save_history() {
    Lock l(mutex_);
    prefs_.putBytes("hist", history_, sizeof(history_));
}

void GrindSync::reset_profile_to_start(int profile) {
    if (profile < 0 || profile >= SYNC_RECIPE_SLOTS) return;
    Config c = config();
    Lock l(mutex_);
    st_.profile_setting[profile] = c.recipe[profile].start_setting;
    char key[6];
    snprintf(key, sizeof(key), "ps%d", profile);
    prefs_.putFloat(key, st_.profile_setting[profile]);
    last_followed_profile_ = -1;
}

void GrindSync::submit_manual_shot(float time_s, float weight_g) {
    manual_time_ = time_s;
    manual_weight_ = weight_g;
    manual_shot_ = true;
}

void GrindSync::request_reconnect() {
    Lock l(mutex_);
    identity_checked_ = false;
    machine_addr_ = IPAddress();
    last_resolve_ms_ = 0;
    last_wifi_attempt_ms_ = 0;
}

void GrindSync::select_recipe(int recipe) {
    if (recipe < 0 || recipe >= SYNC_RECIPE_SLOTS) return;
    Lock l(mutex_);
    active_recipe_ = recipe;
    last_followed_profile_ = -1;
    user_selected_ = true;
}

void GrindSync::follow_profile() {
    // SGBW profile change on the main screen selects recipe 1-3
    if (current_profile_) {
        const int p = current_profile_();
        Lock l(mutex_);
        if (p != last_sgbw_profile_) {
            if (p >= 0 && p < SYNC_PROFILE_SLOTS) {
                active_recipe_ = p;
                last_followed_profile_ = -1;
            }
            last_sgbw_profile_ = p;
        }
    }
    Config c = config();
    const int r = active_recipe_;
    float target;
    bool user;
    {
        Lock l(mutex_);
        if (r == last_followed_profile_) return;
        target = st_.profile_setting[r];
        if (isnan(target) && !isnan(cfg_.recipe[r].start_setting)) target = cfg_.recipe[r].start_setting;
        user = user_selected_;
    }
    if (!(c.follow_profile || user) || isnan(target)) { Lock l(mutex_); last_followed_profile_ = r; user_selected_ = false; return; }
    const DialMotor::Status ds = dial_motor.status();
    if (!ds.calibrated || !ds.position_trusted || ds.moving) return;
    if (!isnan(ds.setting) && fabsf(ds.setting - target) < 0.05f) { Lock l(mutex_); last_followed_profile_ = r; user_selected_ = false; return; }
    if (dial_motor.move_to_setting(target, true)) { Lock l(mutex_); last_followed_profile_ = r; user_selected_ = false; }
    // refused (grinder busy): retried on the next loop
}

// ============================================================================
// Setup access point: open an AP with a one-time password shown on the display.
// The web UI (sync_web) is served on it with a captive-portal DNS; credentials go to NVS only.
void GrindSync::portal_loop() {
    if (portal_stop_req_) {
        portal_stop_req_ = false;
        portal_req_ = false;
        if (st_.portal_active) {
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
        }
        Lock l(mutex_);
        st_.portal_active = false;
        last_wifi_attempt_ms_ = 0;
        return;
    }
    if (portal_req_ && !st_.portal_active) {
        portal_req_ = false;
        char ssid[33];
        strlcpy(ssid, "EurekaMate-Setup", sizeof(ssid));
        char pass[12];
        snprintf(pass, sizeof(pass), "%08lu", static_cast<unsigned long>(esp_random() % 100000000UL));
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(ssid, pass);
        Lock l(mutex_);
        st_.portal_active = true;
        strlcpy(st_.portal_ssid, ssid, sizeof(st_.portal_ssid));
        strlcpy(st_.portal_pass, pass, sizeof(st_.portal_pass));
        LOG_BLE("[SYNC] setup AP started: %s (192.168.4.1)\n", ssid);
    }
}

void GrindSync::set_wifi_credentials(const char* ssid, const char* pass) {
    {
        Lock l(mutex_);
        if (ssid && strlen(ssid) > 0 && strlen(ssid) <= 32) prefs_.putString("ssid", ssid);
        if (pass && strlen(pass) > 0 && strlen(pass) <= 63) prefs_.putString("pass", pass);
        last_wifi_attempt_ms_ = 0;
    }
    wifi_reconnect_req_ = true;
}

String GrindSync::wifi_ssid() {
    Lock l(mutex_);
    return prefs_.getString("ssid", "");
}

// ============================================================================
// Mock mode: a simulated GaggiMate. After each grind a shot starts 4 s later; its
// time depends on the dial setting (coarser = faster) around a drifting optimum.
void GrindSync::mock_tick() {
    static uint32_t shot_start_ms = 0;
    static uint32_t shot_len_ms = 0;
    static uint32_t done_until_ms = 0;
    static uint32_t grind_ms = 0;
    static bool shot_scheduled = false;
    static float optimum = 30.0f;
    const uint32_t now = millis();

    {
        Lock l(mutex_);
        st_.wifi_connected = true;
        st_.machine_seen = true;
        st_.identity_ok = true;
        strlcpy(st_.ip, "mock", sizeof(st_.ip));
        strlcpy(st_.machine_ip, "mock", sizeof(st_.machine_ip));
    }
    if (grind_done_flag_) {
        grind_done_flag_ = false;
        Lock l(mutex_);
        link_.grind_done(grind_profile_, grind_dose_, now);
        st_.shot_pending_grind = true;
        grind_ms = now;
        shot_scheduled = true;
    }
    gbs::MachineStatus ms;
    ms.state = gbs::MachineState::Ready;
    if (shot_scheduled && now - grind_ms > 4000 && shot_start_ms == 0) {
        const DialMotor::Status ds = dial_motor.status();
        const float setting = isnan(ds.setting) ? optimum : ds.setting;
        optimum += ((esp_random() % 100) / 100.0f - 0.5f) * 0.6f;  // beans drift
        const float t = config().recipe[grind_profile_ % SYNC_RECIPE_SLOTS].target_time_s + 2.0f * (optimum - setting) + ((esp_random() % 100) / 100.0f - 0.5f);
        shot_len_ms = static_cast<uint32_t>((t < 5 ? 5 : t) * 1000.0f);
        shot_start_ms = now;
        shot_scheduled = false;
        LOG_BLE("[SYNC][MOCK] shot started, will run %.1f s (setting %.1f, optimum %.1f)\n", t, setting, optimum);
    }
    if (shot_start_ms) {
        const uint32_t elapsed = now - shot_start_ms;
        if (elapsed < shot_len_ms) {
            ms.state = gbs::MachineState::Brewing;
            ms.extraction_active = true;
            ms.extraction_ms = elapsed;
        } else {
            shot_start_ms = 0;
            done_until_ms = now + 5000;
        }
    }
    if (done_until_ms && now < done_until_ms) {
        ms.state = gbs::MachineState::ShotDone;
        ms.extraction_active = true;
        ms.extraction_ms = shot_len_ms;
        ms.volume_g = grind_dose_ * config().ratio;
    } else if (done_until_ms) {
        done_until_ms = 0;
    }
    brew_req_ = false;

    gbs::ShotResult shot;
    gbs::ShotDetector::Event ev;
    {
        Lock l(mutex_);
        st_.machine_state = ms.state;
        st_.live_extraction_ms = ms.extraction_ms;
        ev = detector_.feed(ms, now, shot);
        if (ev == gbs::ShotDetector::Event::ShotStarted) {
            link_.shot_started(now, SYNC_SHOT_WINDOW_MS);
            st_.shot_pending_grind = false;
        }
    }
    if (ev == gbs::ShotDetector::Event::ShotFinished) handle_shot(shot);
}
