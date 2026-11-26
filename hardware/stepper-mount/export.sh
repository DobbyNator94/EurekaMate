#!/usr/bin/env bash
# Export all printable parts (STL) and PNG previews. Needs openscad (+ xvfb-run for PNGs on headless boxes).
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p stl preview
X=""; command -v xvfb-run >/dev/null && X="xvfb-run -a"
stl() { echo "STL  $3"; openscad -q -o "stl/$3.stl" -D "part=\"$2\"" "scad/$1.scad"; }
png() { echo "PNG  $3"; $X openscad -q --render -o "preview/$3.png" --imgsize=1000,800 --autocenter --viewall --colorscheme=Tomorrow ${4:-} -D "part=\"$2\"" "scad/$1.scad"; }
stl pinion        pinion     pinion
stl pinion        magnet_cap magnet_cap
stl motor_housing housing    motor_housing
stl motor_housing lid        housing_lid
stl side_brace    -          side_brace
stl strap         segment    strap_segment
stl strap         slider     strap_tension_slider
png assembly      -          assembly        "--camera=0,0,0,55,0,35,0"
png assembly      -          assembly_front  "--camera=0,0,0,80,0,0,0"
png pinion        pinion     pinion          "--camera=0,0,0,50,0,30,0"
png motor_housing housing    motor_housing   "--camera=0,0,0,50,0,30,0"
png motor_housing lid        housing_lid     "--camera=0,0,0,50,0,210,0"
png side_brace    -          side_brace      "--camera=0,0,0,60,0,240,0"
png strap         all        strap           "--camera=0,0,0,40,0,20,0"
