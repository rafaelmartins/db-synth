/*
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: CERN-OHL-S-2.0
 */

include <lib/screw-base.scad>
include <settings.scad>

width_ = width - 2 * (thickness + gap);
height_ = height - thickness - gap;


difference() {
    union() {
        cube([width_, length, thickness]);
        cube([width_, thickness, height_]);
        translate([0, length - thickness, 0])
            cube([width_, thickness, height_]);

        translate([0, 0, 0])
            rotate([90, 0, 90])
                screw_base_add(screw_d, screw_h);
        translate([0, length, 0])
            rotate([90, -90, 90])
                screw_base_add(screw_d, screw_h);
        translate([width_, 0, 0])
            rotate([0, -90, 0])
                screw_base_add(screw_d, screw_h);
        translate([width_, length, 0])
            rotate([90, 0, -90])
                screw_base_add(screw_d, screw_h);

        translate([4, thickness / 2, height_])
            cube([width_ - 8, thickness / 2, thickness / 2]);
        translate([4, length - thickness, height_])
            cube([width_ - 8, thickness / 2, thickness / 2]);
    }

    translate([0, 0, 0])
        rotate([90, 0, 90])
            screw_base_sub(screw_d, screw_h);
    translate([0, length, 0])
        rotate([90, -90, 90])
            screw_base_sub(screw_d, screw_h);
    translate([width_, 0, 0])
        rotate([0, -90, 0])
            screw_base_sub(screw_d, screw_h);
    translate([width_, length, 0])
        rotate([90, 0, -90])
            screw_base_sub(screw_d, screw_h);
}
