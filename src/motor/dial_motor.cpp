#include "dial_motor.h"

#include <Wire.h>
#include <esp_system.h>
#include <math.h>
#include "../config/grind_sync.h"
#include "../config/logging.h"

DialMotor dial_motor;

namespace {
constexpr const char* NVS_NS = "dial";
constexpr uint8_t AS5600_ADDR = 0x36;
constexpr int32_t ENC_SLIP_TOLERANCE = 120;   // encoder counts (~10 deg)
constexpr uint32_t LEG_TIMEOUT_MS = 120000;
constexpr uint32_t HOMING_BACKOFF_STEPS = 200;
TwoWire& enc_wire() { return Wire1; }

class Lock {
public:
    explicit Lock(SemaphoreHandle_t m) : m_(m) { if (m_) xSemaphoreTake(m_, portMAX_DELAY); }
    ~Lock() { if (m_) xSemaphoreGive(m_); }
private:
    SemaphoreHandle_t m_;
};

uint8_t mres_for(uint16_t microsteps) {
    switch (microsteps) {
        case 256: return 0; case 128: return 1; case 64: return 2; case 32: return 3;
        case 16: return 4; case 8: return 5; case 4: return 6; case 2: return 7; default: return 8;
    }
}
}  // namespace

// ============================================================================
void DialMotor::init() {
    if (initialized_) return;
    mutex_ = xSemaphoreCreateMutex();
    queue_ = xQueueCreate(4, sizeof(Cmd));

    pinMode(DIAL_STEP_PIN, OUTPUT);
    pinMode(DIAL_DIR_PIN, OUTPUT);
    pinMode(DIAL_EN_PIN, OUTPUT);
    digitalWrite(DIAL_STEP_PIN, LOW);
    digitalWrite(DIAL_EN_PIN, HIGH);  // disabled
#if DIAL_ENDSTOP_PIN >= 0
    pinMode(DIAL_ENDSTOP_PIN, INPUT_PULLUP);
#endif
#if DIAL_TMC_DIAG_PIN >= 0
    pinMode(DIAL_TMC_DIAG_PIN, INPUT_PULLDOWN);
#endif
#if DIAL_TMC_UART_TX_PIN >= 0
    Serial1.begin(DIAL_TMC_UART_BAUD, SERIAL_8N1, -1, DIAL_TMC_UART_TX_PIN);
#endif

    load_state();

#if !defined(MOCK_BUILD)
    if (cfg_.use_as5600) {
        enc_wire().begin(DIAL_AS5600_SDA_PIN, DIAL_AS5600_SCL_PIN, DIAL_AS5600_I2C_HZ);
        uint16_t raw;
        enc_present_ = encoder_read(raw);
        if (enc_present_) {
            enc_last_raw_ = raw;
        }
    }
#endif

    configure_driver();

    esp_timer_create_args_t args = {};
    args.callback = &DialMotor::timer_cb;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "dial_step";
    esp_timer_create(&args, &timer_);

    xTaskCreatePinnedToCore(&DialMotor::task_entry, "dial_motor", 4096, this, 2, &task_, 0);
    initialized_ = true;

    if (cfg_.home_on_boot && home_ref_valid_ && cal_.valid) {
        request_home();
    }
    LOG_BLE("[DIAL] init pos=%ld trusted=%d cal=%d enc=%d brownout_move=%d\n", (long)pos_, trusted_,
            cal_.valid, enc_present_, brownout_move_);
}

