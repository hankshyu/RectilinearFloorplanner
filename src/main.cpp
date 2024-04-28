#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <cfloat>
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <cctype>


#include "parser.h"
#include "rgsolver.h"
#include "globalResult.h"
#include "cSException.h"
#include "floorplan.h"
#include "colours.h"
#include "DFSLegalizer.h"
#include "refineEngine.h"

std::string INPUT_FILE_NAME = "";
std::string CONFIG_FILE_NAME = "";
std::string OUTPUT_FILE_NAME = "";

std::ifstream cifs;
std::ofstream ofs;

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

double HYPERPARAM_LEG_BW_UTIL_WEIGHT = 1000;
double HYPERPARAM_LEG_BW_UTIL_POS_REIN = -500;
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
double HYPERPARAM_LEG_LEGALMODE = 2;


double REFINE_ENGINE_GRID_SEARCH = 1.0; // will cast to bool

double HYPERPARAM_REF_MAX_ITERATION = 30; // will downcast to int
double HYPERPARAM_REF_USE_GRADIENT_ORDER = 1.0; // will downcast to bool
double HYPERPARAM_REF_INITIAL_MOMENTUM = 2.0; // will downcast to len_t
double HYPERPARAM_REF_MOMENTUM_GROWTH = 2.0;
double HYPERPARAM_REF_USE_GRADIENT_GROW = 1.0; // will downcast to bool
double HYPERPARAM_REF_USE_GRADIENT_SHRINK = 1.0; // will downcast to bool

int DONE_GLOBAL = 0;
flen_t HPWL_DONE_GLOBAL = -1;
double OVERLAP_DONE_GLOBAL = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_GLOBAL;
std::chrono::steady_clock::time_point TIME_POINT_END_GLOBAL;

int DONE_CS = 0;
bool LEGAL_CS = false;
flen_t HPWL_DONE_CS = -1;
double OVERLAP_DONE_CS = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_CS;
std::chrono::steady_clock::time_point TIME_POINT_END_CS;

int DONE_POR = 0;
bool LEGAL_POR = false;
flen_t HPWL_DONE_POR = -1;
double OVERLAP_DONE_POR = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_POR;
std::chrono::steady_clock::time_point TIME_POINT_END_POR;

int DONE_LEG = 0;
bool LEGAL_LEG = false;
flen_t HPWL_DONE_LEG = -1;
double OVERLAP_DONE_LEG = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_LEG;
std::chrono::steady_clock::time_point TIME_POINT_END_LEG;

int DONE_REF = 0;
bool LEGAL_REF = false;
flen_t HPWL_DONE_REF = -1;
double OVERLAP_DONE_REF = -1;
std::chrono::steady_clock::time_point TIME_POINT_START_REF;
std::chrono::steady_clock::time_point TIME_POINT_END_REF;

double TOTAL_RUNTIME_IN_SECONDS = -1;

void parseConfigurations();

void runGlobalFloorplanning(GlobalResult &globalResult);
Floorplan *runCSInsertion(GlobalResult &globalResult);
void runPrimitiveOverlapRemoval(Floorplan *&floorplan);
void runLegalization(Floorplan *&floorplan);
void runRerineEngine(Floorplan *&floorplan);


void programExitSuccessTask(Floorplan *&floorplan);
void programExitFailTask(int failStage);
void printTimingHPWLReport(Floorplan *&floorplan);
void showChange(double before, double after, char *result);


