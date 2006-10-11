module main;

reg a, b, c;

always @*
  begin : foo
    a = b;
    begin : bar
      b = c;
    end
  end

initial begin
`ifndef VPI
        $dumpfile( "slist3.vcd" );
        $dumpvars( 0, main );
`endif
        c = 1'b0;
        #10;
        c = 1'b1;
        #10;
        $finish;
end

endmodule
