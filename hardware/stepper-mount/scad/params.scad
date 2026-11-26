// =====================================================================
// EurekaMate - motor mount parameters (all dimensions in mm)
// Target: Eureka Mignon Specialita + aftermarket gear-knob kit
//         (big ring gear on the grind dial + small aluminium knob gear)
//
// Values marked TODO-MEASURE are best guesses from public data and MUST
// be checked with a caliper on your grinder/kit before printing.
// See ../README.md for how to measure.
// =====================================================================

$fn = 64;

// ---------------- Grinder body (Eureka datasheet: W 120 x D 140 x H 350) --------
body_w        = 120;   // width  (Eureka spec sheet / product page)
body_d        = 140;   // depth of the metal body (spec sheet; old manual says 180 incl. fork)  TODO-MEASURE
body_r        = 18;    // vertical corner radius of the body                                     TODO-MEASURE
body_top_z    = 205;   // height of the top surface (where the dial sits) above the table       TODO-MEASURE
clearance     = 0.6;   // fit clearance against the body (per side)

// ---------------- Gear-knob kit ------------------------------------------------
// Kit small gear (the aluminium knob that meshes with the big ring gear)
kit_gear_module   = 1.0;   // gear module of the kit                                             TODO-MEASURE
kit_small_teeth   = 20;    // teeth on the small aluminium knob gear                             TODO-MEASURE (count!)
kit_ring_teeth    = 66;    // teeth on the big ring gear on the dial                              TODO-MEASURE (count!)
kit_knob_od       = kit_gear_module * (kit_small_teeth + 2); // tip diameter of the small gear
kit_knob_height   = 12;    // tooth face width of the small gear                                  TODO-MEASURE
// Position of the small knob gear axis relative to the RIGHT body side and the FRONT face
knob_from_right   = 22;    // distance axis -> right side wall (inwards)                         TODO-MEASURE
knob_from_front   = 30;    // distance axis -> front face (backwards)                             TODO-MEASURE
knob_top_above_body = 14;  // top of the small knob above the body top surface                    TODO-MEASURE

// ---------------- Drive pinion on the stepper ----------------------------------
pinion_teeth      = 12;    // fewer teeth = more reduction/torque, slower travel (min ~10)
pinion_module     = kit_gear_module;
pressure_angle    = 20;
pinion_face       = 10;    // tooth face width
pinion_hub_d      = 12;
pinion_hub_h      = 6;
shaft_d           = 5.0;   // NEMA17 shaft
shaft_flat        = 0.5;   // D-flat depth (standard 0.5 mm)
shaft_clear       = 0.15;  // print tolerance on the bore
setscrew_d        = 3.2;   // M3 grub screw (radial)
nut_w             = 5.6;   // M3 nut trap across flats
backlash          = 0.15;  // tooth thinning per flank

// Centre distance pinion <-> small kit gear (standard gearing)
center_distance   = pinion_module * (pinion_teeth + kit_small_teeth) / 2;

// ---------------- Stepper motor ------------------------------------------------
// Default: 17HS4401 (42.3 x 42.3 x 40 mm, ~0.45 Nm). Pancake: motor_len = 20..25.
motor_w           = 42.3;
motor_len         = 40;
motor_boss_d      = 22.2;  // centring boss
motor_boss_h      = 2.0;
motor_hole_pitch  = 31.0;  // M3 hole spacing
motor_shaft_len   = 24;    // shaft length beyond the face

// ---------------- Electronics bay ----------------------------------------------
tmc_w = 15.5; tmc_l = 20.5; tmc_h = 22;    // TMC2209 module on pin headers incl. heatsink (BTT/FYSETC style)
buck_w = 17.5; buck_l = 22.5; buck_h = 6;   // MP1584 (22x17); Mini560 (~20x15) also fits
cap_d = 10.5;  cap_h = 17;                  // 100-470 uF / 25 V electrolytic (upright)
as5600_w = 23.5; as5600_l = 23.5;           // common AS5600 breakout (magnet 6x2.5 mm in the knob)
wall = 2.4;
floor_t = 3.0;
lid_t = 2.4;
lid_screw_d = 2.6;  // M3 self-tapping into PETG (or 3.2 + heat insert 4.2)
vent_w = 1.6;

// Mesh adjustment: slot length along the pinion->knob line
mesh_slot_len = 6;   // +-3 mm

// ---------------- Side brace + strap -------------------------------------------
brace_h        = 70;    // height of the plate on the right side
brace_t        = 4;     // plate thickness
brace_lip      = 8;     // hook lip over the top edge (slides on from below, then hooks)
brace_depth    = 90;    // front-back length of the plate
strap_w        = 12;    // strap width
strap_t        = 1.6;   // strap thickness (TPU 95A recommended, PETG works with 1.2 mm)
strap_z        = 35;    // strap height on the brace (from top edge, downwards)
strap_segments = 4;     // loop split into printable segments

// Housing floor height above the body top (pinion must clear the body; AS5600 gap)
floor_gap       = 4;      // floor underside sits this far above the knob top
// Optional AS5600 sits on the PINION AXIS: it reads a diametric magnet on the REAR shaft
// of a dual-shaft NEMA17 (e.g. "17HS4401 dual shaft"). Single-shaft motor -> as5600 = false.
as5600         = true;
rear_shaft_len = 10;      // rear shaft stick-out of a dual-shaft motor          TODO-MEASURE
magnet_d = 6; magnet_h = 2.5;  // diametric magnet 6x2.5 mm (often shipped with AS5600 boards)
as5600_gap     = 1.5;     // chip to magnet (datasheet: 0.5..3 mm)
as5600_chip_h  = 1.5;
as5600_pcb_t   = 1.6;

// ---------------- Derived housing geometry (shared by housing, brace, assembly) --
rear_stack = as5600 ? rear_shaft_len + magnet_h + as5600_gap + as5600_chip_h + as5600_pcb_t + 1.5 : 4;
inner_h   = motor_len + rear_stack;
// height of the AS5600 PCB underside above the floor top (chip hangs below it)
as5600_pcb_z = motor_len + rear_shaft_len + magnet_h + as5600_gap + as5600_chip_h;
bay_w     = max(tmc_w, buck_w, cap_d) + 2;
bay_l     = tmc_l + buck_l + cap_d + 8;
motor_box = motor_w + 1.0;
knob_y    = wall + kit_knob_od / 2 + 2;            // knob axis in housing coords
motor_y   = knob_y + center_distance;               // motor axis in housing coords
motor_x   = wall + motor_box / 2;
box_w     = motor_box + bay_w + 3 * wall;
box_l     = max(motor_y + motor_box / 2 + wall, bay_l + 2 * wall);
box_h     = floor_t + inner_h;
foot_len  = 10;
foot_slots = [[-foot_len / 2 - 1, motor_y], [box_w + foot_len / 2 + 1, motor_y]];

// World placement: body top = z 0, front face = y 0, right side = x +body_w/2
knob_world  = [body_w / 2 - knob_from_right, knob_from_front];
floor_z     = knob_top_above_body + floor_gap;      // housing underside
housing_org = [knob_world[0] - motor_x, knob_world[1] - knob_y, floor_z];
