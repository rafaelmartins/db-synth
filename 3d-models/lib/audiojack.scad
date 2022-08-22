/*
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: CERN-OHL-S-2.0
 */

audiojack_d = 6.5;
audiojack_out_d = 8.5;
audiojack_dim = audiojack_out_d;


module audiojack_add(thickness) {
    translate([-audiojack_out_d / 2, -audiojack_out_d / 2, 0])
        cube([audiojack_out_d, audiojack_out_d, thickness]);
}


module audiojack_sub(thickness) {
    cylinder(thickness, d=audiojack_d, $fn=20);
    translate([0, 0, thickness - 0.6]) {
        difference() {
            cylinder(thickness, d=audiojack_out_d, $fn=20);
            translate([-audiojack_out_d / 2, audiojack_d / 2, 0])
                cube([audiojack_out_d, (audiojack_out_d - audiojack_d) / 2, 0.6]);
            translate([-audiojack_out_d / 2, -audiojack_out_d / 2, 0])
                cube([audiojack_out_d, (audiojack_out_d - audiojack_d) / 2, 0.6]);
        }
    }
}

