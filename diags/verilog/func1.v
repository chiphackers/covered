module main;

reg  b;
wire a = invert( b );

initial begin
`ifndef VPI
        $dumpfile( "func1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
        #10;
	b = 1'b1;
	#10;
        $finish;
end

function invert;

input a;

begin
  invert = ~a;
end

endfunction

endmodule
