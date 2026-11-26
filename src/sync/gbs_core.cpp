#include "gbs_core.h"

#include <ArduinoJson.h>
#include <math.h>

namespace gbs {

namespace {

float number_like(JsonVariantConst v, float fallback = 0.0f) {
    if (v.is<float>() || v.is<int>() || v.is<long>() || v.is<double>()) return v.as<float>();
    if (v.is<const char*>()) {
        const char* s = v.as<const char*>();
        if (!s || !*s) return fallback;
        char* end = nullptr;
        double d = strtod(s, &end);
        if (end == s) return fallback;
        return static_cast<float>(d);
    }
    if (v.is<bool>()) return v.as<bool>() ? 1.0f : 0.0f;
    return fallback;
}

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

}  // namespace

bool parse_status(const char* json, size_t len, MachineStatus& out) {
    if (!json || len == 0) return false;
    JsonDocument doc;
    if (deserializeJson(doc, json, len) != DeserializationError::Ok) return false;
    if (!doc.is<JsonObject>()) return false;
    JsonObjectConst o = doc.as<JsonObjectConst>();
    if (o["MA_STATUS"].isNull()) return false;

    MachineStatus s;
    const int st = static_cast<int>(number_like(o["MA_STATUS"], -1));
    switch (st) {
        case 0: s.state = MachineState::Standby; break;
        case 1: s.state = MachineState::Ready; break;
        case 3: s.state = MachineState::Brewing; break;
        case 4: s.state = MachineState::ShotDone; break;
        case 5: s.state = MachineState::Service; break;
        case 6: s.state = MachineState::Error; break;
        default: s.state = MachineState::Unknown; break;
    }
    s.extraction_active = number_like(o["MA_EXTRACTION_STATUS"]) >= 0.5f;
    const float ms = number_like(o["REAL_EXTRACTION_TIME_MS"]);
    s.extraction_ms = ms > 0 ? static_cast<uint32_t>(ms) : 0;
    const float vol = number_like(o["PU_SENS_FLOW_METER_VOLUME"]);
    s.volume_g = (isfinite(vol) && vol > 0) ? vol : 0.0f;
    s.set_temp_c = number_like(o["BG_SET_TEMP"]);
    s.temp_c = number_like(o["BG_SENS_TEMP"]);
    s.water_ok = o["TANK_LEVEL"].isNull() ? true : number_like(o["TANK_LEVEL"]) >= 0.5f;
    out = s;
    return true;
}

bool parse_identity(const char* json, size_t len, int& machine_type) {
    if (!json || len == 0) return false;
    JsonDocument doc;
    if (deserializeJson(doc, json, len) != DeserializationError::Ok) return false;
    if (doc["MA_TYPE"].isNull()) return false;
    machine_type = static_cast<int>(number_like(doc["MA_TYPE"], -1));
    return true;
}

size_t build_grind_done(char* buf, size_t size, float beverage_weight_g, int recipe) {
    if (!buf || size == 0) return 0;
    JsonDocument doc;
    // one decimal is what the grinder UI uses
    doc["SYNC_BEVERAGE_WEIGHT"] = roundf(beverage_weight_g * 10.0f) / 10.0f;
    doc["SYNC_FILTER"] = recipe;
    const size_t n = serializeJson(doc, buf, size);
    return n < size ? n : 0;
}

size_t build_start_brew(char* buf, size_t size) {
    if (!buf || size == 0) return 0;
    JsonDocument doc;
    doc["ID"] = SCRIPT_START_BREW;
    const size_t n = serializeJson(doc, buf, size);
    return n < size ? n : 0;
}

const char* state_name(MachineState s) {
    switch (s) {
        case MachineState::Standby: return "standby";
        case MachineState::Ready: return "ready";
        case MachineState::Brewing: return "brewing";
        case MachineState::ShotDone: return "shot done";
        case MachineState::Service: return "service";
        case MachineState::Error: return "error";
        default: return "unknown";
    }
}

// ---------------------------------------------------------------------------
ShotDetector::Event ShotDetector::feed(const MachineStatus& s, uint32_t now_ms, ShotResult& result) {
    (void)now_ms;
    const bool brewing_now = s.state == MachineState::Brewing;
    const bool done_now = s.state == MachineState::ShotDone;

    if (lost_ && phase_ == Phase::Brewing && !brewing_now) {
        // We missed the end while offline.
        lost_ = false;
        if (done_now) {
            result = ShotResult{};
            result.time_s = (s.extraction_ms ? s.extraction_ms : last_ms_) / 1000.0f;
            result.weight_g = s.volume_g;
            phase_ = Phase::Reported;
            return Event::ShotFinished;
        }
        result = ShotResult{};
        result.time_s = last_ms_ / 1000.0f;
        result.aborted = true;
        phase_ = Phase::Idle;
        return Event::ShotFinished;
    }
    lost_ = false;

    switch (phase_) {
        case Phase::Idle:
            if (brewing_now) {
                phase_ = Phase::Brewing;
                last_ms_ = s.extraction_ms;
                return Event::ShotStarted;
            }
            return Event::None;

        case Phase::Brewing:
            if (brewing_now) {
                if (s.extraction_ms > last_ms_) last_ms_ = s.extraction_ms;
                return Event::None;
            }
            result = ShotResult{};
            if (done_now) {
                result.time_s = (s.extraction_ms ? s.extraction_ms : last_ms_) / 1000.0f;
                result.weight_g = s.volume_g;
                phase_ = Phase::Reported;
            } else {
                // left brewing without "shot done": stopped/aborted or error
                result.time_s = last_ms_ / 1000.0f;
                result.aborted = true;
                phase_ = Phase::Idle;
            }
            return Event::ShotFinished;

        case Phase::Reported:
            if (brewing_now) {  // a new shot right after the previous one
                phase_ = Phase::Brewing;
                last_ms_ = s.extraction_ms;
                return Event::ShotStarted;
            }
            if (!done_now) phase_ = Phase::Idle;
            return Event::None;
    }
    return Event::None;
}

void ShotDetector::connection_lost(uint32_t now_ms) {
    (void)now_ms;
    if (phase_ == Phase::Brewing) lost_ = true;
}

// ---------------------------------------------------------------------------
Decision evaluate_shot(const RegulatorConfig& cfg, const ShotResult& shot, float target_weight_g) {
    Decision d;
    d.error_s = shot.time_s - cfg.target_time_s;
    if (shot.aborted) { d.verdict = Verdict::Aborted; return d; }
    if (!(shot.time_s > cfg.min_valid_s)) { d.verdict = Verdict::TooShort; return d; }
    if (shot.time_s > cfg.target_time_s + cfg.max_over_s) { d.verdict = Verdict::TooLong; return d; }
    if (shot.weight_g > 0.0f && target_weight_g > 0.0f) {
        const float rel = fabsf(shot.weight_g / target_weight_g - 1.0f);
        if (rel > cfg.yield_tolerance) { d.verdict = Verdict::YieldOff; return d; }
    }
    if (fabsf(d.error_s) <= cfg.deadband_s) { d.verdict = Verdict::InDeadband; return d; }

    // Only the part of the error outside the deadband counts, so the
    // correction is continuous at the deadband edge.
    const float effective = d.error_s > 0 ? d.error_s - cfg.deadband_s : d.error_s + cfg.deadband_s;
    float delta = clampf(cfg.gain_per_s * effective, -cfg.max_step, cfg.max_step);
    if (cfg.resolution > 0.0f) {
        delta = roundf(delta / cfg.resolution) * cfg.resolution;
        // never round a needed correction to zero
        if (delta == 0.0f) delta = effective > 0 ? cfg.resolution : -cfg.resolution;
    }
    d.delta = delta;
    d.verdict = Verdict::Adjust;
    return d;
}

const char* verdict_name(Verdict v) {
    switch (v) {
        case Verdict::Adjust: return "adjusted";
        case Verdict::InDeadband: return "on target";
        case Verdict::Aborted: return "aborted shot";
        case Verdict::TooShort: return "too short (<15s)";
        case Verdict::TooLong: return "too long";
        case Verdict::YieldOff: return "yield off";
        default: return "disabled";
    }
}

float apply_delta(float current, float delta, float min_setting) {
    return clampf(current + delta, min_setting, 100.0f);
}

// ---------------------------------------------------------------------------
void GrindShotLink::grind_done(int profile, float dose_g, uint32_t now_ms) {
    pending_ = true;
    pending_profile_ = profile;
    pending_dose_ = dose_g;
    grind_ms_ = now_ms;
}

int GrindShotLink::shot_started(uint32_t now_ms, uint32_t window_ms) {
    active_profile_ = -1;
    active_dose_ = 0.0f;
    if (!pending_) return -1;
    pending_ = false;  // one grind regulates at most one shot
    if (now_ms - grind_ms_ > window_ms) return -1;
    active_profile_ = pending_profile_;
    active_dose_ = pending_dose_;
    return active_profile_;
}

// ---------------------------------------------------------------------------
bool DialCalibration::span_ok() const {
    const int32_t span = coarse_steps - zero_steps;
    return (span >= 0 ? span : -span) >= MIN_SPAN_STEPS;
}

float DialCalibration::steps_to_setting(int32_t steps) const {
    if (!span_ok()) return 0.0f;
    return 100.0f * static_cast<float>(steps - zero_steps) / static_cast<float>(coarse_steps - zero_steps);
}

int32_t DialCalibration::setting_to_steps(float setting) const {
    const float s = clampf(setting, 0.0f, 100.0f);
    const float steps = zero_steps + (coarse_steps - zero_steps) * (s / 100.0f);
    return static_cast<int32_t>(lroundf(steps));
}

float DialCalibration::clamp_setting(float setting) const {
    return clampf(setting, safety_margin, 100.0f);
}

int32_t encoder_diff(uint16_t a, uint16_t b) {
    int32_t d = static_cast<int32_t>(a & 0x0FFF) - static_cast<int32_t>(b & 0x0FFF);
    while (d > 2047) d -= 4096;
    while (d < -2048) d += 4096;
    return d;
}

// ---------------------------------------------------------------------------
uint8_t tmc_crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int j = 0; j < 8; j++) {
            if (((crc >> 7) ^ (byte & 0x01)) != 0) {
                crc = static_cast<uint8_t>((crc << 1) ^ 0x07);
            } else {
                crc = static_cast<uint8_t>(crc << 1);
            }
            byte >>= 1;
        }
    }
    return crc;
}

void tmc_current_scale(float rms_ma, float rsense_ohm, uint8_t& cs, bool& vsense) {
    const float i = rms_ma / 1000.0f;
    const float r = rsense_ohm + 0.02f;
    auto scale_for = [&](float vfs) { return 32.0f * 1.41421356f * i * r / vfs - 1.0f; };
    float c = scale_for(0.325f);
    vsense = false;
    if (c < 16.0f) {  // better resolution with the high-sensitivity range
        c = scale_for(0.180f);
        vsense = true;
    }
    if (c < 0.0f) c = 0.0f;
    if (c > 31.0f) c = 31.0f;
    cs = static_cast<uint8_t>(c);  // round down: never exceed the requested current
}

void tmc_write_datagram(uint8_t addr, uint8_t reg, uint32_t value, uint8_t out[8]) {
    out[0] = 0x05;
    out[1] = addr;
    out[2] = static_cast<uint8_t>(reg | 0x80);
    out[3] = static_cast<uint8_t>(value >> 24);
    out[4] = static_cast<uint8_t>(value >> 16);
    out[5] = static_cast<uint8_t>(value >> 8);
    out[6] = static_cast<uint8_t>(value);
    out[7] = tmc_crc8(out, 7);
}

}  // namespace gbs
