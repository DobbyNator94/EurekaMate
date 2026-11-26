#pragma once
// Motorized grind-dial driver: stepper (TMC2209 STEP/DIR/EN + optional UART)
// on the small knob gear of an aftermarket Eureka Mignon gear-knob kit.
//
// Safety model
//  * The dial never moves while the grinder motor runs. Moves are refused while the
//    busy predicate is true, an active move is stopped within one step when the grinder
//    becomes busy, and Grinder::start() calls stop_and_wait() before closing the relay.
//  * Position 0 = burrs touching (calibrated). Normal moves never go below the
//    safety margin; only the calibration wizard may jog without soft limits.
//  * Low run current (default 350 mA RMS) so the motor skips instead of forcing the
//    burrs together; driver disabled when idle (hold current 0 % by default).
//  * Position is persisted in NVS. A "move in progress" flag detects resets/brownouts
//    during a move; the position is then untrusted until recovered (AS5600, homing)
//    or re-calibrated.

#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_timer.h>
#include "../sync/gbs_core.h"

class DialMotor {
public:
    struct Settings {
        uint16_t microsteps;
        uint16_t run_current_ma;
        uint16_t boost_current_ma;   // break-away current at the start of a move (0 = off)
        uint8_t stall_threshold;     // StallGuard SGTHRS (0 = off)
        uint8_t hold_current_pct;
        uint16_t max_speed_sps;      // fine speed
        uint16_t fast_speed_sps;     // travel speed for recipe switches
        uint16_t fast_accel_sps2;
        uint16_t accel_sps2;
        uint16_t backlash_steps;
        bool invert_dir;
        bool use_as5600;
        bool use_endstop;
        bool home_on_boot;
    };

    struct Status {
        bool initialized;
        bool moving;
        bool calibrated;
        bool position_trusted;
        bool calibrating;
        bool homed_reference;      // a home reference position has been learned
        bool encoder_present;
        bool slip_detected;
        bool brownout_during_move; // last reset was a brownout while the dial moved
        bool last_move_blocked;    // refused because grinder busy / untrusted
        bool stalled;              // StallGuard tripped during the last move
        bool cal_has_zero;
        bool cal_has_coarse;
        int32_t position_steps;
        int32_t target_steps;
        float setting;             // NAN if not calibrated
        float min_setting;
        int32_t zero_steps;
        int32_t coarse_steps;
        const char* last_error;
        uint8_t progress_pct;      // of the current move
        uint32_t eta_ms;           // remaining time estimate
        float target_setting;      // NAN if unknown
        bool dose_allowed;         // interlock: setting reached + confirmed, no move pending
        bool grinder_start_allowed;
    };

    void init();
    // Must be cheap and thread-safe; returns true while the grinder motor may run.
    void set_busy_predicate(bool (*busy)()) { busy_ = busy; }

    // --- Normal operation (soft limits, calibrated only) ---
    // fast = recipe switch (fast travel + slow final approach); false = fine GbS correction
    bool move_to_setting(float setting, bool fast = false);
    bool jog_setting(float delta);
    // --- Raw jog (calibration wizard): ignores soft limits, bounded by DIAL_MAX_JOG_STEPS ---
    bool jog_raw(int32_t steps);
    void stop();
    // Called right before the grinder relay closes: stops and waits for the dial to halt.
    void stop_and_wait(uint32_t timeout_ms);
    bool request_home();

    // --- Calibration wizard: 0 = burrs touching, 100 = coarsest ---
    void begin_calibration();
    bool set_zero_here();
    bool set_coarse_here();
    bool finish_calibration(float safety_margin);
    void cancel_calibration();
    bool set_safety_margin(float margin);
    // Declare the current position correct (user verified the dial visually).
    void trust_position();

    Status status();
    Settings settings();
    bool apply_settings(const Settings& s);
    bool is_moving() const { return moving_; }
    // moving or a move command is queued (thread-safe, cheap)
    bool move_pending() const;
    // Interlock snapshot (see gbs::grinder_start_allowed / gbs::dose_allowed)
    gbs::InterlockInputs interlock_inputs() const;
    bool grinder_start_allowed() const { return gbs::grinder_start_allowed(interlock_inputs()); }
    // Future hopper/dosing gate: true only when the setting move is complete and confirmed
    bool dose_allowed() const { return gbs::dose_allowed(interlock_inputs()); }

private:
    enum class CmdType : uint8_t { MoveAbs, Home, ApplyDriver };
    struct Cmd {
        CmdType type;
        int32_t target;
        bool raw;
        bool fast;
    };

    static void task_entry(void* arg);
    static void timer_cb(void* arg);
    void task_loop();
    bool enqueue(const Cmd& c);
    bool execute_leg(int32_t target, bool homing, bool fast = false);
    uint32_t estimate_ms(uint32_t steps, bool fast) const;
    void do_move(int32_t target, bool raw, bool fast);
    void do_home();
    void driver_enable(bool on);
    void configure_driver();
    void set_run_current(uint16_t ma);
    void tmc_write(uint8_t reg, uint32_t value);
    void persist_position(bool moving_flag, int32_t target);
    void load_state();
    bool busy() const { return busy_ && busy_(); }

    // encoder
    bool encoder_read(uint16_t& raw);
    void encoder_track();
    void encoder_verify_after_move();
    void encoder_boot_recover(bool was_moving, int32_t move_start, int32_t move_target);

    Preferences prefs_;
    SemaphoreHandle_t mutex_ = nullptr;
    QueueHandle_t queue_ = nullptr;
    TaskHandle_t task_ = nullptr;
    esp_timer_handle_t timer_ = nullptr;
    bool (*busy_)() = nullptr;

    Settings cfg_{};
    gbs::DialCalibration cal_{};
    gbs::DialCalibration pending_cal_{};
    bool calibrating_ = false;
    bool have_zero_ = false;
    bool have_coarse_ = false;

    // motion state shared with the timer callback
    volatile int32_t pos_ = 0;
    volatile int32_t leg_target_ = 0;
    volatile int8_t dir_ = 1;
    volatile bool moving_ = false;
    volatile bool cmd_active_ = false;   // a dequeued command is executing
    volatile bool stop_req_ = false;
    volatile bool homing_ = false;
    volatile bool endstop_hit_ = false;
    volatile bool stall_hit_ = false;
    volatile bool stall_armed_ = false;
    bool stalled_ = false;
    volatile uint32_t steps_done_ = 0;
    volatile uint32_t steps_total_ = 0;
    volatile int8_t last_dir_ = 0;
    int32_t final_target_ = 0;
    volatile bool leg_fast_ = false;
    uint32_t move_total_steps_ = 0;
    uint32_t move_done_before_leg_ = 0;
    uint32_t move_eta_total_ms_ = 0;
    uint32_t move_started_ms_ = 0;

    bool initialized_ = false;
    bool trusted_ = false;
    bool brownout_move_ = false;
    bool blocked_ = false;
    bool slip_ = false;
    const char* last_error_ = "";

    // homing reference
    bool home_ref_valid_ = false;
    int32_t home_ref_steps_ = 0;

    // AS5600
    bool enc_present_ = false;
    uint16_t enc_last_raw_ = 0;
    int32_t enc_total_ = 0;          // unwrapped counts
    int32_t enc_ref_total_ = 0;      // encoder total at pos_ref
    int32_t enc_pos_ref_ = 0;
    float enc_counts_per_step_ = 0;  // learned during calibration (signed)
    int32_t enc_cal_zero_total_ = 0;
};

extern DialMotor dial_motor;
