module main;

wire     a;
reg      b;

assign a = 1'b0 + 1'd0 + 1'h0 + 1'o0;

initial begin
`ifdef DUMP
        $dumpfile( "static1.vcd" );
        $dumpvars( 0, main );
`endif
        b = 1'b0;
        #10;
end

endmodule
