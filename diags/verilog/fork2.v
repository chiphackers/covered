module main;

reg b;

initial begin
	fork : foo_fork
	  reg a;
          begin
            a = 1'b0;
            #10;
            a = 1'b1;
          end
          begin
            b = 1'b0;
	    #20;
            b = 1'b1;
          end
        join
end

initial begin
`ifdef DUMP
        $dumpfile( "fork2.vcd" );
        $dumpvars( 0, main );
`endif
        #100;
        $finish;
end

endmodule
