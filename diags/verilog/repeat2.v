module main;

reg        a, b;
reg [31:0] c;

initial begin
	a = 1'b0;
	#10;
	repeat( c )
	  begin
	   a = ~a;
           @(b);
          end
	a = ~a;
end

initial begin
`ifndef VPI
        $dumpfile( "repeat2.vcd" );
        $dumpvars( 0, main );
`endif
        b = 1'b0;
	c = 10;
        #10;
	b = 1'b1;
	#10;
	b = 1'b0;
	#10;
	b = 1'b1;
	#10;
        $finish;
end

endmodule
