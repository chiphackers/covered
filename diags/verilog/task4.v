module main;

mod_a a();
mod_b b();

initial begin
        $dumpfile( "task4.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

endmodule


module mod_a;

initial foo;

task foo;
  reg a;
  begin
   a = 1'b1;
   #5;
   a = 1'b0;
  end
endtask

endmodule


module mod_b;

initial foo;

task foo;
  reg b;
  begin
   b = 1'b0;
   #5;
   b = 1'b1;
  end
endtask

endmodule
