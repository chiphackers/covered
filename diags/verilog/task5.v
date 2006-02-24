module main;

initial begin
        $dumpfile( "task5.vcd" );
        $dumpvars( 0, main );
        #10;
        $finish;
end

task foo;
  begin
    $display( "Hello" );
  end
endtask

task foo;
  begin
    $display( "World" );
  end
endtask

endmodule

/* HEADER
GROUPS task5 all iv vcd lxt
SIM    task5 all iv vcd  : iverilog task5.v; ./a.out                             : task5.vcd
SIM    task5 all iv lxt  : iverilog task5.v; ./a.out -lxt2; mv task5.vcd task5.lxt : task5.lxt
SCORE  task5.vcd     : -t main -vcd task5.vcd -o task5.cdd -v task5.v >& task5.err : task5.err : 1
SCORE  task5.lxt     : -t main -lxt task5.lxt -o task5.cdd -v task5.v >& task5.err : task5.err : 1
*/

/* OUTPUT task5.err
ERROR!  Multiple identical task/function/named-begin-end names (foo) found in module main, file task5.v

*/
