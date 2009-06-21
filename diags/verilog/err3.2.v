/*
 Name:     err3.2.v
 Author:   Trevor Williams  (phase1geo@gmail.com)
 Date:     06/20/2009
 Purpose:  Verifies the the proper error message is specified if we
           have given a bad -i option value.
*/

module main;

foo a( 1'b0 );

initial begin
	$dumpfile( "err3.2.vcd" );
	$dumpvars( 0, main );
	#100;
	$finish;
end

endmodule

//-------------------------------

module foo(
  input b
);

reg a;

initial begin
	a = b;
	#10;
	a = ~b;
end

endmodule
