module main;

wire [1:0] a = return_two( 1'b0 );

initial begin
`ifndef VPI
        $dumpfile( "func3.5.vcd" );
        $dumpvars( 0, main );
`endif
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
