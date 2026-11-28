module test (
    input  wire CLK,
    output wire [4:0] LEDS
);

reg [24:0] counter;
always @(posedge CLK) counter <= counter + 1;

assign LEDS[0] = counter[24];
assign LEDS[1] = counter[23];
assign LEDS[2] = counter[22];
assign LEDS[3] = counter[21];
assign LEDS[4] = counter[20];

endmodule
