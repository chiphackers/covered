/*
 Name:     disable3.v
 Author:   Trevor Williams  (trevorw@charter.net)
 Date:     11/20/2006
 Purpose:  Verifies that a disable statement that is not reached is handled
           properly by the report command.
*/

module main;

reg a;

initial begin
	fork
          begin : foo
           a = 1'b0;
           #5;
           a = 1'b1;
          end
          begin : bar
           #10;
           if( !a )
             disable foo;
          end
        join
end

initial begin
`ifndef VPI
        $dumpfile( "disable3.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
