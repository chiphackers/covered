module main;

reg   a, b, c;

wire d = ~a;

always @(c)
  begin
   b = ~c;
   a = b;
  end

initial begin
`ifndef VPI
        $dumpfile( "bassign1.vcd" );
        $dumpvars( 0, main );
`endif
	c = 1'b1;
        #10;
	c = 1'b0;
	#10;
        $finish;
end

endmodule
