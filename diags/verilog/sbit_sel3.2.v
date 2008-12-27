/*
 Name:        sbit_sel3.2.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        03/29/2008
 Purpose:     Verify that an automatic function call used on the LHS works properly.
 Simulators:  IV CVER VERIWELL VCS
 Modes:       VCD LXT VPI
*/

module main;

reg [31:0] a, b;

initial begin
	a = 0;
	b = 2;
	#5;
	a[foo(b)] = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "sbit_sel3.2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

// Recursive way to add 1 to the given value
function automatic [31:0] foo;
  input [31:0] x;
  begin
    if( x == 0 )
      foo = 1;
    else
      foo = foo( x - 1 ) + 1;
  end
endfunction

endmodule
