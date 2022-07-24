thickness = 1.6;

pcb_width = 41.402;
pcb_length = 36.322;
pcb_thickness = 1.6;
pcb_screw_padding = 3.556;
pcb_base_d = 5;
pcb_base_screw_d = 1.8;
pcb_oled_pin_y = 2.54;

width = 2 * (thickness + 3) + pcb_width;
length = 2 * thickness + pcb_length;
height = 30;

pcb0_x = (width - pcb_width) / 2;
pcb0_y = (length - pcb_length) / 2;

oled_pcb_thickness = 1.6;
oled_pcb_base_height = 2;
oled_pcb_base_d = 4;
oled_pcb_base_screw_d = 1.8;
oled_pcb_base_spacing_x = 23.5;
oled_pcb_base_spacing_y = 23.8;
oled0_x = (width - oled_pcb_base_spacing_x) / 2;
oled0_y = pcb0_y + pcb_oled_pin_y + 0.43;
oled_screen_width = 25.5;
oled_screen_height = 14.5;
oled_screen_distance_x = (oled_pcb_base_spacing_x - oled_screen_width) / 2;
oled_screen_distance_y = 2.3;

pcb_base_height = 10 + oled_pcb_thickness + oled_pcb_base_height;


module pcb_base(x, y) {
    translate([x, y, 0]) {
        difference() {
            cylinder(pcb_base_height, d=pcb_base_d, $fn=20);
            cylinder(pcb_base_height, d=pcb_base_screw_d, $fn=20);
        }
    }
}


module oled_base(x, y) {
    translate([x, y, 0]) {
        difference() {
            cylinder(oled_pcb_base_height, d=oled_pcb_base_d, $fn=20);
            cylinder(oled_pcb_base_height, d=oled_pcb_base_screw_d, $fn=20);
        }
    }
}


difference() {
    union() {
        cube([width, length, thickness]);
        cube([thickness, length, height]);
        translate([width - thickness, 0, 0])
            cube([thickness, length, height]);
        
        cube([4, 8, 4]);
        translate([0, length - 8, 0])
            cube([4, 8, 4]);
        translate([width - 4, 0, 0])
            cube([4, 8, 4]);
        translate([width - 4, length - 8, 0])
            cube([4, 8, 4]);
        
        translate([pcb0_x, pcb0_y, thickness]) {
            pcb_base(pcb_screw_padding, pcb_screw_padding);
            pcb_base(pcb_width - pcb_screw_padding, pcb_screw_padding);
            pcb_base(pcb_screw_padding, pcb_length - pcb_screw_padding);
            pcb_base(pcb_width - pcb_screw_padding, pcb_length - pcb_screw_padding); 
        }
        
        translate([oled0_x, oled0_y, thickness]) {
            oled_base(0, 0);
            oled_base(0, oled_pcb_base_spacing_y);
            oled_base(oled_pcb_base_spacing_x, 0);
            oled_base(oled_pcb_base_spacing_x, oled_pcb_base_spacing_y);
        }
    }
    
    translate([oled0_x, oled0_y, 0]) {
        translate([oled_screen_distance_x, oled_screen_distance_y, 0])
            cube([oled_screen_width, oled_screen_height, thickness]);
    }
    
    rotate([0, 90, 0]) {
        translate([-thickness - pcb_base_height / 2, pcb0_y + 2 * pcb_screw_padding + 4, width - thickness])
            cylinder(h=thickness, d=8.5, $fn=20);
        translate([-thickness - pcb_base_height / 2, pcb0_y + pcb_length - 2 * pcb_screw_padding - 5, 0])
            cylinder(h=width, d=6.5, $fn=20);
        
        translate([-height + 2, 2, 0])
            cylinder(h=width, d=2.2, $fn=20);
        translate([-height + 2, length - 2, 0])
            cylinder(h=width, d=2.2, $fn=20);
    }
    
    rotate([-90, 0, 0]) {
        translate([2, -2, 0])
            cylinder(7, d=1.8, $fn=20);
        translate([width - 2, -2, 0])
            cylinder(7, d=1.8, $fn=20);
        translate([2, -2, length - 6])
            cylinder(7, d=1.8, $fn=20);
        translate([width - 2, -2, length - 6])
            cylinder(7, d=1.8, $fn=20);
    }
}