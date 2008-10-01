/*
 Name:        time1.1.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/30/2008
 Purpose:     Verify that the same $time expression can properly evaluate if it used more than once.
*/

module main;

time    curr_time;
reg     a;
integer i;

always @(curr_time)
  if( curr_time[0] == 0 )
    a = 1'b0;
  else
    a = 1'b1;

initial begin
	for( i=0; i<2; i=i+1 ) begin
          curr_time = $time;
          #3;
        end
end

initial begin
`ifdef DUMP
        $dumpfile( "time1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

endmodule
