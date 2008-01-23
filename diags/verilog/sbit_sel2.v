/*
 Name:     sbit_sel2.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     01/23/2008
 Purpose:  Verify that Covered does the correct thing when a vector is
           indexed larger and smaller than its width.
*/

module main;

reg [4:1] a;
reg       b;
integer   i;

initial begin
        a = 4'h8;
	for( i=0; i<6; i=i+1 ) begin
          b = a[i];
          #5;
        end
end

initial begin
`ifndef VPI
        $dumpfile( "sbit_sel2.vcd" );
        $dumpvars( 0, main );
`endif
        #50;
        $finish;
end

endmodule
