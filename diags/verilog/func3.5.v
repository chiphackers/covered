module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
        $dumpfile( "func3.5.vcd" );
        $dumpvars( 0, main );
        #100;
        $finish;
end

function [1:0] return_two;
input a;
begin
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
GROUPS func3.5 all iv vcd lxt
SIM    func3.5 all iv vcd  : iverilog func3.5.v; ./a.out                             : func3.5.vcd
SIM    func3.5 all iv lxt  : iverilog func3.5.v; ./a.out -lxt2; mv func3.5.vcd func3.5.lxt : func3.5.lxt
SCORE  func3.5.vcd     : -t main -vcd func3.5.vcd -o func3.5.cdd -v func3.5.v >& func3.5.err : func3.5.err : 1
SCORE  func3.5.lxt     : -t main -lxt func3.5.lxt -o func3.5.cdd -v func3.5.v >& func3.5.err : func3.5.err : 1
*/

/* OUTPUT func3.5.err
ERROR!  Attempting to use a delay, task call, non-blocking assign or event controls in function main.return_two, file func3.5.v, line 15
*/
