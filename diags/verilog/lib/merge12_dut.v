module merge12_dut(
  input [3:0] a
);

reg [3:0] b;

always @* begin : foo
  reg [3:0] c;
  c = 0;
  b = a + c;
end

endmodule
