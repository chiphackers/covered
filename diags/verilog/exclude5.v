module main;

reg  b, d;
wire c;

wire a = b & c;

assign c = ~b; 

initial begin
	b = 1'b0;
	#10;
	b = 1'b1;
end

always @(posedge b) d <= ~a;

initial begin
`ifndef VPI
        $dumpfile( "exclude5.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
