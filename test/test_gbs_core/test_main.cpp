// Native unit tests for the Grind-by-Sync logic: pio test -e native
#include <math.h>
#include <string.h>
#include <unity.h>
#include "sync/gbs_core.h"

using namespace gbs;

void setUp() {}
void tearDown() {}

static ShotResult shot(float t, float w = 0, bool aborted = false) {
    ShotResult s;
    s.time_s = t;
    s.weight_g = w;
    s.aborted = aborted;
    return s;
}

// ---------------- regulator ----------------
void test_too_fast_goes_finer() {
    RegulatorConfig c;  // target 28, deadband 1.5, gain 0.3, max 2
    Decision d = evaluate_shot(c, shot(24.0f), 0);
    TEST_ASSERT_EQUAL(Verdict::Adjust, d.verdict);
    TEST_ASSERT_TRUE(d.delta < 0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.8f, d.delta);  // (4 - 1.5) * 0.3 = 0.75 -> 0.8
}

void test_too_slow_goes_coarser() {
    RegulatorConfig c;
    Decision d = evaluate_shot(c, shot(33.0f), 0);
    TEST_ASSERT_EQUAL(Verdict::Adjust, d.verdict);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.1f, d.delta);  // (5 - 1.5) * 0.3 = 1.05 -> 1.1 (round half away)
}

void test_deadband() {
    RegulatorConfig c;
    TEST_ASSERT_EQUAL(Verdict::InDeadband, evaluate_shot(c, shot(29.4f), 0).verdict);
    TEST_ASSERT_EQUAL(Verdict::InDeadband, evaluate_shot(c, shot(26.5f), 0).verdict);
    TEST_ASSERT_EQUAL(0.0f, evaluate_shot(c, shot(28.0f), 0).delta);
}

void test_validity_window_like_gbs() {
    RegulatorConfig c;
    TEST_ASSERT_EQUAL(Verdict::TooShort, evaluate_shot(c, shot(15.0f), 0).verdict);  // must be > 15 s
    TEST_ASSERT_EQUAL(Verdict::Adjust, evaluate_shot(c, shot(15.1f), 0).verdict);
    TEST_ASSERT_EQUAL(Verdict::Adjust, evaluate_shot(c, shot(43.0f), 0).verdict);   // <= target + 15
    TEST_ASSERT_EQUAL(Verdict::TooLong, evaluate_shot(c, shot(43.1f), 0).verdict);
    TEST_ASSERT_EQUAL(Verdict::Aborted, evaluate_shot(c, shot(25.0f, 0, true), 0).verdict);
}

void test_step_is_limited() {
    RegulatorConfig c;
    c.max_step = 1.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, evaluate_shot(c, shot(16.0f), 0).delta);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, evaluate_shot(c, shot(42.0f), 0).delta);
}

void test_small_error_never_rounds_to_zero() {
    RegulatorConfig c;
    c.gain_per_s = 0.01f;
    Decision d = evaluate_shot(c, shot(30.0f), 0);  // 0.5 s outside deadband * 0.01 = 0.005
    TEST_ASSERT_EQUAL(Verdict::Adjust, d.verdict);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.1f, d.delta);
}

void test_yield_check() {
    RegulatorConfig c;
    TEST_ASSERT_EQUAL(Verdict::YieldOff, evaluate_shot(c, shot(24.0f, 20.0f), 36.0f).verdict);
    TEST_ASSERT_EQUAL(Verdict::Adjust, evaluate_shot(c, shot(24.0f, 35.0f), 36.0f).verdict);
    TEST_ASSERT_EQUAL(Verdict::Adjust, evaluate_shot(c, shot(24.0f, 0.0f), 36.0f).verdict);  // unknown weight ok
}

void test_apply_delta_respects_safety_margin() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, apply_delta(5.5f, -2.0f, 5.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, apply_delta(99.5f, 2.0f, 5.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 31.2f, apply_delta(30.0f, 1.2f, 5.0f));
}

// ---------------- shot detector ----------------
static MachineStatus st(MachineState s, uint32_t ms, float vol = 0) {
    MachineStatus m;
    m.state = s;
    m.extraction_active = (s == MachineState::Brewing || s == MachineState::ShotDone);
    m.extraction_ms = ms;
    m.volume_g = vol;
    return m;
}

