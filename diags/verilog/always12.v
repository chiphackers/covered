module main;

reg   a, b;

always @(a)
  begin
   foobar( 1'b0 );
   if( a )
     begin
      if( b )
        begin
         a = 1'b0;
        end
        b = 1'b0;
     end
  end

initial begin
`ifdef DUMP
        $dumpfile( "always12.vcd" );
        $dumpvars( 0, main );
`endif
	b = 1'b0;
	#10;
	a = 1'b0;
        #10;
        $finish;
end

task foobar;
input b;
reg a;
begin
 a = b;
end
endtask

endmodule
