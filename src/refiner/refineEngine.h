
#ifndef __REFINE_ENGINE__
#define __REFINE_ENGINE__
#include <vector>
#include <unordered_map>
#include <math.h>
#include "floorplan.h"
#include "units.h"

class RefineEngine{
private:
    std::unordered_map<Rectilinear *, double> rectConnWeightSum;
    // the most dense connection at [0], ... last is the most loosely connected
    std::vector<Rectilinear *> rectConnOrder;

    double REFINE_ATTATCHED_MIN = 0.15;

    int REFINE_MAX_ITERATION = 30;
    bool REFINE_USE_GRADIENT_ORDER = true;
    len_t REFINE_INITIAL_MOMENTUM = 2;
    double REFINE_MOMENTUM_GROWTH = 2;
    bool REFINE_USE_GRADIENT_GROW = true;
    bool REFINE_USE_GRADIENT_SHRINK = true;

public:
    Floorplan *fp;
    RefineEngine(Floorplan *floorplan, double attatchedMin, int maxIter, bool useGradientOrder, len_t initMomentum, double momentumGrowth, bool growGradient, bool shrinkGradient);

    Floorplan *refine();
    bool refineRectilinear(Rectilinear *rect) const;
    
    bool fillBoundingBox(Rectilinear *rect) const;

    bool refineByGrowing(Rectilinear *rect) const;
    bool growingTowardNorth(Rectilinear *rect, len_t depth) const;
    bool growingTowardSouth(Rectilinear *rect, len_t depth) const;
    bool growingTowardEast(Rectilinear *rect, len_t depth) const;
    bool growingTowardWest(Rectilinear *rect, len_t depth) const;

    bool fixAspectRatioEastSideGrow(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, 
    std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors, Rectangle &eastGrow) const;
    bool fixAspectRatioWestSideGrow(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, 
    std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors, Rectangle &westGrow) const;
    bool fixAspectRatioNorthSideGrow(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, 
    std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors, Rectangle &northGrow) const;
    bool fixAspectRatioSouthSideGrow(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, 
    std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors, Rectangle &southGrow) const;

    bool fixUtilizationGrowBoundingBox(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors) const;  

    bool forecastGrowthBenefits(Rectilinear *rect, DoughnutPolygonSet &currentRectDPS, std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors) const;

    bool refineByTrimming(Rectilinear *rect) const;
    bool executeRefineTrimming(Rectilinear *rect, direction2D direction, len_t depth) const;


    /* general helper functions */

    bool trialGrow(DoughnutPolygonSet &dpSet, Rectilinear*dpSetRect, Rectangle &growArea, std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbor) const;
    
    angle_t calculateBestAngle(Rectilinear *rect) const;
    // direction2D calculateCorrespondDirection(angle_t angle) const;
};

// quadrant translatetrimDirection(angle_t angle);

#endif // __REFINE_ENGINE__