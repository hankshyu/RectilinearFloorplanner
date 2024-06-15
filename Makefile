SRCPATH = ./src
GBL_SRCPATH = $(SRCPATH)/globalFloorplanner
INF_SRCPATH = $(SRCPATH)/infrastructure
LEG_SRCPATH = $(SRCPATH)/legaliser
REF_SRCPATH = $(SRCPATH)/refiner
BINPATH = ./bin
OBJPATH = ./obj
BOOSTPATH = ./lib/boost_1_84_0/
# GLPKPATH = ./lib/glpk-5.0/src/

# CXX = /usr/bin/g++
CXX = g++
FLAGS = -std=c++17 -I $(GBL_SRCPATH) -I $(INF_SRCPATH) -I $(LEG_SRCPATH) -I $(REF_SRCPATH) -DNDEBUG
CFLAGS = -c 
OPTFLAGS = -O3
DEBUGFLAGS = -g
LINKFLAGS = -lm
# LINKFLAGS = -lglpk -lm 
# GLPKLINKPATH = /usr/local/lib


GBL_OBJS = parser.o cluster.o globmodule.o rgsolver.o

INF_OBJS = cSException.o units.o cord.o rectangle.o doughnutPolygon.o doughnutPolygonSet.o \
	tile.o line.o lineTile.o Segment.o eVector.o \
	connection.o  globalResult.o legalResult.o cornerStitching.o rectilinear.o floorplan.o

LEG_OBJS = DFSLConfig.o DFSLEdge.o DFSLegalizer.o DFSLNode.o

REF_OBJS = refineEngine.o

_OBJS = main.o $(GBL_OBJS) $(INF_OBJS) $(LEG_OBJS) $(REF_OBJS)

OBJS = $(patsubst %,$(OBJPATH)/%,$(_OBJS))
DBG_OBJS = $(patsubst %.o, $(OBJPATH)/%_dbg.o, $(_OBJS))

all: rfrun
debug: rfrun_debug

rfrun: $(OBJS)
	$(CXX) $(FLAGS) -L $(GLPKLINKPATH) $(LINKFLAGS) $^ -o $(BINPATH)/$@

$(OBJPATH)/main.o: $(SRCPATH)/main.cpp 
	$(CXX) $(FLAGS) -I $(BOOSTPATH) $(CFLAGS) $(OPTFLAGS) -DCOMPILETIME="\"`date`\"" $^ -o $@

$(OBJPATH)/%.o: $(GBL_SRCPATH)/%.cpp $(GBL_SRCPATH)/%.h
	$(CXX) $(FLAGS) -I $(BOOSTPATH) $(CFLAGS) $(OPTFLAGS) $< -o $@

$(OBJPATH)/%.o: $(INF_SRCPATH)/%.cpp $(INF_SRCPATH)/%.h
	$(CXX) $(FLAGS) -I $(BOOSTPATH) $(CFLAGS) $(OPTFLAGS) $< -o $@

$(OBJPATH)/%.o: $(LEG_SRCPATH)/%.cpp $(LEG_SRCPATH)/%.h
	$(CXX) $(FLAGS) -I $(BOOSTPATH) $(CFLAGS) $(OPTFLAGS) $< -o $@

$(OBJPATH)/%.o: $(REF_SRCPATH)/%.cpp $(REF_SRCPATH)/%.h
	$(CXX) $(FLAGS) -I $(BOOSTPATH) $(CFLAGS) $(OPTFLAGS) $< -o $@


rfrun_debug: $(DBG_OBJS)
	$(CXX) $(FLAGS) $(DEBUGFLAGS) $(LINKFLAGS) $^ -o $(BINPATH)/$@

$(OBJPATH)/main_dbg.o: $(SRCPATH)/main.cpp 
	$(CXX) $(FLAGS) $(DEBUGFLAGS) -I $(BOOSTPATH) $(CFLAGS) -DCOMPILETIME="\"`date`\"" $^ -o $@

$(OBJPATH)/%_dbg.o: $(GBL_SRCPATH)/%.cpp $(GBL_SRCPATH)/%.h
	$(CXX) $(FLAGS) $(DEBUGFLAGS) -I $(BOOSTPATH) $(CFLAGS) $< -o $@

$(OBJPATH)/%_dbg.o: $(INF_SRCPATH)/%.cpp $(INF_SRCPATH)/%.h
	$(CXX) $(FLAGS) $(DEBUGFLAGS) -I $(BOOSTPATH) $(CFLAGS) $< -o $@

$(OBJPATH)/%_dbg.o: $(LEG_SRCPATH)/%.cpp $(LEG_SRCPATH)/%.h
	$(CXX) $(FLAGS) $(DEBUGFLAGS) -I $(BOOSTPATH) $(CFLAGS) $< -o $@

$(OBJPATH)/%_dbg.o: $(REF_SRCPATH)/%.cpp $(REF_SRCPATH)/%.h
	$(CXX) $(FLAGS) $(DEBUGFLAGS) -I $(BOOSTPATH) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJPATH)/* $(BINPATH)/* 
