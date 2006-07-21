module main;

parameter mod = 1;

generate
  if( mod < 1 )
    foo a();
  else
    bar a();
endgenerate

initial begin
        $dumpfile( "generate3.1.vcd" );
        $dumpvars( 0, main );
        #20;
        $finish;
end

initial begin
	a.x = 1'b0;
	#10;
	a.x = 1'b1;
end

endmodule

//--------------------------------

module foo;

reg x;

endmodule

//--------------------------------

module bar;
 
reg x;

endmodule 
