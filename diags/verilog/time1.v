/*
 Name:        time1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/30/2008
 Purpose:     Verifies the ability to properly handle the $time system call.
*/

module main;

time curr_time;
reg  a;

always @(curr_time)
  if( curr_time < 12 )
    a = 1'b0;
  else
    a = 1'b1;

initial begin
	curr_time = $time;
	#6;
	curr_time = $time;
	#5;
	curr_time = $time;
	#2;
	curr_time = $time;
end

initial begin
`ifdef DUMP
        $dumpfile( "time1.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