int main(int argc, char *argv[]) {


	// parsing input argument

	int opt;

    // Parsing command line arguments using getopt
    while ((opt = getopt(argc, argv, "i:o:c:")) != -1) {
        switch (opt) {
            case 'i':
                INPUT_FILE_NAME = optarg;
                break;
            case 'o':
				OUTPUT_FILE_NAME = optarg;
                break;
            case 'c':
				CONFIG_FILE_NAME = optarg;
                break;
            case '?':
                std::cout << "Unknown option or missing argument" << std::endl;
                return 4;
            default:
                break;
        }
    }

	if(INPUT_FILE_NAME.empty()){
		std::cout << "[Missing Arguments] Missing Input File " << std::endl;
		exit(4);
	}else std::cout << "Input File: " << INPUT_FILE_NAME << std::endl;

	if(CONFIG_FILE_NAME.empty()){
		std::cout << "[Missing Arguments] Missing Configuration File " << std::endl;
		exit(4);
	}else std::cout << "Input File: " << CONFIG_FILE_NAME << std::endl;
	cifs.open(CONFIG_FILE_NAME);
	if(!cifs.is_open()){
		std::cout << "Configuraton Stream Cannot Open" << std::endl;
		exit(4);
	}

	if(OUTPUT_FILE_NAME.empty()){
		std::cout << "[Missing Arguments] Missing Output File " << std::endl;
		exit(4);
	}else std::cout << "Output File: " << OUTPUT_FILE_NAME << std::endl;
	ofs.open(OUTPUT_FILE_NAME);
	if(!ofs.is_open()){
		std::cout << "Output Stream Open Fail" << std::endl;
		exit(4);
	}

	// parse input configs
	parseConfigurations();


    rectilinearIllegalType rit;
    /* PHASE 1: Global Floorplanning */
    GlobalResult globalResult;
    runGlobalFloorplanning(globalResult);


	/* PHASE 2: Port Global Result to infrastructure */
	Floorplan *floorplan;
	floorplan = runCSInsertion(globalResult);

	if(floorplan->checkFloorplanLegal(rit) == nullptr){
		std::cout << std::endl << "[CSInsertion] Complete, floorplan Legal" << std::endl << std::endl;
		LEGAL_CS = true;
		LEGAL_POR = true;
		LEGAL_LEG = true;
		DONE_POR = 2;
		DONE_LEG = 2;
		HPWL_DONE_LEG = HPWL_DONE_CS;
		OVERLAP_DONE_LEG = HPWL_DONE_CS;

		goto PHASE_REFINE_ENGINE;
	
	}else{
		std::cout << std::endl << "[CSInsertion] Complete, floorplan NOT Legal" << std::endl << std::endl;
	}


	/* PHASE 3: Primitive Overlap Removal */
	runPrimitiveOverlapRemoval(floorplan);
	if(floorplan->checkFloorplanLegal(rit) == nullptr){
		std::cout << std::endl << "[PrimitiveOverlapRemoval] Complete, floorplan Legal" << std::endl << std::endl;
		LEGAL_POR = true;
		LEGAL_LEG = true;
		DONE_LEG = 2;
		HPWL_DONE_LEG = HPWL_DONE_POR;
		OVERLAP_DONE_LEG = HPWL_DONE_POR;

		goto PHASE_REFINE_ENGINE;
	
	}else{
		std::cout << std::endl << "[PrimitiveOverlapRemoval] Complete, floorplan NOT Legal" << std::endl << std::endl;

	}

	/* PHASE 4: DFSL Legaliser: Overlap Migration via Graph Traversal */
	runLegalization(floorplan);


	// floorplan->visualiseFloorplan("./outputs/case07-legalised.txt");


PHASE_REFINE_ENGINE:{
	std::cout << std::endl;	
        /* PHASE 5: Refinement Engine */
	if(bool(REFINE_ENGINE_GRID_SEARCH)){

        Floorplan *refineBestFloorplan = floorplan;
        flen_t refineBestHPWL = FLEN_T_MAX;
		LEGAL_REF = (floorplan->checkFloorplanLegal(rit) == nullptr);
        bool refineBestUseGradientOrder;
        len_t refineBestMomentum;
        double refineBestMomentumGrowth;
        bool refineBestGrowGradient;
        bool refineBestShrinkGradient;

        // Try momentum setup: (init growth) = (1, 2), (2, 1.5), (2, 1.75), (2, 2), (4, 2) (8, 2)
        for(int xMomentum = 0; xMomentum < 4; ++xMomentum){
            len_t expMomentum;
            double expMomentumGrowth;
            if(xMomentum == 0){
                expMomentum = 1;
                expMomentumGrowth = 2;
            }else if(xMomentum == 1){
                expMomentum = 2;
                expMomentumGrowth = 1.5;
            }else if(xMomentum == 2){ 
                expMomentum = 2;
                expMomentumGrowth = 1.75;
            }else if(xMomentum == 3){
                expMomentum = 2;
                expMomentumGrowth = 2;
            }else if(xMomentum == 4){
                expMomentum = 4;
                expMomentumGrowth = 2;
            }else{
                expMomentum = 8;
                expMomentumGrowth = 2;
			}
            
            for(int xGradient = 0; xGradient < 4; ++ xGradient){
                bool expGrowGradient = xGradient / 2;
                bool expShrinkGradient = xGradient % 2;
                for(int xGradientOrder = 0; xGradientOrder < 2; ++xGradientOrder){
                    bool expUseGradientOrder = bool(xGradientOrder);
                    Floorplan *expFloorplan = new Floorplan(*floorplan);

                    std::chrono::steady_clock::time_point expStartTime = std::chrono::steady_clock::now();
                    RefineEngine expRF(expFloorplan, 30,expUseGradientOrder, expMomentum, expMomentumGrowth, expGrowGradient, expShrinkGradient);
                    expFloorplan = expRF.refine();
                    std::chrono::steady_clock::time_point expEndTime = std::chrono::steady_clock::now();

                    flen_t expResult = expFloorplan->calculateHPWL();
					bool expResultLegal = (expFloorplan->checkFloorplanLegal(rit) == nullptr);

					if(expResultLegal){
						printf("[LegaliseEngine] Legal floorplan Found Using (%d, %d, %4.2lf, %d, %d), HPWL = %-14.2lf\n",
						bool(expUseGradientOrder), len_t(expMomentum), expMomentumGrowth, 
						bool(expGrowGradient), bool(expShrinkGradient), expResult);
					}else{

						printf("[LegaliseEngine] Legal floorplan Found Using (%d, %d, %4.2lf, %d, %d)\n",
						bool(expUseGradientOrder), len_t(expMomentum), expMomentumGrowth, 
						bool(expGrowGradient), bool(expShrinkGradient));
					}

                    if(expResult < refineBestHPWL && expResultLegal){
                        if(refineBestHPWL != FLEN_T_MAX) delete refineBestFloorplan;
						LEGAL_REF = true;
                        refineBestFloorplan = expFloorplan;
                        refineBestHPWL = expResult;
                        refineBestUseGradientOrder = expUseGradientOrder;
                        refineBestMomentum = expMomentum;
                        refineBestMomentumGrowth = expMomentumGrowth;
                        refineBestGrowGradient = expGrowGradient;
                        refineBestShrinkGradient = expShrinkGradient;

                        TIME_POINT_START_REF = expStartTime; 
                        TIME_POINT_END_REF = expEndTime;
                    }else{
                        delete expFloorplan;
                    }
                }
            }
        }
		floorplan = refineBestFloorplan;
        HPWL_DONE_REF = refineBestHPWL;
        OVERLAP_DONE_REF = refineBestFloorplan->calculateOverlapRatio();
        DONE_REF = 1;

		double HYPERPARAM_REF_USE_GRADIENT_ORDER = (refineBestUseGradientOrder)? 1.0 : 0.0;
		double HYPERPARAM_REF_INITIAL_MOMENTUM = double(refineBestMomentum);
		double HYPERPARAM_REF_MOMENTUM_GROWTH = refineBestMomentumGrowth;
		double HYPERPARAM_REF_USE_GRADIENT_GROW = (refineBestGrowGradient)? 1.0 : 0.0;
		double HYPERPARAM_REF_USE_GRADIENT_SHRINK = (refineBestShrinkGradient)? 1.0 : 0.0;

        // refineBestFloorplan->visualiseFloorplan("./outputs/case05-refined.txt");

	}else{
        TIME_POINT_START_REF = std::chrono::steady_clock::now();
        // RefineEngine refineEngine(floorplan, 0.3, 30, false, 2, 1.75, true, false);
        RefineEngine refineEngine(floorplan, int(HYPERPARAM_REF_MAX_ITERATION), bool(HYPERPARAM_REF_USE_GRADIENT_ORDER),
			len_t(HYPERPARAM_REF_INITIAL_MOMENTUM), HYPERPARAM_REF_MOMENTUM_GROWTH, bool(HYPERPARAM_REF_USE_GRADIENT_GROW), bool(HYPERPARAM_REF_USE_GRADIENT_SHRINK));

        floorplan = refineEngine.refine();
        TIME_POINT_END_REF = std::chrono::steady_clock::now();

		LEGAL_REF = (floorplan->checkFloorplanLegal(rit) == nullptr);
        HPWL_DONE_REF = floorplan->calculateHPWL();
        OVERLAP_DONE_REF = floorplan->calculateOverlapRatio();
        DONE_REF = 1;
		if(LEGAL_REF){
			printf("[LegaliseEngine] Legal floorplan Found Using (%d, %d, %4.2lf, %d, %d), HPWL = %-14.2lf\n",
			bool(HYPERPARAM_REF_USE_GRADIENT_ORDER), len_t(HYPERPARAM_REF_INITIAL_MOMENTUM), HYPERPARAM_REF_MOMENTUM_GROWTH, 
			bool(HYPERPARAM_REF_USE_GRADIENT_GROW), bool(HYPERPARAM_REF_USE_GRADIENT_SHRINK), HPWL_DONE_REF);
		}else{

			printf("[LegaliseEngine] Legal floorplan Found Using (%d, %d, %4.2lf, %d, %d)\n",
			bool(HYPERPARAM_REF_USE_GRADIENT_ORDER), len_t(HYPERPARAM_REF_INITIAL_MOMENTUM), HYPERPARAM_REF_MOMENTUM_GROWTH, 
			bool(HYPERPARAM_REF_USE_GRADIENT_GROW), bool(HYPERPARAM_REF_USE_GRADIENT_SHRINK));
		}
	}


        // floorplan->visualiseFloorplan("./outputs/case07-refined.txt");

		std::cout << std::endl;
		if(!LEGAL_REF) programExitFailTask(5);

        programExitSuccessTask(floorplan);

}
}

