PARAMETERS = -lpthread -std=c++11

all: main.cpp abstract.o
	@g++ $(PARAMETERS) -o abstractor.out main.cpp abstract.o 
	@echo "Program compiled successfully, executable abstractor.out is created."

abstract.o: abstract.h abstract.cpp
	@g++ -c -std=c++11 -o abstract.o abstract.cpp


clean:
	@rm -r *.o *.out


