/*
 Name:        task6.v
 Author:      Trevor Williams  (phase1geo@gmail.com)
 Date:        09/10/2008
 Purpose:     Verifies the use of a multi-port task.
*/

module main;

reg a, b, c;

initial begin
	a = 1'b1;
	b = 1'b1;
	c = 1'b0;
	foo_task( a, b, c );
end
	

initial begin
`ifdef DUMP
        $dumpfile( "task6.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

task foo_task;
  input a;
  input b;
  input c;

  reg d;

  begin
    d = (a & b) | c;
  end
endtask

endmodule
