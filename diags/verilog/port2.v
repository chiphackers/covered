module main;

wire a;

foo bar ( a );

initial begin
`ifdef DUMP
        $dumpfile( "port2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule


module foo (
  output reg a = (1'b0 ^ 1'b1)
);

endmodule
