module main;

parameter A       = 0;
parameter STATE_A = 1 << A;

parameter B       = 1;
parameter STATE_B = 1 << B;

parameter C       = 2;
parameter STATE_C = 1 << C;

reg         clock;
reg   [2:0] display_tagto_current_state;
//reg   [2:0] st;
reg   [2:0] next_st;
reg   [7:0] pcix_pio_timeout_recorder;
wire        split_resp_active;
wire  [2:0] split_resp_tag;
reg   [2:0] tag_timeout;
wire        clr_tag_toed;
reg  [79:0] ascii_display_tagto_state;


always @(display_tagto_current_state
//always @(st
   or   pcix_pio_timeout_recorder
   or   split_resp_active
   or   split_resp_tag
   or   tag_timeout
   or   clr_tag_toed) begin // {
    next_st = display_tagto_current_state;
   // next_st = st;
   case(1'b1) // synopsys parallel_case full_case

   //---------------------------------------------------------
   // Idle state: Wait till there is at least one tag timeout.
   //---------------------------------------------------------
   display_tagto_current_state[A]: begin // {
   // st[A]: begin // {
      // synopsys translate_off
      ascii_display_tagto_state = "STATE_A";
      // synopsys translate_on

      if (|pcix_pio_timeout_recorder)
         next_st = STATE_B;
   end // }

   //---------------------------------------------------------
   // Show state: Signal to the rest of the ckty that there is
   // a tag that has timed out. This is a pulse of one pcixclk.
   //---------------------------------------------------------
   display_tagto_current_state[B]: begin // {
   // st[B]: begin // {
      // synopsys translate_off
      ascii_display_tagto_state = "STATE_B";
      // synopsys translate_on

      next_st = STATE_C;
   end // }

   //---------------------------------------------------------
   // No Show state: Wait until all the reuired actions have
   // been taken for the tag that has timed out.
   //---------------------------------------------------------
   display_tagto_current_state[C]: begin // {
   // st[C]: begin // {
      // synopsys translate_off
      ascii_display_tagto_state = "STATE_C";
      // synopsys translate_on

      if (clr_tag_toed
      ||  (split_resp_active && (split_resp_tag == tag_timeout)))
         next_st = STATE_A;
   end // }
   endcase
end // }

always @(posedge clock) display_tagto_current_state <= next_st;
// always @(posedge clock) st <= next_st;

initial begin
	$dumpfile( "case4.1.vcd" );
	$dumpvars( 0, main );
	#50;
	$finish;
end

initial begin
	clock = 1'b0;
	forever #(1) clock = ~clock;
end

endmodule
