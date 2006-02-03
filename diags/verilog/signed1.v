module main;

reg [3:0] a;
reg       b;

integer c;

initial begin
	a = 4'h0;
	b = a[2'sh0];
end

initial begin
	c = -20 + 10;
end

initial begin
        $dumpfile( "signed1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
