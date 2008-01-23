module main;

reg coreclk, sreset, bfr_wr, ld_bfr_data;
reg [31:0] bfr_rd_addr, bfr_wr_addr;

always @(posedge coreclk) begin

  if (sreset)
    bfr_wr_addr <= 3'b0;
  else if (bfr_wr)
    bfr_wr_addr <= bfr_wr_addr + 1'b1;
  else
    bfr_wr_addr <= bfr_wr_addr;

  if (sreset)
    bfr_rd_addr <= 3'b0;
  else if (ld_bfr_data)
    bfr_rd_addr <= bfr_rd_addr + 1'b1;
  else
    bfr_rd_addr <= bfr_rd_addr;

end

initial begin
`ifdef DUMP
        $dumpfile( "elseif1.vcd" );
        $dumpvars( 0, main );
`endif
	sreset  = 1'b1;
	coreclk = 1'b0;
	#5;
	coreclk = 1'b1;
	#5;
	coreclk = 1'b0;
        #10;
        $finish;
end

endmodule
