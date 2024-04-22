# CXX = /usr/bin/g++
CXX = g++
FLAGS = -std=c++17
CFLAGS = -c -w
OPTFLAGS = -o3
DEBUGFLAGS = -g

SRCPATH = ./src
BINPATH = ./bin
OBJPATH = ./obj
BOOSTPATH = ./lib/boost_1_84_0/ # ! Specify boost library path
GLPKPATH = ./lib/glpk-5.0/src/  # ! Sepcify glpk

all: rfrun
debug: rfrun_debug

# LINKFLAGS = -pedantic -Wall -fomit-frame-pointer -funroll-all-loops -O3
LINKFLAGS = -lglpk -lm 
GLPKLINKPATH = /usr/local/lib

_OBJS =	main.o \
 	cSException.o units.o cord.o rectangle.o doughnutPolygon.o doughnutPolygonSet.o \
	tile.o line.o lineTile.o Segment.o eVector.o \
	connection.o  globalResult.o legalResult.o cornerStitching.o rectilinear.o floorplan.o \
	DFSLConfig.o DFSLEdge.o DFSLegalizer.o DFSLNode.o \
	refineEngine.o  
		       


OBJS = $(patsubst %,$(OBJPATH)/%,$(_OBJS))
DBG_OBJS = $(patsubst %.o, $(OBJPATH)/%_dbg.o, $(_OBJS))

rfrun: $(OBJS)
	$(CXX) $(FLAGS) -L $(GLPKLINKPATH) $(LINKFLAGS) $^ -o $(BINPATH)/$@

$(OBJPATH)/main.o: $(SRCPATH)/main.cpp 
	$(CXX) $(FLAGS) -I $(BOOSTPATH) -I $(GLPKPATH) $(CFLAGS) $(OPTFLAGS) -DCOMPILETIME="\"`date`\"" $^ -o $@

$(OBJPATH)/%.o: $(SRCPATH)/%.cpp $(SRCPATH)/%.h
	$(CXX) $(FLAGS) -I $(BOOSTPATH) -I $(GLPKPATH) $(CFLAGS) $(OPTFLAGS) $< -o $@


rfrun_debug: $(DBG_OBJS)
	$(CXX) $(DEBUGFLAGS) -L $(GLPKLINKPATH) $(LINKFLAGS) $^ -o $(BINPATH)/$@

$(OBJPATH)/main_dbg.o: $(SRCPATH)/main.cpp 
	$(CXX) $(DEBUGFLAGS) -I $(BOOSTPATH) -I $(GLPKPATH) $(CFLAGS) -DCOMPILETIME="\"`date`\"" $^ -o $@

$(OBJPATH)/%_dbg.o: $(SRCPATH)/%.cpp $(SRCPATH)/%.h
	$(CXX) $(DEBUGFLAGS) -I $(BOOSTPATH) -I $(GLPKPATH) $(CFLAGS) $< -o $@



.PHONY: clean
clean:
	rm -rf *.gch *.out $(OBJPATH)/* $(BINPATH)/* 

