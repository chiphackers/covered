module main;

reg [0:3][4:1] mem[2:0][0:6];

initial begin
	mem[0][0][0][1] = 1'b0;
	mem[0][0][0][1] = 1'b1;
end

initial begin
        $dumpfile( "mem3.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
