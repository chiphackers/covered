/*
 Name:     report1.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     11/03/2006
 Purpose:  Verifies that the report command properly handles
           expressions that stress the memory allocation calculations.
*/

module main;

wire [1:0] a;
reg        b, c;
reg  [1:0] d;

assign a = ({
           b,
           ~c
           } &
           d
           );

initial begin
`ifdef DUMP
        $dumpfile( "report1.vcd" );
        $dumpvars( 0, main );
`endif
        b = 1'b0;
        c = 1'b1;
        d = 2'h0;
        #10;
        d = 2'h3;
        #10;
        $finish;
end

endmodule
