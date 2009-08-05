/*
 Name:        mem7.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        08/04/2009
 Purpose:     Verifies that memories assigned via non-blocking assignments
              within a for loop do not generate BLKLOOPINIT errors.
*/

module main( input verilatorclock );

integer    i;
reg [31:0] mem[0:127];

always @(posedge verilatorclock) begin
  if( $time == 1 )
    for( i=0; i<128; i=i+2 ) begin
      mem[i+0] <= (i + 0);
      mem[i+1] <= (i + 1);
    end
  else if( $time == 11 )
    $finish;
end

endmodule
