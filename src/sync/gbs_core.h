#pragma once
// Pure, hardware-independent logic for the Grind-by-Sync client.
// Compiled both for the ESP32 firmware and for native unit tests (pio test -e native).
//
// Protocol notes (grinder side of Mahlkoenig Grind-by-Sync, as spoken by an
// E64 WS to a "Xenia" machine; GaggiMate >= v1.10.0 emulates that machine):
//   GET  /api/v2/machine          -> {"MA_TYPE":1, ...}            identity check
//   GET  /api/v2/mako             -> live status, polled ~1 Hz
//   POST /api/v2/brewratio        <- {"SYNC_BEVERAGE_WEIGHT":36.0,"SYNC_FILTER":2}
//   POST /api/v2/scripts/execute  <- {"ID":9}                       start a brew
// This file is an independent implementation written from the observable
// HTTP/JSON interface; it contains no GaggiMate source code.

#include <stddef.h>
#include <stdint.h>

namespace gbs {

// ---------------------------------------------------------------------------
// Wire constants
// ---------------------------------------------------------------------------
constexpr const char* PATH_IDENTITY = "/api/v2/machine";
constexpr const char* PATH_STATUS = "/api/v2/mako";
constexpr const char* PATH_GRIND_DONE = "/api/v2/brewratio";
constexpr const char* PATH_RUN_SCRIPT = "/api/v2/scripts/execute";
constexpr int SCRIPT_START_BREW = 9;
constexpr int MACHINE_TYPE_XENIA = 1;

enum class MachineState : int {
    Standby = 0,
    Ready = 1,
    Brewing = 3,
    ShotDone = 4,
    Service = 5,
    Error = 6,
    Unknown = -1,
};

struct MachineStatus {
    MachineState state = MachineState::Unknown;
    bool extraction_active = false;   // MA_EXTRACTION_STATUS == 1
    uint32_t extraction_ms = 0;       // REAL_EXTRACTION_TIME_MS
    float volume_g = 0.0f;            // PU_SENS_FLOW_METER_VOLUME (scale weight on GaggiMate)
    float set_temp_c = 0.0f;          // BG_SET_TEMP
    float temp_c = 0.0f;              // BG_SENS_TEMP
    bool water_ok = true;             // TANK_LEVEL == 1
};

// JSON helpers (ArduinoJson inside). Return false on malformed input.
bool parse_status(const char* json, size_t len, MachineStatus& out);
bool parse_identity(const char* json, size_t len, int& machine_type);
// Build request bodies. Return number of bytes written (0 on error).
size_t build_grind_done(char* buf, size_t size, float beverage_weight_g, int recipe);
size_t build_start_brew(char* buf, size_t size);
const char* state_name(MachineState s);

// ---------------------------------------------------------------------------
// Shot detection from the polled status stream
// ---------------------------------------------------------------------------
struct ShotResult {
    float time_s = 0.0f;
    float weight_g = 0.0f;   // 0 = unknown
    bool aborted = false;    // machine left "brewing" without reporting "shot done"
};

class ShotDetector {
public:
    enum class Event { None, ShotStarted, ShotFinished };

    // Feed one poll result. Returns the event; on ShotFinished `result` is filled.
    Event feed(const MachineStatus& s, uint32_t now_ms, ShotResult& result);
    // Connection lost: an ongoing shot is reported as aborted on the next feed.
    void connection_lost(uint32_t now_ms);
    bool brewing() const { return phase_ == Phase::Brewing; }
    void reset() { phase_ = Phase::Idle; last_ms_ = 0; }

private:
    enum class Phase { Idle, Brewing, Reported };
    Phase phase_ = Phase::Idle;
    uint32_t last_ms_ = 0;      // last extraction time seen while brewing
    bool lost_ = false;
};

// ---------------------------------------------------------------------------
// Regulation (what Grind-by-Sync does on the grinder)
// Setting scale: 0 = finest (burrs touching, calibrated), 100 = coarsest.
// Shot too fast -> finer (negative delta); too slow -> coarser.
// ---------------------------------------------------------------------------
struct RegulatorConfig {
    float target_time_s = 28.0f;
    float deadband_s = 1.5f;        // |error| <= deadband -> no change
    float gain_per_s = 0.3f;        // setting units per second of error
    float max_step = 2.0f;          // max |delta| per shot (setting units)
    float min_valid_s = 15.0f;      // GbS rule: shot must be longer than 15 s
    float max_over_s = 15.0f;       // ... and not longer than target + 15 s
    float yield_tolerance = 0.25f;  // ignore shots whose yield is off by more than 25 %
    float resolution = 0.1f;        // round deltas to this step
};

enum class Verdict {
    Adjust,          // delta != 0
    InDeadband,      // valid shot, no change needed
    Aborted,
    TooShort,
    TooLong,
    YieldOff,
    Disabled,
};

struct Decision {
    Verdict verdict = Verdict::Disabled;
    float delta = 0.0f;   // setting units
    float error_s = 0.0f; // measured - target
};

Decision evaluate_shot(const RegulatorConfig& cfg, const ShotResult& shot, float target_weight_g);
const char* verdict_name(Verdict v);
// Apply a delta and clamp into [min_setting, 100].
float apply_delta(float current, float delta, float min_setting);

// ---------------------------------------------------------------------------
// Grind -> shot association (a shot only regulates the recipe that was ground for it)
// ---------------------------------------------------------------------------
class GrindShotLink {
public:
    void grind_done(int profile, float dose_g, uint32_t now_ms);
    // Call when a shot starts. Returns the profile it belongs to, or -1.
    int shot_started(uint32_t now_ms, uint32_t window_ms);
    void clear() { pending_ = false; active_profile_ = -1; }
    int active_profile() const { return active_profile_; }
    float active_dose() const { return active_dose_; }
    bool pending() const { return pending_; }

private:
    bool pending_ = false;
    int pending_profile_ = -1;
    float pending_dose_ = 0.0f;
    uint32_t grind_ms_ = 0;
    int active_profile_ = -1;
    float active_dose_ = 0.0f;
};

// ---------------------------------------------------------------------------
// Dial calibration: map motor steps <-> 0..100 setting
// ---------------------------------------------------------------------------
struct DialCalibration {
    bool valid = false;
    int32_t zero_steps = 0;     // burrs touching (finest)
    int32_t coarse_steps = 0;   // coarsest usable point (=100)
    float safety_margin = 5.0f; // lowest allowed setting

