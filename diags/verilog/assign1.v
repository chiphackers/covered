module main;

reg	    a, b, c;
reg [1:0]   d;

wire        w0;
wire        w1, w2, w3, w4;
// wire        w5, w6; 
wire        w7, w8, w9;
wire        w10, w11, w12, w13, w14, w15, w16, w17, w18, w19; 
wire        w20, w21, w22, w23, w24, w25, w26, w27;
// wire [31:0] x0;
wire [31:0] x1; 
// wire [31:0] x2; 
wire [31:0] x3, x4, x5, x6;
wire [1:0]  y0;
wire [1:0]  y1;
// wire [1:0]  y2;

integer     i; 
integer     j, k, l;

assign w0 = 0;             // EXP_OP_NONE
assign w1 = a;             // EXP_OP_SIG  -- wrong
assign w2 = a ^ b;         // EXP_OP_XOR
// assign x0 = a * b;         // EXP_OP_MULTIPLY  -- unable to parse correctly
assign x1 = a / 1;         // EXP_OP_DIVIDE
// assign x2 = a % 1;         // EXP_OP_MOD  -- iverilog cannot handle
assign x3 = a + b;         // EXP_OP_ADD
assign x4 = a - b;         // EXP_OP_SUBTRACT
assign w3 = a & b;	   // EXP_OP_AND
assign w4 = a | b;         // EXP_OP_OR
// assign w5 = a ~& b;        // EXP_OP_NAND -- iverilog cannot parse
// assign w6 = a ~| b;        // EXP_OP_NOR  -- iverilog cannot parse
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
assign y0  = d;            // EXP_OP_MBIT_SEL
assign y1  = {2{a}};       // EXP_OP_EXPAND
// assign y2  = {a, b};       // EXP_OP_CONCAT -- wrong

initial begin
	$dumpfile( "assign1.vcd" );
        $dumpvars( 0, main );
	for( i=0; i<2; i=i+1 )
          begin
           a = i;
           for( j=0; j<2; j=j+1 )
             begin
              b = j;
              for( k=0; k<2; k=k+1 )
                begin
                 c = k;
                 for( l=0; l<4; l=l+1 )
                   begin
                    d = l;
                    #5;
                   end
                end
             end
          end 
        a = 0;
        b = 0;
        #5;
end

endmodule
