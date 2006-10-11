module main;

initial begin
	#5;
	call_task( 1'b0 );
end

initial begin
	#10;
	call_task( 1'b1 );
end
	
initial begin
`ifndef VPI
        $dumpfile( "task3.vcd" );
        $dumpvars( 0, main );
`endif
        #1000;
        $finish;
end

task call_task;

input a;

begin
 #100;
 $display( $time, "Task called for a=%h", a );
end

endtask

endmodule
