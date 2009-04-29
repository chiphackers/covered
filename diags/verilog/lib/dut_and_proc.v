module dut_and_proc( a, b, c );

output reg  a;
input  wire b;
input  wire c;

always @* begin
  a = b & c;
end

endmodule
