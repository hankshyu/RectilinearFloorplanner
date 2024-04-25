#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <cfloat>
#include <stdio.h>
#include <unistd.h>
#include <chrono>

#include "parser.h"
#include "rgsolver.h"
#include "globalResult.h"
#include "cSException.h"
#include "floorplan.h"
#include "colours.h"
#include "DFSLegalizer.h"
#include "refineEngine.h"

std::string inputFileName;

double SPEC_ASPECT_RATIO_MIN = 0.5;
double SPEC_ASPECT_RATIO_MAX = 2.0;
double SPEC_UTILIZATION_MIN = 0.8;


double HYPERPARAM_GLOBAL_PUNISHMENT = 0.0054730000000000004;
double HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE = 0.9;
double HYPERPARAM_GLOBAL_LEARNING_RATE = 46e-4;

bool DONE_GLOBAL = false;
std::chrono::steady_clock::time_point TIME_POINT_START_GLOBAL;
std::chrono::steady_clock::time_point TIME_POINT_END_GLOBAL;

bool DONE_CS = false;
std::chrono::steady_clock::time_point TIME_POINT_START_CS;
std::chrono::steady_clock::time_point TIME_POINT_END_CS;

bool DONE_POR = false;
std::chrono::steady_clock::time_point TIME_POINT_START_POR;
std::chrono::steady_clock::time_point TIME_POINT_END_POR;

bool DONE_LEG = false;
std::chrono::steady_clock::time_point TIME_POINT_START_LEG;
std::chrono::steady_clock::time_point TIME_POINT_END_LEG;

bool DONE_REF = false;
std::chrono::steady_clock::time_point TIME_POINT_START_REF;
std::chrono::steady_clock::time_point TIME_POINT_END_REF;


int main(int argc, char *argv[]) {
    // parsing command line arguments
    std::string inputFileName = "./inputs/rawInputs/case02-input.txt";
    
    /* PHASE 1: Global Floorplanning */
    Parser parser;
    GlobalSolver solver;

    if (!parser.parseInput(inputFileName, SPEC_ASPECT_RATIO_MAX, HYPERPARAM_GLOBAL_PUNISHMENT, HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE)) {
        std::cout << "[GlobalSolver] ERROR: Input file does not exist." << std::endl;
        return -1;
    }
    solver.readFloorplan(parser);
    solver.readConfig(parser);

    // learning rate: case02&09: 50e-4, others: 2e-4;
    solver.setLearningRate(HYPERPARAM_GLOBAL_LEARNING_RATE);

    TIME_POINT_START_GLOBAL = std::chrono::steady_clock::now();
    solver.solve();
    solver.markExtremeOverlap();
    if (parser.getInflationRatio() > 0) solver.inflate();
    solver.finetune();
    TIME_POINT_END_GLOBAL = std::chrono::steady_clock::now();

    GlobalResult globalResult;
    DONE_GLOBAL = solver.exportGlobalFloorplan(globalResult);
    if(!DONE_GLOBAL) goto PROG_EXIT_FAIL;

    /* PHASE 2: Port Global Result to infrastructure */



PORG_EXIT_SUCCESS:{
    std::cout << "Timing Report" << std::endl;
    


    std::cout << YELLOW << "Program Terminate successfully" << COLORRST << std::endl;
    return 0;
}
PROG_EXIT_FAIL:{



    return 4;
}

}
