all:
	- doxygen covered.dox
	- cd ./user; make all

install:
	- cd ./user; make install

uninstall:
	- cd ./user; make uninstall

clean:
	rm -rf ./devel/html ./devel/latex
	- cd ./user; make clean
