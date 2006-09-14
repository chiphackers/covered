module main;

reg	a;
reg	b, c, d;

always @(a, posedge b, negedge c) d <= ~d;

initial begin
	$dumpfile( "always5.1.vcd" );
	$dumpvars( 0, main );
        a = 1'b0;
	b = 1'b0;
	c = 1'b0;
	d = 1'b0;
        fork
          forever #(5) c = ~c;
          forever #(7) b = ~b;
          begin
           #22;
           a = 1'b1;
           #5;
           a = 1'b0;
          end
        join
end

initial begin
        #100;
        $finish;
end

endmodule
