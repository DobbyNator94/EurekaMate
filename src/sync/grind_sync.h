#pragma once
// Grind-by-Sync client: the grinder side of the protocol a Mahlkoenig E64 WS speaks
// with its "Xenia" espresso machine. GaggiMate (>= v1.10.0, Settings -> Plugins ->
// "Mahlkoenig E64 WS Grind-by-Sync") plays that machine, so this grinder appears to
// GaggiMate as an E64 WS: it polls the machine status, reports finished grinds
// (recipe + target beverage weight) and can ask the machine to start a brew.
// After each valid shot it corrects the motorized dial (see DialMotor).

#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "gbs_core.h"
#include "../config/grind_sync.h"

class GrindSync {
public:
    // Per recipe (= SGBW profile slot 1..3 = GbS recipe / SYNC_FILTER on GaggiMate)
    struct Recipe {
        char name[16];
        bool gbs;                 // "Enable GbS" for this recipe
        float target_time_s;      // "Brew Time"
        float brew_weight_g;      // "Brew Weight" (0 = dose x ratio)
        float start_setting;      // "GbS start DD" equivalent (NAN = none)
        int8_t hopper_slot;       // future 4-bean hopper: slot 0..SYNC_HOPPER_SLOTS-1, -1 = none
        uint8_t portions;         // dosing grinds per recipe (1 = normal, filter e.g. 3)
    };

    struct Config {
        bool enabled;             // WiFi + polling on
        bool auto_adjust;         // master switch for automatic grind correction
        bool send_recipe;         // POST brewratio after each grind
        bool start_brew_after_grind;
        bool follow_profile;      // "Auto adjust to recipe DD": move dial when the recipe changes
        float ratio;              // default beverage weight = dose * ratio
        gbs::RegulatorConfig reg; // shared regulation parameters (target time comes from the recipe)
        Recipe recipe[SYNC_RECIPE_SLOTS];
        char host[64];            // GaggiMate host/IP (default gaggimate.local)
        bool hopper_enabled;      // future: select recipes from hopper slot sensors
    };

    struct ShotRecord {
        uint32_t seq;
        uint32_t uptime_s;
        float time_s;
        float weight_g;
        float target_s;
        float delta;
        float setting_after;      // NAN if unknown
        int8_t profile;           // -1 = not started from this grinder
        uint8_t verdict;          // gbs::Verdict
        uint8_t manual;           // entered by hand
        uint8_t reserved;
    };
    static constexpr int HISTORY_SIZE = 20;

    struct Status {
        bool wifi_connected;
        bool machine_seen;        // last poll succeeded
        bool identity_ok;         // /api/v2/machine reports a Xenia (MA_TYPE 1)
        bool portal_active;
        char portal_ssid[33];
        char portal_pass[12];
        char ip[16];
        char machine_ip[16];
        gbs::MachineState machine_state;
        uint32_t live_extraction_ms;
        bool shot_pending_grind;  // a grind is waiting for its shot
        // last shot
        bool has_last_shot;
        float last_time_s;
        float last_weight_g;
        int last_profile;
        gbs::Verdict last_verdict;
        float last_delta;
        uint32_t shots_seen;
        uint32_t adjustments;
        uint32_t poll_errors;
        float profile_setting[SYNC_RECIPE_SLOTS]; // per-recipe grind setting, NAN = not stored yet
        int active_recipe;                         // 0-based
        uint8_t portion_done;                      // portions ground for the active recipe
        uint8_t portion_total;
        int8_t hopper_slot;                        // last detected hopper slot, -1 = none/unknown
    };

    void init();
    // From the grind control task: a grind finished (cheap, just records it).
    void notify_grind_completed(int profile, float dose_g);
    // UI / button
    void request_brew_start();
    void start_setup_portal();
    void stop_setup_portal();
    void remember_current_setting_for_profile(int profile);
    void reset_profile_to_start(int profile);
    // Select a recipe slot (0-based) on display/web: dial travels there (fast profile).
    void select_recipe(int recipe);
    int active_recipe() const { return active_recipe_; }
    // Manual brew-time entry (like the E64 WS "Brew Time" input without a synced machine)
    void submit_manual_shot(float time_s, float weight_g);
    // Re-run identity + status check now (web "Test connection")
    void request_reconnect();
    int history(ShotRecord* out, int max_items); // newest first
    float recipe_target_weight(int profile, float dose_g);
    // Future 4-bean hopper hook: a slot sensor reports slot (0..3) or -1 (none).
    // Selects the recipe mapped to that slot when hopper_enabled.
    void on_hopper_slot_changed(int slot);
    // Interlock for a future dosing gate (hopper chamber -> burrs): only release beans
    // when this returns true (dial move finished, position confirmed). Never bypass.
    static bool dose_allowed();
    // Discard a partly ground multi-portion recipe.
    void reset_portions();

    Config config();
    void set_config(const Config& c);
    Status status();
    bool has_wifi_credentials();
    void set_wifi_credentials(const char* ssid, const char* pass);
    String wifi_ssid();
    void set_profile_provider(int (*current_profile)()) { current_profile_ = current_profile; }

private:
    static void task_entry(void* arg);
    void task_loop();
    void load_config();
    void save_config();
    void ensure_wifi();
    bool resolve_host(IPAddress& out);
    bool http_get(const char* path, String& body);
    bool http_post(const char* path, const char* body);
    void poll_once();
    void handle_shot(const gbs::ShotResult& shot);
    void handle_pending_actions();
    void follow_profile();
    void portal_loop();
    void mock_tick();
    void add_history(const ShotRecord& r);
    void load_history();
    void save_history();
    void poll_hopper_sensors();

    Preferences prefs_;
    SemaphoreHandle_t mutex_ = nullptr;
    Config cfg_{};
    Status st_{};
    gbs::ShotDetector detector_;
    gbs::GrindShotLink link_;
    int (*current_profile_)() = nullptr;
    int last_followed_profile_ = -1;   // recipe the dial was last moved to
    int last_sgbw_profile_ = -1;
    volatile int active_recipe_ = 0;
    bool user_selected_ = false;

    // pending flags set from other tasks
    volatile bool grind_done_flag_ = false;
    volatile int grind_profile_ = 0;
    volatile float grind_dose_ = 0;
    volatile bool brew_req_ = false;
    volatile bool portal_req_ = false;
    volatile bool portal_stop_req_ = false;
    volatile bool config_dirty_ = false;
    volatile bool wifi_reconnect_req_ = false;

    bool identity_checked_ = false;
    IPAddress machine_addr_;
    uint16_t machine_port_ = 80;
    gbs::PortionCounter portions_;
    int portions_recipe_ = -1;
    int8_t hopper_raw_ = -1;
    uint32_t hopper_change_ms_ = 0;
    uint32_t last_resolve_ms_ = 0;
    uint32_t last_wifi_attempt_ms_ = 0;
    uint8_t consecutive_errors_ = 0;
    float pending_target_weight_ = 0;
    ShotRecord history_[HISTORY_SIZE] = {};
    int history_count_ = 0;
    int history_head_ = 0;   // next write index
    uint32_t history_seq_ = 0;
    volatile bool manual_shot_ = false;
    volatile float manual_time_ = 0;
    volatile float manual_weight_ = 0;
};

extern GrindSync grind_sync;
