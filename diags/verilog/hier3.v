module main;

foo a();

initial begin
`ifndef VPI
        $dumpfile( "hier3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

//---------------------------

module foo;

bar a();
bar b();
bar c();

test t();

endmodule

//---------------------------

module bar;

reg y;

endmodule

//---------------------------

module test;

initial begin
	b.y = 1'b1;
	#5;
	b.y = 1'b0;
end

boo a();

endmodule

//---------------------------------

module boo;

reg y;

endmodule
