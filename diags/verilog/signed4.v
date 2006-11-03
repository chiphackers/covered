/*
 Name:     signed4.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/03/2006
 Purpose:  
*/

module main;

foo f( -1 );

initial begin
`ifndef VPI
        $dumpfile( "signed4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

//-------------------------------------------

module foo (
  a
);

input wire signed [3:0] a;

wire b = a[3];

endmodule
