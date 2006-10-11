module main;

logic                a = 0;
logic [1:0]          b = 0;
logic signed         c = 0;
logic signed [3:0]   d = 0;
logic unsigned       e = 0;
logic unsigned [5:0] f = 0;

initial begin
`ifndef VPI
        $dumpfile( "logic1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
