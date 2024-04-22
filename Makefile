CXX = /usr/bin/g++
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
		connection.o cord.o cornerStitching.o cSException.o \
		DFSLConfig.o DFSLEdge.o DFSLegalizer.o DFSLNode.o \
		doughnutPolygon.o doughnutPolygonSet.o floorplan.o globalResult.o units.o \
		line.o rectangle.o rectilinear.o Segment.o tile.o eVector.o refineEngine.o lineTile.o \
		legalResult.o


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

