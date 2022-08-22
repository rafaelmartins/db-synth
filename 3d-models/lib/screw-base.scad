/*
 * SPDX-FileCopyrightText: 2022 Rafael G. Martins <rafael@rafaelmartins.eng.br>
 * SPDX-License-Identifier: CERN-OHL-S-2.0
 */

function screw_base_dim(screw_d) = screw_d + 2;
function screw_base_height(screw_h) = screw_h + 1;


module screw_base_add(screw_d, screw_h) {
    cube([screw_base_dim(screw_d), screw_base_dim(screw_d), screw_base_height(screw_h)]);
}


module screw_base_sub(screw_d, screw_h) {
    translate([screw_base_dim(screw_d) / 2, screw_base_dim(screw_d) / 2, 0])
        cylinder(screw_h, d=screw_d - 0.2, $fn=20);
}

