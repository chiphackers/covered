module main;

reg       clock;
reg       reset;
reg       a, b, d;
reg [1:0] c;

reg [3:0] addr;
reg       wr;
reg [7:0] data;

always @(posedge clock)
  begin
   if( reset )
     a <= 1'b0;

   if( reset )
     b <= 1'b0;
   else if( (addr == 4'h0) && wr )
     b <= 1'b1;

   if( reset )
     c <= 2'h0;
   else if( (addr == 4'h1) && wr && data[7] )
     c <= 2'h1;
   else if( addr == 4'h1 && wr)
     c <= 2'h2;
       
   if( reset )
     d <= 1'b0;
   else if( (addr == 4'h2) && wr )
     d <= 1'b1;
end

`ifdef DEPRECATED
    if (reset)
        gfx_detach <= 1'b0;

        /* SW Write to TIO_CA_GFX_DETACH MMR */
    else if (mmr_addr == 16'h0030 && mmr_wr)
        gfx_detach <= mmr_wr_data[0];


     /* GFX Detach Wait State */
    if (reset)
        gfx_detach_wait <= 1'b0;

    /* Set wait state and wait for ACK */
    else if (set_gfx_detach)
        gfx_detach_wait <= 1'b1;

    /* Don't set wait state if the same value is being written. */
    else if (same_detach_state)
        gfx_detach_wait <= 1'b0;

    /* Clear when ACK is received */
    else if (gfx_detach_ack)
        gfx_detach_wait <= 1'b0;

        
    /// INTA Interrupt Destination Address/Vector Writes //////////////

        /* SW Write to TIO_CA_INTA_DEST_ADDR MMR */
    if (mmr_addr == 16'h0038 && mmr_wr) begin
        inta_int_addr <= mmr_wr_data[53:3];
        inta_int_vect_fld <= mmr_wr_data[63:56];
    end

        
    /// INTB Interrupt Destination Address/Vector Writes //////////////

        /* SW Write to TIO_CA_INTB_DEST_ADDR MMR */
    if (mmr_addr == 16'h0040 && mmr_wr) begin
        intb_int_addr <= mmr_wr_data[53:3];
        intb_int_vect_fld <= mmr_wr_data[63:56];
    end
    if (mmr_addr == 16'h0048 && mmr_wr) begin
        err_int_addr <= mmr_wr_data[53:3];
        err_int_vect_fld <= mmr_wr_data[63:56];
    end


    /// Error Status //////////////////////////////////////////////////

        /* SW Write to TIO_CA_INT_STATUS MMR */
    if (mmr_addr == 16'h0050 && mmr_wr) begin
        pci_err_stat            <= mmr_wr_data[0];
        gfx_wr_sram_perr_stat   <= mmr_wr_data[1];
        adma_wr_sram_perr_stat  <= mmr_wr_data[2];
        adma_rd_sram_perr_stat  <= mmr_wr_data[3];
        gart_fetch_err_stat     <= mmr_wr_data[4];
        gfx_wr_ovflw_stat       <= mmr_wr_data[5];
        pio_req_ovflw_stat      <= mmr_wr_data[6];
        crm_pkterr_stat         <= mmr_wr_data[7];
        crm_dverr_stat          <= mmr_wr_data[8];
        crm_tnumto_stat         <= mmr_wr_data[9];
        cxm_rsp_cred_ovflw_stat <= mmr_wr_data[10];
        cxm_req_cred_ovflw_stat <= mmr_wr_data[11];
        pio_invalid_addr_stat   <= mmr_wr_data[12];
        pci_arb_to_stat         <= mmr_wr_data[13];
        as_req_oflow_stat       <= mmr_wr_data[14];
        as_sba_type1_err_stat   <= mmr_wr_data[15];
        agp_wbf_to_err_stat     <= mmr_wr_data[16];
        inta_stat               <= mmr_wr_data[17];
        intb_stat               <= mmr_wr_data[18];
        mult_inta_stat          <= mmr_wr_data[19];
        mult_intb_stat          <= mmr_wr_data[20];
        gfx_credit_ovflw_stat   <= mmr_wr_data[21];
    end
    else if (mmr_addr == 16'h0058 && mmr_wr) begin
        pci_err_stat            <= pci_err_stat            & ~mmr_wr_data[0];
        gfx_wr_sram_perr_stat   <= gfx_wr_sram_perr_stat   & ~mmr_wr_data[1];
        adma_wr_sram_perr_stat  <= adma_wr_sram_perr_stat  & ~mmr_wr_data[2];
        adma_rd_sram_perr_stat  <= adma_rd_sram_perr_stat  & ~mmr_wr_data[3];
        gart_fetch_err_stat     <= gart_fetch_err_stat     & ~mmr_wr_data[4];
        gfx_wr_ovflw_stat       <= gfx_wr_ovflw_stat       & ~mmr_wr_data[5];
        pio_req_ovflw_stat      <= pio_req_ovflw_stat      & ~mmr_wr_data[6];
        crm_pkterr_stat         <= crm_pkterr_stat         & ~mmr_wr_data[7];
        crm_dverr_stat          <= crm_dverr_stat          & ~mmr_wr_data[8];
        crm_tnumto_stat         <= crm_tnumto_stat         & ~mmr_wr_data[9];
        cxm_rsp_cred_ovflw_stat <= cxm_rsp_cred_ovflw_stat & ~mmr_wr_data[10];
        cxm_req_cred_ovflw_stat <= cxm_req_cred_ovflw_stat & ~mmr_wr_data[11];
        pio_invalid_addr_stat   <= pio_invalid_addr_stat   & ~mmr_wr_data[12];
        pci_arb_to_stat         <= pci_arb_to_stat         & ~mmr_wr_data[13];
        as_req_oflow_stat       <= as_req_oflow_stat       & ~mmr_wr_data[14];
        as_sba_type1_err_stat   <= as_sba_type1_err_stat   & ~mmr_wr_data[15];
        agp_wbf_to_err_stat     <= agp_wbf_to_err_stat     & ~mmr_wr_data[16];
        inta_stat               <= inta_stat               & ~mmr_wr_data[17];
        intb_stat               <= intb_stat               & ~mmr_wr_data[18];
        mult_inta_stat          <= mult_inta_stat          & ~mmr_wr_data[19];
        mult_intb_stat          <= mult_intb_stat          & ~mmr_wr_data[20];
        gfx_credit_ovflw_stat   <= gfx_credit_ovflw_stat   & ~mmr_wr_data[21];
    end 

        /* HW Write to TIO_CA_INT_STATUS MMR */
    else begin
        if (pci_err)              pci_err_stat            <= 1'b1;
        if (gfx_wr_sram_perr)     gfx_wr_sram_perr_stat   <= 1'b1;
        if (adma_wr_sram_perr)    adma_wr_sram_perr_stat  <= 1'b1;
        if (adma_rd_sram_perr)    adma_rd_sram_perr_stat  <= 1'b1;
        if (gart_fetch_err)       gart_fetch_err_stat     <= 1'b1;
        if (gfx_wr_ovflw)         gfx_wr_ovflw_stat       <= 1'b1;
        if (pio_req_ovflw)        pio_req_ovflw_stat      <= 1'b1;
        if (crm_pkterr)           crm_pkterr_stat         <= 1'b1;
        if (crm_dverr)            crm_dverr_stat          <= 1'b1;
        if (crm_tnumto)           crm_tnumto_stat         <= 1'b1;
        if (cxm_rsp_cred_ovflw)   cxm_rsp_cred_ovflw_stat <= 1'b1;
        if (cxm_req_cred_ovflw)   cxm_req_cred_ovflw_stat <= 1'b1;
        if (pio_invalid_addr)     pio_invalid_addr_stat   <= 1'b1;
        if (pci_arb_to)           pci_arb_to_stat         <= 1'b1;
        if (as_req_oflow_set)     as_req_oflow_stat       <= 1'b1;
        if (as_sba_type1_err_set) as_sba_type1_err_stat   <= 1'b1;
        if (agp_wbf_to_err_set)   agp_wbf_to_err_stat     <= 1'b1;
        if (inta)                 inta_stat               <= 1'b1;
        if (intb)                 intb_stat               <= 1'b1;
        if (mult_inta)            mult_inta_stat          <= 1'b1;
        if (mult_intb)            mult_intb_stat          <= 1'b1;
        if (gfx_credit_ovflw)     gfx_credit_ovflw_stat   <= 1'b1;
    end
    if (reset)
        int_mask <= {22{1'd1}};

        /*  SW Write to TIO_CA_INT_MASK MMR */
    else if (mmr_addr == 16'h0078 && mmr_wr)
        int_mask <= mmr_wr_data[21:0];


    ///  Multi Error Status  //////////////////////////////////////////

        /*  SW Write to TIO_CA_MULT_ERROR MMR */
    if (mmr_addr == 16'h0060 && mmr_wr) begin
        mult_pci_err            <= mmr_wr_data[0];
        mult_gfx_wr_sram_perr   <= mmr_wr_data[1];
        mult_adma_wr_sram_perr  <= mmr_wr_data[2];
        mult_adma_rd_sram_perr  <= mmr_wr_data[3];
        mult_gart_fetch_err     <= mmr_wr_data[4];
        mult_gfx_wr_ovflw       <= mmr_wr_data[5];
        mult_pio_req_ovflw      <= mmr_wr_data[6];
        mult_crm_pkterr         <= mmr_wr_data[7];
        mult_crm_dverr          <= mmr_wr_data[8];
        mult_crm_tnumto         <= mmr_wr_data[9];
        mult_cxm_rsp_cred_ovflw <= mmr_wr_data[10];
        mult_cxm_req_cred_ovflw <= mmr_wr_data[11];
        mult_pio_invalid_addr   <= mmr_wr_data[12];
        mult_pci_arb_to         <= mmr_wr_data[13];
        mult_as_req_oflow       <= mmr_wr_data[14];
        mult_as_sba_type1_err   <= mmr_wr_data[15];
        mult_agp_wbf_to_err     <= mmr_wr_data[16];
        mult_gfx_credit_ovflw   <= mmr_wr_data[17];
    end
    else if (mmr_addr == 16'h0068 && mmr_wr) begin
        mult_pci_err            <= mult_pci_err            & ~mmr_wr_data[0];
        mult_gfx_wr_sram_perr   <= mult_gfx_wr_sram_perr   & ~mmr_wr_data[1];
        mult_adma_wr_sram_perr  <= mult_adma_wr_sram_perr  & ~mmr_wr_data[2];
        mult_adma_rd_sram_perr  <= mult_adma_rd_sram_perr  & ~mmr_wr_data[3];
        mult_gart_fetch_err     <= mult_gart_fetch_err     & ~mmr_wr_data[4];
        mult_gfx_wr_ovflw       <= mult_gfx_wr_ovflw       & ~mmr_wr_data[5];
        mult_pio_req_ovflw      <= mult_pio_req_ovflw      & ~mmr_wr_data[6];
        mult_crm_pkterr         <= mult_crm_pkterr         & ~mmr_wr_data[7];
        mult_crm_dverr          <= mult_crm_dverr          & ~mmr_wr_data[8];
        mult_crm_tnumto         <= mult_crm_tnumto         & ~mmr_wr_data[9];
        mult_cxm_rsp_cred_ovflw <= mult_cxm_rsp_cred_ovflw & ~mmr_wr_data[10];
        mult_cxm_req_cred_ovflw <= mult_cxm_req_cred_ovflw & ~mmr_wr_data[11];
        mult_pio_invalid_addr   <= mult_pio_invalid_addr   & ~mmr_wr_data[12];
        mult_pci_arb_to         <= mult_pci_arb_to         & ~mmr_wr_data[13];
        mult_as_req_oflow       <= mult_as_req_oflow       & ~mmr_wr_data[14];
        mult_as_sba_type1_err   <= mult_as_sba_type1_err   & ~mmr_wr_data[15];
        mult_agp_wbf_to_err     <= mult_agp_wbf_to_err     & ~mmr_wr_data[16];
        mult_gfx_credit_ovflw   <= mult_gfx_credit_ovflw   & ~mmr_wr_data[17];
    end 
    else begin
        if (pci_err_stat            &&
           (pci_err || pci_mult_err))                        mult_pci_err            <= 1'b1;

        if (gfx_wr_sram_perr_stat   &&
           (gfx_wr_sram_perr || gfx_wr_sram_mult_perr))      mult_gfx_wr_sram_perr   <= 1'b1;

        if (adma_wr_sram_perr_stat  && adma_wr_sram_perr)    mult_adma_wr_sram_perr  <= 1'b1;

        if (adma_rd_sram_perr_stat  &&
           (adma_rd_sram_perr || adma_rd_sram_mult_perr))    mult_adma_rd_sram_perr  <= 1'b1;

        if (gart_fetch_err_stat     && gart_fetch_err)       mult_gart_fetch_err     <= 1'b1;
        if (gfx_wr_ovflw_stat       && gfx_wr_ovflw)         mult_gfx_wr_ovflw       <= 1'b1;
        if (pio_req_ovflw_stat      && pio_req_ovflw)        mult_pio_req_ovflw      <= 1'b1;
        if (crm_pkterr_stat         && crm_pkterr)           mult_crm_pkterr         <= 1'b1;
        if (crm_dverr_stat          && crm_dverr)            mult_crm_dverr          <= 1'b1;

        if (crm_tnumto_stat         &&
           (crm_tnumto || crm_tnumto_mult))                  mult_crm_tnumto         <= 1'b1;

        if (cxm_rsp_cred_ovflw_stat && cxm_rsp_cred_ovflw)   mult_cxm_rsp_cred_ovflw <= 1'b1;
        if (cxm_req_cred_ovflw_stat && cxm_req_cred_ovflw)   mult_cxm_req_cred_ovflw <= 1'b1;
        if (pio_invalid_addr_stat   && pio_invalid_addr)     mult_pio_invalid_addr   <= 1'b1;
        if (pci_arb_to_stat         && pci_arb_to)           mult_pci_arb_to         <= 1'b1;
        if (as_req_oflow_stat       && as_req_oflow_set)     mult_as_req_oflow       <= 1'b1;
        if (as_sba_type1_err_stat   && as_sba_type1_err_set) mult_as_sba_type1_err   <= 1'b1;
        if (agp_wbf_to_err_stat     && agp_wbf_to_err_set)   mult_agp_wbf_to_err     <= 1'b1;
        if (gfx_credit_ovflw_stat   && gfx_credit_ovflw)     mult_gfx_credit_ovflw   <= 1'b1;
    end


    ///  CRM TNUM Timeout   ///////////////////////////////////////////

            /* SW Write TIO_CA_CRM_TNUMTO MMR */
    if (mmr_addr == 16'h00A0 && mmr_wr) begin
        crm_tnumto_val_stat <= mmr_wr_data[7:0];
        crm_tnumto_wr_stat  <= mmr_wr_data[8];
    end

            /* Load up new TNUM timeout info */
            /* HW Write TIO_CA_CRM_TNUMTO MMR */
    else if (crm_tnumto && ~crm_tnumto_stat) begin
        crm_tnumto_val_stat <= crm_tnumto_val;
        crm_tnumto_wr_stat  <= crm_tnumto_wr;
    end
    if (mmr_addr == 16'h00A8 && mmr_wr) begin
        gart_err_src_stat  <= mmr_wr_data[1:0];
        gart_err_addr_stat <= mmr_wr_data[39:4];
    end

            /* Load up new TNUM timeout info */
            /* HW Write to TIO_CA_GART_ERR MMR */
    else if (gart_fetch_err && ~gart_fetch_err_stat) begin
        gart_err_src_stat  <= gart_err_src;
        gart_err_addr_stat <= gart_err_addr;
    end


    ///  PCI Error Detail   ///////////////////////////////////////////

            /* SW Write to TIO_CA_PCIERR_TYPE MMR */
    if (mmr_addr == 16'h00B0 && mmr_wr) begin
        pcierr_data           <= mmr_wr_data[31:0];
        pcierr_enb            <= mmr_wr_data[35:32];
        pcierr_cmd            <= mmr_wr_data[39:36];
        pcierr_a64            <= mmr_wr_data[40];
        pcierr_slv_serr       <= mmr_wr_data[41];
        pcierr_slv_wr_perr    <= mmr_wr_data[42];
        pcierr_slv_rd_perr    <= mmr_wr_data[43];
        pcierr_mst_serr       <= mmr_wr_data[44];
        pcierr_mst_wr_perr    <= mmr_wr_data[45];
        pcierr_mst_rd_perr    <= mmr_wr_data[46];
        pcierr_mst_mabt       <= mmr_wr_data[47];
        pcierr_mst_tabt       <= mmr_wr_data[48];
        pcierr_mst_retry_tout <= mmr_wr_data[49];
    end
       
            /* Load up new PCI Error info */
            /* HW Write to TIO_CA_PCIERR_TYPE MMR */
    else if (pci_err && ~pci_err_stat) begin
        pcierr_data           <= pci_err_info[99:68];
        pcierr_enb            <= pci_err_info[103:100];
        pcierr_cmd            <= pci_err_info[67:64];
        pcierr_a64            <= pci_err_info[104];
        pcierr_slv_serr       <= pci_err_info[105];
        pcierr_slv_wr_perr    <= pci_err_info[106];
        pcierr_slv_rd_perr    <= pci_err_info[107];
        pcierr_mst_serr       <= pci_err_info[108];
        pcierr_mst_wr_perr    <= pci_err_info[109];
        pcierr_mst_rd_perr    <= pci_err_info[110];
        pcierr_mst_mabt       <= pci_err_info[111];
        pcierr_mst_tabt       <= pci_err_info[112];
        pcierr_mst_retry_tout <= pci_err_info[113];
    end
    if (mmr_addr == 16'h00B8 && mmr_wr)
        pcierr_addr <= mmr_wr_data;

            /* Load up new PCI Error Addr */
            /* HW Write to TIO_CA_PCIERR_ADDR MMR */
    else if (pci_err && ~pci_err_stat)
        pcierr_addr <= pci_err_info[63:0];


    ///  ADMA WR Buffer Errors ////////////////////////////////////////

            /* SW Write to TIO_CA_SRAM_PERR_AW MMR */
    if (mmr_addr == 16'h00C0 && mmr_wr) begin
        sram_perr_aw_group <= mmr_wr_data[15:0];
        sram_perr_aw_data  <= mmr_wr_data[31:16];
        sram_perr_aw_par   <= mmr_wr_data[32];
        sram_perr_aw_addr  <= mmr_wr_data[40:33];
    end

            /* Load up new ADMA WR Buffer Error */
            /* HW Write to TIO_CA_SRAM_PERR_AW MMR */
    else if (adma_wr_sram_perr && ~adma_wr_sram_perr_stat) begin
        sram_perr_aw_group <= ad_sram_err_info[15:0];
        sram_perr_aw_data  <= ad_sram_err_info[31:16];
        sram_perr_aw_par   <= ad_sram_err_info[32];
        sram_perr_aw_addr  <= ad_sram_err_info[40:33];
    end


    ///  ADMA RD Buffer Errors ////////////////////////////////////////

            /* SW Write to TIO_CA_SRAM_PERR_AR MMR */
    if (mmr_addr == 16'h00C8 && mmr_wr) begin
        sram_perr_ar_group <= mmr_wr_data[15:0];
        sram_perr_ar_data  <= mmr_wr_data[31:16];
        sram_perr_ar_par   <= mmr_wr_data[32];
        sram_perr_ar_addr  <= mmr_wr_data[39:33];
    end

            /* Load up new ADMA RD Buffer Error */
            /* HW Write to TIO_CA_SRAM_PERR_AR MMR */
    else if (adma_rd_sram_perr && ~adma_rd_sram_perr_stat) begin
        sram_perr_ar_group <= as_sram_err_info[15:0];
        sram_perr_ar_data  <= as_sram_err_info[31:16];
        sram_perr_ar_par   <= as_sram_err_info[32];
        sram_perr_ar_addr  <= as_sram_err_info[39:33];
    end
    if (mmr_addr == 16'h00D0 && mmr_wr) begin
        sram_perr_gw_group <= mmr_wr_data[15:0];
        sram_perr_gw_data  <= mmr_wr_data[31:16];
        sram_perr_gw_par   <= mmr_wr_data[32];
        sram_perr_gw_addr  <= mmr_wr_data[39:33];
    end

            /* Load up new ADMA RD Buffer Error */
            /* HW Write to TIO_CA_SRAM_PERR_GW MMR */
    else if (gfx_wr_sram_perr && ~gfx_wr_sram_perr_stat) begin
        sram_perr_gw_group <= pm_sram_err_info[15:0];
        sram_perr_gw_data  <= pm_sram_err_info[31:16];
        sram_perr_gw_par   <= pm_sram_err_info[32];
        sram_perr_gw_addr  <= pm_sram_err_info[39:33];
    end
       

    ///  PCI DMA Address Extension   //////////////////////////////////

            /* HW Write to TIO_CA_PCI_DMA_ADDR_EXTN MMR */
    if (reset) begin
        pd_addr_extn[5:0]  <=  6'd0; // Upper Node Offset
        pd_addr_extn[7:6]  <=  2'd0; // Chiplet ID
        pd_addr_extn[21:8] <= 14'd0; // Node ID
        pd_addr_extn[22]   <=  1'd0; // PIO/~Mem
    end

            /* SW Write to TIO_CA_PCI_DMA_ADDR_EXTN MMR */
    else if (mmr_addr == 16'h00E0 && mmr_wr) begin
        pd_addr_extn[5:0]  <= mmr_wr_data[5:0];   // Upper Node Offset
        pd_addr_extn[7:6]  <= mmr_wr_data[9:8];   // Chiplet ID
        pd_addr_extn[21:8] <= mmr_wr_data[25:12]; // Node ID
        pd_addr_extn[22]   <= mmr_wr_data[28];    // PIO/~Mem
    end
       

    ///  AGP DMA Address Extension   //////////////////////////////////

            /* HW Write to TIO_CA_AGP_DMA_ADDR_EXTN MMR */
    if (reset) begin
        ad_addr_extn[5:0] <= 6'd0; // Node ID (upper 6 bits)
        ad_addr_extn[6]   <= 1'd0; // PIO/~Mem
    end

            /* SW Write to TIO_CA_AGP_DMA_ADDR_EXTN MMR */
    else if (mmr_addr == 16'h00E8 && mmr_wr) begin
        ad_addr_extn[5:0] <= mmr_wr_data[25:20]; // Node ID
        ad_addr_extn[6]   <= mmr_wr_data[28];    // PIO/~Mem
    end
    if (mmr_addr == 16'h0100 && mmr_wr) begin
        mn_debug_addr <= mmr_wr_data[3:0];
        pp_debug_addr <= mmr_wr_data[7:4];
        gw_debug_addr <= mmr_wr_data[11:8];
        gt_debug_addr <= mmr_wr_data[15:12];
        pd_debug_addr <= mmr_wr_data[19:16];
        ad_debug_addr <= mmr_wr_data[23:20];
        cx_debug_addr <= mmr_wr_data[27:24];
        cr_debug_addr <= mmr_wr_data[31:28];
        ba_debug_addr <= mmr_wr_data[35:32];
        pe_debug_addr <= mmr_wr_data[39:36];
        bo_debug_addr <= mmr_wr_data[43:40];
        bi_debug_addr <= mmr_wr_data[47:44];
        as_debug_addr <= mmr_wr_data[51:48];
        ps_debug_addr <= mmr_wr_data[55:52];
        pm_debug_addr <= mmr_wr_data[59:56];
    end


    ///  Core To Level Debug Selects  //////////////////////////////////

            /* SW Write to TIO_CA_DEBUG_MUX_CORE_SEL MMR */
    if (mmr_addr == 16'h0108 && mmr_wr) begin

        debug_core0_msel <= mmr_wr_data[2:0];
            // Skip a bit.
        debug_core0_nsel <= mmr_wr_data[6:4];
            // Skip a bit.

        debug_core1_msel <= mmr_wr_data[10:8];
            // Skip a bit.
        debug_core1_nsel <= mmr_wr_data[14:12];
            // Skip a bit.

        debug_core2_msel <= mmr_wr_data[18:16];
            // Skip a bit.
        debug_core2_nsel <= mmr_wr_data[22:20];
            // Skip a bit.

        debug_core3_msel <= mmr_wr_data[26:24];
            // Skip a bit.
        debug_core3_nsel <= mmr_wr_data[30:28];
            // Skip a bit.

        debug_core4_msel <= mmr_wr_data[34:32];
            // Skip a bit.
        debug_core4_nsel <= mmr_wr_data[38:36];
            // Skip a bit.

        debug_core5_msel <= mmr_wr_data[42:40];
        debug_core5_nsel <= mmr_wr_data[46:44];
            // Skip a bit.

        debug_core6_msel <= mmr_wr_data[50:48];
            // Skip a bit.
        debug_core6_nsel <= mmr_wr_data[54:52];
            // Skip a bit.

        debug_core7_msel <= mmr_wr_data[58:56];
            // Skip a bit.
        debug_core7_nsel <= mmr_wr_data[62:60];
    end


    ///  Debug Core Nibble Selects ////////////////////////////////////

            /* SW Write to TIO_CA_DEBUG_MUX_PCI_SEL MMR */
    if (mmr_addr == 16'h0110 && mmr_wr) begin

        ba_debug0_msel <= mmr_wr_data[2:0];
            // Skip a bit.
        ba_debug0_nsel <= mmr_wr_data[6:4];
            // Skip a bit.

        ba_debug1_msel <= mmr_wr_data[10:8];
            // Skip a bit.
        ba_debug1_nsel <= mmr_wr_data[14:12];
            // Skip a bit.

        ba_debug2_msel <= mmr_wr_data[18:16];
            // Skip a bit.
        ba_debug2_nsel <= mmr_wr_data[22:20];
            // Skip a bit.

        ba_debug3_msel <= mmr_wr_data[26:24];
            // Skip a bit.
        ba_debug3_nsel <= mmr_wr_data[30:28];
            // Skip a bit.

        ba_debug4_msel <= mmr_wr_data[34:32];
            // Skip a bit.
        ba_debug4_nsel <= mmr_wr_data[38:36];
            // Skip a bit.

        ba_debug5_msel <= mmr_wr_data[42:40];
            // Skip a bit.
        ba_debug5_nsel <= mmr_wr_data[46:44];
            // Skip a bit.

        ba_debug6_msel <= mmr_wr_data[50:48];
        ba_debug6_nsel <= mmr_wr_data[54:52];
            // Skip a bit.

        ba_debug7_msel <= mmr_wr_data[58:56];
            // Skip a bit.
        ba_debug7_nsel <= mmr_wr_data[62:60];
    end


    ///  Debug Top Level Domain Selects ////////////////////////////////////

            /* SW Write to TIO_CA_DEBUG_DOMAIN_SEL MMR */
    if (mmr_addr == 16'h0118 && mmr_wr) begin
        debug_domain_l   <= mmr_wr_data[0];
        debug_domain_h   <= mmr_wr_data[1];
    end


    ///  GART Pointer  ////////////////////////////////////////////////

            /* HW Write to TIO_CA_GART_PTR_TABLE MMR */
            /*  Clear Pointer Valid  */
    if (reset) gart_ptr_val <= 1'b0;

            /* SW Write to TIO_CA_GART_PTR_TABLE MMR */
    else if (mmr_addr == 16'h0200 && mmr_wr) begin
        gart_ptr_val  <= mmr_wr_data[0];
        gart_ptr_addr <= mmr_wr_data[55:12];
    end

end
`endif

initial begin
	$dumpfile( "always9.vcd" );
	$dumpvars( 0, main );
	reset = 1'b1;
	addr  = 4'h0;
	#20;
	reset = 1'b0;
	#20;
	wr    = 1'b1;
	data  = 8'h45;
	$finish;
end

initial begin
	clock = 1'b0;
	forever #(1) clock = ~clock;
end

endmodule
