`define FOO (a & \
	     b & \
             c)

module main;

reg a, b, c;

wire d = `FOO;

initial begin
        $dumpfile( "define5.vcd" );
        $dumpvars( 0, main );
	a = 1'b0;
	b = 1'b0;
	c = 1'b0;
        #10;
        $finish;
end

endmodule
