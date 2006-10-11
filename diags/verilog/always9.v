module main;

reg        clock;
reg        reset;
reg        a, b;
reg [21:0] c, d;

reg [3:0] addr;
reg       wr;

always @(posedge clock)
  begin
   if( reset )
     begin
      a <= 1'b0;
      b <= 1'b0;
      c <= 22'h0;
     end
   else if( (addr == 4'h0) && wr)
     a <= 1'b1;
   else if( (addr == 4'h1) && wr)
     b <= 1'b1;
   else 
     begin
      if( d[ 0] ) c[0]  <= 1'b1;
      if( d[ 1] ) c[1]  <= 1'b1;
      if( d[ 2] ) c[2]  <= 1'b1;
      if( d[ 3] ) c[3]  <= 1'b1;
      if( d[ 4] ) c[4]  <= 1'b1;
      if( d[ 5] ) c[5]  <= 1'b1;
      if( d[ 6] ) c[6]  <= 1'b1;
      if( d[ 7] ) c[7]  <= 1'b1;
      if( d[ 8] ) c[8]  <= 1'b1;
      if( d[ 9] ) c[9]  <= 1'b1;
      if( d[10] ) c[10] <= 1'b1;
      if( d[11] ) c[11] <= 1'b1;
      if( d[12] ) c[12] <= 1'b1;
      if( d[13] ) c[13] <= 1'b1;
      if( d[14] ) c[14] <= 1'b1;
      if( d[15] ) c[15] <= 1'b1;
      if( d[16] ) c[16] <= 1'b1;
      if( d[17] ) c[17] <= 1'b1;
      if( d[18] ) c[18] <= 1'b1;
      if( d[19] ) c[19] <= 1'b1;
      if( d[20] ) c[20] <= 1'b1;
      if( d[21] ) c[21] <= 1'b1;
     end
  end

initial begin
`ifndef VPI
	$dumpfile( "always9.vcd" );
	$dumpvars( 0, main );
`endif
	reset = 1'b1;
	addr  = 4'h0;
	d[8]  = 1'b1;
        wr    = 1'b0;
	#20;
	reset = 1'b0;
	#20;
	wr    = 1'b1;
	#20;
	$finish;
end

initial begin
	clock = 1'b0;
	forever #(1) clock = ~clock;
end

endmodule
