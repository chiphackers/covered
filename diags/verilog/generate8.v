module main;

localparam A = 0;

generate
  if( A < 1 )
    begin
     foo f();
     initial begin
             f.a = 1'b0;
             #10;
             f.a = 1'b1;
     end
    end
endgenerate

initial begin
`ifndef VPI
        $dumpfile( "generate8.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule

//--------------------------------------

module foo;

reg a;

endmodule
