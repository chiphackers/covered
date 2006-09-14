module main;

reg [31:0] i;
reg [2:0]  a;

initial begin
	for( i=0; i<10; i=i+1 )
          begin
           a = i;
          end
	i = 3'b0;
end

initial begin
        $dumpfile( "for1.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule
