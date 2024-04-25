#ifndef PARSER_H
#define PARSER_H

#include "globmodule.h"
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <unordered_set>


class Parser {
private:
    bool configExists;
    // info of floorplan
    int DieWidth, DieHeight;
    int softModuleNum, fixedModuleNum, moduleNum, connectionNum;
    std::vector<GlobalModule *> modules;
    std::vector<ConnectionInfo *> connectionList;
    // info of solver

    // altered by Hank, make all hyper parameters type align
    double punishment; 
    double max_aspect_ratio;
    double lr;
    double inflationRatio;
    double extremePushScale;
    bool dumpInflation;
    std::vector< std::vector<std::string> > ShapeConstraintMods;
public:
    Parser();
    ~Parser();
    bool parseInput(std::string file_name,double globalAspectRatioMax, double punishmentArg, double maxAspectRatioRateArg);
    // bool parseConfig(std::string file_name);
    // info of floorplan
    int getDieWidth();
    int getDieHeight();
    int getSoftModuleNum();
    int getFixedModuleNum();
    int getModuleNum();
    int getConnectionNum();
    GlobalModule *getModule(int index);
    ConnectionInfo *getConnection(int index);
    // info of solver
    double getPunishment();
    double getMaxAspectRatio();
    double getLearnRate();
    double getInflationRatio();
    bool getDumpInflation();
    double getExtremePushScale();
    std::vector< std::vector<std::string> > getShapeConstraints();
};

#endif