void DialMotor::load_state() {
    prefs_.begin(NVS_NS, false);
    cfg_.microsteps = prefs_.getUShort("ustep", DIAL_TMC_UART_TX_PIN >= 0 ? DIAL_DEFAULT_MICROSTEPS : 8);
    cfg_.run_current_ma = prefs_.getUShort("irun", DIAL_DEFAULT_RUN_CURRENT_MA);
    cfg_.boost_current_ma = prefs_.getUShort("iboost", DIAL_DEFAULT_BOOST_CURRENT_MA);
    cfg_.stall_threshold = prefs_.getUChar("sgthrs", DIAL_DEFAULT_STALL_THRESHOLD);
    cfg_.hold_current_pct = prefs_.getUChar("ihold", DIAL_DEFAULT_HOLD_CURRENT_PCT);
    cfg_.max_speed_sps = prefs_.getUShort("vmax", DIAL_DEFAULT_MAX_SPEED_SPS);
    cfg_.fast_speed_sps = prefs_.getUShort("vfast", DIAL_DEFAULT_FAST_SPEED_SPS);
    cfg_.fast_accel_sps2 = prefs_.getUShort("afast", DIAL_DEFAULT_FAST_ACCEL_SPS2);
    cfg_.accel_sps2 = prefs_.getUShort("acc", DIAL_DEFAULT_ACCEL_SPS2);
    cfg_.backlash_steps = prefs_.getUShort("lash", DIAL_DEFAULT_BACKLASH_STEPS);
    cfg_.invert_dir = prefs_.getBool("inv", DIAL_DEFAULT_DIR_INVERT);
    cfg_.use_as5600 = prefs_.getBool("as5600", false);
    cfg_.use_endstop = prefs_.getBool("endstop", false);
    cfg_.home_on_boot = prefs_.getBool("homeboot", false);

    cal_.valid = prefs_.getBool("cal", false);
    cal_.zero_steps = prefs_.getInt("zero", 0);
    cal_.coarse_steps = prefs_.getInt("coarse", 0);
    cal_.safety_margin = prefs_.getFloat("margin", DIAL_DEFAULT_SAFETY_MARGIN);
    if (cal_.valid && !cal_.span_ok()) cal_.valid = false;

    home_ref_valid_ = prefs_.getBool("homeok", false);
    home_ref_steps_ = prefs_.getInt("homeref", 0);
    enc_counts_per_step_ = prefs_.getFloat("enccps", 0.0f);
    enc_last_raw_ = prefs_.getUShort("encraw", 0);

    pos_ = prefs_.getInt("pos", 0);
    const bool was_moving = prefs_.getBool("moving", false);
    const int32_t move_start = prefs_.getInt("mvstart", pos_);
    const int32_t move_target = prefs_.getInt("mvtarget", pos_);
    trusted_ = prefs_.getBool("trusted", false) && !was_moving;

    const esp_reset_reason_t rr = esp_reset_reason();
    if (was_moving) {
        brownout_move_ = (rr == ESP_RST_BROWNOUT);
        last_error_ = brownout_move_ ? "Brownout during dial move - check supply rail"
                                     : "Reset during dial move - position unknown";
        LOG_BLE("[DIAL] WARNING: %s (start=%ld target=%ld)\n", last_error_, (long)move_start, (long)move_target);
    }
    // Encoder based recovery happens in init() once the bus is up; store context.
    if (was_moving) {
        prefs_.putInt("mvstart", move_start);
        prefs_.putInt("mvtarget", move_target);
    }
}

// ============================================================================
// TMC2209 single-wire UART (write only)
void DialMotor::tmc_write(uint8_t reg, uint32_t value) {
#if DIAL_TMC_UART_TX_PIN >= 0
    uint8_t dg[8];
    gbs::tmc_write_datagram(DIAL_TMC_ADDRESS, reg, value, dg);
    Serial1.write(dg, sizeof(dg));
    Serial1.flush();
    delayMicroseconds(500);
#else
    (void)reg; (void)value;
#endif
}

void DialMotor::set_run_current(uint16_t ma) {
#if DIAL_TMC_UART_TX_PIN >= 0
    uint8_t cs = 0;
    bool vsense = false;
    gbs::tmc_current_scale(ma, DIAL_TMC_RSENSE_OHM, cs, vsense);
    // keep vsense consistent with configure_driver(): recompute CS for the chosen range
    const uint8_t ihold = static_cast<uint8_t>((cs * cfg_.hold_current_pct) / 100);
    uint32_t chop = 0x10000053u;
    chop &= ~(0x0Fu << 24);
    chop |= static_cast<uint32_t>(mres_for(cfg_.microsteps)) << 24;
    if (vsense) chop |= (1u << 17);
    tmc_write(0x6C, chop);
    tmc_write(0x10, (4u << 16) | (static_cast<uint32_t>(cs) << 8) | ihold);
#else
    (void)ma;
#endif
}

