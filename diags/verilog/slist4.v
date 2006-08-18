module main;

reg  a, b, reset;

always @*
  a = b | reset;

always @*
  begin
   b = 1'b0;
   b = a;
  end

initial begin
        $dumpfile( "slist4.vcd" );
        $dumpvars( 0, main );
	reset = 1'b1;
	#10;
	reset = 1'b0;
        #10;
        $finish;
end

endmodule
