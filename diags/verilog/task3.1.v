module main;

initial begin
	call_task1( 1'b0 );
end

initial begin
`ifdef DUMP
        $dumpfile( "task3.1.vcd" );
        $dumpvars( 0, main );
`endif
        #1000;
        $finish;
end

task call_task1;

input a;

begin
 call_task2( ~a );
end

endtask

task call_task2;

input a;

begin
 #100;
 $display( $time, "Task called for a=%h", a );
end

endtask

endmodule
