#pragma once
// LVGL pages for Grind-by-Sync and the motorized dial, added to the existing
// settings menu (lv_menu) in the same visual style as the stock SGBW pages:
//   Grind Sync  - gauge 0..100, move progress/ETA, recipe selection, last shot, start shot
//   Dial        - jog +-0.1/+-1, stop, save to recipe, confirm position, homing
//   Calibrate   - wizard: point 0 (burrs touching) / point 100 (coarsest) / safety margin
//   Sync Setup  - target time per recipe, GbS per recipe, regulation, behaviour toggles
//   Network     - WiFi/GaggiMate status, web UI address, setup hotspot
// A small status chip is also shown on the ready screen.
#include <lvgl.h>

class GrindSyncUI {
public:
    void create(lv_obj_t* menu, lv_obj_t* main_page);
    void attach_ready_chip(lv_obj_t* ready_screen);
    void apply_from_widgets();  // push switch/slider state into GrindSync config

private:
    static void refresh_cb(lv_timer_t* t);
    void refresh();
    void create_sync_page(lv_obj_t* page);
    void create_dial_page(lv_obj_t* page);
    void create_cal_page(lv_obj_t* page);
    void create_setup_page(lv_obj_t* page);
    void create_net_page(lv_obj_t* page);

    lv_obj_t* menu_ = nullptr;
    // sync page
    lv_obj_t* arc_ = nullptr;
    lv_obj_t* arc_value_ = nullptr;
    lv_obj_t* arc_sub_ = nullptr;
    lv_obj_t* progress_ = nullptr;
    lv_obj_t* progress_label_ = nullptr;
    lv_obj_t* conn_label_ = nullptr;
    lv_obj_t* shot_label_ = nullptr;
    lv_obj_t* recipe_btns_[6] = {};
    lv_obj_t* recipe_lbls_[6] = {};
    lv_obj_t* sw_enabled_ = nullptr;
    lv_obj_t* sw_auto_ = nullptr;
    // dial page
    lv_obj_t* dial_value_ = nullptr;
    lv_obj_t* dial_info_ = nullptr;
    lv_obj_t* save_recipe_btn_label_ = nullptr;
    // cal page
    lv_obj_t* cal_step_ = nullptr;
    lv_obj_t* cal_pos_ = nullptr;
    lv_obj_t* cal_margin_slider_ = nullptr;
    lv_obj_t* cal_margin_label_ = nullptr;
    // setup page
    lv_obj_t* tgt_slider_ = nullptr;
    lv_obj_t* tgt_label_ = nullptr;
    lv_obj_t* sw_recipe_gbs_ = nullptr;
    lv_obj_t* step_slider_ = nullptr;
    lv_obj_t* step_label_ = nullptr;
    lv_obj_t* dead_slider_ = nullptr;
    lv_obj_t* dead_label_ = nullptr;
    lv_obj_t* ratio_slider_ = nullptr;
    lv_obj_t* ratio_label_ = nullptr;
    lv_obj_t* sw_follow_ = nullptr;
    lv_obj_t* sw_send_ = nullptr;
    lv_obj_t* sw_brew_after_ = nullptr;
    lv_obj_t* setup_recipe_label_ = nullptr;
    // net page
    lv_obj_t* net_label_ = nullptr;
    lv_obj_t* ap_label_ = nullptr;
    // ready chip
    lv_obj_t* chip_ = nullptr;
    lv_obj_t* chip_label_ = nullptr;

    int setup_loaded_recipe_ = -1;
    bool toggles_loaded_ = false;
};

extern GrindSyncUI grind_sync_ui;
