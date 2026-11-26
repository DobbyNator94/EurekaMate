// Assembly preview on a dummy Mignon body (not for printing).
include <params.scad>
use <motor_housing.scad>
use <side_brace.scad>
use <pinion.scad>
use <gears.scad>

module dummy_body() {
    color("Gainsboro", 0.6) translate([0, 0, -body_top_z + 0.01]) hull()
        for (x = [-body_w / 2 + body_r, body_w / 2 - body_r], y = [body_r, body_d - body_r])
            translate([x, y, 0]) cylinder(r = body_r, h = body_top_z - 0.02);
    // dial ring gear + kit knob gear (approximate)
    ring_r = kit_gear_module * kit_ring_teeth / 2;
    kd = kit_gear_module * (kit_ring_teeth + kit_small_teeth) / 2;
    color("DimGray") translate([knob_world[0] - kd, knob_world[1], 2]) spur_gear(kit_gear_module, kit_ring_teeth, 8, 20, 0.1);
    color("Silver") translate([knob_world[0], knob_world[1], knob_top_above_body - kit_knob_height])
        spur_gear(kit_gear_module, kit_small_teeth, kit_knob_height, 20, 0.1);
}

explode = 0;      // 0 = assembled, 1 = exploded view
show_lid = true;
show_brace = true;
show_strap = true;
show_housing = true;
e = explode;
bay_x0 = motor_box + 2 * wall;

dummy_body();
if (show_brace) color("SteelBlue") translate([45 * e, 0, 0]) brace();
if (show_housing) translate(housing_org + [45 * e, 0, 40 * e]) {
    color("Orange") housing();
    if (show_lid) color("Orange", 0.45) translate([0, 0, box_h + 0.5 + 90 * e]) lid();
    // motor
    color("DimGray") translate([motor_x - motor_w / 2, motor_y - motor_w / 2, floor_t + 45 * e]) cube([motor_w, motor_w, motor_len]);
    // electronics (visual): TMC2209 (red PCB + heatsink), buck (blue), capacitor (black)
    translate([0, 0, 30 * e]) {
        color("Crimson") translate([bay_x0 + 1, wall + 5, floor_t + 1]) cube([tmc_w, tmc_l, 3]);
        color("Silver") translate([bay_x0 + 3, wall + 7, floor_t + 4]) cube([tmc_w - 4, tmc_l - 4, 9]);
        color("RoyalBlue") translate([bay_x0 + 1, wall + tmc_l + 11, floor_t + 1.2]) cube([buck_w - 1, buck_l, 4]);
        color("Black") translate([bay_x0 + bay_w / 2, wall + tmc_l + buck_l + 16 + cap_d / 2, floor_t + 1]) cylinder(d = cap_d, h = cap_h);
    }
    // pinion under the floor
    color("Gold") translate([motor_x, motor_y, -pinion_face - pinion_hub_h - 25 * e]) pinion();
}
// strap loop (visual)
if (show_strap) color("DarkSlateGray") translate([0, 0, -strap_z - strap_w / 2]) linear_extrude(strap_w) difference() {
    offset(r = clearance + strap_t) hull() for (x = [-body_w / 2 + body_r, body_w / 2 - body_r], y = [body_r, body_d - body_r]) translate([x, y]) circle(r = body_r);
    offset(r = clearance) hull() for (x = [-body_w / 2 + body_r, body_w / 2 - body_r], y = [body_r, body_d - body_r]) translate([x, y]) circle(r = body_r);
}
