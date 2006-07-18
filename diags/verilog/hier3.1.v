module main;

foo a();

initial begin
        $dumpfile( "hier3.1.vcd" );
        $dumpvars( 0, main );
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
	a.y = 1'b1;
	#5;
	a.y = 1'b0;
end

boo a();

endmodule

//---------------------------------

module boo;

reg y;

endmodule
