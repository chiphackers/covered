module main;

reg a;

initial begin : foo
	reg b;
	begin : bar
	 reg c;
	 a = 1'b1;
	 b = 1'b1;
	 c = 1'b0;
	end
end

initial begin
`ifdef DUMP
        $dumpfile( "nested_block1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
