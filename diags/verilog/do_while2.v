module main;

reg [31:0] a;
reg [31:0] b;

always @(b)
  begin
   a = b;
   do
     #1 a = a + 1;
   while( a < (b + 10) );
  end

initial begin
`ifdef DUMP
        $dumpfile( "do_while2.vcd" );
        $dumpvars( 0, main );
`endif
	#1000;
	b = 0;
	#1000;
	b = 100;
	#1000;
        $finish;
end

endmodule
