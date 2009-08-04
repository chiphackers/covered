module main( input verilatorclock );

parameter BAR0 = 1'b0;
parameter BAR1 = 1'b1;

reg m[0:1];

initial begin : foo
        m[0] = BAR0;
	m[1] = BAR1;
end

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time == 11 ) $finish;
end
/* coverage on */

endmodule