void test_detector_normal_shot() {
    ShotDetector d;
    ShotResult r;
    TEST_ASSERT_EQUAL(ShotDetector::Event::None, d.feed(st(MachineState::Ready, 0), 0, r));
    TEST_ASSERT_EQUAL(ShotDetector::Event::ShotStarted, d.feed(st(MachineState::Brewing, 500), 1000, r));
    TEST_ASSERT_EQUAL(ShotDetector::Event::None, d.feed(st(MachineState::Brewing, 15000), 15000, r));
    // GaggiMate keeps "brewing" while the scale settles, then reports shot done with time + weight
    TEST_ASSERT_EQUAL(ShotDetector::Event::None, d.feed(st(MachineState::Brewing, 27400), 30000, r));
    TEST_ASSERT_EQUAL(ShotDetector::Event::ShotFinished, d.feed(st(MachineState::ShotDone, 27400, 36.2f), 31000, r));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 27.4f, r.time_s);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 36.2f, r.weight_g);
    TEST_ASSERT_FALSE(r.aborted);
    // shot-done stays visible for a few polls -> no duplicate
    TEST_ASSERT_EQUAL(ShotDetector::Event::None, d.feed(st(MachineState::ShotDone, 27400, 36.2f), 32000, r));
    TEST_ASSERT_EQUAL(ShotDetector::Event::None, d.feed(st(MachineState::Ready, 0), 36000, r));
}

void test_detector_aborted_shot() {
    ShotDetector d;
    ShotResult r;
    d.feed(st(MachineState::Brewing, 100), 0, r);
    d.feed(st(MachineState::Brewing, 8000), 8000, r);
    TEST_ASSERT_EQUAL(ShotDetector::Event::ShotFinished, d.feed(st(MachineState::Ready, 0), 9000, r));
    TEST_ASSERT_TRUE(r.aborted);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 8.0f, r.time_s);
}

void test_detector_connection_lost() {
    ShotDetector d;
    ShotResult r;
    d.feed(st(MachineState::Brewing, 1000), 0, r);
    d.connection_lost(5000);
    TEST_ASSERT_EQUAL(ShotDetector::Event::ShotFinished, d.feed(st(MachineState::Ready, 0), 60000, r));
    TEST_ASSERT_TRUE(r.aborted);
}

void test_detector_back_to_back() {
    ShotDetector d;
    ShotResult r;
    d.feed(st(MachineState::Brewing, 100), 0, r);
    d.feed(st(MachineState::ShotDone, 26000, 30), 27000, r);
    TEST_ASSERT_EQUAL(ShotDetector::Event::ShotStarted, d.feed(st(MachineState::Brewing, 300), 29000, r));
}

// ---------------- grind/shot link ----------------
void test_link_window() {
    GrindShotLink l;
    l.grind_done(1, 18.0f, 1000);
    TEST_ASSERT_EQUAL(1, l.shot_started(5000, 300000));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 18.0f, l.active_dose());
    // one grind regulates at most one shot
    TEST_ASSERT_EQUAL(-1, l.shot_started(9000, 300000));
    l.grind_done(0, 9.0f, 10000);
    TEST_ASSERT_EQUAL(-1, l.shot_started(10000 + 300001, 300000));
}

// ---------------- dial calibration ----------------
void test_dial_mapping_positive() {
    DialCalibration c;
    c.zero_steps = 1000;
    c.coarse_steps = 11000;
    c.safety_margin = 5;
    TEST_ASSERT_TRUE(c.span_ok());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, c.steps_to_setting(1000));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, c.steps_to_setting(11000));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, c.steps_to_setting(3500));
    TEST_ASSERT_EQUAL_INT32(6000, c.setting_to_steps(50));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, c.clamp_setting(1.0f));
    TEST_ASSERT_EQUAL(1, c.coarse_sign());
}

void test_dial_mapping_negative_direction() {
    DialCalibration c;
    c.zero_steps = 0;
    c.coarse_steps = -8000;  // motor turns the other way for "coarser"
    TEST_ASSERT_EQUAL(-1, c.coarse_sign());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, c.steps_to_setting(-4000));
    TEST_ASSERT_EQUAL_INT32(-800, c.setting_to_steps(10));
    TEST_ASSERT_EQUAL_INT32(0, c.setting_to_steps(-5));  // clamps to 0..100
}

void test_dial_span_too_small() {
    DialCalibration c;
    c.zero_steps = 0;
    c.coarse_steps = 50;
    TEST_ASSERT_FALSE(c.span_ok());
}

void test_encoder_diff_wraps() {
    TEST_ASSERT_EQUAL_INT32(10, encoder_diff(5, 4091));
    TEST_ASSERT_EQUAL_INT32(-10, encoder_diff(4091, 5));
    TEST_ASSERT_EQUAL_INT32(100, encoder_diff(200, 100));
}

// ---------------- protocol JSON ----------------
void test_parse_status() {
    const char* j = "{\"REAL_EXTRACTION_TIME_MS\":27400,\"PU_SENS_FLOW_METER_VOLUME\":36.5,\"BG_SET_TEMP\":93,"
                    "\"BG_SENS_TEMP\":92.8,\"TANK_LEVEL\":1,\"script\":\"\",\"TIMESTAMP\":\"\","
                    "\"MA_EXTRACTION_STATUS\":1,\"MA_STATUS\":4}";
    MachineStatus m;
    TEST_ASSERT_TRUE(parse_status(j, strlen(j), m));
    TEST_ASSERT_EQUAL(MachineState::ShotDone, m.state);
    TEST_ASSERT_TRUE(m.extraction_active);
    TEST_ASSERT_EQUAL_UINT32(27400, m.extraction_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 36.5f, m.volume_g);
    TEST_ASSERT_TRUE(m.water_ok);
}

