// Simple involute spur gear (own implementation, no external library)
// m = module, z = teeth, pa = pressure angle, bl = backlash per flank (mm)
function inv(a) = tan(a) - a * PI / 180;
function polar(r, a) = [r * cos(a), r * sin(a)];
function inv_pt(rb, t) = // t in degrees of roll
    let(tr = t * PI / 180) [rb * (cos(t) + tr * sin(t)), rb * (sin(t) - tr * cos(t))];

module involute_gear_2d(m = 1, z = 12, pa = 20, bl = 0.1) {
    rp = m * z / 2;
    rb = rp * cos(pa);
    ra = rp + m;
    rf = max(rp - 1.25 * m, 0.5);
    // half tooth angle at pitch circle, thinned by backlash
    half = 90 / z - (bl / rp) * 180 / PI;
    a_pitch = (tan(pa) - pa * PI / 180) * 180 / PI;  // involute angle at pitch radius
    tmax = sqrt(ra * ra / (rb * rb) - 1) * 180 / PI;
    steps = 8;
    union() {
        circle(r = rf, $fn = z * 4);
        for (i = [0 : z - 1]) rotate(i * 360 / z) {
            flank = [for (k = [0 : steps]) let(t = tmax * k / steps, p = inv_pt(rb, t)) p];
            // rotate the flank so that the tooth is symmetric about the x axis
            rot = half + a_pitch;
            right = [for (p = flank) [p[0] * cos(-rot) - p[1] * sin(-rot), p[0] * sin(-rot) + p[1] * cos(-rot)]];
            left = [for (k = [steps : -1 : 0]) let(p = right[k]) [p[0], -p[1]]];
            base = rb > rf ? [polar(rf, -rot - 2), polar(rf, rot + 2)] : [];
            polygon(concat([polar(rf * 0.98, -rot - 4)], right, left, [polar(rf * 0.98, rot + 4)]));
        }
    }
}

module spur_gear(m = 1, z = 12, h = 8, pa = 20, bl = 0.1) {
    linear_extrude(height = h) involute_gear_2d(m, z, pa, bl);
}
