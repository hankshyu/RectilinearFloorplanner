#ifndef RGSOLVER_H
#define RGSOLVER_H

#include "globmodule.h"
#include "parser.h"
#include "cluster.h"
#include "globalResult.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <unordered_map>
#include <iomanip>


class GlobalSolver {
private:
    double DieWidth_, DieHeight_;
    int softModuleNum_, fixedModuleNum_, moduleNum_, connectionNum_;
    std::vector<GlobalModule *> modules_;
    std::unordered_map<GlobalModule *, int> module2index_;
    std::vector<ConnectionInfo *> connectionList_;
    std::vector<double> xGradient_, yGradient_;
    std::vector<double> wGradient_, hGradient_;
    std::vector<double> xFirstMoment_, yFirstMoment_;
    std::vector<double> xSecondMoment_, ySecondMoment_;
    std::vector< std::vector<GlobalModule *> > sameShapeMods_;
    std::set<GlobalModule *> bigOverlapModules_;
    double lr_;
    double xMaxMovement_, yMaxMovement_;
    double sizeScalar_;
    double punishment_;
    double maxAspectRatio_;
    double connectNormalize_;
    bool toggle_;
    int timeStep_;
    double inflationRatio_;
    bool dumpInflation_;
    double extremePushScale_;
public:
    GlobalSolver();
    ~GlobalSolver();
    // Functions for setting up
    void setOutline(int width, int height);
    void setSoftModuleNum(int num);
    void setFixedModuleNum(int num);
    void setConnectionNum(int num);
    void setLearningRate(double lr);
    void setPunishment(double force);
    void setMaxAspectRatio(double aspect_ratio);
    void setMaxMovement(double ratio = 0.001);
    void addModule(GlobalModule *in_module);
    void addConnection(const std::vector<std::string> &in_modules, double in_value);
    void readFloorplan(Parser &parser);
    void readConfig(Parser &parser);

    // Functions for core algorithm
    enum PunishmentScheme { DEFAULT, SCALEPUSH };
    void solve();
    void inflate();
    void finetune();
    void calcGradient(PunishmentScheme punishmentScheme = DEFAULT);
    void gradientDescent(double lr, bool squeeze = false);
    void setSizeScalar(double scalar, bool overlap_aware = false);
    bool inflateModules();
    void markExtremeOverlap();
    void checkShapeConstraint();
    void roundToInteger();
    void generateCluster();
    void resetOptimizer();

    // Functions for output and report
    void currentPosition2txt(std::string file_name);
    double calcDeadspace();
    double calcEstimatedHPWL();
    double calcOverlapRatio();
    bool isAreaLegal();
    bool isAspectRatioLegal();
    void reportOverlap();
    void reportDeadSpace();
    void reportInflation();
    bool exportGlobalFloorplan(GlobalResult &result);
};


#endif
