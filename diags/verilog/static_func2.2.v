module main;

parameter FOO = 4;

reg [const_func(FOO)-1:0] a;

initial begin
`ifndef VPI
        $dumpfile( "static_func2.2.vcd" );
        $dumpvars( 0, main );
`endif
	a = 0;
        #10;
        $finish;
end

function [31:0] const_func;
  input [31:0] size;
  begin
    const_func = 0;
    const_func[size] = 1;
  end
endfunction

endmodule