    static constexpr int32_t MIN_SPAN_STEPS = 200;

    bool span_ok() const;
    float steps_to_setting(int32_t steps) const;
    int32_t setting_to_steps(float setting) const;
    float min_setting() const { return safety_margin; }
    float clamp_setting(float setting) const;
    // Direction that moves the dial coarser: +1 or -1
    int coarse_sign() const { return coarse_steps >= zero_steps ? 1 : -1; }
};

// Wrap-aware difference of two 12-bit encoder readings, in [-2048, 2047].
int32_t encoder_diff(uint16_t a, uint16_t b);

// TMC2209 helpers (pure math, testable)
uint8_t tmc_crc8(const uint8_t* data, size_t len);
// Compute current-scale (CS 0..31) and vsense for a target RMS current.
void tmc_current_scale(float rms_ma, float rsense_ohm, uint8_t& cs, bool& vsense);
// Build an 8-byte write datagram.
void tmc_write_datagram(uint8_t addr, uint8_t reg, uint32_t value, uint8_t out[8]);

// ---------------------------------------------------------------------------
// Multi-portion recipes (e.g. filter: 3 dosing turns before the brew).
// Each completed grind adds a portion; the recipe counts as "ground" (report to
// GaggiMate, start the shot link) only after the last portion.
// ---------------------------------------------------------------------------
struct PortionCounter {
    uint8_t total = 1;      // portions per recipe (1..MAX_PORTIONS)
    uint8_t done = 0;
    float dose_sum_g = 0.0f;
    static constexpr uint8_t MAX_PORTIONS = 10;

    void start(uint8_t portions);          // clamps to 1..MAX_PORTIONS, resets progress
    // Returns true when this grind completed the recipe (then the counter resets).
    bool add(float dose_g, float* total_dose_g = nullptr);
    void reset() { done = 0; dose_sum_g = 0.0f; }
};

// ---------------------------------------------------------------------------
// Future 4-bean hopper: map a hopper slot (0..n-1, from a slot sensor) to the
// recipe that has it assigned. Returns the recipe index or -1.
// hopper_slot_of_recipe[i] = slot of recipe i, or -1 if the recipe has none.
// ---------------------------------------------------------------------------
int recipe_for_hopper_slot(const int8_t* hopper_slot_of_recipe, int recipe_count, int slot);

// ---------------------------------------------------------------------------
// Interlocks
//  * grinder_start_allowed: the grinder motor must never start while the dial
//    stepper moves or has a move queued.
//  * dose_allowed: (future hopper / dosing gate) beans may only be released to
//    the burrs once the grind-setting move is COMPLETE and the position is
//    confirmed (calibrated + trusted, not calibrating, no stall).
// ---------------------------------------------------------------------------
struct InterlockInputs {
    bool motor_present = true;  // dial drive initialised
    bool moving = false;
    bool move_pending = false;  // command queued but not started
    bool calibrated = false;
    bool position_trusted = false;
    bool calibrating = false;
    bool stalled = false;
};
bool grinder_start_allowed(const InterlockInputs& in);
bool dose_allowed(const InterlockInputs& in);

}  // namespace gbs
