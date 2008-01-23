module main;

parameter A = 1;

generate
  case( A )
    0 : foo f();
    1 : bar f();
    default : bozo f();
  endcase
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate7.1.vcd" );
        $dumpvars( 0, main );
`endif
	f.x = 1'b0;
        #10;
	f.x = 1'b1;
	#10;
        $finish;
end

endmodule

//---------------------------------------

module foo;

reg x;

endmodule

//---------------------------------------

module bar;

reg x;

endmodule

//---------------------------------------

module bozo;

reg x;

endmodule
