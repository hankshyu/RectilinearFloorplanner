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

double HYPERPARAM_POR_USAGE = 1.0; // Will downcast to boolean

double HYPERPARAM_LEG_MAX_COST_CUTOFF = 5000.0;

double HYPERPARAM_LEG_OB_AREA_WEIGHT = 30.0;
double HYPERPARAM_LEG_OB_UTIL_WEIGHT = 100.0;
double HYPERPARAM_LEG_OB_ASP_WEIGHT = 70.0;
double HYPERPARAM_LEG_OB_UTIL_POS_REIN = -500.0;

double HYPERPARAM_LEG_BW_UTIL_WEIGHT = 1000.0;
double HYPERPARAM_LEG_BW_UTIL_POS_REIN = -500.0;
double HYPERPARAM_LEG_BW_ASP_WEIGHT = 70.0;

double HYPERPARAM_LEG_BB_AREA_WEIGHT = 10.0;
double HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT = 500.0;
double HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN = -500.0;
double HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT = 500.0;
double HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN = -500.0;
double HYPERPARAM_LEG_BB_ASP_WEIGHT = 30.0;
double HYPERPARAM_LEG_BB_FLAT_COST = 5.0;

// Legal Mode 0: resolve area big -> area small
// Legal Mode 1: resolve area small -> area big
// Legal Mode 2: resolve overlaps near center -> outer edge (euclidean distance)
// Legal Mode 3: completely random
// Legal Mode 4: resolve overlaps near center -> outer edge (manhattan distance)
double HYPERPARAM_LEG_LEGALMODE = 0.0; // Will downcast to int




bool DONE_GLOBAL = false;
flen_t HPWL_DONE_GLOBAL = -1;
double OVERLAP_DONE_GLOBAL = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_GLOBAL;
std::chrono::steady_clock::time_point TIME_POINT_END_GLOBAL;

bool DONE_CS = false;
flen_t HPWL_DONE_CS = -1;
double OVERLAP_DONE_CS = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_CS;
std::chrono::steady_clock::time_point TIME_POINT_END_CS;

bool DONE_POR = false;
flen_t HPWL_DONE_POR = -1;
double OVERLAP_DONE_POR = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_POR;
std::chrono::steady_clock::time_point TIME_POINT_END_POR;

bool DONE_LEG = false;
flen_t HPWL_DONE_LEG = -1;
double OVERLAP_DONE_LEG = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_LEG;
std::chrono::steady_clock::time_point TIME_POINT_END_LEG;

bool DONE_REF = false;
flen_t HPWL_DONE_REF = -1;
double OVERLAP_DONE_REF = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_REF;
std::chrono::steady_clock::time_point TIME_POINT_END_REF;

