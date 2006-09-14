module main;

reg a;

initial begin : foobar
         a = 1'b0;
	 #10;
	 a = 1'b1;
         #10;
         disable foobar;
         #10;
         a = 1'b0;
end

initial begin
        $dumpfile( "disable1.1.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

endmodule
