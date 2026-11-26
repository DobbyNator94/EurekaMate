// Enclosed motor housing for the EurekaMate dial drive.
//  - NEMA17 stands on the floor, shaft DOWN through the floor; the pinion sits below the
//    floor and meshes with the small aluminium knob gear of the kit.
//  - Electronics bay next to the motor: TMC2209 module (heatsink towards the vent slots),
//    5 V buck (MP1584/Mini560), 100-470 uF bulk capacitor.
//  - Optional AS5600 on the pinion axis, held by the lid above the REAR shaft magnet.
//  - Cable exit through the floor towards the grinder bottom, zip-tie strain relief.
//  - Removable lid with 4x M3x8 screws (heat inserts in the bosses, see params.scad).
//  - Motor: 4x M3x6 from below (NEMA17 threads are only ~4.5 mm deep - M3x8 would bottom out).
//  - Feet with slots (along the pinion->knob line) for gear-mesh adjustment.
include <params.scad>

module rounded_box(w, l, h, r = 3) {
    hull() for (x = [r, w - r], y = [r, l - r]) translate([x, y, 0]) cylinder(r = r, h = h);
}

function lid_screws() = [[wall + 3.5, wall + 3.5], [box_w - wall - 3.5, wall + 3.5],
                         [wall + 3.5, box_l - wall - 3.5], [box_w - wall - 3.5, box_l - wall - 3.5]];

module slot(len, d, h) {
    hull() {
        translate([0, -len / 2, 0]) cylinder(d = d, h = h);
        translate([0, len / 2, 0]) cylinder(d = d, h = h);
    }
}

bay_x = motor_box + 2 * wall;   // x start of the electronics bay

module housing() {
    difference() {
        union() {
            rounded_box(box_w, box_l, box_h);
            // feet with mesh-adjustment slots
            for (p = foot_slots) hull() {
                translate([p[0] - foot_len / 2, p[1] - mesh_slot_len / 2 - 5, 0])
                    cube([foot_len, mesh_slot_len + 10, floor_t]);
                translate([p[0] < 0 ? 0 : box_w - 1, p[1] - mesh_slot_len / 2 - 8, 0])
                    cube([1, mesh_slot_len + 16, floor_t + 8]);
            }
        }
        // main cavity
        translate([wall, wall, floor_t]) rounded_box(box_w - 2 * wall, box_l - 2 * wall, box_h, 2);
        // motor: centring boss + 4x M3 from below (counterbored)
        translate([motor_x, motor_y, -1]) cylinder(d = motor_boss_d + 0.8, h = floor_t + 2);
        for (sx = [-1, 1], sy = [-1, 1])
            translate([motor_x + sx * motor_hole_pitch / 2, motor_y + sy * motor_hole_pitch / 2, -1]) {
                cylinder(d = 3.4, h = floor_t + 2);
                cylinder(d = 6.2, h = 1.6);
            }
        // lid screw holes
        for (p = lid_screws()) translate([p[0], p[1], box_h - 10]) cylinder(d = lid_screw_d, h = 11);
        // cable exit (towards the grinder bottom) + zip-tie slots for strain relief
        translate([box_w - wall - 7, box_l - wall - 7, -1]) cylinder(d = 7, h = floor_t + 2);
        for (dx = [-6, 6]) translate([box_w - wall - 7 + dx - 1, box_l - wall - 20, -1]) cube([2, 4, floor_t + 2]);
        // vent slots on the bay side wall (TMC2209 heatsink)
        for (i = [0 : 7]) translate([box_w - wall - 1, wall + 5 + i * 3.6, floor_t + 5])
            cube([wall + 2, vent_w, inner_h - 12]);
        // low vent slots on the front wall of the bay
        for (i = [0 : 2]) translate([bay_x + 3 + i * 4, -1, floor_t + 4]) cube([vent_w, wall + 2, 14]);
        // feet slots
        for (p = foot_slots) translate([p[0], p[1], -1]) slot(mesh_slot_len, 3.4, 20);
    }
    // TMC2209 cradle (module stands on its headers, heatsink towards the side vents)
    translate([bay_x, wall + 4, floor_t]) difference() {
        cube([bay_w, tmc_l + 2, 4]);
        translate([1, 1, 1]) cube([bay_w - 2, tmc_l, 4]);
    }
    // buck converter tray
    translate([bay_x, wall + tmc_l + 10, floor_t]) difference() {
        cube([bay_w, buck_l + 2, 3]);
        translate([1, 1, 1.2]) cube([bay_w - 2, buck_l, 3]);
    }
    // bulk capacitor ring
    translate([bay_x + bay_w / 2, wall + tmc_l + buck_l + 16 + cap_d / 2, floor_t]) difference() {
        cylinder(d = cap_d + 3, h = 6);
        translate([0, 0, 1]) cylinder(d = cap_d + 0.4, h = 7);
    }
    // lid screw bosses
    for (p = lid_screws()) translate([p[0], p[1], box_h - 10]) difference() {
        cylinder(d = lid_boss_d, h = 10);
        translate([0, 0, -1]) cylinder(d = lid_screw_d, h = 12);
    }
}

module lid() {
    difference() {
        union() {
            rounded_box(box_w, box_l, lid_t);
            // locating rim
            translate([wall + 0.3, wall + 0.3, -2]) difference() {
                rounded_box(box_w - 2 * wall - 0.6, box_l - 2 * wall - 0.6, 2, 2);
                translate([1.6, 1.6, -1]) rounded_box(box_w - 2 * wall - 3.8, box_l - 2 * wall - 3.8, 4, 1);
                for (p = lid_screws()) translate([p[0] - wall - 0.3, p[1] - wall - 0.3, -1]) cylinder(d = lid_boss_d + 1, h = 5);
            }
            // AS5600 holder above the rear shaft (pinion axis)
            if (as5600) {
                hang = box_h - floor_t - as5600_pcb_z - as5600_pcb_t;   // from lid underside to PCB top
                translate([motor_x - as5600_w / 2 - 1.6, motor_y - as5600_l / 2 - 1.6, -hang - as5600_pcb_t - 1])
                difference() {
                    cube([as5600_w + 3.2, as5600_l + 3.2, hang + as5600_pcb_t + 1]);
                    // PCB pocket (chip down) with a clip lip; inner opening for the chip
                    translate([1.6, 1.6, 1]) cube([as5600_w, as5600_l, as5600_pcb_t + 0.2]);
                    translate([4, 4, -1]) cube([as5600_w - 4.8, as5600_l - 4.8, hang + 5]);
                    translate([-1, 1.6 + 4, 1]) cube([as5600_w + 6, as5600_l - 8, as5600_pcb_t + 0.2]); // slide-in slot
                }
            }
        }
        for (p = lid_screws()) translate([p[0], p[1], -3]) cylinder(d = 3.4, h = 10);
        for (i = [0 : 5]) translate([bay_x + 2, wall + 5 + i * 3.6, -3]) cube([bay_w - 4, vent_w, 10]);
    }
}

part = "housing";  // "housing" | "lid"
if (part == "housing") housing();
else translate([0, box_l, lid_t]) rotate([180, 0, 0]) lid();
