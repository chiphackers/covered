module main;

foo a();
foo b();

initial begin
`ifdef DUMP
        $dumpfile( "hier4.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

//---------------------------

module foo;

bar x[1:0]();

endmodule

//---------------------------

module bar;

reg z;

endmodule
