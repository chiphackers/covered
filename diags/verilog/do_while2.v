module main;

reg [31:0] a;
reg [31:0] b;

always @(b)
  begin
   a = b;
   do
     a = a + 1;
   while( a < (b + 10) );
  end

initial begin
`ifdef DUMP
        $dumpfile( "do_while2.vcd" );
        $dumpvars( 0, main );
`endif
	#10;
	b = 0;
	#10;
	b = 100;
	#10;
        $finish;
end

endmodule