void DialMotor::configure_driver() {
#if DIAL_TMC_UART_TX_PIN >= 0
    // GCONF: pdn_disable | mstep_reg_select | multistep_filt, StealthChop, internal current reference
    tmc_write(0x00, (1u << 6) | (1u << 7) | (1u << 8));
    // TPOWERDOWN: ~0.4 s to standstill current
    tmc_write(0x11, 20);
    set_run_current(cfg_.run_current_ma);
    // StallGuard4 (works in StealthChop): DIAG goes high when SG_RESULT <= 2*SGTHRS.
    // TCOOLTHRS enables the DIAG output above a minimum speed (TSTEP below this value).
    tmc_write(0x14, cfg_.stall_threshold ? 0xFFFFF : 0);
    tmc_write(0x40, cfg_.stall_threshold);
    LOG_BLE("[DIAL] TMC2209 configured: irun=%umA boost=%umA ihold=%u%% ustep=%u sgthrs=%u\n",
            cfg_.run_current_ma, cfg_.boost_current_ma, cfg_.hold_current_pct, cfg_.microsteps, cfg_.stall_threshold);
#endif
}

void DialMotor::driver_enable(bool on) {
    digitalWrite(DIAL_EN_PIN, on ? LOW : HIGH);
}

// ============================================================================
// AS5600 magnetic encoder (optional)
bool DialMotor::encoder_read(uint16_t& raw) {
#if defined(MOCK_BUILD)
    return false;
#else
    TwoWire& w = enc_wire();
    w.beginTransmission(AS5600_ADDR);
    w.write(0x0B);  // STATUS
    if (w.endTransmission(false) != 0) return false;
    if (w.requestFrom((int)AS5600_ADDR, 1) != 1) return false;
    const uint8_t status = w.read();
    if ((status & 0x20) == 0) return false;  // MD: magnet not detected
    w.beginTransmission(AS5600_ADDR);
    w.write(0x0C);  // RAW ANGLE
    if (w.endTransmission(false) != 0) return false;
    if (w.requestFrom((int)AS5600_ADDR, 2) != 2) return false;
    const uint16_t hi = w.read();
    const uint16_t lo = w.read();
    raw = static_cast<uint16_t>(((hi << 8) | lo) & 0x0FFF);
    return true;
#endif
}

void DialMotor::encoder_track() {
    if (!enc_present_) return;
    uint16_t raw;
    if (!encoder_read(raw)) return;
    enc_total_ += gbs::encoder_diff(raw, enc_last_raw_);
    enc_last_raw_ = raw;
}

void DialMotor::encoder_verify_after_move() {
    if (!enc_present_ || enc_counts_per_step_ == 0.0f || !trusted_) return;
    encoder_track();
    const float expected = enc_ref_total_ + (pos_ - enc_pos_ref_) * enc_counts_per_step_;
    const float err = enc_total_ - expected;
    if (fabsf(err) > ENC_SLIP_TOLERANCE) {
        slip_ = true;
        trusted_ = false;
        last_error_ = "Encoder mismatch - dial slipped or motor skipped";
        LOG_BLE("[DIAL] slip detected: enc=%ld expected=%.0f\n", (long)enc_total_, expected);
    }
}

void DialMotor::encoder_boot_recover(bool was_moving, int32_t move_start, int32_t move_target) {
    // Uses the stored raw angle; valid as long as the shaft moved < half an encoder turn
    // (or, after an interrupted move, exactly one candidate lies on the move path).
    if (!enc_present_ || enc_counts_per_step_ == 0.0f) return;
    uint16_t raw;
    if (!encoder_read(raw)) return;
    const uint16_t stored = prefs_.getUShort("encraw", raw);
    const int32_t diff = gbs::encoder_diff(raw, stored);
    if (!was_moving) {
        if (abs(diff) > 8) {
            pos_ = pos_ + static_cast<int32_t>(lroundf(diff / enc_counts_per_step_));
            LOG_BLE("[DIAL] dial turned while off: corrected by %ld counts\n", (long)diff);
        }
        trusted_ = prefs_.getBool("trusted", false);
    } else {
        const int32_t lo = min(move_start, move_target) - 20;
        const int32_t hi = max(move_start, move_target) + 20;
        int found = 0;
        int32_t best = 0;
        for (int k = -64; k <= 64; k++) {
            const float counts = diff + 4096.0f * k;
            const int32_t cand = move_start + static_cast<int32_t>(lroundf(counts / enc_counts_per_step_));
            if (cand >= lo && cand <= hi) { found++; best = cand; }
        }
        if (found == 1) {
            pos_ = best;
            trusted_ = true;
            last_error_ = "Recovered dial position from encoder after reset";
            LOG_BLE("[DIAL] recovered position %ld from encoder\n", (long)best);
        }
    }
    enc_last_raw_ = raw;
    enc_total_ = 0;
    enc_ref_total_ = 0;
    enc_pos_ref_ = pos_;
}

