module main;

parameter A = 0,
          B = 1,
          C = 2;

reg               clock;
reg [C:A]         st;
reg [C:A]         next_st;
reg [((8*8)-1):0] str;

reg  b, c, d, e, f;

always @(st or b or c or d or e or f)
  begin
   next_st = st;
   case( 1'b1 )
     st[A] :
       begin
        str = "A_STATE";
        if( b )
          next_st = (3'b1 << B);
       end
     st[B] :
       begin
        str = "B_STATE";
        next_st = (3'b1 << C);
       end
     st[C] :
       begin
        str = "C_STATE";
        if( c || (d && (e == f)) )
          next_st = (3'b1 << A);
       end
   endcase
  end

always @(posedge clock) st <= next_st;
      
initial begin
	$dumpfile( "case4.vcd" );
	$dumpvars( 0, main );
	st = 3'b001;
	b  = 1'b0;
	c  = 1'b0;
	d  = 1'b0;
	e  = 1'b0;
	f  = 1'b0;
	#5;
	b  = 1'b1;
	#20;
	d  = 1'b1;
	#10;
	$finish;
end

initial begin
	clock = 1'b0;
	forever #(2) clock = ~clock;
end

endmodule
