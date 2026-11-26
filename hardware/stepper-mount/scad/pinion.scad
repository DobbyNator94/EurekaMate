// Drive pinion for a NEMA17 5 mm D-shaft, meshing with the small gear of the kit.
include <params.scad>
use <gears.scad>

module d_bore(h) {
    r = shaft_d / 2 + shaft_clear;
    difference() {
        cylinder(r = r, h = h);
        translate([r - shaft_flat, -r, -1]) cube([r, 2 * r, h + 2]);
    }
}

module pinion() {
    difference() {
        union() {
            spur_gear(pinion_module, pinion_teeth, pinion_face, pressure_angle, backlash);
            // hub with grub screw on the motor side
            translate([0, 0, pinion_face]) cylinder(d = pinion_hub_d, h = pinion_hub_h);
        }
        translate([0, 0, -1]) d_bore(pinion_face + pinion_hub_h + 2);
        // radial M3 grub screw onto the D-flat + nut trap
        translate([0, 0, pinion_face + pinion_hub_h / 2]) rotate([0, 90, 0]) cylinder(d = setscrew_d, h = pinion_hub_d);
        translate([shaft_d / 2 + 1.0, -nut_w / 2, pinion_face + pinion_hub_h / 2 - 1.4])
            cube([2.6, nut_w, pinion_hub_h]);
        // chamfer the lead-in
        translate([0, 0, -0.01]) cylinder(r1 = shaft_d / 2 + 0.6, r2 = 0, h = 1.2);
    }
}

part = "pinion";  // "pinion" | "magnet_cap"
if (part == "pinion") pinion(); else magnet_cap();

// Magnet cap for the REAR shaft of a dual-shaft NEMA17 (optional AS5600)
module magnet_cap() {
    h = 6 + magnet_h;
    difference() {
        cylinder(d = 11, h = h);
        translate([0, 0, -1]) d_bore(7);
        translate([0, 0, h - magnet_h]) cylinder(d = magnet_d + 0.2, h = magnet_h + 1);
    }
}
