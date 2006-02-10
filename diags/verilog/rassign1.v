module main;

parameter ONE  = 1,
          ZERO = 0;

reg a = ONE & ZERO;
reg b;

initial begin
	b = a;
	a = 1'b1;
end

initial begin
        $dumpfile( "rassign1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
