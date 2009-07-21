module main(input verilatorclock);

reg         a, b, c;
reg [2:0]   d;

wire        w0;
wire        w1, w2, w3, w4;
wire        w5;
wire        w6; 
wire        w7, w8, w9;
wire        w10, w11, w12, w13, w14, w15, w16, w17, w18, w19; 
wire        w20, w21, w22, w23, w24, w25, w26, w27;
wire [31:0] x0;
wire [31:0] x1; 
wire [31:0] x2; 
wire [31:0] x3, x4, x5, x6;
wire [1:0]  y0;
wire [1:0]  y1;
wire [1:0]  y2;

integer     i; 
integer     j, k, l;

assign w0 = 0;             // EXP_OP_NONE
assign w1 = a;             // EXP_OP_SIG  -- wrong
assign w2 = a ^ b;         // EXP_OP_XOR
assign x0 = a * b;         // EXP_OP_MULTIPLY
assign x1 = a / 1;         // EXP_OP_DIVIDE
assign x2 = a % 2;         // EXP_OP_MOD
assign x3 = a + b;         // EXP_OP_ADD
assign x4 = a - b;         // EXP_OP_SUBTRACT
assign w3 = a & b;	   // EXP_OP_AND
assign w4 = a | b;         // EXP_OP_OR
assign w5 = a ~& b;        // EXP_OP_NAND
assign w6 = a ~| b;        // EXP_OP_NOR
assign w7 = a ~^ b;        // EXP_OP_NXOR
assign w8 = a < b;         // EXP_OP_LT
assign w9 = a > b;         // EXP_OP_GT
assign x5 = a << b;        // EXP_OP_LSHIFT
assign x6 = a >> b;        // EXP_OP_RSHIFT
assign w10 = a == b;       // EXP_OP_EQ
assign w11 = a === b;      // EXP_OP_CEQ
assign w12 = a <= b;       // EXP_OP_LE
assign w13 = a >= b;       // EXP_OP_GE
assign w14 = a != b;       // EXP_OP_NE
assign w15 = a !== b;      // EXP_OP_CNE
assign w16 = a || b;       // EXP_OP_LOR
assign w17 = a && b;       // EXP_OP_LAND
assign w18 = a ? b : c;    // EXP_OP_COND/EXP_OP_COND_SEL
assign w19 = ~a;           // EXP_OP_UINV
assign w20 = &d;           // EXP_OP_UAND
assign w21 = !a;           // EXP_OP_UNOT
assign w22 = |d;           // EXP_OP_UOR
assign w23 = ^d;           // EXP_OP_UXOR
assign w24 = ~&d;          // EXP_OP_UNAND
assign w25 = ~|d;          // EXP_OP_UNOR
assign w26 = ~^d;          // EXP_OP_UNXOR
assign w27 = d[1];         // EXP_OP_SBIT_SEL
assign y0  = d[2:1];       // EXP_OP_MBIT_SEL
assign y1  = {2{a}};       // EXP_OP_EXPAND
assign y2  = {a, b};       // EXP_OP_CONCAT -- wrong