// ============================================================================
void DialMotor::persist_position(bool moving_flag, int32_t target) {
    prefs_.putInt("pos", pos_);
    prefs_.putBool("trusted", trusted_);
    if (moving_flag) {
        prefs_.putInt("mvstart", pos_);
        prefs_.putInt("mvtarget", target);
    }
    prefs_.putBool("moving", moving_flag);
    if (enc_present_) prefs_.putUShort("encraw", enc_last_raw_);
}

bool DialMotor::enqueue(const Cmd& c) {
    if (!initialized_ || !queue_) return false;
    return xQueueSend(queue_, &c, 0) == pdTRUE;
}

bool DialMotor::move_to_setting(float setting, bool fast) {
    Lock l(mutex_);
    if (!cal_.valid || !trusted_) { blocked_ = true; last_error_ = "Dial not calibrated / position untrusted"; return false; }
    if (busy()) { blocked_ = true; last_error_ = "Grinder running - dial move refused"; return false; }
    if (slip_) { blocked_ = true; return false; }
    blocked_ = false;
    const float s = cal_.clamp_setting(setting);
    return enqueue({CmdType::MoveAbs, cal_.setting_to_steps(s), false, fast});
}

bool DialMotor::jog_setting(float delta) {
    float current;
    {
        Lock l(mutex_);
        if (!cal_.valid) return false;
        current = cal_.steps_to_setting(moving_ ? final_target_ : pos_);
    }
    return move_to_setting(current + delta);
}

bool DialMotor::jog_raw(int32_t steps) {
    Lock l(mutex_);
    if (busy()) { blocked_ = true; last_error_ = "Grinder running - dial move refused"; return false; }
    if (steps > DIAL_MAX_JOG_STEPS) steps = DIAL_MAX_JOG_STEPS;
    if (steps < -DIAL_MAX_JOG_STEPS) steps = -DIAL_MAX_JOG_STEPS;
    blocked_ = false;
    // Outside the wizard a raw jog still respects the soft limit on the fine side.
    int32_t target = (moving_ ? final_target_ : pos_) + steps;
    if (!calibrating_ && cal_.valid && trusted_) {
        const float s = cal_.steps_to_setting(target);
        if (s < cal_.min_setting()) target = cal_.setting_to_steps(cal_.min_setting());
    }
    return enqueue({CmdType::MoveAbs, target, true, false});
}

bool DialMotor::request_home() {
    const bool sensorless = cfg_.stall_threshold > 0 && DIAL_TMC_DIAG_PIN >= 0;
    if (!(cfg_.use_endstop && DIAL_ENDSTOP_PIN >= 0) && !sensorless) { last_error_ = "No endstop / StallGuard configured"; return false; }
    if (busy()) return false;
    return enqueue({CmdType::Home, 0, true, false});
}

void DialMotor::stop() {
    stop_req_ = true;
    xQueueReset(queue_);
}

void DialMotor::stop_and_wait(uint32_t timeout_ms) {
    if (!initialized_) return;
    if (!moving_) { xQueueReset(queue_); return; }
    stop();
    const uint32_t start = millis();
    while (moving_ && millis() - start < timeout_ms) delay(1);
    driver_enable(false);
}

// ============================================================================
void DialMotor::task_entry(void* arg) { static_cast<DialMotor*>(arg)->task_loop(); }