void programExitSuccessTask();
void programExitFailTask();
void printTimingHPWLReport();
void showChange(double before, double after, char *result);


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
    HPWL_DONE_GLOBAL = solver.calcEstimatedHPWL();
    OVERLAP_DONE_GLOBAL = solver.calcOverlapRatio();
    if(!DONE_GLOBAL) programExitFailTask();


    /* PHASE 2: Port Global Result to infrastructure */
    TIME_POINT_START_CS = std::chrono::steady_clock::now();
    Floorplan floorplan(globalResult, SPEC_ASPECT_RATIO_MIN, SPEC_ASPECT_RATIO_MAX, SPEC_UTILIZATION_MIN);
    TIME_POINT_END_CS = std::chrono::steady_clock::now();
    HPWL_DONE_CS = floorplan.calculateHPWL();
    OVERLAP_DONE_CS = floorplan.calculateOverlapRatio();
    DONE_CS = true;


    /* PHASE 3: Primitive Overlap Removal */
    if(bool(HYPERPARAM_POR_USAGE)){
        std::cout << std::endl;
        TIME_POINT_START_POR = std::chrono::steady_clock::now();
        floorplan.removePrimitiveOvelaps(true);
        TIME_POINT_END_POR = std::chrono::steady_clock::now();
        HPWL_DONE_POR = floorplan.calculateHPWL();
        OVERLAP_DONE_POR = floorplan.calculateOverlapRatio();
        DONE_POR = true;
        std::cout << std::endl;
    }else{
        HPWL_DONE_POR = HPWL_DONE_CS;
    }


    /* PHASE 4: DFSL Legaliser: Overlap Migration via Graph Traversal */

    // read config file 
    DFSL::ConfigList configs;
    configs.initAllConfigs();
    configs.setConfigValue<double>("MaxCostCutoff"      , HYPERPARAM_LEG_MAX_COST_CUTOFF);

    configs.setConfigValue<double>("OBAreaWeight"       , HYPERPARAM_LEG_OB_AREA_WEIGHT);
    configs.setConfigValue<double>("OBUtilWeight"       , HYPERPARAM_LEG_OB_UTIL_WEIGHT);
    configs.setConfigValue<double>("OBAspWeight"        , HYPERPARAM_LEG_OB_ASP_WEIGHT);
    configs.setConfigValue<double>("OBUtilPosRein"      , HYPERPARAM_LEG_OB_UTIL_POS_REIN);

    configs.setConfigValue<double>("BWUtilWeight"       ,HYPERPARAM_LEG_BW_UTIL_WEIGHT);
    configs.setConfigValue<double>("BWUtilPosRein"      ,HYPERPARAM_LEG_BW_UTIL_POS_REIN);
    configs.setConfigValue<double>("BWAspWeight"        ,HYPERPARAM_LEG_BW_ASP_WEIGHT);

    configs.setConfigValue<double>("BBAreaWeight"       ,HYPERPARAM_LEG_BB_AREA_WEIGHT);
    configs.setConfigValue<double>("BBFromUtilWeight"   ,HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT);
    configs.setConfigValue<double>("BBFromUtilPosRein"  ,HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN);
    configs.setConfigValue<double>("BBToUtilWeight"     ,HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT);
    configs.setConfigValue<double>("BBToUtilPosRein"    ,HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN);
    configs.setConfigValue<double>("BBAspWeight"        ,HYPERPARAM_LEG_BB_ASP_WEIGHT);
    configs.setConfigValue<double>("BBFlatCost"         ,HYPERPARAM_LEG_BB_FLAT_COST);

    configs.setConfigValue<int>("OutputLevel", DFSL::DFSL_PRINTLEVEL::DFSL_WARNING);
    
    // Create Legalise Object
    DFSL::DFSLegalizer dfsl;
    dfsl.initDFSLegalizer(&floorplan, &configs);
    // dfsl.printTiledFloorplan(initFloorplanPath, (const std::string)casename);
    DFSL::RESULT legalResult = DFSL::RESULT::OVERLAP_NOT_RESOLVED;

    // start legalizing
    TIME_POINT_START_LEG = std::chrono::steady_clock::now();
    legalResult = dfsl.legalize(int(HYPERPARAM_LEG_LEGALMODE));
    TIME_POINT_END_LEG = std::chrono::steady_clock::now();
    if(legalResult == DFSL::RESULT::SUCCESS){
        HPWL_DONE_LEG = floorplan.calculateHPWL(); 
        OVERLAP_DONE_LEG = floorplan.calculateOverlapRatio();
        DONE_LEG = true;
    }else{
        programExitFailTask();
    }

    programExitSuccessTask();
}

void programExitSuccessTask(){
    std::cout << "Timing Report" << std::endl;
    printTimingHPWLReport();
    std::cout << YELLOW << "Program Terminate successfully" << COLORRST << std::endl;
    exit(0);
}

void programExitFailTask(){
    std::cout << MAGENTA << "Program Fail to resolve task" << COLORRST << std::endl;
    abort();
    exit(4);
}

