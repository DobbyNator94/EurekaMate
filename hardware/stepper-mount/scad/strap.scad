// Closed strap around the whole grinder body, split into printable segments.
// Print in TPU 95A (preferred) or PETG (strap_t 1.2 mm). Segments link with a T-hook into a
// slot. One end loops through the front slot of the brace, the other hooks into the
// tensioner slider (M3 screw) in the rear rail of the brace.
include <params.scad>

perimeter  = 2 * (body_w + 2 * clearance + 2 * strap_t) + 2 * (body_d + 2 * clearance + 2 * strap_t)
             - 8 * body_r + 2 * PI * body_r;
free_len   = perimeter - brace_depth + 20;          // what the segments must cover (+fold-back)
seg_len    = free_len / strap_segments;
hook_l     = 6;

module segment(first = false) {
    difference() {
        union() {
            cube([seg_len, strap_w, strap_t]);
            // T-hook at the +x end (hooks into the slot of the next segment)
            translate([seg_len, strap_w / 2 - 2.5, 0]) cube([hook_l, 5, strap_t]);
            translate([seg_len + hook_l - 2.5, 0.5, 0]) cube([2.5, strap_w - 1, strap_t * 1.6]);
        }
        // slot for the previous segment's hook
        translate([2.5, strap_w / 2 - 2.9, -1]) cube([2.8, 5.8, strap_t + 2]);
    }
}

module tension_slider() {
    // slides in the brace rail; strap hook catches the bar, M3 nut trap pulls it backwards
    difference() {
        cube([6, 18, strap_w]);
        translate([-1, 4, 1.5]) cube([8, 3.2, strap_w - 3]);          // strap slot
        translate([3, 10, strap_w / 2]) rotate([-90, 0, 0]) cylinder(d = 3.4, h = 10);
        translate([0.2, 11, strap_w / 2 - 2.8]) cube([5.6, 2.6, 5.6]); // M3 nut trap
    }
}

part = "segment";   // "segment" | "slider" | "all"
if (part == "segment") segment();
else if (part == "slider") tension_slider();
else {
    for (i = [0 : strap_segments - 1]) translate([0, i * (strap_w + 4), 0]) segment();
    translate([0, strap_segments * (strap_w + 4), 0]) tension_slider();
}
echo(str("strap perimeter = ", perimeter, " mm, segment length = ", seg_len, " mm"));