/* coverage off */
always @(posedge verilatorclock) begin
  if( $time ==   1 ) begin i = 0;  j = 0;  k = 0;  l = 0;  end
  if( $time ==   3 ) begin i = 0;  j = 0;  k = 0;  l = 1;  end
  if( $time ==   5 ) begin i = 0;  j = 0;  k = 0;  l = 2;  end
  if( $time ==   7 ) begin i = 0;  j = 0;  k = 0;  l = 3;  end
  if( $time ==   9 ) begin i = 0;  j = 0;  k = 0;  l = 4;  end
  if( $time ==  11 ) begin i = 0;  j = 0;  k = 0;  l = 5;  end
  if( $time ==  13 ) begin i = 0;  j = 0;  k = 0;  l = 6;  end
  if( $time ==  15 ) begin i = 0;  j = 0;  k = 0;  l = 7;  end
  if( $time ==  17 ) begin i = 0;  j = 0;  k = 1;  l = 0;  end
  if( $time ==  19 ) begin i = 0;  j = 0;  k = 1;  l = 1;  end
  if( $time ==  21 ) begin i = 0;  j = 0;  k = 1;  l = 2;  end
  if( $time ==  23 ) begin i = 0;  j = 0;  k = 1;  l = 3;  end
  if( $time ==  25 ) begin i = 0;  j = 0;  k = 1;  l = 4;  end
  if( $time ==  27 ) begin i = 0;  j = 0;  k = 1;  l = 5;  end
  if( $time ==  29 ) begin i = 0;  j = 0;  k = 1;  l = 6;  end
  if( $time ==  31 ) begin i = 0;  j = 0;  k = 1;  l = 7;  end
  if( $time ==  33 ) begin i = 0;  j = 1;  k = 0;  l = 0;  end
  if( $time ==  35 ) begin i = 0;  j = 1;  k = 0;  l = 1;  end
  if( $time ==  37 ) begin i = 0;  j = 1;  k = 0;  l = 2;  end
  if( $time ==  39 ) begin i = 0;  j = 1;  k = 0;  l = 3;  end
  if( $time ==  41 ) begin i = 0;  j = 1;  k = 0;  l = 4;  end
  if( $time ==  43 ) begin i = 0;  j = 1;  k = 0;  l = 5;  end
  if( $time ==  45 ) begin i = 0;  j = 1;  k = 0;  l = 6;  end
  if( $time ==  47 ) begin i = 0;  j = 1;  k = 0;  l = 7;  end
  if( $time ==  49 ) begin i = 0;  j = 1;  k = 1;  l = 0;  end
  if( $time ==  51 ) begin i = 0;  j = 1;  k = 1;  l = 1;  end
  if( $time ==  53 ) begin i = 0;  j = 1;  k = 1;  l = 2;  end
  if( $time ==  55 ) begin i = 0;  j = 1;  k = 1;  l = 3;  end
  if( $time ==  57 ) begin i = 0;  j = 1;  k = 1;  l = 4;  end
  if( $time ==  59 ) begin i = 0;  j = 1;  k = 1;  l = 5;  end
  if( $time ==  61 ) begin i = 0;  j = 1;  k = 1;  l = 6;  end
  if( $time ==  63 ) begin i = 0;  j = 1;  k = 1;  l = 7;  end
  if( $time ==  65 ) begin i = 1;  j = 0;  k = 0;  l = 0;  end
  if( $time ==  66 ) begin i = 1;  j = 0;  k = 0;  l = 1;  end
  if( $time ==  67 ) begin i = 1;  j = 0;  k = 0;  l = 2;  end
  if( $time ==  69 ) begin i = 1;  j = 0;  k = 0;  l = 3;  end
  if( $time ==  71 ) begin i = 1;  j = 0;  k = 0;  l = 4;  end
  if( $time ==  73 ) begin i = 1;  j = 0;  k = 0;  l = 5;  end
  if( $time ==  75 ) begin i = 1;  j = 0;  k = 0;  l = 6;  end
  if( $time ==  77 ) begin i = 1;  j = 0;  k = 0;  l = 7;  end
  if( $time ==  79 ) begin i = 1;  j = 0;  k = 1;  l = 0;  end
  if( $time ==  81 ) begin i = 1;  j = 0;  k = 1;  l = 1;  end
  if( $time ==  83 ) begin i = 1;  j = 0;  k = 1;  l = 2;  end
  if( $time ==  85 ) begin i = 1;  j = 0;  k = 1;  l = 3;  end
  if( $time ==  87 ) begin i = 1;  j = 0;  k = 1;  l = 4;  end
  if( $time ==  89 ) begin i = 1;  j = 0;  k = 1;  l = 5;  end
  if( $time ==  91 ) begin i = 1;  j = 0;  k = 1;  l = 6;  end
  if( $time ==  93 ) begin i = 1;  j = 0;  k = 1;  l = 7;  end
  if( $time ==  95 ) begin i = 1;  j = 1;  k = 0;  l = 0;  end
  if( $time ==  97 ) begin i = 1;  j = 1;  k = 0;  l = 1;  end
  if( $time ==  99 ) begin i = 1;  j = 1;  k = 0;  l = 2;  end
  if( $time == 101 ) begin i = 1;  j = 1;  k = 0;  l = 3;  end
  if( $time == 103 ) begin i = 1;  j = 1;  k = 0;  l = 4;  end
  if( $time == 105 ) begin i = 1;  j = 1;  k = 0;  l = 5;  end
  if( $time == 107 ) begin i = 1;  j = 1;  k = 0;  l = 6;  end
  if( $time == 109 ) begin i = 1;  j = 1;  k = 0;  l = 7;  end
  if( $time == 111 ) begin i = 1;  j = 1;  k = 1;  l = 0;  end
  if( $time == 113 ) begin i = 1;  j = 1;  k = 1;  l = 1;  end
  if( $time == 115 ) begin i = 1;  j = 1;  k = 1;  l = 2;  end
  if( $time == 117 ) begin i = 1;  j = 1;  k = 1;  l = 3;  end
  if( $time == 119 ) begin i = 1;  j = 1;  k = 1;  l = 4;  end
  if( $time == 121 ) begin i = 1;  j = 1;  k = 1;  l = 5;  end
  if( $time == 123 ) begin i = 1;  j = 1;  k = 1;  l = 6;  end
  if( $time == 125 ) begin i = 1;  j = 1;  k = 1;  l = 7;  end
  if( $time == 127 ) begin
    a = 0;
    b = 0;
    i = 2;
    j = 2;
    k = 2;
    l = 8;
  end else begin
    a = i;
    b = j;
    c = k;
    d = l;
  end
  if( $time == 129 ) $finish;

end
/* coverage on */

endmodule