void DialMotor::task_loop() {
    // Boot-time encoder recovery needs the persisted context
    {
        const bool was_moving = prefs_.getBool("moving", false);
        const int32_t s = prefs_.getInt("mvstart", pos_);
        const int32_t t = prefs_.getInt("mvtarget", pos_);
        encoder_boot_recover(was_moving, s, t);
        if (was_moving) persist_position(false, pos_);
    }
    Cmd cmd;
    for (;;) {
        if (xQueueReceive(queue_, &cmd, pdMS_TO_TICKS(500)) != pdTRUE) continue;
        stop_req_ = false;
        switch (cmd.type) {
            case CmdType::MoveAbs: do_move(cmd.target, cmd.raw, cmd.fast); break;
            case CmdType::Home: do_home(); break;
            case CmdType::ApplyDriver: configure_driver(); break;
        }
    }
}

uint32_t DialMotor::estimate_ms(uint32_t steps, bool fast) const {
    const float v = fast ? cfg_.fast_speed_sps : cfg_.max_speed_sps;
    const float a = fast ? cfg_.fast_accel_sps2 : cfg_.accel_sps2;
    if (v <= 0 || a <= 0) return 0;
    const float ramp_steps = v * v / (2.0f * a);  // per ramp
    float t;
    if (steps >= 2 * ramp_steps) t = 2.0f * v / a + (steps - 2 * ramp_steps) / v;
    else t = 2.0f * sqrtf(steps / a);
    return static_cast<uint32_t>(t * 1000.0f) + 10;  // + driver wake-up
}

void DialMotor::do_move(int32_t target, bool raw, bool fast) {
    if (busy()) { blocked_ = true; return; }
    final_target_ = target;
    stalled_ = false;
    persist_position(true, target);

    // Plan legs:
    //  1. fast travel (recipe switch) or fine speed (GbS correction)
    //  2. moving coarser: overshoot by the backlash and come back finer, so every target is
    //     approached from the coarse side (backlash always on the same flank)
    //  3. never enter the zone just above the safety margin at full speed: the last part
    //     toward the burrs is done at fine speed
    const int sign = cal_.valid ? cal_.coarse_sign() : 1;
    struct Leg { int32_t to; bool fast; };
    Leg legs[3];
    int n = 0;
    int32_t from = pos_;
    if (cfg_.backlash_steps > 0 && (target - from) * sign > 0) {
        legs[n++] = {target + sign * static_cast<int32_t>(cfg_.backlash_steps), fast};
        legs[n++] = {target, false};
    } else if (fast && cal_.valid && !raw) {
        const int32_t slow_boundary = cal_.setting_to_steps(cal_.min_setting() + DIAL_SLOW_ZONE);
        const bool target_in_zone = (target - slow_boundary) * sign < 0;
        const bool start_above = (from - slow_boundary) * sign > 0;
        if (target_in_zone && start_above) {
            legs[n++] = {slow_boundary, true};
            legs[n++] = {target, false};
        } else {
            legs[n++] = {target, !target_in_zone};
        }
    } else {
        legs[n++] = {target, fast};
    }
    move_total_steps_ = 0;
    move_eta_total_ms_ = 0;
    int32_t p = from;
    for (int i = 0; i < n; i++) {
        const uint32_t st = static_cast<uint32_t>(abs(legs[i].to - p));
        move_total_steps_ += st;
        move_eta_total_ms_ += estimate_ms(st, legs[i].fast);
        p = legs[i].to;
    }
    move_done_before_leg_ = 0;
    move_started_ms_ = millis();

    bool ok = true;
    for (int i = 0; i < n && ok; i++) {
        const uint32_t st = static_cast<uint32_t>(abs(legs[i].to - pos_));
        ok = execute_leg(legs[i].to, false, legs[i].fast);
        move_done_before_leg_ += st;
    }
    move_total_steps_ = 0;
    if (!raw || !calibrating_) encoder_verify_after_move();
    persist_position(false, pos_);
    if (!ok) LOG_BLE("[DIAL] move interrupted at %ld (target %ld)\n", (long)pos_, (long)target);
}

