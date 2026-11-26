// Side brace: slides on from BELOW along the RIGHT body side until the hook lip catches the
// top edge. Carries the motor housing (two M3 posts, mesh slots are in the housing feet)
// and is held by a closed strap running once around the body (no drilling).
// World coords: body top = z 0, front = y 0, right side = x +body_w/2.
include <params.scad>

side_x   = body_w / 2 + clearance;                   // inner face of the plate
// housing feet in world coords
foot_w   = [for (p = foot_slots) [housing_org[0] + p[0], housing_org[1] + p[1]]];
inner_ft = foot_w[0];
outer_ft = foot_w[1];
y0       = min(inner_ft[1] - 14, knob_world[1] - 10);   // plate front edge
plate_l  = max(brace_depth, outer_ft[1] - y0 + 14);
post_d   = 10;
arm_t    = 3;
// strap passes through two slots near the plate ends
strap_slot_z = -strap_z;

module rounded_rect_ring_profile() {}

module brace() {
    difference() {
        union() {
            // vertical plate on the right side
            translate([side_x, y0, -brace_h]) cube([brace_t, plate_l, brace_h + arm_t]);
            // top arm lying on the body top, from the side edge inwards to the inner foot
            hull() {
                translate([side_x - 1, y0, 0]) cube([brace_t + 1, plate_l, arm_t]);
                translate([inner_ft[0], inner_ft[1], 0]) cylinder(d = post_d + 6, h = arm_t);
            }
            // inner post (housing foot rests on it)
            translate([inner_ft[0], inner_ft[1], 0]) cylinder(d = post_d, h = floor_z);
            // outer shelf: from the plate outwards to the outer foot, with a gusset
            hull() {
                translate([side_x, outer_ft[1] - post_d / 2, floor_z - arm_t - 2]) cube([brace_t, post_d, arm_t + 2]);
                translate([outer_ft[0], outer_ft[1], floor_z - arm_t - 2]) cylinder(d = post_d, h = arm_t + 2);
            }
            hull() {   // gusset under the shelf down onto the plate
                translate([side_x, outer_ft[1] - 2, -25]) cube([brace_t, 4, 1]);
                translate([side_x, outer_ft[1] - 2, floor_z - 3]) cube([brace_t, 4, 1]);
                translate([outer_ft[0] - post_d / 2, outer_ft[1] - 2, floor_z - 3]) cube([1, 4, 1]);
            }
            hull() {   // plate up to the shelf height
                translate([side_x, outer_ft[1] - post_d / 2, 0]) cube([brace_t, post_d, 1]);
                translate([side_x, outer_ft[1] - post_d / 2, floor_z - 3]) cube([brace_t, post_d, 1]);
            }
            // tensioner rail at the rear end of the plate (see strap.scad)
            translate([side_x + brace_t, y0 + plate_l - 26, strap_slot_z - strap_w / 2 - 4]) cube([10, 26, strap_w + 8]);
        }
        // M3 holes in both posts: heat insert (m3_thread_d = 4.0) or self-tapping (2.6), see params.scad
        for (f = foot_w) translate([f[0], f[1], -brace_h]) cylinder(d = m3_thread_d, h = brace_h + floor_z + 5);
        // front strap slot: the strap end loops through it
        translate([side_x - 1, y0 + 4, strap_slot_z - strap_w / 2 - 0.5]) cube([brace_t + 2, 3, strap_w + 1]);
        // rear tensioner channel: slider runs in y, pulled by an M3 screw from the rear end
        translate([side_x + brace_t - 0.01, y0 + plate_l - 24, strap_slot_z - strap_w / 2 - 1]) cube([6.5, 22, strap_w + 2]);
        translate([side_x + brace_t + 3.2, y0 + plate_l - 3, strap_slot_z]) rotate([-90, 0, 0]) cylinder(d = 3.4, h = 10);
        // window so the strap can enter the channel from the body side
        translate([side_x - 1, y0 + plate_l - 24, strap_slot_z - strap_w / 2 - 1]) cube([brace_t + 2, 12, strap_w + 2]);
        // weight-saving / cooling cut-outs in the plate
        translate([side_x - 1, y0 + 14, -brace_h + 8]) cube([brace_t + 2, plate_l - 44, max(brace_h - strap_z - 20, 1)]);
    }
}

brace();
