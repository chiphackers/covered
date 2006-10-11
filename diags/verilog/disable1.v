module main;

reg a;

initial begin
	 fork
           begin : foo
	     a = 1'b0;
             #20;
             a = 1'b1;
	   end
           begin
             #19;
             disable foo;
           end
         join
end

initial begin
`ifndef VPI
        $dumpfile( "disable1.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
