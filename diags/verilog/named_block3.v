module main;

reg a, b;

initial begin
	#5;
	case( a )
          1'b0 :
            begin : foo
             b = 1'b0;
            end
          1'b1 :
            begin : bar
             b = 1'b1;
            end
          default : b = 1'bx;
        endcase
end

initial begin
	a = 1'b0;
end

initial begin
`ifndef VPI
        $dumpfile( "named_block3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
