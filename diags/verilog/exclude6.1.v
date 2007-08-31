/*
 Name:     exclude6.1.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     08/20/2007
 Purpose:  Verifies that functions can be excluded when there are multiple functions in the same module.
*/

module main;

reg a, b;

initial begin
	a = 1'b0;
	#5;
	a = foo_func1( 1'b0 );
end

initial begin
	b = 1'b0;
	#5;
	b = foo_func2( 1'b0 );
end

initial begin
`ifndef VPI
        $dumpfile( "exclude6.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function foo_func1;
  input  a;
  begin
    foo_func1 = a;
  end
endfunction

function foo_func2;
  input a;
  begin
    foo_func2 = ~a;
  end
endfunction

endmodule
