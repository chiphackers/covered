/*
 Name:        if1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/24/2008
 Purpose:     Verifies that if a system command is used within an IF statement which is
              embedded in a block that also contains a system command, that the IF statement
              block is not output to the CDD file (which can cause errors because the parent
              functional unit will not be displayed to the CDD file).
*/

module main;

integer a, b;

initial begin
        a = $rtoi( 0.1 );
	begin
          b = a;
          if( a == 0 ) begin
            a = $rtoi( 0.1 );
          end
        end
end

initial begin
`ifdef DUMP
        $dumpfile( "if1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
