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

dummy_body();
color("SteelBlue") brace();
translate(housing_org) {
    color("Orange") housing();
    color("Orange", 0.35) translate([0, 0, box_h + 0.5]) lid();
    // motor
    color("Black", 0.8) translate([motor_x - motor_w / 2, motor_y - motor_w / 2, floor_t]) cube([motor_w, motor_w, motor_len]);
    // pinion under the floor
    color("Gold") translate([motor_x, motor_y, -pinion_face - pinion_hub_h]) pinion();
}
// strap loop (visual)
color("DarkSlateGray") translate([0, 0, -strap_z - strap_w / 2]) linear_extrude(strap_w) difference() {
    offset(r = clearance + strap_t) hull() for (x = [-body_w / 2 + body_r, body_w / 2 - body_r], y = [body_r, body_d - body_r]) translate([x, y]) circle(r = body_r);
    offset(r = clearance) hull() for (x = [-body_w / 2 + body_r, body_w / 2 - body_r], y = [body_r, body_d - body_r]) translate([x, y]) circle(r = body_r);
}