void printTimingHPWLReport(){

    long long globalRuntimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(TIME_POINT_END_GLOBAL - TIME_POINT_START_GLOBAL).count();
    double globalRuntimeS = globalRuntimeMS / 1000.0;

    long long csRunTimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(TIME_POINT_END_CS - TIME_POINT_START_CS).count();
    double csRuntimeS = csRunTimeMS / 1000.0;
    
    long long porRuntimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(TIME_POINT_END_POR - TIME_POINT_START_POR).count();
    double porRuntimeS = porRuntimeMS / 1000.0;

    long long legRuntimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(TIME_POINT_END_LEG - TIME_POINT_START_LEG).count();
    double legRuntimeS = legRuntimeMS / 1000.0;

    long long refRuntimeMS = std::chrono::duration_cast<std::chrono::milliseconds>(TIME_POINT_END_REF - TIME_POINT_START_REF).count();
    double refRuntimeS = refRuntimeMS / 1000.0;

    double totalRuntimeS = globalRuntimeS + csRuntimeS + porRuntimeS + legRuntimeS +refRuntimeS;
    double globalRuntimePTG =  100.0 * globalRuntimeS / totalRuntimeS;
    double csRuntimePTG = 100.0 * csRuntimeS / totalRuntimeS;
    double porRuntimePTG = 100.0 * porRuntimeS / totalRuntimeS;
    double legRuntimePTG = 100.0 * legRuntimeS / totalRuntimeS;
    double refRuntimePTG = 100.0 * refRuntimeS / totalRuntimeS;

    char globalDeltaHPWL [17], csDeltaHPWL [17], porDeltaHPWL [17], legDeltaHPWL [17], refDeltaHPWL [17];
    showChange(HPWL_DONE_GLOBAL, HPWL_DONE_GLOBAL, globalDeltaHPWL);
    showChange(HPWL_DONE_GLOBAL, HPWL_DONE_CS, csDeltaHPWL);
    showChange(HPWL_DONE_CS, HPWL_DONE_POR, porDeltaHPWL);
    showChange(HPWL_DONE_POR, HPWL_DONE_LEG, legDeltaHPWL);
    showChange(HPWL_DONE_LEG, HPWL_DONE_REF, refDeltaHPWL);

    char globalDeltaOVERLAP [17], csDeltaOVERLAP [17], porDeltaOVERLAP [17], legDeltaOVERLAP [17], refDeltaOVERLAP [17];
    showChange(OVERLAP_DONE_GLOBAL, OVERLAP_DONE_GLOBAL, globalDeltaOVERLAP);
    showChange(OVERLAP_DONE_GLOBAL, OVERLAP_DONE_CS, csDeltaOVERLAP);
    showChange(OVERLAP_DONE_CS, OVERLAP_DONE_POR, porDeltaOVERLAP);
    showChange(OVERLAP_DONE_POR, OVERLAP_DONE_LEG, legDeltaOVERLAP);
    showChange(OVERLAP_DONE_LEG, OVERLAP_DONE_REF, refDeltaOVERLAP);

    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ Stage                     │     Runtime (s)      |            HPWL            |       Overlap        ║\n");
    printf("╟───────────────────────────|──────────────────────|────────────────────────────|──────────────────────╢\n");
    printf("║ Global Floorplan          │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s ║\n",
        globalRuntimeS, globalRuntimePTG, HPWL_DONE_GLOBAL, globalDeltaHPWL, 100 * OVERLAP_DONE_GLOBAL, globalDeltaOVERLAP);
    printf("║ Infrastructure Building   │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s ║\n",
        csRuntimeS, csRuntimePTG, HPWL_DONE_CS, csDeltaHPWL, 100 * OVERLAP_DONE_CS, csDeltaOVERLAP);

    if(DONE_POR){
        printf("║ Primitive Overlap Removal │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s ║\n",
            porRuntimeS, porRuntimePTG, HPWL_DONE_POR, porDeltaHPWL, 100 * OVERLAP_DONE_POR, porDeltaOVERLAP);
    }

    printf("║ DFSL Legaliser            │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s ║\n",
        legRuntimeS, legRuntimePTG, HPWL_DONE_LEG, legDeltaHPWL, 100 * OVERLAP_DONE_LEG, legDeltaOVERLAP);
    printf("║ Refine Engine             │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s ║\n",
        refRuntimeS, refRuntimePTG, HPWL_DONE_REF, refDeltaHPWL, 100 * OVERLAP_DONE_LEG, legDeltaOVERLAP);

    printf("╚══════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
}

 void showChange(double before, double after, char *result){
    
    double delta = after - before;
    if(delta >= 0){
        snprintf(result, 12,  "(+%7.4f%%)", 100 * delta/before);
        
    }else{
        delta = - delta;
        snprintf(result, 12, "(-%7.4f%%)", 100 * delta/before);
    }
}