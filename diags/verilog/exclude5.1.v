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
`ifdef DUMP
        $dumpfile( "exclude5.1.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
