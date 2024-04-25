#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <ctime>
#include "parser.h"
#include "rgsolver.h"

int main(int argc, char *argv[]) {
    // parsing command line arguments
    std::string inputFileName;
    std::string outputFileName;
    std::string configFileName;
    std::string punishmentArg = "";
    std::string maxAspectRatioArg = "";

    int opt;

    // optstring "hc:o:" means:
    // h is an option that doesn't require an argument
    // c: is an option that requires an argument (indicated by the colon)
    // o: is another option that requires an argument
    while ( ( opt = getopt(argc, argv, "hi:o:c:p:a:") ) != -1 ) {
        switch ( opt ) {
        case 'h':
            std::cout << "Usage: " << argv[0] << " [-h] [-i <input>] [-o <output>] [-c <config>] [-p <punishment>] [-a <aspect_ratio>]\n";
            return 0;
        case 'i':
            std::cout << "[GlobalSolver] Note: Input file is set to " << optarg << std::endl;
            inputFileName = optarg;
            break;
        case 'o':
            std::cout << "[GlobalSolver] Note: Output file is set to " << optarg << std::endl;
            outputFileName = optarg;
            break;
        case 'p':
            punishmentArg = optarg;
            break;
        case 'a':
            maxAspectRatioArg = optarg;
            break;
        case 'c':
            std::cout << "[GlobalSolver] Note: Config file is set to " << optarg << std::endl;
            configFileName = optarg;
            break;
        case '?':
            if ( optopt == 'i' || optopt == 'o' || optopt == 'p' || optopt == 'a' || optopt == 'c' ) {
                std::cerr << "Option -" << static_cast< char >( optopt ) << " requires an argument." << std::endl;
            }
            else if ( isprint(optopt) ) {
                std::cerr << "Unknown option `-" << static_cast< char >( optopt ) << "`." << std::endl;
            }
            else {
                std::cerr << "Unknown option character `\\x" << std::hex << optopt << "`." << std::endl;
            }
            return 1;
        default:
            abort();
        }
    }

    // parse input file and config file
    Parser parser;
    GlobalSolver solver;
    if ( !parser.parseInput(inputFileName, punishmentArg, maxAspectRatioArg) ) {
        std::cout << "[GlobalSolver] ERROR: Input file does not exist." << std::endl;
        return -1;
    }
    if ( !parser.parseConfig(configFileName) ) {
        std::cout << "[GlobalSolver] Note: Config file does not exist. Use default configuration." << std::endl;
    }
    solver.readFloorplan(parser);
    solver.readConfig(parser);

    // generate clusters and pack them together
    // solver.generateCluster();

    // Start measuring CPU time
    struct timespec start, end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    // Global Floorplanning
    // learning rate: case02&09: 50e-4, others: 2e-4;
    // solver.setLearningRate(1e-3);
    solver.solve();
    solver.markExtremeOverlap();
    if ( parser.getInflationRatio() > 0 ) {
        solver.inflate();
    }
    solver.finetune();

    // Stop measuring CPU time
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

    // report the result
    std::cout << std::endl;
    solver.reportDeadSpace();
    std::cout << std::endl;
    solver.reportOverlap();
    std::cout << std::endl;

    if ( parser.getInflationRatio() > 0 ) {
        solver.reportInflation();
        std::cout << std::endl;
    }
    
    if ( !solver.isAreaLegal() ) {
        std::cout << "[GlobalSolver] ERROR: Area Constraint Violated.\n";
    }
    else {
        std::cout << "[GlobalSolver] Note: Area Constraint Met.\n";
    }

    if ( !solver.isAspectRatioLegal() ) {
        std::cout << "[GlobalSolver] ERROR: Aspect Ratio Constraint Violated.\n";
    }
    else {
        std::cout << "[GlobalSolver] Note: Aspect Ratio Constraint Met.\n";
    }

    solver.currentPosition2txt(outputFileName);
    std::cout << std::fixed;
    std::cout << "[GlobalSolver] Estimated HPWL: " << std::setprecision(2) << solver.calcEstimatedHPWL() << std::endl;
    double elapsed = ( end.tv_sec - start.tv_sec ) + ( end.tv_nsec - start.tv_nsec ) / 1e9;

    std::cout << "[GlobalSolver] CPU time used: " << elapsed << " seconds." << std::endl;
    return 0;
}