void DialMotor::do_home() {
    // Drive toward the coarse end until the endstop closes. Coarse is the safe direction.
    const int sign = cal_.valid ? cal_.coarse_sign() : 1;
    const int32_t span = cal_.valid ? abs(cal_.coarse_steps - cal_.zero_steps) : DIAL_MAX_JOG_STEPS;
    persist_position(true, pos_ + sign * span);
    homing_ = true;
    endstop_hit_ = false;
    execute_leg(pos_ + sign * static_cast<int32_t>(span * 1.3f + 500), true);
    homing_ = false;
    if (!endstop_hit_) {
        last_error_ = "Homing failed - endstop not reached";
        trusted_ = false;
        persist_position(false, pos_);
        return;
    }
    if (calibrating_ || !home_ref_valid_) {
        // learn reference: position (in calibrated coordinates) where the switch closes
        home_ref_steps_ = pos_;
        home_ref_valid_ = true;
        prefs_.putInt("homeref", home_ref_steps_);
        prefs_.putBool("homeok", true);
    } else {
        pos_ = home_ref_steps_;
        trusted_ = true;
        slip_ = false;
    }
    enc_pos_ref_ = pos_;
    encoder_track();
    enc_ref_total_ = enc_total_;
    // back off the switch, then return to the stored setting
    execute_leg(pos_ - sign * static_cast<int32_t>(HOMING_BACKOFF_STEPS), false);
    persist_position(false, pos_);
    LOG_BLE("[DIAL] homed, pos=%ld\n", (long)pos_);
}

bool DialMotor::execute_leg(int32_t target, bool homing, bool fast) {
    if (target == pos_) return true;
    if (busy()) return false;
    const int8_t dir = target > pos_ ? 1 : -1;
    dir_ = dir;
    leg_target_ = target;
    steps_total_ = static_cast<uint32_t>(abs(target - pos_));
    steps_done_ = 0;
    leg_fast_ = fast;
    const bool level = (dir > 0) ^ cfg_.invert_dir;
    digitalWrite(DIAL_DIR_PIN, level ? HIGH : LOW);
    driver_enable(true);
    delay(5);  // driver wake-up + DIR setup time
    const bool boost = cfg_.boost_current_ma > cfg_.run_current_ma;  // break-away + fast travel
    if (boost) set_run_current(cfg_.boost_current_ma);
    stall_hit_ = false;
    stall_armed_ = false;
    moving_ = true;
    last_dir_ = dir;
    esp_timer_start_once(timer_, 1000);

    const uint32_t start = millis();
    bool boosted = boost;
    while (moving_) {
        vTaskDelay(pdMS_TO_TICKS(20));
        encoder_track();
        const uint32_t t = millis() - start;
        if (boosted && t > DIAL_BOOST_MS && !fast) { set_run_current(cfg_.run_current_ma); boosted = false; }
        // StallGuard is unreliable while accelerating: arm it after the ramp
        if (!stall_armed_ && cfg_.stall_threshold && t > 250) stall_armed_ = true;
        if (t > LEG_TIMEOUT_MS) { stop_req_ = true; }
    }
    if (boosted) set_run_current(cfg_.run_current_ma);
    if (stall_hit_) {
        stalled_ = true;
        if (!homing) {
            trusted_ = false;  // steps were lost
            last_error_ = "Motor stalled (StallGuard) - dial too stiff or end reached";
            LOG_BLE("[DIAL] STALL at pos %ld\n", (long)pos_);
        }
    }
    if (cfg_.hold_current_pct == 0) driver_enable(false);
    (void)homing;
    return pos_ == target || (homing && (endstop_hit_ || stall_hit_));
}

