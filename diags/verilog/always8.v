module main;

parameter A = 2'b00,
          B = 2'b01,
          C = 2'b10,
          D = 2'b11;

reg [1:0]     a;
reg [(7*7):1] b;

always @(a)
  case (a)
    A:  b = "STUFF1";
    B:  b = "STUFF2";
    C:  b = "STUFF3";
    D:  b = "STUFF4";
  endcase

initial begin
	$dumpfile( "always8.vcd" );
	$dumpvars( 0, main );
	a = B;
	#5;
	a = D;
	#5;
	$finish;
end

endmodule
