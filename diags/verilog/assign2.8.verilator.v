module main( input verilatorclock );
/* verilator lint_off WIDTH */
reg	    a, b;

supply0        w0 = a ^ b;
supply0 [31:0] w1 = a << b;
supply0        w2 = ~a;
supply0 [1:0]  w3 = w0 & |w1 & w2;

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time == 1 ) begin
    a <= 1'b0;
    b <= 1'b0;
  end
  if( $time == 5 )  a <= 1'b1;
  if( $time == 11 ) b <= 1'b1;
  if( $time == 15 ) $finish;
end
/* coverage on */

/* verilator lint_on WIDTH */
endmodule
