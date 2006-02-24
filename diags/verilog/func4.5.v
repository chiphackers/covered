module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func4.5.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin : return_two_block
  do_nothing;
  return_two = 2'b10;
end

endfunction

task do_nothing;

begin
  #10;
end

endtask

endmodule

/* HEADER
GROUPS func4.5 all iv vcd lxt
SIM    func4.5 all iv vcd  : iverilog func4.5.v; ./a.out                             : func4.5.vcd
SIM    func4.5 all iv lxt  : iverilog func4.5.v; ./a.out -lxt2; mv func4.5.vcd func4.5.lxt : func4.5.lxt
SCORE  func4.5.vcd     : -t main -vcd func4.5.vcd -o func4.5.cdd -v func4.5.v >& func4.5.err : func4.5.err : 1
SCORE  func4.5.lxt     : -t main -lxt func4.5.lxt -o func4.5.cdd -v func4.5.v >& func4.5.err : func4.5.err : 1
*/

/* OUTPUT func4.5.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func4.5.v, line 15
*/