void test_parse_status_strings_and_garbage() {
    const char* j = "{\"MA_STATUS\":\"3\",\"REAL_EXTRACTION_TIME_MS\":\"1200\",\"TANK_LEVEL\":0}";
    MachineStatus m;
    TEST_ASSERT_TRUE(parse_status(j, strlen(j), m));
    TEST_ASSERT_EQUAL(MachineState::Brewing, m.state);
    TEST_ASSERT_EQUAL_UINT32(1200, m.extraction_ms);
    TEST_ASSERT_FALSE(m.water_ok);
    TEST_ASSERT_FALSE(parse_status("{}", 2, m));
    TEST_ASSERT_FALSE(parse_status("not json", 8, m));
}

void test_identity() {
    const char* j = "{\"MA_TYPE\":1,\"MA_SN\":\"0000000001\",\"FW_VERSION_MAJOR\":4}";
    int t = 0;
    TEST_ASSERT_TRUE(parse_identity(j, strlen(j), t));
    TEST_ASSERT_EQUAL(1, t);
}

void test_build_bodies() {
    char buf[96];
    TEST_ASSERT_TRUE(build_grind_done(buf, sizeof(buf), 36.04f, 2) > 0);
    TEST_ASSERT_EQUAL_STRING("{\"SYNC_BEVERAGE_WEIGHT\":36,\"SYNC_FILTER\":2}", buf);
    TEST_ASSERT_TRUE(build_grind_done(buf, sizeof(buf), 40.55f, 1) > 0);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"SYNC_FILTER\":1"));
    TEST_ASSERT_TRUE(build_start_brew(buf, sizeof(buf)) > 0);
    TEST_ASSERT_EQUAL_STRING("{\"ID\":9}", buf);
}

// ---------------- TMC2209 helpers ----------------
void test_tmc_crc_known_vector() {
    uint8_t dg[8];
    tmc_write_datagram(0, 0x00, 0x000001C0, dg);
    TEST_ASSERT_EQUAL_HEX8(0x05, dg[0]);
    TEST_ASSERT_EQUAL_HEX8(0x80, dg[2]);
    TEST_ASSERT_EQUAL_HEX8(tmc_crc8(dg, 7), dg[7]);
    // GCONF read request "05 00 00" -> CRC 0x48 (commonly quoted TMC2209 example)
    const uint8_t rd[3] = {0x05, 0x00, 0x00};
    TEST_ASSERT_EQUAL_HEX8(0x48, tmc_crc8(rd, 3));
}

void test_tmc_current_scale() {
    uint8_t cs;
    bool vs;
    tmc_current_scale(400, 0.11f, cs, vs);
    TEST_ASSERT_TRUE(vs);  // low current -> high sensitivity range
    // I = (cs+1)/32 * 0.18/0.13 / sqrt2 must not exceed 400 mA
    const float i = (cs + 1) / 32.0f * 0.18f / 0.13f / 1.41421356f;
    TEST_ASSERT_TRUE(i <= 0.401f);
    TEST_ASSERT_TRUE(i > 0.33f);
    tmc_current_scale(1200, 0.11f, cs, vs);
    TEST_ASSERT_FALSE(vs);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_too_fast_goes_finer);
    RUN_TEST(test_too_slow_goes_coarser);
    RUN_TEST(test_deadband);
    RUN_TEST(test_validity_window_like_gbs);
    RUN_TEST(test_step_is_limited);
    RUN_TEST(test_small_error_never_rounds_to_zero);
    RUN_TEST(test_yield_check);
    RUN_TEST(test_apply_delta_respects_safety_margin);
    RUN_TEST(test_detector_normal_shot);
    RUN_TEST(test_detector_aborted_shot);
    RUN_TEST(test_detector_connection_lost);
    RUN_TEST(test_detector_back_to_back);
    RUN_TEST(test_link_window);
    RUN_TEST(test_dial_mapping_positive);
    RUN_TEST(test_dial_mapping_negative_direction);
    RUN_TEST(test_dial_span_too_small);
    RUN_TEST(test_encoder_diff_wraps);
    RUN_TEST(test_parse_status);
    RUN_TEST(test_parse_status_strings_and_garbage);
    RUN_TEST(test_identity);
    RUN_TEST(test_build_bodies);
    RUN_TEST(test_tmc_crc_known_vector);
    RUN_TEST(test_tmc_current_scale);
    return UNITY_END();
}
