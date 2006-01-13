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

task foo;
  reg a;
  begin
   a = 1'b0;
  end
endtask

endmodule


module mod_b;

task foo;
  reg a;
  begin
   a = 1'b1;
  end
endtask

endmodule
