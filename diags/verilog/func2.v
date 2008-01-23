module main;

reg  b;
wire a = invert( b );

initial begin
`ifdef DUMP
        $dumpfile( "func2.vcd" );
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
  invert = ~buffer( a );
end
endfunction

function buffer;
input c;
begin
  buffer = c;
end
endfunction

endmodule
