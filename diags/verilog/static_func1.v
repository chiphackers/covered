module main;

parameter FOO = static_func( 4 );

wire [(FOO-1):0] a;

initial begin
`ifdef DUMP
        $dumpfile( "static_func1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function [31:0] static_func;
  input [4:0] bit_to_set;
  begin
    static_func             = 0;
    static_func[bit_to_set] = 1'b1;
  end
endfunction

endmodule