void DialMotor::timer_cb(void* arg) {
    DialMotor* self = static_cast<DialMotor*>(arg);
    // Abort conditions are checked before every single step.
    if (self->stop_req_ || self->busy()) {
        self->moving_ = false;
        return;
    }
#if DIAL_TMC_DIAG_PIN >= 0
    if (self->stall_armed_ && digitalRead(DIAL_TMC_DIAG_PIN) == HIGH) {
        self->stall_hit_ = true;
        if (self->homing_) self->endstop_hit_ = true;  // sensorless homing on the coarse stop
        self->moving_ = false;
        return;
    }
#endif
#if DIAL_ENDSTOP_PIN >= 0
    if (self->homing_ && self->cfg_.use_endstop && digitalRead(DIAL_ENDSTOP_PIN) == LOW) {
        self->endstop_hit_ = true;
        self->moving_ = false;
        return;
    }
#endif
    if (self->pos_ == self->leg_target_) {
        self->moving_ = false;
        return;
    }
    digitalWrite(DIAL_STEP_PIN, HIGH);
    esp_rom_delay_us(3);
    digitalWrite(DIAL_STEP_PIN, LOW);
    self->pos_ = self->pos_ + self->dir_;
    self->steps_done_ = self->steps_done_ + 1;

    // trapezoidal speed profile: v = min(vmax, sqrt(2 a s)) from both ends
    const bool fast = self->leg_fast_;
    const float a = fast ? (self->cfg_.fast_accel_sps2 > 0 ? self->cfg_.fast_accel_sps2 : 3000)
                         : (self->cfg_.accel_sps2 > 0 ? self->cfg_.accel_sps2 : 1000);
    float vmax = fast ? (self->cfg_.fast_speed_sps > 0 ? self->cfg_.fast_speed_sps : 2000)
                      : (self->cfg_.max_speed_sps > 0 ? self->cfg_.max_speed_sps : 400);
    if (self->homing_ && vmax > DIAL_HOMING_SPEED_SPS) vmax = DIAL_HOMING_SPEED_SPS;
    const float s_done = self->steps_done_;
    const float s_left = static_cast<float>(self->steps_total_) - s_done;
    float v = sqrtf(2.0f * a * (s_done < s_left ? s_done : s_left));
    if (v < 60.0f) v = 60.0f;
    if (v > vmax) v = vmax;
    esp_timer_start_once(self->timer_, static_cast<uint64_t>(1000000.0f / v));
}

// ============================================================================
void DialMotor::begin_calibration() {
    Lock l(mutex_);
    calibrating_ = true;
    have_zero_ = have_coarse_ = false;
    pending_cal_ = cal_;
    slip_ = false;
}

bool DialMotor::set_zero_here() {
    Lock l(mutex_);
    if (!calibrating_ || moving_) return false;
    pending_cal_.zero_steps = pos_;
    have_zero_ = true;
    if (enc_present_) { encoder_track(); enc_cal_zero_total_ = enc_total_; }
    return true;
}

bool DialMotor::set_coarse_here() {
    Lock l(mutex_);
    if (!calibrating_ || moving_ || !have_zero_) return false;
    pending_cal_.coarse_steps = pos_;
    have_coarse_ = true;
    return pending_cal_.span_ok();
}

bool DialMotor::finish_calibration(float safety_margin) {
    Lock l(mutex_);
    if (!calibrating_ || !have_zero_ || !have_coarse_ || !pending_cal_.span_ok()) return false;
    pending_cal_.safety_margin = safety_margin < 0 ? 0 : (safety_margin > 30 ? 30 : safety_margin);
    pending_cal_.valid = true;
    cal_ = pending_cal_;
    calibrating_ = false;
    trusted_ = true;
    slip_ = false;
    brownout_move_ = false;
    last_error_ = "";
    if (enc_present_) {
        encoder_track();
        const int32_t steps = cal_.coarse_steps - cal_.zero_steps;
        enc_counts_per_step_ = steps != 0 ? static_cast<float>(enc_total_ - enc_cal_zero_total_) / steps : 0.0f;
        prefs_.putFloat("enccps", enc_counts_per_step_);
        enc_pos_ref_ = pos_;
        enc_ref_total_ = enc_total_;
    }
    // a new calibration invalidates a previously learned home reference
    home_ref_valid_ = false;
    prefs_.putBool("homeok", false);
    prefs_.putInt("zero", cal_.zero_steps);
    prefs_.putInt("coarse", cal_.coarse_steps);
    prefs_.putFloat("margin", cal_.safety_margin);
    prefs_.putBool("cal", true);
    persist_position(false, pos_);
    LOG_BLE("[DIAL] calibrated zero=%ld coarse=%ld margin=%.1f enc_cps=%.4f\n", (long)cal_.zero_steps,
            (long)cal_.coarse_steps, cal_.safety_margin, enc_counts_per_step_);
    return true;
}

void DialMotor::cancel_calibration() {
    Lock l(mutex_);
    calibrating_ = false;
}

