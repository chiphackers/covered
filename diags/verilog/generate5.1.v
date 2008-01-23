module main;

localparam off = 1;

reg [3:0] a;

generate
  if( off < 1 )
    initial begin
            a = 4'h0;
            #10;	
            a = 4'ha;
    end

endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate5.1.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
