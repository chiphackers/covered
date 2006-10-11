module main;

reg b, c, d;

wire a = b & c;

initial begin
        b = 1'b0;
        c = 1'b1;
	#10;
	b = 1'b1;
end

final begin
      d = ~a;
end

initial begin
`ifndef VPI
        $dumpfile( "final1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