bool DialMotor::set_safety_margin(float margin) {
    Lock l(mutex_);
    if (margin < 0 || margin > 30) return false;
    cal_.safety_margin = margin;
    prefs_.putFloat("margin", margin);
    return true;
}

void DialMotor::trust_position() {
    Lock l(mutex_);
    trusted_ = true;
    slip_ = false;
    brownout_move_ = false;
    last_error_ = "";
    enc_pos_ref_ = pos_;
    enc_ref_total_ = enc_total_;
    prefs_.putBool("trusted", true);
}

DialMotor::Status DialMotor::status() {
    Lock l(mutex_);
    Status s{};
    s.initialized = initialized_;
    s.moving = moving_;
    s.calibrated = cal_.valid;
    s.position_trusted = trusted_;
    s.calibrating = calibrating_;
    s.homed_reference = home_ref_valid_;
    s.encoder_present = enc_present_;
    s.slip_detected = slip_;
    s.brownout_during_move = brownout_move_;
    s.last_move_blocked = blocked_;
    s.stalled = stalled_;
    s.cal_has_zero = have_zero_;
    s.cal_has_coarse = have_coarse_;
    s.position_steps = pos_;
    s.target_steps = final_target_;
    s.setting = cal_.valid ? cal_.steps_to_setting(pos_) : NAN;
    s.min_setting = cal_.min_setting();
    s.zero_steps = calibrating_ && have_zero_ ? pending_cal_.zero_steps : cal_.zero_steps;
    s.coarse_steps = cal_.coarse_steps;
    s.last_error = last_error_;
    s.target_setting = cal_.valid ? cal_.steps_to_setting(final_target_) : NAN;
    if (moving_ && move_total_steps_ > 0) {
        const uint32_t done = move_done_before_leg_ + steps_done_;
        s.progress_pct = static_cast<uint8_t>(done >= move_total_steps_ ? 100 : (100ULL * done) / move_total_steps_);
        const uint32_t el = millis() - move_started_ms_;
        s.eta_ms = el < move_eta_total_ms_ ? move_eta_total_ms_ - el : 0;
    } else {
        s.progress_pct = moving_ ? 0 : 100;
        s.eta_ms = 0;
    }
    return s;
}

DialMotor::Settings DialMotor::settings() {
    Lock l(mutex_);
    return cfg_;
}

bool DialMotor::apply_settings(const Settings& s) {
    {
        Lock l(mutex_);
        if (moving_) return false;
        cfg_ = s;
        if (cfg_.run_current_ma < 100) cfg_.run_current_ma = 100;
        if (cfg_.run_current_ma > 1200) cfg_.run_current_ma = 1200;  // well below 17HS4401 rating, protects the 12 V rail
        if (cfg_.hold_current_pct > 50) cfg_.hold_current_pct = 50;
        prefs_.putUShort("ustep", cfg_.microsteps);
        prefs_.putUShort("irun", cfg_.run_current_ma);
        if (cfg_.boost_current_ma > 1200) cfg_.boost_current_ma = 1200;
        prefs_.putUShort("iboost", cfg_.boost_current_ma);
        prefs_.putUChar("sgthrs", cfg_.stall_threshold);
        prefs_.putUChar("ihold", cfg_.hold_current_pct);
        if (cfg_.fast_speed_sps > 20000) cfg_.fast_speed_sps = 20000;
        if (cfg_.max_speed_sps > cfg_.fast_speed_sps) cfg_.fast_speed_sps = cfg_.max_speed_sps;
        prefs_.putUShort("vmax", cfg_.max_speed_sps);
        prefs_.putUShort("vfast", cfg_.fast_speed_sps);
        prefs_.putUShort("afast", cfg_.fast_accel_sps2);
        prefs_.putUShort("acc", cfg_.accel_sps2);
        prefs_.putUShort("lash", cfg_.backlash_steps);
        prefs_.putBool("inv", cfg_.invert_dir);
        prefs_.putBool("as5600", cfg_.use_as5600);
        prefs_.putBool("endstop", cfg_.use_endstop);
        prefs_.putBool("homeboot", cfg_.home_on_boot);
    }
    return enqueue({CmdType::ApplyDriver, 0, false, false});
}
