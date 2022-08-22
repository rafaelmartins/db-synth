/*
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: CERN-OHL-S-2.0
 */

ssd1306_pcb_thickness = 1.6;
ssd1306_pcb_base_height = 2;
ssd1306_pcb_base_d = 4;
ssd1306_pcb_base_screw_d = 1.8;
ssd1306_pcb_base_spacing_x = 23.5;
ssd1306_pcb_base_spacing_y = 23.8;
ssd1306_screen_width = 25.5;
ssd1306_screen_height = 14.5;
ssd1306_screen_distance_x = (ssd1306_pcb_base_spacing_x - ssd1306_screen_width) / 2;
ssd1306_screen_distance_y = 2.3;
ssd1306_pin_distance_y = 0.45;


module ssd1306_add(thickness) {
    translate([-ssd1306_pcb_base_d / 2, -ssd1306_pcb_base_d / 2, 0])
        cube([ssd1306_pcb_base_spacing_x + ssd1306_pcb_base_d,
              ssd1306_pcb_base_spacing_y + ssd1306_pcb_base_d,
              thickness]);

    for(i=[0:1])
        for(j=[0:1])
            translate([i * ssd1306_pcb_base_spacing_x, j * ssd1306_pcb_base_spacing_y, thickness])
                cylinder(ssd1306_pcb_base_height, d=ssd1306_pcb_base_d, $fn=20);
}


module ssd1306_sub(thickness) {
    translate([ssd1306_screen_distance_x, ssd1306_screen_distance_y, 0])
        cube([ssd1306_screen_width, ssd1306_screen_height, thickness]);

    for(i=[0:1])
        for(j=[0:1])
            translate([i * ssd1306_pcb_base_spacing_x, j * ssd1306_pcb_base_spacing_y, thickness])
                cylinder(ssd1306_pcb_base_height, d=ssd1306_pcb_base_screw_d, $fn=20);
}

