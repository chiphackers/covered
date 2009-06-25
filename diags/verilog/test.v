/*
 Name:        generate18.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        05/20/2009
 Purpose:     
*/

module main;

reg RST0;
reg CLK0;
reg CE0;
wire [21:0] P, R;
reg [21:0] Q;
reg A, B;

always @(posedge RST0 or posedge CLK0)
  if (RST0)
    Q <= {22{1'b0}};
  else
  if (CE0)
    Q <= { {11{1'b0}}, A} * { {11{1'b0}}, B};
assign P = Q;
assign R = P;

initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
