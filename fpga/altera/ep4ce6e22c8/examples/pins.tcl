#set_global_assignment -name FAMILY "Cyclone IV E"
#set_global_assignment -name DEVICE EP4CE6E22C8
#set_global_assignment -name TOP_LEVEL_ENTITY top

set_location_assignment PIN_24 -to CLK
set_location_assignment PIN_88 -to nRST

set_location_assignment PIN_1  -to LEDS[0]
set_location_assignment PIN_2  -to LEDS[1]
set_location_assignment PIN_3  -to LEDS[2]
set_location_assignment PIN_7  -to LEDS[3]
set_location_assignment PIN_11 -to LEDS[4]

set_location_assignment PIN_73 -to BUTTONS[3]
set_location_assignment PIN_80 -to BUTTONS[2]
set_location_assignment PIN_89 -to BUTTONS[1]
set_location_assignment PIN_114 -to BUTTONS[0]

set_location_assignment PIN_10 -to SERIAL_TX
set_location_assignment PIN_23 -to SERIAL_RX

set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to LEDS[4]
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to LEDS[3]
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to LEDS[2]
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to LEDS[1]
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to LEDS[0]

set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to BUTTONS[3]
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to BUTTONS[2]
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to BUTTONS[1]
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to BUTTONS[0]

set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to nRST
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to CLK
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to SERIAL_TX
set_instance_assignment -name IO_STANDARD "3.3-V LVCMOS" -to SERIAL_RX