void parseConfigurations(){
	std::string line;
    while (std::getline(cifs, line)) {
        // Find the position of the '=' character
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            // Extract parameter name and value
            std::string parameter_name = line.substr(0, pos);
            double parameter_value = std::stod(line.substr(pos + 1));

           // Assign the value to the corresponding global variable
			if (parameter_name.find("SPEC_ASPECT_RATIO_MIN") != std::string::npos) SPEC_ASPECT_RATIO_MIN = parameter_value;
            else if (parameter_name.find("SPEC_ASPECT_RATIO_MAX") != std::string::npos) SPEC_ASPECT_RATIO_MAX = parameter_value;
            else if (parameter_name.find("SPEC_UTILIZATION_MIN") != std::string::npos) SPEC_UTILIZATION_MIN = parameter_value;
            else if (parameter_name.find("HYPERPARAM_GLOBAL_PUNISHMENT") != std::string::npos) HYPERPARAM_GLOBAL_PUNISHMENT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE") != std::string::npos) HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE = parameter_value;
            else if (parameter_name.find("HYPERPARAM_GLOBAL_LEARNING_RATE") != std::string::npos) HYPERPARAM_GLOBAL_LEARNING_RATE = parameter_value;
            else if (parameter_name.find("HYPERPARAM_POR_USAGE") != std::string::npos) HYPERPARAM_POR_USAGE = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_MAX_COST_CUTOFF") != std::string::npos) HYPERPARAM_LEG_MAX_COST_CUTOFF = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_OB_AREA_WEIGHT") != std::string::npos) HYPERPARAM_LEG_OB_AREA_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_OB_UTIL_WEIGHT") != std::string::npos) HYPERPARAM_LEG_OB_UTIL_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_OB_ASP_WEIGHT") != std::string::npos) HYPERPARAM_LEG_OB_ASP_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_OB_UTIL_POS_REIN") != std::string::npos) HYPERPARAM_LEG_OB_UTIL_POS_REIN = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BW_UTIL_WEIGHT") != std::string::npos) HYPERPARAM_LEG_BW_UTIL_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BW_UTIL_POS_REIN") != std::string::npos) HYPERPARAM_LEG_BW_UTIL_POS_REIN = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BW_ASP_WEIGHT") != std::string::npos) HYPERPARAM_LEG_BW_ASP_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BB_AREA_WEIGHT") != std::string::npos) HYPERPARAM_LEG_BB_AREA_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT") != std::string::npos) HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN") != std::string::npos) HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT") != std::string::npos) HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN") != std::string::npos) HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BB_ASP_WEIGHT") != std::string::npos) HYPERPARAM_LEG_BB_ASP_WEIGHT = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_BB_FLAT_COST") != std::string::npos) HYPERPARAM_LEG_BB_FLAT_COST = parameter_value;
            else if (parameter_name.find("HYPERPARAM_LEG_LEGALMODE") != std::string::npos) HYPERPARAM_LEG_LEGALMODE = parameter_value;
            else if (parameter_name.find("REFINE_ENGINE_GRID_SEARCH") != std::string::npos) REFINE_ENGINE_GRID_SEARCH = parameter_value;
            else if (parameter_name.find("HYPERPARAM_REF_MAX_ITERATION") != std::string::npos) HYPERPARAM_REF_MAX_ITERATION = parameter_value;
            else if (parameter_name.find("HYPERPARAM_REF_USE_GRADIENT_ORDER") != std::string::npos) HYPERPARAM_REF_USE_GRADIENT_ORDER = parameter_value;
            else if (parameter_name.find("HYPERPARAM_REF_INITIAL_MOMENTUM") != std::string::npos) HYPERPARAM_REF_INITIAL_MOMENTUM = parameter_value;
            else if (parameter_name.find("HYPERPARAM_REF_MOMENTUM_GROWTH") != std::string::npos) HYPERPARAM_REF_MOMENTUM_GROWTH = parameter_value;
            else if (parameter_name.find("HYPERPARAM_REF_USE_GRADIENT_GROW") != std::string::npos) HYPERPARAM_REF_USE_GRADIENT_GROW = parameter_value;
            else if (parameter_name.find("HYPERPARAM_REF_USE_GRADIENT_SHRINK") != std::string::npos) HYPERPARAM_REF_USE_GRADIENT_SHRINK = parameter_value;
        }
    }
	cifs.close();
}

