module main;

reg c;

mod_a a( c );
mod_a b( ~c );

initial begin
        $dumpfile( "task4.1.vcd" );
        $dumpvars( 0, main );
	c = 1'b0;
        #10;
        $finish;
end

endmodule


module mod_a( input wire c );

always @(c) foo( c );

task foo;
  input c;
  reg a;
  begin
   a = c;
   #5;
   a = ~c;
  end
endtask

endmodule
