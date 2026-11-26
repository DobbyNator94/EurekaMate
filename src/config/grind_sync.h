#pragma once
// ============================================================================
// GRIND-BY-SYNC + MOTORIZED DIAL CONFIGURATION
// ============================================================================
// Pin choices use GPIOs that are broken out on the Waveshare
// ESP32-S3-Touch-AMOLED-1.64 header and are not used by the stock
// smart-grind-by-weight firmware (HX711 = GPIO2/3, relay = GPIO18,
// touch I2C = GPIO47/48). VERIFY against the Waveshare schematic before
// soldering - see docs/GAGGIMATE_GBS.md.

// ---- TMC2209 stepper driver (STEP/DIR/EN, optional single-wire UART) ----
#define DIAL_STEP_PIN 5          // TMC2209 STEP
#define DIAL_DIR_PIN 6           // TMC2209 DIR
#define DIAL_EN_PIN 7            // TMC2209 EN (active LOW)
#define DIAL_TMC_UART_TX_PIN 8   // TMC2209 PDN_UART via 1 kOhm series resistor (-1 to disable)
#define DIAL_TMC_UART_BAUD 115200
#define DIAL_TMC_ADDRESS 0       // MS1 = MS2 = GND

// ---- Optional sensors ----
#define DIAL_TMC_DIAG_PIN 1      // TMC2209 DIAG (StallGuard stall output, active HIGH) (-1 to disable)
#define DIAL_ENDSTOP_PIN 44      // Optional coarse-end endstop on U0RXD, switch to GND (-1 to disable)
#define DIAL_AS5600_SDA_PIN 15   // Optional AS5600 magnetic encoder (I2C port 1)
#define DIAL_AS5600_SCL_PIN 16
#define DIAL_AS5600_I2C_HZ 400000

// ---- Optional external button (e.g. Eureka front button / extra push button) ----
#define SYNC_BREW_BUTTON_PIN 17  // Active LOW, starts a brew on GaggiMate (-1 to disable)

// ---- Motion defaults (Eureka Mignon Specialita + aftermarket gear-knob kit) ----
#define DIAL_DEFAULT_MICROSTEPS 16
#define DIAL_DEFAULT_RUN_CURRENT_MA 400   // RMS while moving (17HS4401: rated 1.7 A; low on purpose for the 12 V rail)
#define DIAL_DEFAULT_BOOST_CURRENT_MA 600 // RMS for the first DIAL_BOOST_MS of every move (break-away torque of a stiff dial)
#define DIAL_BOOST_MS 400
#define DIAL_DEFAULT_STALL_THRESHOLD 0    // TMC2209 SGTHRS 0..255 (0 = StallGuard off; tune on the grinder)
#define DIAL_DEFAULT_HOLD_CURRENT_PCT 0   // 0 = driver disabled when idle
#define DIAL_DEFAULT_MAX_SPEED_SPS 800    // fine moves (GbS corrections), microsteps per second
#define DIAL_DEFAULT_FAST_SPEED_SPS 4000  // fast travel (recipe switch), microsteps per second
#define DIAL_DEFAULT_FAST_ACCEL_SPS2 6000
#define DIAL_SLOW_ZONE 5.0f              // setting units above the safety margin: final approach at fine speed
#define DIAL_DEFAULT_ACCEL_SPS2 1500      // microsteps per second^2 (slow ramp, low current peaks)
#define DIAL_DEFAULT_BACKLASH_STEPS 0     // extra microsteps for final approach from the coarse side
#define DIAL_DEFAULT_SAFETY_MARGIN 5.0f   // setting units kept above calibrated 0 (burrs touching)
#define DIAL_DEFAULT_SAFETY_MARGIN_F 5.0f
#define DIAL_DEFAULT_DIR_INVERT false
#define DIAL_TMC_RSENSE_OHM 0.11f         // typical TMC2209 module sense resistor
#define DIAL_MAX_JOG_STEPS 20000          // hard limit for a single uncalibrated jog
#define DIAL_HOMING_SPEED_SPS 300

// ---- Grind-by-Sync client defaults ----
#define SYNC_DEFAULT_HOST "gaggimate.local"
#define SYNC_POLL_INTERVAL_MS 1000        // GaggiMate keeps "shot done" visible ~5 s
#define SYNC_HTTP_TIMEOUT_MS 1500
#define SYNC_SHOT_WINDOW_MS (5UL * 60UL * 1000UL) // shot must start within 5 min after the grind
#define SYNC_DEFAULT_TARGET_TIME_S 28.0f
#define SYNC_DEFAULT_RATIO 2.0f
#define SYNC_PROFILE_SLOTS 3   // recipes 1-3 follow the SGBW profiles (Single/Double/Custom)
#define SYNC_RECIPE_SLOTS 6    // recipes 4-6 are extra dial presets (e.g. Filter, Decaf) selected on display/web

// ---- Future 4-bean hopper (reserved: data model + hooks only, no hardware yet) ----
// One presence/position sensor per hopper slot (active LOW, e.g. hall sensor or microswitch).
// The recipe that has the slot assigned is selected when that slot is detected.
#define SYNC_HOPPER_SLOTS 4
#define SYNC_HOPPER_SENSOR_PINS {-1, -1, -1, -1}   // GPIOs per slot, -1 = not fitted
#define SYNC_HOPPER_DEBOUNCE_MS 150
// Portions per recipe: number of dosing grinds before the recipe counts as done (filter: e.g. 3)
#define SYNC_MAX_PORTIONS 10
