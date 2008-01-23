module main;

reg b;

foo a[3:0] ();

initial b = a[1].b[2].c;

initial begin
`ifdef DUMP
        $dumpfile( "hier2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule

//----------------------------------------

module foo;

bar b[3:0] ();

endmodule

//----------------------------------------

module bar;

reg c;

endmodule
