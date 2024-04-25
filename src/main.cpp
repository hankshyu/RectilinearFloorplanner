#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cfloat>
#include <stdio.h>
#include <unistd.h>
#include <chrono>

#include "cSException.h"
#include "globalResult.h"
#include "floorplan.h"
#include "colours.h"
#include "DFSLegalizer.h"
#include "refineEngine.h"

std::string showChange(double before, double after);

int main(int argc, char *argv[]) {
    int legalStrategy = 0;
    int legalMode = 0;
    std::string inputFilePath = "";
    std::string casename = "";
    std::string outputDir = "./outputs";
    std::string configFilePath = "";
    bool verbose = false;
    bool debugMode = false;
    bool useCustomConf = false;
    
    printf("%sIRIS Lab Rectilinear Floorplanner (Backend)%s\n", CYAN, COLORRST);

    int cmd_opt;
    // "a": -a doesn't require argument
    // "a:": -a requires a argument
    // "a::" argument is optional for -a 
    while((cmd_opt = getopt(argc, argv, ":hi:o:f:c:m:s:vd")) != -1) {
        switch (cmd_opt) {
        case 'h':
            std::cout << "Usage: " << argv[0] << " [-h] [-i <input file>] [-o <output directory>] [-f <floorplan name>] [-m <legalization mode 0-3>] [-s <legalization strategy 0-4>] [-c <custom .conf file>] [-v]\n";
            std::cout << "\tNote: If a custom config file (-c) is provided, then the -s option will be ignored.\n";
            return 0;
        case 'i':
            inputFilePath = optarg;
            break;
        case 'o':
            outputDir = optarg;
            break;
        case 'f':
            casename = optarg;
            break;
        case 'm':
            legalMode = std::stoi(optarg);
            break;
        case 's':
            legalStrategy = std::stoi(optarg);
            break;
        case 'c':
            configFilePath = optarg;
            useCustomConf = true;
            break;
        case 'v':
            verbose = true;
            break;
        case 'd':
            debugMode = true;
            break;
        case '?':
            fprintf(stderr, "Illegal option:-%c\n", isprint(optopt)?optopt:'#');
            break;
        case ':': // requires first char of optstring to be ':'
            fprintf(stderr, "Option -%c requires an argument\n", optopt);
        default:
            return 1;
            break;
        }
    }

    // go thru arguments not parsed by getopt
    if (argc > optind) {
        for (int i = optind; i < argc; i++) {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
        }
    }

    if (inputFilePath == ""){
        std::cout << "Usage: " << argv[0] << " [-h] [-i <input file>] [-o <output directory>] [-f <floorplan name>] [-m <legalization mode 0-3>] [-s <legalization strategy 0-4>] [-c <custom .conf file>]\n";
        std::cerr << "Where is your input file?\n";
        return 1;
    }    

    // find casename
    if (casename == ""){
        std::size_t casenameStart = inputFilePath.find_last_of("/\\");
        std::size_t casenameEnd = inputFilePath.find_last_of(".");
        if (casenameStart == std::string::npos){
            casenameStart = 0;
        }
        else {
            casenameStart++;
        }
        size_t casenameLength;
        if (casenameEnd <= casenameStart || casenameEnd == std::string::npos){
            casenameLength = std::string::npos;
        }
        else {
            casenameLength = casenameEnd - casenameStart;
        }
        casename = inputFilePath.substr(casenameStart, casenameLength);
    }

    std::cout << "Input file: " << inputFilePath << '\n';
    std::cout << "Output directory: " << outputDir << '\n';
    std::cout << "Case Name: " << casename << '\n';
    std::cout << "Legalization mode: " << legalMode << '\n';
    if (useCustomConf){
        std::cout << "Reading Configs from: " << configFilePath << '\n';
    }
    else {
        std::cout << "Legalization strategy: " << legalStrategy << '\n';
    }
    std::cout << std::endl;

    // Start measuring CPU time
    struct timespec start, end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    // get global result
    std::cout << "Reading Global Input..." << std::endl;
    GlobalResult gr;
    gr.readGlobalResult(inputFilePath);

    std::chrono::steady_clock::time_point CSInsertionTimePoint = std::chrono::steady_clock::now(); 

    // convert global to FP
    std::cout << "Creating Floorplan..." << std::endl;
    Floorplan fp(gr, 0.5, 2.0, 0.8);
    flen_t CSInsertionHPWL = fp.calculateHPWL();
    std::cout << "Global Floorplanning HPWL = " << CSInsertionHPWL << std::endl;

    fp.removePrimitiveOvelaps(true);
    std::chrono::steady_clock::time_point DonePORTimePoint = std::chrono::steady_clock::now(); 
    flen_t DonePORHPWL = fp.calculateHPWL();
    std::cout << CYAN << "Primitive OverlapRemoval" << COLORRST << "HPWL = " << DonePORHPWL << " " << showChange(CSInsertionHPWL, DonePORHPWL) << std::endl;
    std::cout << "Runtime = " << std::chrono::duration_cast<std::chrono::milliseconds>(DonePORTimePoint - CSInsertionTimePoint).count() / 1000.0 << " s" << std::endl;


    // create object
    DFSL::DFSLegalizer dfsl;

    // read config file 
    DFSL::ConfigList configs;
    configs.initAllConfigs();
    
    // set configs file
    if (useCustomConf){
        bool success = configs.readConfigFile(configFilePath);
        if (!success){
            std::cerr << "Error opening configs\n";
            return 0;
        }
    }
    else { if (legalStrategy == 1){
            // prioritize area 
            std::cout << "legalStrategy = 1, prioritizing area\n";
            configs.readConfigFile("configs/strategy1.conf");
        }
        else if (legalStrategy == 2){
            // prioritize util
            std::cout << "legalStrategy = 2, prioritizing utilization\n";
            configs.readConfigFile("configs/strategy2.conf");
        }
        else if (legalStrategy == 3){
            // prioritize aspect ratio
            std::cout << "legalStrategy = 3, prioritizing aspect ratio\n";
            configs.readConfigFile("configs/strategy3.conf");
        }
        else if (legalStrategy == 4){
            // favor block -> block flow more
            std::cout << "legalStrategy = 4, prioritizing block -> block flow\n";
            configs.readConfigFile("configs/strategy4.conf");
        }
        else {
            std::cout << "legalStrategy = 0, using default configs\n";
            // default configs
        }
    }

    // if verbose set output level
    if (debugMode){
        configs.setConfigValue<int>("OutputLevel", DFSL::DFSL_PRINTLEVEL::DFSL_DEBUG);
    }
    else if (verbose){
        configs.setConfigValue<int>("OutputLevel", DFSL::DFSL_PRINTLEVEL::DFSL_VERBOSE);
    }

    // initialize legalizer
    std::cout << "Initializing Legalizer..." << std::endl;
    dfsl.initDFSLegalizer(&(fp), &(configs));


    // visualize initial floorplan
    const std::string initFloorplanPath = outputDir + "/" + casename + "_init.txt";
    std::cout << "Visualizing initial floorplan: " << initFloorplanPath << "\n";
    dfsl.printTiledFloorplan(initFloorplanPath, (const std::string)casename);

    // set legalize mode
    std::cout << "Legalization mode = " << legalMode << ",";
    switch (legalMode)
    {
    case 0:
        std::cout << "resolve area big -> area small\n";
        break;
    case 1:
        std::cout << "resolve area small -> area big\n";
        break;
    case 2:
        std::cout << "resolve overlaps near center -> outer edge (euclidean distance)\n";
        break;
    case 3:
        std::cout << "completely random\n";
        break;
    case 4:
        std::cout << "resolve overlaps near center -> outer edge (manhattan distance)\n";
        break;
    default:
        break;
    }

    std::cout << "\n";
    DFSL::RESULT legalResult = DFSL::RESULT::OVERLAP_NOT_RESOLVED;
    
    // start legalizing
    try{
        legalResult = dfsl.legalize(legalMode);
        if(legalResult == DFSL::RESULT::SUCCESS){
            std::cout << "legalise Success, run refiner << HPWL = " << fp.calculateHPWL() << std::endl;
            RefineEngine re(&fp);
            re.refine();
        }
    }
    catch (CSException& e){
        std::cout << std::flush;
        std::cerr << "uhoh\n";
        std::cerr << e.what() << std::endl;
    }
    // Stop measuring CPU time

    // output result
    dfsl.printTiledFloorplan(outputDir + "/" + casename + "_legal.txt", casename);
    if (legalResult != DFSL::RESULT::OVERLAP_NOT_RESOLVED){
        dfsl.printCompleteFloorplan(outputDir + "/" + casename + "_legal_fp.txt", casename);
        
    }
    // get hpwl
    fp.visualiseFloorplan("./outputs/myoutput.txt");
    double finalScore = fp.calculateHPWL();
    printf("Final Score = %12.6f\n", finalScore);

    std::chrono::steady_clock::time_point exitTimePoint = std::chrono::steady_clock::now(); 
    flen_t exitHPWL = fp.calculateHPWL();
    std::cout << CYAN << "Legalization " << COLORRST << "HPWL = " << exitHPWL << " " << showChange(DonePORHPWL, exitHPWL) << std::endl;
    std::cout << "Runtime = " << std::chrono::duration_cast<std::chrono::milliseconds>(exitTimePoint - DonePORTimePoint).count() / 1000.0 << " s" << std::endl;
}

std::string showChange(double before, double after){
    double delta = before - after;
    if(delta >= 0){
        printf("%s+%.2f(%.2f%%)%s", GREEN, delta, delta / before, COLORRST);
    }else{
        printf("%s%.2f(%.2f%%)%s", RED, delta, delta / before, COLORRST);
    }
    return "";
}
