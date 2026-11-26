#include "grind_sync_ui.h"

#include <Arduino.h>
#include <math.h>
#include "../../config/constants.h"
#include "../../config/build_info.h"
#include "../../config/grind_sync.h"
#include "../../motor/dial_motor.h"
#include "../../sync/grind_sync.h"
#include "../ui_helpers.h"

GrindSyncUI grind_sync_ui;

namespace {

lv_color_t c(uint32_t hex) { return lv_color_hex(hex); }

void style_card(lv_obj_t* o) {
    lv_obj_set_style_bg_color(o, c(0x141418), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, c(0x2A2A30), 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_radius(o, 16, 0);
    lv_obj_set_style_pad_all(o, 12, 0);
}

void page_layout(lv_obj_t* page) {
    lv_obj_set_layout(page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(page, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_pad_row(page, 10, 0);
}

lv_obj_t* separator(lv_obj_t* parent, const char* text) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(l, c(THEME_COLOR_ACCENT), 0);
    lv_obj_set_style_margin_top(l, 8, 0);
    return l;
}

lv_obj_t* text(lv_obj_t* parent, const char* t, const lv_font_t* font = &lv_font_montserrat_24,
               uint32_t color = THEME_COLOR_TEXT_SECONDARY, int width = 260) {
    lv_obj_t* l = lv_label_create(parent);
    lv_label_set_text(l, t);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, c(color), 0);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, width);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    return l;
}

lv_obj_t* button(lv_obj_t* parent, const char* t, lv_event_cb_t cb, void* ud, int w = 260, int h = 70,
                 uint32_t color = 0x26262C, lv_obj_t** out_label = nullptr) {
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, c(color), 0);
    lv_obj_set_style_radius(b, 14, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, t);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    if (out_label) *out_label = l;
    return b;
}

