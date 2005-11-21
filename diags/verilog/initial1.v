module main;

reg a;

initial begin
	a = 1'b0;
	#10;
	a = 1'b1;
end

initial begin
        $dumpfile( "initial1.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

endmodule
