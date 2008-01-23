module main;

reg  a, c;
wire b = c;

always @*
  begin : foo
    reg x;
    a = x | b | bar.a;
  end

foobar bar();

initial begin
`ifdef DUMP
        $dumpfile( "slist3.2.vcd" );
        $dumpvars( 0, main );
`endif
	c = 1'b0;
	#10;
	c = 1'b1;
        #10;
        $finish;
end

endmodule

//------------------------------------

module foobar;

reg a;

endmodule
