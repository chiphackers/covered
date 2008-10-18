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

integer a, b, c;

initial begin
        a = $fopen( "if1a.dat" );
	begin
          b = 0;
          if( a == 0 ) begin
            c = $fopen( "if1a.dat" );
            $fclose( c );
          end
        end
	$fclose( a );
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
