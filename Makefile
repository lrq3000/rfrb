
.PHONY: pathmaker clean

CC=g++
OPT=-g -m64 -std=c++11 -O3 -Wall -Wshadow -Wextra -pedantic
OPT+=$(FLAG)
LIBS=-pthread
INCPATH=./include/
SRCPATH=./src/
INC=-I$(INCPATH) -I$(SRCPATH)
MAIN=$(SRCPATH)main.cpp

BINPATH=./bin/
OBJPATH=./obj/

objects=$(OBJPATH)raidSystem.o\
		$(OBJPATH)fileHandler.o\
		$(OBJPATH)fileReader.o\
		$(OBJPATH)bufferedFileReader.o\
		$(OBJPATH)fileWriter.o\
		$(OBJPATH)raidRecover.o\

all: exe

pathmaker:
	mkdir -p obj/ bin/

exe: pathmaker $(objects) $(MAIN)
	$(CC) $(OPT) $(INC) $(LIBS) -o $(BINPATH)rfrb $(objects) $(MAIN)

$(OBJPATH)raidSystem.o: $(INCPATH)defines.h $(INCPATH)raidSystem.h $(SRCPATH)raidSystem.cpp
	$(CC) $(OPT) $(INC) $(LIBS) -c -o $(OBJPATH)raidSystem.o $(SRCPATH)raidSystem.cpp

$(OBJPATH)fileReader.o: $(INCPATH)defines.h $(INCPATH)fileReader.h $(SRCPATH)fileReader.cpp
	$(CC) $(OPT) $(INC) $(LIBS) -c -o $(OBJPATH)fileReader.o $(SRCPATH)fileReader.cpp

$(OBJPATH)bufferedFileReader.o: $(INCPATH)bufferedFileReader.h $(SRCPATH)bufferedFileReader.cpp
	$(CC) $(OPT) $(INC) $(LIBS) -c -o $(OBJPATH)bufferedFileReader.o $(SRCPATH)bufferedFileReader.cpp

$(OBJPATH)fileHandler.o: $(INCPATH)defines.h $(INCPATH)fileHandler.h $(SRCPATH)fileHandler.cpp
	$(CC) $(OPT) $(INC) $(LIBS) -c -o $(OBJPATH)fileHandler.o $(SRCPATH)fileHandler.cpp

$(OBJPATH)fileWriter.o: $(INCPATH)defines.h $(INCPATH)fileWriter.h $(SRCPATH)fileWriter.cpp
	$(CC) $(OPT) $(INC) $(LIBS) -c -o $(OBJPATH)fileWriter.o $(SRCPATH)fileWriter.cpp

$(OBJPATH)raidRecover.o: $(INCPATH)defines.h $(INCPATH)raidRecover.h $(SRCPATH)raidRecover.cpp
	$(CC) $(OPT) $(INC) $(LIBS) -c -o $(OBJPATH)raidRecover.o $(SRCPATH)raidRecover.cpp

clean:
	rm -rf $(OBJPATH) $(BINPATH)
