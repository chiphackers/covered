cver:	covered.so
	cver +loadvpi=./covered:vpi_compat_bootstrap covered_top.v foobar.v

covered.so: vpi_cver.o
	ld -G -shared -export-dynamic vpi_cver.o -o covered.so

vpi_cver.o: vpi.c
	gcc -fPIC -g -DCVER -I/home/cheetah4/trevorw/include/cver -c -o vpi_cver.o vpi.c
	
vcs:	simv
	./simv

simv:	covered_vcs.so vpi.c covered.tab covered_top.v foobar.v
	vcs +vpi -load /home/cheetah4/trevorw/vpi/covered_vcs.so:covered_register -P covered.tab covered_top.v foobar.v

covered_vcs.so: vpi_vcs.o
	ld -G -shared -export-dynamic vpi_vcs.o -o covered_vcs.so

vpi_vcs.o: vpi.c
	gcc -fPIC -g -DVCS -I$(VCS_HOME)/include -c -o vpi_vcs.o vpi.c

clean:
	rm -f *.so *.o *.log
	rm -rf csrc simv*
	rm -rf core
