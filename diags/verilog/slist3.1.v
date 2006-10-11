module main;

reg a, b, c, d;

always @*
  fork : foo
    a = b;
    c = d;
  join

initial begin
`ifndef VPI
        $dumpfile( "slist3.1.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	d = 1'b1;
        #10;
	b = 1'b1;
	#10;
        $finish;
end

endmodule
