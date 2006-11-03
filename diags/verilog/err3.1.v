/*
 Name:     err3.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/03/2006
 Purpose:  Verifies the the proper error message is specified if we
           have given a bad -i option value.
*/

module main;

foo a( 1'b0 );

initial begin
	$dumpfile( "err3.1.vcd" );
	$dumpvars( 0, main );
	#100;
	$finish;
end

endmodule

//-------------------------------

module foo (
  input wire b
);

reg a;

initial begin
	a = b;
	#10;
	a = 1'b1;
end

endmodule
