module main;

reg   a, b, c;

wire d = ~a;

always @(c)
  begin
   b = ~c;
   a = b;
  end

initial begin
        $dumpfile( "bassign1.vcd" );
        $dumpvars( 0, main );
	c = 1'b1;
        #10;
        $finish;
end

endmodule
