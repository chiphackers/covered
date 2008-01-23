module main;

reg       a;
reg [3:0] b, c;

always @(a)
  begin
   b = (4'h1 <<< 2);
   c = (4'h2 >>> 3);
  end

initial begin
`ifdef DUMP
        $dumpfile( "ashift2.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
