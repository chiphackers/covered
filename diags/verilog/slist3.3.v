module main;

reg [1:0]  a;
reg [31:0] b;

always @*
  begin : foo
    reg [1:0] mem[1:0];
    mem[0] = 2'b01;
    mem[1] = 2'b10;
    a = mem[b];
  end

initial begin
        $dumpfile( "slist3.3.vcd" );
        $dumpvars( 0, main );
	b = 1'b0;
        #10;
        $finish;
end

endmodule