void runGlobalFloorplanning(GlobalResult &globalResult){
    /* PHASE 1: Global Floorplanning */
    Parser parser;
    GlobalSolver solver;

    if (!parser.parseInput(INPUT_FILE_NAME, SPEC_ASPECT_RATIO_MAX, HYPERPARAM_GLOBAL_PUNISHMENT, HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE)) {
        std::cout << "[GlobalSolver] ERROR: Input file does not exist." << std::endl;
        exit(4);
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

    DONE_GLOBAL = int(solver.exportGlobalFloorplan(globalResult));
    if(DONE_GLOBAL == 0) programExitFailTask(1);

    HPWL_DONE_GLOBAL = solver.calcEstimatedHPWL();
    OVERLAP_DONE_GLOBAL = solver.calcOverlapRatio();
}

Floorplan *runCSInsertion(GlobalResult &globalResult){
    /* PHASE 2: Port Global Result to infrastructure */
    TIME_POINT_START_CS = std::chrono::steady_clock::now();
    Floorplan *floorplan = new Floorplan(globalResult, SPEC_ASPECT_RATIO_MIN, SPEC_ASPECT_RATIO_MAX, SPEC_UTILIZATION_MIN);
    TIME_POINT_END_CS = std::chrono::steady_clock::now();
    HPWL_DONE_CS = floorplan->calculateHPWL();
    OVERLAP_DONE_CS = floorplan->calculateOverlapRatio();
    DONE_CS = 1;

    return floorplan;
}

void runPrimitiveOverlapRemoval(Floorplan *&floorplan){
	if(bool(HYPERPARAM_POR_USAGE)){
		std::cout << std::endl;
		TIME_POINT_START_POR = std::chrono::steady_clock::now();
		floorplan->removePrimitiveOvelaps(true);
		TIME_POINT_END_POR = std::chrono::steady_clock::now();
		HPWL_DONE_POR = floorplan->calculateHPWL();
		OVERLAP_DONE_POR = floorplan->calculateOverlapRatio();
		DONE_POR = true;
		std::cout << std::endl;
	}else{
		HPWL_DONE_POR = HPWL_DONE_CS;
	}
}

void runLegalization(Floorplan *&floorplan){
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

	// configs.setConfigValue<int>("OutputLevel", DFSL::DFSL_PRINTLEVEL::dfs);
	
	// Create Legalise Object
	DFSL::DFSLegalizer dfsl;
	dfsl.initDFSLegalizer(floorplan, &configs);
	// dfsl.printTiledFloorplan(initFloorplanPath, (const std::string)casename);
	DFSL::RESULT legalResult = DFSL::RESULT::OVERLAP_NOT_RESOLVED;

	// start legalizing
	TIME_POINT_START_LEG = std::chrono::steady_clock::now();
	legalResult = dfsl.legalize(int(HYPERPARAM_LEG_LEGALMODE));
	TIME_POINT_END_LEG = std::chrono::steady_clock::now();
	bool legalResultSuccess = (legalResult == DFSL::RESULT::SUCCESS);
	bool legalResultAreaFail = (legalResult == DFSL::RESULT::AREA_CONSTRAINT_FAIL);
	if(legalResultSuccess || legalResultAreaFail){
		if(legalResultSuccess) LEGAL_LEG = true;

		HPWL_DONE_LEG = floorplan->calculateHPWL(); 
		OVERLAP_DONE_LEG = floorplan->calculateOverlapRatio();
		DONE_LEG = 1;
	}else{
		programExitFailTask(4);
	}

}

void runRerineEngine(Floorplan *&floorplan){
	TIME_POINT_START_REF = std::chrono::steady_clock::now();
	// RefineEngine refineEngine(floorplan, 0.3, 30, false, 2, 1.75, true, false);
	RefineEngine refineEngine(floorplan, 30, false, 2, 1.75, true, true);

	floorplan = refineEngine.refine();
	TIME_POINT_END_REF = std::chrono::steady_clock::now();

	HPWL_DONE_REF = floorplan->calculateHPWL();
	OVERLAP_DONE_REF = floorplan->calculateOverlapRatio();
	DONE_LEG = true;
}

void programExitSuccessTask(Floorplan *&floorplan){
    printTimingHPWLReport(floorplan);
	ofs << "1" << std::endl;
	ofs << floorplan->calculateHPWL() << std::endl;
	ofs << TOTAL_RUNTIME_IN_SECONDS << std::endl;


	ofs << "SPEC_ASPECT_RATIO_MIN = " << SPEC_ASPECT_RATIO_MIN << std::endl;
    ofs << "SPEC_ASPECT_RATIO_MAX = " << SPEC_ASPECT_RATIO_MAX << std::endl;
    ofs << "SPEC_UTILIZATION_MIN = " << SPEC_UTILIZATION_MIN << std::endl;

    ofs << "HYPERPARAM_GLOBAL_PUNISHMENT = " << HYPERPARAM_GLOBAL_PUNISHMENT << std::endl;
    ofs << "HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE = " << HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE << std::endl;
    ofs << "HYPERPARAM_GLOBAL_LEARNING_RATE = " << HYPERPARAM_GLOBAL_LEARNING_RATE << std::endl;

    ofs << "HYPERPARAM_POR_USAGE = " << HYPERPARAM_POR_USAGE << std::endl;

    ofs << "HYPERPARAM_LEG_MAX_COST_CUTOFF = " << HYPERPARAM_LEG_MAX_COST_CUTOFF << std::endl;

    ofs << "HYPERPARAM_LEG_OB_AREA_WEIGHT = " << HYPERPARAM_LEG_OB_AREA_WEIGHT << std::endl;
    ofs << "HYPERPARAM_LEG_OB_UTIL_WEIGHT = " << HYPERPARAM_LEG_OB_UTIL_WEIGHT << std::endl;
    ofs << "HYPERPARAM_LEG_OB_ASP_WEIGHT = " << HYPERPARAM_LEG_OB_ASP_WEIGHT << std::endl;
    ofs << "HYPERPARAM_LEG_OB_UTIL_POS_REIN = " << HYPERPARAM_LEG_OB_UTIL_POS_REIN << std::endl;

    ofs << "HYPERPARAM_LEG_BW_UTIL_WEIGHT = " << HYPERPARAM_LEG_BW_UTIL_WEIGHT << std::endl;
    ofs << "HYPERPARAM_LEG_BW_UTIL_POS_REIN = " << HYPERPARAM_LEG_BW_UTIL_POS_REIN << std::endl;
    ofs << "HYPERPARAM_LEG_BW_ASP_WEIGHT = " << HYPERPARAM_LEG_BW_ASP_WEIGHT << std::endl;

    ofs << "HYPERPARAM_LEG_BB_AREA_WEIGHT = " << HYPERPARAM_LEG_BB_AREA_WEIGHT << std::endl;
    ofs << "HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT = " << HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT << std::endl;
    ofs << "HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN = " << HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN << std::endl;
    ofs << "HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT = " << HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT << std::endl;
    ofs << "HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN = " << HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN << std::endl;
    ofs << "HYPERPARAM_LEG_BB_ASP_WEIGHT = " << HYPERPARAM_LEG_BB_ASP_WEIGHT << std::endl;
    ofs << "HYPERPARAM_LEG_BB_FLAT_COST = " << HYPERPARAM_LEG_BB_FLAT_COST << std::endl;

    ofs << "HYPERPARAM_LEG_LEGALMODE = " << HYPERPARAM_LEG_LEGALMODE << std::endl;

    ofs << "REFINE_ENGINE_GRID_SEARCH = " << REFINE_ENGINE_GRID_SEARCH  << std::endl;

    ofs << "HYPERPARAM_REF_MAX_ITERATION = " << HYPERPARAM_REF_MAX_ITERATION << std::endl;
    ofs << "HYPERPARAM_REF_USE_GRADIENT_ORDER = " << HYPERPARAM_REF_USE_GRADIENT_ORDER << std::endl;
    ofs << "HYPERPARAM_REF_INITIAL_MOMENTUM = " << HYPERPARAM_REF_INITIAL_MOMENTUM << std::endl;
    ofs << "HYPERPARAM_REF_MOMENTUM_GROWTH = " << HYPERPARAM_REF_MOMENTUM_GROWTH << std::endl;
    ofs << "HYPERPARAM_REF_USE_GRADIENT_GROW = " << HYPERPARAM_REF_USE_GRADIENT_GROW << std::endl;
    ofs << "HYPERPARAM_REF_USE_GRADIENT_SHRINK = " << HYPERPARAM_REF_USE_GRADIENT_SHRINK << std::endl;

    exit(0);
}

void programExitFailTask(int failPhase){
	ofs << -failPhase << std::endl;
	ofs.close();	
    exit(4);
}

void printTimingHPWLReport(Floorplan *&floorplan){

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
	TOTAL_RUNTIME_IN_SECONDS = totalRuntimeS;

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

	char csLegal[30], porLegal[30], legLegal[30], refLegal[30];
	if(LEGAL_CS)strcpy(csLegal, "       LEGAL        ");
	else strcpy(csLegal, "      ILLEGAL       ");
	
	if(LEGAL_POR) strcpy(porLegal, "       LEGAL        ");
	else strcpy(porLegal, "      ILLEGAL       ");

	if(LEGAL_LEG) strcpy(legLegal, "       LEGAL        ");
	else{
		if(DONE_LEG) strcpy(legLegal, "      AREA_FAIL     ");
		else strcpy(legLegal, "     OTHER_FAIL     ");
	}

	if(LEGAL_REF) strcpy(refLegal, "       LEGAL        ");
	else{
		rectilinearIllegalType rectIllegalCause;
		floorplan->checkFloorplanLegal(rectIllegalCause);
		if(rectIllegalCause == rectilinearIllegalType::OVERLAP) strcpy(refLegal, "  OVERLAP_ILLEGAL   ");
		else if(rectIllegalCause == rectilinearIllegalType::AREA) strcpy(refLegal, "    AREA_ILLEGAL    ");
		else if(rectIllegalCause == rectilinearIllegalType::ASPECT_RATIO) strcpy(refLegal, "ASPECT_RATIO_ILLEGAL");
		else if(rectIllegalCause == rectilinearIllegalType::UTILIZATION) strcpy(refLegal, "UTILIZATION_ILLEGAL ");
		else if(rectIllegalCause == rectilinearIllegalType::HOLE) strcpy(refLegal, "    HOLE_ILLEGAL    ");
		else if(rectIllegalCause == rectilinearIllegalType::TWO_SHAPE) strcpy(refLegal, " TWO_SHAPE_ILLEGAL  ");
		else  strcpy(refLegal, "  STRANGE_ILLEGAL   ");
	}
	
	std::cout << "Report Used Hyperparameters: " << std::endl;
	std::cout << "-> Aspect Raio: " << SPEC_ASPECT_RATIO_MIN << " ~ " << SPEC_ASPECT_RATIO_MAX <<", Utilization Min = " << SPEC_UTILIZATION_MIN << std::endl << std::endl;

	std::cout << "-> Global Floorplan Used Hyperparameters" << std::endl;
	std::cout << "Punisment = " << HYPERPARAM_GLOBAL_PUNISHMENT << ", Learning Rate = " << HYPERPARAM_GLOBAL_LEARNING_RATE;
	std::cout << ", Max AspectRatio Rate = " << HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE  << std::endl << std::endl;

	std::cout << "-> Primitive Overlap Removal applied = " << HYPERPARAM_POR_USAGE << std::endl << std::endl; 

	std::cout << "-> DFSL Legaliser Hyperparameters" << std::endl;
	std::cout << "Max Cost Cutoff = " << HYPERPARAM_LEG_MAX_COST_CUTOFF << std::endl;
	std::cout << "OB Area Weight = " << HYPERPARAM_LEG_OB_AREA_WEIGHT;
	std::cout << ", OB Util Weight = " << HYPERPARAM_LEG_OB_UTIL_WEIGHT;
	std::cout << ", OB ASP Weight = " << HYPERPARAM_LEG_OB_ASP_WEIGHT;
	std::cout << ", OB Util Pos Rein = " << HYPERPARAM_LEG_OB_UTIL_POS_REIN << std::endl;
	std::cout << "BW Util Weight = " << HYPERPARAM_LEG_BW_UTIL_WEIGHT;
	std::cout << ", BW Util Pos Rein = " << HYPERPARAM_LEG_BW_UTIL_POS_REIN;
	std::cout << ", BW ASP Weight = " << HYPERPARAM_LEG_BW_ASP_WEIGHT << std::endl;
	std::cout << "BB Area Weight = " << HYPERPARAM_LEG_BB_AREA_WEIGHT;
	std::cout << ", BB From Util Weight = " << HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT;
	std::cout << ", BB From Util Pos Rein = " << HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN << std::endl;
	std::cout << "BB To Util Weight = " << HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT;
	std::cout << ", BB To Util Pos Rein = " << HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN << std::endl;
	std::cout << "BB ASP Weight = " <<HYPERPARAM_LEG_BB_ASP_WEIGHT;
	std::cout << ", BB Flat Cost = " <<HYPERPARAM_LEG_BB_FLAT_COST << std::endl;
	std::cout << "Legal Mode = " << int(HYPERPARAM_LEG_LEGALMODE) << std::endl << std::endl;

	std::cout << "-> RefineEngine Hyperparameters" << std::endl;
	std::cout << "Max Iteration = " <<HYPERPARAM_REF_MAX_ITERATION << std::endl;
	std::cout << "Use Gradient Order = " << bool(HYPERPARAM_REF_USE_GRADIENT_ORDER) << std::endl;
	std::cout << "Momentum init = " <<HYPERPARAM_REF_INITIAL_MOMENTUM  << ", growth = " <<HYPERPARAM_REF_MOMENTUM_GROWTH << "x" << std::endl;
	std::cout << "Use Gradient Grow = " << bool(HYPERPARAM_REF_USE_GRADIENT_GROW) << ", Use Gradient Shrink = " << bool(HYPERPARAM_REF_USE_GRADIENT_SHRINK) << std::endl;

	

    printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ Stage                     │     Runtime (s)      |            HPWL            |       Overlap        |       Legality        ║\n");
    printf("╟───────────────────────────|──────────────────────|────────────────────────────|──────────────────────|───────────────────────╢\n");
    printf("║ Global Floorplan          │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s |%-23s║\n",
        globalRuntimeS, globalRuntimePTG, HPWL_DONE_GLOBAL, globalDeltaHPWL, 100 * OVERLAP_DONE_GLOBAL, globalDeltaOVERLAP, "");
    printf("║ Infrastructure Building   │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s |%-23s║\n",
        csRuntimeS, csRuntimePTG, HPWL_DONE_CS, csDeltaHPWL, 100 * OVERLAP_DONE_CS, csDeltaOVERLAP, csLegal);

    if(DONE_POR == 1){
        printf("║ Primitive Overlap Removal │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s |%-23s║\n",
            porRuntimeS, porRuntimePTG, HPWL_DONE_POR, porDeltaHPWL, 100 * OVERLAP_DONE_POR, porDeltaOVERLAP, porLegal);
    }else if(DONE_POR == 2){
        printf("║ Primitive Overlap Removal │ Skipped %89s║\n", "");
	}else{
        printf("║ Primitive Overlap Removal │ Not Executed %84s║\n", "");
	}

    if(DONE_LEG == 1){
            printf("║ DFSL Legaliser            │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s |%-23s║\n",
        legRuntimeS, legRuntimePTG, HPWL_DONE_LEG, legDeltaHPWL, 100 * OVERLAP_DONE_LEG, legDeltaOVERLAP, legLegal);
    }else if(DONE_LEG == 2){
		printf("║ DFSL Legaliser            │ Skipped %89s║\n", "");
	}else{
		printf("║ DFSL Legaliser            │ Not Executed %84s║\n", "");
	}	



	if(DONE_REF == 1){
		printf("║ Refine Engine             │ %11.3lf (%5.2lf%%) │ %14.2lf %s │ %7.4lf%% %s |%-23s║\n",
			refRuntimeS, refRuntimePTG, HPWL_DONE_REF, refDeltaHPWL, 100 * OVERLAP_DONE_REF, refDeltaOVERLAP, refLegal);
	}else if(DONE_REF == 2){
		printf("║ Refine Engine             │ Skipped %89s║\n", "");
	}else{
		printf("║ Refine Engine             │ Not Executed %84s║\n", "");
	}



    printf("╟───────────────────────────|──────────────────────|────────────────────────────|──────────────────────|───────────────────────╢\n");
	if(LEGAL_REF){
		printf("║ Summary                   │ %11.3lf %s │ %14.2lf %s │ %7.4lf%% %s |%-23s║\n",
			totalRuntimeS, "        ", HPWL_DONE_REF, "           ", 100 * OVERLAP_DONE_REF, refDeltaOVERLAP, refLegal);
	}else{
		printf("║ Summary                   │ RESULT ILLEGAL %82s║\n", "");
	}
    printf("╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
}

void showChange(double before, double after, char *result){
    
    double delta = after - before;
    if(delta == 0){
        snprintf(result, 12, "(    -    )");
    }else if(delta > 0){
        snprintf(result, 12,  "(+%7.4f%%)", 100 * delta/before);
        
    }else{
        delta = - delta;
        snprintf(result, 12, "(-%7.4f%%)", 100 * delta/before);
    }
}

