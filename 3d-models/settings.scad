/*
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: CERN-OHL-S-2.0
 */

include <lib/screw-base.scad>
include <lib/ssd1306.scad>

thickness = 1.6;

pcb_width = 46.99;
pcb_length = 36.576;
pcb_thickness = 1.6;
pcb_screw_padding = 2.794;
pcb_screw_spacing_x = pcb_width - 2 * pcb_screw_padding;
pcb_screw_spacing_y = pcb_length - 2 * pcb_screw_padding;
pcb_base_d = 5;
pcb_base_screw_d = 1.8;
pcb_base_height = 10 + ssd1306_pcb_thickness + ssd1306_pcb_base_height;
pcb_oled_pin_y = 2.54;

jack_distance = 13;

width = 2 * (thickness + 1.5) + pcb_width;
length = 2 * thickness + 1.2 + pcb_length;
height = 30;

pcb0_x = (width - pcb_width) / 2;
pcb0_y = (length - pcb_length) / 2;

oled0_x = (width - ssd1306_pcb_base_spacing_x) / 2;
oled0_y = pcb0_y + pcb_oled_pin_y + ssd1306_pin_distance_y;

screw_d = 2;
screw_h = 7;

gap = thickness * 0.2;
