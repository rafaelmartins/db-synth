/*
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: CERN-OHL-S-2.0
 */

include <lib/audiojack.scad>
include <lib/screw-base.scad>
include <lib/ssd1306.scad>
include <settings.scad>


difference() {
    union() {
        cube([width, length, thickness]);
        cube([thickness, length, height]);
        translate([width - thickness, 0, 0])
            cube([thickness, length, height]);

        for (i=[0:1])
            for (j=[0:1])
                translate([pcb_screw_padding + pcb0_x + i * pcb_screw_spacing_x,
                           pcb_screw_padding + pcb0_y + j * pcb_screw_spacing_y, thickness])
                    difference() {
                        cylinder(pcb_base_height, d=pcb_base_d, $fn=20);
                        cylinder(pcb_base_height, d=pcb_base_screw_d, $fn=20);
                    }

        translate([oled0_x, oled0_y, 0])
            ssd1306_add(thickness);

        translate([0, pcb0_y + pcb_length - 2 * pcb_screw_padding - 5, thickness + pcb_base_height / 2])
            rotate([0, 90, 0])
                audiojack_add(thickness);
        translate([width, pcb0_y + pcb_length - 2 * pcb_screw_padding - 5, thickness + pcb_base_height / 2])
            rotate([0, -90, 0])
                audiojack_add(thickness);
    }

    translate([oled0_x, oled0_y, 0])
        ssd1306_sub(thickness);

    translate([0, pcb0_y + pcb_length - 2 * pcb_screw_padding - 5, thickness + pcb_base_height / 2])
        rotate([0, 90, 0])
            audiojack_sub(thickness);
    translate([width, pcb0_y + pcb_length - 2 * pcb_screw_padding - 5, thickness + pcb_base_height / 2])
        rotate([0, -90, 0])
            audiojack_sub(thickness);
    translate([width - thickness, pcb0_y + 2 * pcb_screw_padding + 4, thickness + pcb_base_height / 2])
        rotate([0, 90, 0])
            cylinder(h=thickness, d=8.5, $fn=20);

    for (i=[0:1])
        for (j=[0:1])
            for (k=[0:1])
                translate([i * (width - thickness),
                           screw_base_dim(screw_d) / 2 + j * (length - screw_base_dim(screw_d)),
                           thickness + gap + screw_base_dim(screw_d) / 2 + k * hole_distance_z])
                    rotate([0, 90, 0])
                        cylinder(thickness, d=screw_d * 1.1, $fn=20);
}

