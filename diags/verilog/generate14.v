/*
 Name:     generate14.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     08/31/2007
 Purpose:  Verify that when the VCD dumpfile down/up-scopes more than one hierachy at a time
           the Covered correctly handles it.
*/

module main;

reg clock;

generate
  genvar i;
  for( i=0; i<4; i=i+1 )
    begin : U
     initial begin : foo
       reg [1:0] a;
       a <= 0;
       @(posedge clock) a <= i;
     end
    end
endgenerate

initial begin
`ifndef VPI
        $dumpfile( "generate14.vcd" );
        $dumpvars( 0, main );
`endif
	clock = 1'b0;
	#5;
	clock = 1'b1;
        #10;
        $finish;
end

endmodule
