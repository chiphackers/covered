module main;

integer i, j;
event   e, f;

initial begin
	for( i=0; i<2000000; i=i+1 ) begin
	  #2;
	  ->e;
        end
end

initial begin
	#1;
	for( j=0; j<2000000; j=j+1 ) begin
	  #2;
	  ->f; 
	end
        $finish;
end

always @(e) begin
	@(f);
end
	  
initial begin
`ifdef DUMP
        $dumpfile( "test.vcd" );
        $dumpvars( 0, main );
`endif
end

endmodule
