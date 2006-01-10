module main;

reg a, b, c, d, e, f;

always @*
  begin
   a = b | ~c;
   d = a ? e : f;
  end

initial begin
	b = 1'b0;
	c = 1'b0;
	e = 1'b0;
	f = 1'b1;
	#5;
	c = 1'b1;
end

initial begin
        $dumpfile( "slist1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
