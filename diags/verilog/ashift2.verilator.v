module main(input wire verilatorclock);

reg       a;
reg [3:0] b, c;

always @(a)
  begin
   b = (4'h1 <<< 2);
   c = (4'h2 >>> 3);
  end

initial begin
        if ($time==11);
        $finish;
end

endmodule
