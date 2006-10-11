module main;

reg a, b;

always @(b)
  invert_a( b );

initial begin
`ifndef VPI
        $dumpfile( "task1.1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
        #10;
	b = 1'b1;
	#10;
        $finish;
end

task invert_a;

input b;

begin
 a = ~b;
end

endtask

endmodule
