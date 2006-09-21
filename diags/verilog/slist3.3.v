module main;

reg [1:0]  a;
reg [31:0] b;

always @*
  begin : foo
    real mem0, mem1;
    mem0 = 2'b01;
    mem1 = 2'b10;
    a = b ? mem1 : mem0;
  end

initial begin
        $dumpfile( "slist3.3.vcd" );
        $dumpvars( 0, main );
	b = 1'b0;
        #10;
        $finish;
end

endmodule
