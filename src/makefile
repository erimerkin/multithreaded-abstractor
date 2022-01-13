all: main.cpp abstract.o
	@g++ -std=c++11 -o abstractor.out main.cpp abstract.o -lpthread
	@echo "Program compiled successfully, executable abstractor.out is created."

abstract.o: abstract.h abstract.cpp
	@g++ -c -std=c++11 -o abstract.o abstract.cpp


clean:
	@rm -rf *.o *.out 
	@echo "Object and executable files are removed successfully."

