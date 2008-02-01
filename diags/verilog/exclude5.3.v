module main;

reg  b, d, e;
wire c;

wire a = b & c;

assign c = ~b; 

initial begin
	b = 1'b0;
	#10;
	b = 1'b1;
end

always @(posedge b) d <= ~a;

final begin
      e = 1'b0;
      e = 1'b1;
end

initial begin
`ifdef DUMP
        $dumpfile( "exclude5.3.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
