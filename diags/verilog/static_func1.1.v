module main;

parameter SIZE = 5;
parameter FOO  = static_func( 4 );

wire [(FOO-1):0] a;

initial begin
`ifndef VPI
        $dumpfile( "static_func1.1.vcd" );
        $dumpvars( 0, main );
`endif
        #10;
        $finish;
end

function [31:0] static_func;
  input [(SIZE-1):0] bit_to_set;
  begin
    static_func             = 0;
    static_func[bit_to_set] = 1'b1;
  end
endfunction

endmodule
