module main;

reg	[7:0]	ir;
reg	[2:0]	state;

always @(ir)
  casez( ir )
    8'b1??????? :  state = 3'h0;
    8'b01?????? :  state = 3'h1;
    8'b001????? :  state = 3'h2;
    8'b0001???? :  state = 3'h3;
    8'b00001??? :  state = 3'h4;
    8'b000001?? :  state = 3'h5;
    8'b0000001? :  state = 3'h6;
    8'b00000000 :  state = 3'h7;
  endcase

initial begin
`ifdef DUMP
	$dumpfile( "casez2.vcd" );
	$dumpvars( 0, main );
`endif
	#5;
	ir = 8'h00;
	#5;
	ir = 8'h02;
	#5;
	ir = 8'h05;
	#5;
	ir = 8'h0f;
	#5;
	ir = 8'h1x;
	#5;
	ir = 8'hff;
	#5;
	$finish;
end

endmodule