lv_obj_t* row(lv_obj_t* parent) {
    lv_obj_t* r = lv_obj_create(parent);
    lv_obj_set_size(r, 270, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_set_layout(r, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(r, 8, 0);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    return r;
}

lv_obj_t* toggle_row(lv_obj_t* parent, const char* label, lv_obj_t** out_sw, lv_event_cb_t cb) {
    lv_obj_t* r = lv_obj_create(parent);
    style_as_button(r);
    lv_obj_set_layout(r, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* l = lv_label_create(r);
    lv_label_set_text(l, label);
    *out_sw = lv_switch_create(r);
    lv_obj_set_size(*out_sw, 80, 40);
    lv_obj_set_ext_click_area(*out_sw, 20);
    lv_obj_add_event_cb(*out_sw, cb, LV_EVENT_VALUE_CHANGED, nullptr);
    return r;
}

lv_obj_t* slider_row(lv_obj_t* parent, lv_obj_t** out_label, lv_obj_t** out_slider, int32_t min, int32_t max,
                     lv_event_cb_t on_change, lv_event_cb_t on_release) {
    lv_obj_t* r = lv_obj_create(parent);
    style_as_button(r, 260, LV_SIZE_CONTENT);
    lv_obj_set_layout(r, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(r, 14, 0);
    lv_obj_set_style_pad_all(r, 18, 0);
    *out_label = lv_label_create(r);
    *out_slider = lv_slider_create(r);
    lv_obj_set_size(*out_slider, 220, 36);
    lv_obj_set_ext_click_area(*out_slider, 20);
    lv_slider_set_range(*out_slider, min, max);
    lv_obj_set_style_bg_color(*out_slider, c(THEME_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*out_slider, c(THEME_COLOR_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(*out_slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_add_event_cb(*out_slider, on_change, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(*out_slider, on_release, LV_EVENT_RELEASED, nullptr);
    return r;
}

lv_obj_t* menu_item(lv_obj_t* menu, lv_obj_t* parent, const char* t, lv_obj_t* target_page) {
    lv_obj_t* cont = lv_menu_cont_create(parent);
    style_as_button(cont);
    lv_obj_set_style_margin_bottom(cont, 10, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t* l = lv_label_create(cont);
    lv_label_set_text(l, t);
    lv_obj_t* ch = lv_label_create(cont);
    lv_label_set_text(ch, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(ch, c(THEME_COLOR_SECONDARY), 0);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_menu_set_load_page_event(menu, cont, target_page);
    return cont;
}

bool sw_on(lv_obj_t* sw) { return lv_obj_has_state(sw, LV_STATE_CHECKED); }
void sw_set(lv_obj_t* sw, bool on) {
    if (!sw) return;
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED); else lv_obj_clear_state(sw, LV_STATE_CHECKED);
}

// ---- event handlers -------------------------------------------------------
void on_recipe(lv_event_t* e) {
    grind_sync.select_recipe(static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e))));
}
void on_start_shot(lv_event_t*) { grind_sync.request_brew_start(); }
void on_jog(lv_event_t* e) {
    const int tenth = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    dial_motor.jog_setting(tenth / 10.0f);
}
void on_raw(lv_event_t* e) {
    dial_motor.jog_raw(static_cast<int32_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e))));
}
void on_stop(lv_event_t*) { dial_motor.stop(); }
void on_home(lv_event_t*) { dial_motor.request_home(); }
void on_trust(lv_event_t*) { dial_motor.trust_position(); }
void on_save_recipe(lv_event_t*) { grind_sync.remember_current_setting_for_profile(grind_sync.active_recipe()); }
void on_cal_begin(lv_event_t*) { dial_motor.begin_calibration(); }
void on_cal_zero(lv_event_t*) { dial_motor.set_zero_here(); }
void on_cal_coarse(lv_event_t*) { dial_motor.set_coarse_here(); }
void on_cal_cancel(lv_event_t*) { dial_motor.cancel_calibration(); }
void on_ap_on(lv_event_t*) { grind_sync.start_setup_portal(); }
void on_ap_off(lv_event_t*) { grind_sync.stop_setup_portal(); }
void on_reconnect(lv_event_t*) { grind_sync.request_reconnect(); }

}  // namespace

// Member-accessing handlers
static void on_cal_finish(lv_event_t*);
static void on_toggle(lv_event_t*);
static void on_setup_slider_change(lv_event_t*);
static void on_setup_slider_release(lv_event_t*);
static void on_margin_change(lv_event_t*);
static void on_margin_release(lv_event_t*) {}

// ============================================================================
void GrindSyncUI::create(lv_obj_t* menu, lv_obj_t* main_page) {
    menu_ = menu;
    lv_obj_t* sync_page = lv_menu_page_create(menu, "Grind Sync");
    lv_obj_t* dial_page = lv_menu_page_create(menu, "Dial");
    lv_obj_t* cal_page = lv_menu_page_create(menu, "Dial Calibration");
    lv_obj_t* setup_page = lv_menu_page_create(menu, "Sync Setup");
    lv_obj_t* net_page = lv_menu_page_create(menu, "Network");
    create_sync_page(sync_page);
    create_dial_page(dial_page);
    create_cal_page(cal_page);
    create_setup_page(setup_page);
    create_net_page(net_page);

    // Entries in the main menu (inserted where MenuScreen calls us)
    lv_obj_t* sep = lv_label_create(main_page);
    lv_label_set_text(sep, "Grind Sync");
    lv_obj_set_style_text_color(sep, c(THEME_COLOR_ACCENT), 0);
    lv_obj_set_style_margin_top(sep, 10, 0);
    lv_obj_set_style_margin_bottom(sep, 10, 0);
    menu_item(menu, main_page, "Grind Sync", sync_page);
    menu_item(menu, main_page, "Dial", dial_page);
    menu_item(menu, main_page, "Calibrate Dial", cal_page);
    menu_item(menu, main_page, "Sync Setup", setup_page);
    menu_item(menu, main_page, "Network", net_page);

    // cross links on the sync page
    menu_item(menu, sync_page, "Dial", dial_page);
    menu_item(menu, sync_page, "Sync Setup", setup_page);
    menu_item(menu, sync_page, "Network", net_page);
    menu_item(menu, dial_page, "Calibrate Dial", cal_page);

    lv_timer_create(&GrindSyncUI::refresh_cb, 300, this);
}

void GrindSyncUI::create_sync_page(lv_obj_t* page) {
    page_layout(page);
    // Gauge 0..100
    lv_obj_t* holder = lv_obj_create(page);
    lv_obj_set_size(holder, 240, 220);
    lv_obj_set_style_bg_opa(holder, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(holder, 0, 0);
    lv_obj_clear_flag(holder, LV_OBJ_FLAG_SCROLLABLE);
    arc_ = lv_arc_create(holder);
    lv_obj_set_size(arc_, 220, 220);
    lv_obj_center(arc_);
    lv_arc_set_rotation(arc_, 135);
    lv_arc_set_bg_angles(arc_, 0, 270);
    lv_arc_set_range(arc_, 0, 1000);
    lv_arc_set_value(arc_, 0);
    lv_obj_remove_style(arc_, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(arc_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_, 18, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_, 18, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_, c(0x2A2A30), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_, c(THEME_COLOR_PRIMARY), LV_PART_INDICATOR);
    arc_value_ = lv_label_create(holder);
    lv_obj_set_style_text_font(arc_value_, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(arc_value_, c(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_label_set_text(arc_value_, "--");
    lv_obj_align(arc_value_, LV_ALIGN_CENTER, 0, -8);
    arc_sub_ = lv_label_create(holder);
    lv_obj_set_style_text_font(arc_sub_, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(arc_sub_, c(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_label_set_text(arc_sub_, "GRIND");
    lv_obj_align(arc_sub_, LV_ALIGN_CENTER, 0, 36);
    lv_obj_t* fine = lv_label_create(holder);
    lv_label_set_text(fine, "fine");
    lv_obj_set_style_text_font(fine, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(fine, c(THEME_COLOR_NEUTRAL), 0);
    lv_obj_align(fine, LV_ALIGN_BOTTOM_LEFT, 22, -6);
    lv_obj_t* coarse = lv_label_create(holder);
    lv_label_set_text(coarse, "coarse");
    lv_obj_set_style_text_font(coarse, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(coarse, c(THEME_COLOR_NEUTRAL), 0);
    lv_obj_align(coarse, LV_ALIGN_BOTTOM_RIGHT, -12, -6);

    progress_ = lv_bar_create(page);
    lv_obj_set_size(progress_, 240, 10);
    lv_bar_set_range(progress_, 0, 100);
    lv_obj_set_style_bg_color(progress_, c(0x2A2A30), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_, c(THEME_COLOR_ACCENT), LV_PART_INDICATOR);
    lv_obj_add_flag(progress_, LV_OBJ_FLAG_HIDDEN);
    progress_label_ = text(page, "", &lv_font_montserrat_16);
    lv_obj_add_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);

    conn_label_ = text(page, "Sync off", &lv_font_montserrat_16);

    lv_obj_t* card = lv_obj_create(page);
    lv_obj_set_size(card, 260, LV_SIZE_CONTENT);
    style_card(card);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    shot_label_ = lv_label_create(card);
    lv_obj_set_width(shot_label_, 236);
    lv_label_set_long_mode(shot_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(shot_label_, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(shot_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(shot_label_, "No shot yet");

    separator(page, "Recipe");
    lv_obj_t* r = row(page);
    for (int i = 0; i < SYNC_RECIPE_SLOTS && i < 6; i++) {
        recipe_btns_[i] = button(r, "", on_recipe, reinterpret_cast<void*>(static_cast<intptr_t>(i)), 125, 74,
                                 0x26262C, &recipe_lbls_[i]);
        lv_obj_set_style_text_font(recipe_lbls_[i], &lv_font_montserrat_16, 0);
    }

    button(page, LV_SYMBOL_PLAY "  START SHOT", on_start_shot, nullptr, 260, 80, THEME_COLOR_PRIMARY);
    toggle_row(page, "Sync", &sw_enabled_, on_toggle);
    toggle_row(page, "Auto adjust", &sw_auto_, on_toggle);
}

void GrindSyncUI::create_dial_page(lv_obj_t* page) {
    page_layout(page);
    dial_value_ = text(page, "--", &lv_font_montserrat_48, THEME_COLOR_TEXT_PRIMARY);
    dial_info_ = text(page, "", &lv_font_montserrat_16);
    lv_obj_t* r1 = row(page);
    button(r1, "-1", on_jog, reinterpret_cast<void*>(static_cast<intptr_t>(-10)), 125, 70);
    button(r1, "+1", on_jog, reinterpret_cast<void*>(static_cast<intptr_t>(10)), 125, 70);
    button(r1, "-0.1", on_jog, reinterpret_cast<void*>(static_cast<intptr_t>(-1)), 125, 70);
    button(r1, "+0.1", on_jog, reinterpret_cast<void*>(static_cast<intptr_t>(1)), 125, 70);
    text(page, "-  finer          coarser  +", &lv_font_montserrat_16, THEME_COLOR_NEUTRAL);
    button(page, LV_SYMBOL_STOP "  STOP", on_stop, nullptr, 260, 70, THEME_COLOR_PRIMARY);
    button(page, "Save for recipe", on_save_recipe, nullptr, 260, 70, 0x26262C, &save_recipe_btn_label_);
    button(page, "Position is correct", on_trust, nullptr, 260, 70);
    button(page, "Home (coarse end)", on_home, nullptr, 260, 70);
}

void GrindSyncUI::create_cal_page(lv_obj_t* page) {
    page_layout(page);
    cal_step_ = text(page, "", &lv_font_montserrat_24, THEME_COLOR_TEXT_PRIMARY);
    cal_pos_ = text(page, "", &lv_font_montserrat_16);
    button(page, "Start wizard", on_cal_begin, nullptr, 260, 70, THEME_COLOR_ACCENT);
    lv_obj_t* r = row(page);
    static const int32_t jogs[] = {-800, 800, -100, 100, -10, 10};
    static const char* jog_txt[] = {LV_SYMBOL_LEFT LV_SYMBOL_LEFT " 800", "800 " LV_SYMBOL_RIGHT LV_SYMBOL_RIGHT,
                                    LV_SYMBOL_LEFT " 100", "100 " LV_SYMBOL_RIGHT, LV_SYMBOL_LEFT " 10", "10 " LV_SYMBOL_RIGHT};
    for (int i = 0; i < 6; i++) {
        button(r, jog_txt[i], on_raw, reinterpret_cast<void*>(static_cast<intptr_t>(jogs[i])), 125, 66);
    }
    text(page, LV_SYMBOL_LEFT " finer       coarser " LV_SYMBOL_RIGHT, &lv_font_montserrat_16, THEME_COLOR_NEUTRAL);
    button(page, LV_SYMBOL_STOP "  STOP", on_stop, nullptr, 260, 66, THEME_COLOR_PRIMARY);
    button(page, "Set point 0", on_cal_zero, nullptr, 260, 70);
    button(page, "Set point 100", on_cal_coarse, nullptr, 260, 70);
    slider_row(page, &cal_margin_label_, &cal_margin_slider_, 0, 30, on_margin_change, on_margin_release);
    lv_slider_set_value(cal_margin_slider_, static_cast<int32_t>(DIAL_DEFAULT_SAFETY_MARGIN), LV_ANIM_OFF);
    lv_label_set_text_fmt(cal_margin_label_, "Safety margin: %d", static_cast<int>(DIAL_DEFAULT_SAFETY_MARGIN));
    button(page, LV_SYMBOL_OK "  Finish", on_cal_finish, nullptr, 260, 70, THEME_COLOR_SUCCESS);
    button(page, "Cancel", on_cal_cancel, nullptr, 260, 66);
}

void GrindSyncUI::create_setup_page(lv_obj_t* page) {
    page_layout(page);
    setup_recipe_label_ = text(page, "Recipe", &lv_font_montserrat_24, THEME_COLOR_TEXT_PRIMARY);
    toggle_row(page, "GbS recipe", &sw_recipe_gbs_, on_toggle);
    slider_row(page, &tgt_label_, &tgt_slider_, 150, 450, on_setup_slider_change, on_setup_slider_release);  // 0.1 s
    separator(page, "Regulation");
    slider_row(page, &step_label_, &step_slider_, 1, 50, on_setup_slider_change, on_setup_slider_release);   // 0.1 units
    slider_row(page, &dead_label_, &dead_slider_, 0, 30, on_setup_slider_change, on_setup_slider_release);   // 0.1 s
    slider_row(page, &ratio_label_, &ratio_slider_, 10, 40, on_setup_slider_change, on_setup_slider_release); // 0.1
    separator(page, "Behaviour");
    toggle_row(page, "Follow recipe", &sw_follow_, on_toggle);
    toggle_row(page, "Send recipe", &sw_send_, on_toggle);
    toggle_row(page, "Brew after", &sw_brew_after_, on_toggle);
    text(page, "Follow: move dial on recipe change. Send: report recipe + target weight to GaggiMate. "
               "Brew after: start the shot when grinding ends.", &lv_font_montserrat_16);
}

void GrindSyncUI::create_net_page(lv_obj_t* page) {
    page_layout(page);
    net_label_ = text(page, "", &lv_font_montserrat_24, THEME_COLOR_TEXT_PRIMARY);
    ap_label_ = text(page, "", &lv_font_montserrat_24, THEME_COLOR_ACCENT);
    button(page, "Setup hotspot", on_ap_on, nullptr, 260, 70, THEME_COLOR_ACCENT);
    button(page, "Hotspot off", on_ap_off, nullptr, 260, 70);
    button(page, "Reconnect", on_reconnect, nullptr, 260, 70);
    text(page, "Configure WiFi, GaggiMate, recipes and motor in the browser: connect to the hotspot "
               "(or your WiFi) and open the address shown above.", &lv_font_montserrat_16);
}

void GrindSyncUI::attach_ready_chip(lv_obj_t* ready_screen) {
    chip_ = lv_obj_create(ready_screen);
    lv_obj_set_size(chip_, LV_SIZE_CONTENT, 34);
    lv_obj_align(chip_, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_radius(chip_, 17, 0);
    lv_obj_set_style_bg_color(chip_, c(0x18181C), 0);
    lv_obj_set_style_border_color(chip_, c(0x2A2A30), 0);
    lv_obj_set_style_border_width(chip_, 1, 0);
    lv_obj_set_style_pad_hor(chip_, 14, 0);
    lv_obj_set_style_pad_ver(chip_, 4, 0);
    lv_obj_clear_flag(chip_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(chip_, LV_OBJ_FLAG_CLICKABLE);
    chip_label_ = lv_label_create(chip_);
    lv_obj_set_style_text_font(chip_label_, &lv_font_montserrat_16, 0);
    lv_obj_center(chip_label_);
    lv_label_set_text(chip_label_, "");
    lv_obj_add_flag(chip_, LV_OBJ_FLAG_HIDDEN);
}

// ============================================================================
static void on_toggle(lv_event_t*) { grind_sync_ui.apply_from_widgets(); }

static void on_setup_slider_change(lv_event_t*) {
    // labels are refreshed in refresh(); values are applied on release
}

static void on_setup_slider_release(lv_event_t*) { grind_sync_ui.apply_from_widgets(); }

void GrindSyncUI::apply_from_widgets() {
    GrindSync::Config cfg = grind_sync.config();
    const int r = grind_sync.active_recipe();
    if (sw_enabled_) cfg.enabled = sw_on(sw_enabled_);
    if (sw_auto_) cfg.auto_adjust = sw_on(sw_auto_);
    if (sw_follow_) cfg.follow_profile = sw_on(sw_follow_);
    if (sw_send_) cfg.send_recipe = sw_on(sw_send_);
    if (sw_brew_after_) cfg.start_brew_after_grind = sw_on(sw_brew_after_);
    if (r >= 0 && r < SYNC_RECIPE_SLOTS && setup_loaded_recipe_ == r) {
        if (sw_recipe_gbs_) cfg.recipe[r].gbs = sw_on(sw_recipe_gbs_);
        if (tgt_slider_) cfg.recipe[r].target_time_s = lv_slider_get_value(tgt_slider_) / 10.0f;
    }
    if (step_slider_) cfg.reg.max_step = lv_slider_get_value(step_slider_) / 10.0f;
    if (dead_slider_) cfg.reg.deadband_s = lv_slider_get_value(dead_slider_) / 10.0f;
    if (ratio_slider_) cfg.ratio = lv_slider_get_value(ratio_slider_) / 10.0f;
    grind_sync.set_config(cfg);
}

static void on_margin_change(lv_event_t* e) {
    lv_obj_t* s = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_obj_t* parent = lv_obj_get_parent(s);
    lv_obj_t* label = lv_obj_get_child(parent, 0);
    lv_label_set_text_fmt(label, "Safety margin: %d", static_cast<int>(lv_slider_get_value(s)));
}

static void on_cal_finish(lv_event_t* e) {
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    // the margin slider lives in the row created just before the Finish button
    lv_obj_t* page = lv_obj_get_parent(btn);
    const int32_t idx = lv_obj_get_index(btn);
    lv_obj_t* slider_row = lv_obj_get_child(page, idx - 1);
    lv_obj_t* slider = lv_obj_get_child(slider_row, 1);
    dial_motor.finish_calibration(static_cast<float>(lv_slider_get_value(slider)));
}

void GrindSyncUI::refresh_cb(lv_timer_t* t) {
    static_cast<GrindSyncUI*>(lv_timer_get_user_data(t))->refresh();
}

void GrindSyncUI::refresh() {
    const DialMotor::Status d = dial_motor.status();
    const GrindSync::Status s = grind_sync.status();
    const GrindSync::Config cfg = grind_sync.config();
    const int ar = s.active_recipe;
    char buf[160];

    // gauge
    if (arc_) {
        const bool ok = d.calibrated && !isnan(d.setting);
        lv_arc_set_value(arc_, ok ? static_cast<int32_t>(d.setting * 10) : 0);
        lv_obj_set_style_arc_color(arc_, c(!ok ? THEME_COLOR_NEUTRAL : (d.position_trusted ? THEME_COLOR_PRIMARY : THEME_COLOR_WARNING)),
                                   LV_PART_INDICATOR);
        if (ok) snprintf(buf, sizeof(buf), "%.1f", d.setting); else snprintf(buf, sizeof(buf), "--");
        lv_label_set_text(arc_value_, buf);
        const char* sub = !d.calibrated ? "CALIBRATE" : (!d.position_trusted ? "CHECK POS" : (d.moving ? "MOVING" : cfg.recipe[ar].name));
        lv_label_set_text(arc_sub_, sub);
    }
    if (progress_) {
        if (d.moving) {
            lv_obj_clear_flag(progress_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(progress_, d.progress_pct, LV_ANIM_OFF);
            snprintf(buf, sizeof(buf), "to %.1f  -  %u%%  -  %.1f s", isnan(d.target_setting) ? 0.0f : d.target_setting,
                     d.progress_pct, d.eta_ms / 1000.0f);
            lv_label_set_text(progress_label_, buf);
        } else if (s.portion_total > 1 && s.portion_done > 0 && s.portion_done < s.portion_total) {
            // multi-portion recipe in progress (e.g. filter 3x)
            lv_obj_clear_flag(progress_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
            lv_bar_set_value(progress_, s.portion_done * 100 / s.portion_total, LV_ANIM_OFF);
            snprintf(buf, sizeof(buf), "Portion %u/%u  -  grind again", s.portion_done, s.portion_total);
            lv_label_set_text(progress_label_, buf);
        } else {
            lv_obj_add_flag(progress_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(progress_label_, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (conn_label_) {
        uint32_t col = THEME_COLOR_NEUTRAL;
        if (!cfg.enabled) snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI "  Sync off");
        else if (s.portal_active && !s.wifi_connected) { snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI "  Setup hotspot on"); col = THEME_COLOR_ACCENT; }
        else if (!s.wifi_connected) { snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI "  No WiFi"); col = THEME_COLOR_WARNING; }
        else if (!s.machine_seen) { snprintf(buf, sizeof(buf), LV_SYMBOL_WARNING "  GaggiMate not found"); col = THEME_COLOR_WARNING; }
        else if (s.machine_state == gbs::MachineState::Brewing) {
            snprintf(buf, sizeof(buf), LV_SYMBOL_PLAY "  Brewing %.1f s", s.live_extraction_ms / 1000.0f);
            col = THEME_COLOR_ACCENT;
        } else {
            snprintf(buf, sizeof(buf), LV_SYMBOL_OK "  GaggiMate %s%s", gbs::state_name(s.machine_state),
                     s.shot_pending_grind ? " - waiting" : "");
            col = THEME_COLOR_SUCCESS;
        }
        lv_label_set_text(conn_label_, buf);
        lv_obj_set_style_text_color(conn_label_, c(col), 0);
    }
    if (shot_label_) {
        if (s.has_last_shot) {
            const float target = (s.last_profile >= 0 && s.last_profile < SYNC_RECIPE_SLOTS)
                                     ? cfg.recipe[s.last_profile].target_time_s : cfg.recipe[ar].target_time_s;
            const char* sign = s.last_delta > 0 ? "+" : "";
            if (s.last_delta != 0.0f)
                snprintf(buf, sizeof(buf), "%.1f s / %.0f s\n%s%.1f  (%s)", s.last_time_s, target, sign, s.last_delta,
                         gbs::verdict_name(s.last_verdict));
            else
                snprintf(buf, sizeof(buf), "%.1f s / %.0f s\n%s", s.last_time_s, target, gbs::verdict_name(s.last_verdict));
            lv_label_set_text(shot_label_, buf);
            const float err = fabsf(s.last_time_s - target);
            lv_obj_set_style_text_color(shot_label_, c(err <= cfg.reg.deadband_s ? THEME_COLOR_SUCCESS
                                                       : (err <= 4 ? THEME_COLOR_WARNING : THEME_COLOR_ERROR)), 0);
        } else {
            lv_label_set_text(shot_label_, "No shot yet");
            lv_obj_set_style_text_color(shot_label_, c(THEME_COLOR_TEXT_SECONDARY), 0);
        }
    }
    for (int i = 0; i < SYNC_RECIPE_SLOTS && i < 6; i++) {
        if (!recipe_btns_[i]) continue;
        const float v = !isnan(s.profile_setting[i]) ? s.profile_setting[i] : cfg.recipe[i].start_setting;
        if (isnan(v)) snprintf(buf, sizeof(buf), "%s\n--", cfg.recipe[i].name);
        else snprintf(buf, sizeof(buf), "%s\n%.1f", cfg.recipe[i].name, v);
        lv_label_set_text(recipe_lbls_[i], buf);
        lv_obj_set_style_bg_color(recipe_btns_[i], c(i == ar ? THEME_COLOR_PRIMARY : 0x26262C), 0);
    }
    if (!toggles_loaded_) {
        sw_set(sw_enabled_, cfg.enabled);
        sw_set(sw_auto_, cfg.auto_adjust);
        sw_set(sw_follow_, cfg.follow_profile);
        sw_set(sw_send_, cfg.send_recipe);
        sw_set(sw_brew_after_, cfg.start_brew_after_grind);
        if (step_slider_) lv_slider_set_value(step_slider_, static_cast<int32_t>(cfg.reg.max_step * 10), LV_ANIM_OFF);
        if (dead_slider_) lv_slider_set_value(dead_slider_, static_cast<int32_t>(cfg.reg.deadband_s * 10), LV_ANIM_OFF);
        if (ratio_slider_) lv_slider_set_value(ratio_slider_, static_cast<int32_t>(cfg.ratio * 10), LV_ANIM_OFF);
        if (cal_margin_slider_ && d.calibrated) {
            lv_slider_set_value(cal_margin_slider_, static_cast<int32_t>(d.min_setting), LV_ANIM_OFF);
            lv_label_set_text_fmt(cal_margin_label_, "Safety margin: %d", static_cast<int>(d.min_setting));
        }
        toggles_loaded_ = true;
    }
    if (setup_loaded_recipe_ != ar && tgt_slider_) {
        lv_slider_set_value(tgt_slider_, static_cast<int32_t>(cfg.recipe[ar].target_time_s * 10), LV_ANIM_OFF);
        sw_set(sw_recipe_gbs_, cfg.recipe[ar].gbs);
        setup_loaded_recipe_ = ar;
    }
    if (setup_recipe_label_) {
        snprintf(buf, sizeof(buf), "Recipe %d: %s", ar + 1, cfg.recipe[ar].name);
        lv_label_set_text(setup_recipe_label_, buf);
    }
    if (tgt_label_) lv_label_set_text_fmt(tgt_label_, "Target time: %d.%d s", (int)lv_slider_get_value(tgt_slider_) / 10,
                                          (int)lv_slider_get_value(tgt_slider_) % 10);
    if (step_label_) lv_label_set_text_fmt(step_label_, "Max step/shot: %d.%d", (int)lv_slider_get_value(step_slider_) / 10,
                                           (int)lv_slider_get_value(step_slider_) % 10);
    if (dead_label_) lv_label_set_text_fmt(dead_label_, "Deadband: +-%d.%d s", (int)lv_slider_get_value(dead_slider_) / 10,
                                           (int)lv_slider_get_value(dead_slider_) % 10);
    if (ratio_label_) lv_label_set_text_fmt(ratio_label_, "Ratio: 1:%d.%d", (int)lv_slider_get_value(ratio_slider_) / 10,
                                            (int)lv_slider_get_value(ratio_slider_) % 10);

    // dial page
    if (dial_value_) {
        if (d.calibrated && !isnan(d.setting)) snprintf(buf, sizeof(buf), "%.1f", d.setting);
        else snprintf(buf, sizeof(buf), "--");
        lv_label_set_text(dial_value_, buf);
        snprintf(buf, sizeof(buf), "%s%s%s%s", !d.calibrated ? "Not calibrated. " : "",
                 d.calibrated && !d.position_trusted ? "Position unsure - confirm or home. " : "",
                 d.stalled ? "Motor stalled! " : "", d.last_error ? d.last_error : "");
        lv_label_set_text(dial_info_, buf);
        lv_obj_set_style_text_color(dial_info_, c((d.stalled || d.slip_detected || d.brownout_during_move) ? THEME_COLOR_ERROR
                                                                                                             : THEME_COLOR_TEXT_SECONDARY), 0);
    }
    if (save_recipe_btn_label_) {
        snprintf(buf, sizeof(buf), "Save for %s", cfg.recipe[ar].name);
        lv_label_set_text(save_recipe_btn_label_, buf);
    }
    // calibration wizard
    if (cal_step_) {
        const char* step;
        if (!d.calibrating) step = d.calibrated ? "Calibrated. Start the wizard to redo it." : "1. Grinder OFF. Tap Start wizard.";
        else if (!d.cal_has_zero) step = "2. Jog finer until the burrs just touch, then Set point 0.";
        else if (!d.cal_has_coarse) step = "3. Jog coarser to the coarsest setting, then Set point 100.";
        else step = "4. Choose the safety margin and tap Finish.";
        lv_label_set_text(cal_step_, step);
        snprintf(buf, sizeof(buf), "Motor position: %ld steps%s", static_cast<long>(d.position_steps), d.moving ? " (moving)" : "");
        lv_label_set_text(cal_pos_, buf);
    }
    // network
    if (net_label_) {
        if (s.wifi_connected)
            snprintf(buf, sizeof(buf), "WiFi: %s\nhttp://eurekamate.local\nGaggiMate: %s", s.ip,
                     s.machine_seen ? (s.machine_ip[0] ? s.machine_ip : "ok") : cfg.host);
        else snprintf(buf, sizeof(buf), "%s", cfg.enabled ? "WiFi not connected" : "Sync is off (Grind Sync page)");
        lv_label_set_text(net_label_, buf);
        if (s.portal_active) snprintf(buf, sizeof(buf), "Hotspot: %s\nPassword: %s\nhttp://192.168.4.1", s.portal_ssid, s.portal_pass);
        else buf[0] = 0;
        lv_label_set_text(ap_label_, buf);
    }
    // ready chip
    if (chip_) {
        if (!cfg.enabled && !d.calibrated) {
            lv_obj_add_flag(chip_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(chip_, LV_OBJ_FLAG_HIDDEN);
            const char* dot = !cfg.enabled ? "" : (s.machine_seen ? LV_SYMBOL_OK " " : LV_SYMBOL_WARNING " ");
            if (d.calibrated && !isnan(d.setting))
                snprintf(buf, sizeof(buf), "%s%s  %.1f%s", dot, cfg.recipe[ar].name, d.setting, d.moving ? " ..." : "");
            else snprintf(buf, sizeof(buf), "%sGrind Sync", dot);
            lv_label_set_text(chip_label_, buf);
            lv_obj_set_style_text_color(chip_label_, c(s.machine_seen ? THEME_COLOR_TEXT_PRIMARY : THEME_COLOR_TEXT_SECONDARY), 0);
        }
    }
}

// ---------------------------------------------------------------------------
// EurekaMate boot splash (top layer, removes itself)
// ---------------------------------------------------------------------------
void GrindSyncUI::show_splash(uint32_t duration_ms) {
    lv_obj_t* s = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s);
    lv_obj_set_size(s, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s, LV_OBJ_FLAG_CLICKABLE);  // swallow touches while visible

    lv_obj_t* ring = lv_arc_create(s);
    lv_obj_set_size(ring, 190, 190);
    lv_arc_set_bg_angles(ring, 0, 360);
    lv_arc_set_value(ring, 72);
    lv_obj_remove_style(ring, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ring, lv_color_hex(0x26262C), LV_PART_MAIN);
    lv_obj_set_style_arc_color(ring, lv_color_hex(THEME_COLOR_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ring, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ring, 10, LV_PART_INDICATOR);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, -60);

    lv_obj_t* title = lv_label_create(s);
    lv_label_set_text(title, "Eureka\nMate");
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(title, ring, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* sub = lv_label_create(s);
    lv_label_set_text(sub, "Grind-by-Sync for GaggiMate");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(THEME_COLOR_ACCENT), 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 70);

    lv_obj_t* credit = lv_label_create(s);
    lv_label_set_text(credit, "based on smart-grind-by-weight\nby jaapp  -  GPLv3\nv" BUILD_FIRMWARE_VERSION);
    lv_obj_set_style_text_align(credit, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(credit, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(credit, lv_color_hex(0x8A8A94), 0);
    lv_obj_align(credit, LV_ALIGN_BOTTOM_MID, 0, -40);

    lv_timer_t* t = lv_timer_create([](lv_timer_t* tm) {
        lv_obj_t* obj = static_cast<lv_obj_t*>(lv_timer_get_user_data(tm));
        lv_obj_delete(obj);
        lv_timer_delete(tm);
    }, duration_ms, s);
    lv_timer_set_repeat_count(t, 1);
}
