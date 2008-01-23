module main;

parameter A = 0,
          B = 0,
          C = 1;

generate
  begin
   if( A < 1 )
     if( B < 1 )
       begin
        reg a;
        initial begin
                a = 1'b0;
                #10;
                a = 1'b1;
        end 
       end
     else
       reg b;
   if( C < 1 )
     begin
      reg c;
      initial begin
              c = 1'b0;
              #10;
              c = 1'b1;
      end
     end
  end
endgenerate

initial begin
`ifdef DUMP
        $dumpfile( "generate8.6.vcd" );
        $dumpvars( 0, main );
`endif
        #20;
        $finish;
end

endmodule
