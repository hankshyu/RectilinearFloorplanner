#ifndef __LEGALIZEENGINE_H__
#define __LEGALIZEENGINE_H__

#include "cord.h"
#include "rectilinear.h"
#include "floorplan.h"

class LegaliseEngine{
private:
    Floorplan *fp;
    area_t minSoftRectilinearRequiredArea;
    area_t maxSoftRectilinearRequiredArea;

    double criticalitySum;

    //define hyperparameters
    const double LEGAL_URGENCY_WEIGHT = 100;

    const double UTILIZATION_URGENCY_WEIGHT = 2.5;
    const int UTILIZATION_SCALING_FACTOR = 16;

    const double OVERLAPRATIO_URGENCY_WEIGHT = 2.0;
    const int OVERLAPRATIO_SCALING_FACTOR = 16;

    const double ASPECTRATIO_URGENCY_WEIGHT = 5.0;
    const int ASPECTRATIO_SCALING_FACTOR = 16;

    const double BORDER_URGENCY_WEIGHT = 5.0;
    const int BORDER_SCALING_FACTOR = 1;
    
    const double CENTRE_URGENCY_WEIGHT = 0.5;
    const int CENTRE_SCALING_FACTOR = 2;

    const double SIZE_URGENCY_WEIGHT = 2.0;
    const int SIZE_SCALING_FACTOR = 2;


    const double BLANK_EDGE_WEIGHT = 0.0;
    const double BLOCK_EDGE_WEIGHT = 0.8;
    const double OVERLAP_EDGE_WEIGHT = 0.95;
    const double DEAD_EDGE_WEIGHT =  1.0;

    const int MAX_LEGALIZE_ITERATION = 40;
    const double HIGH_TMP_RATIO = 0.15;
    const double LOW_TMP_RATIO = 0.25;

    const double LEGAL_CRITICALITY_WEIGHT = 100;

    const double LEGAL_CRITICALITY_AREA = 1.0;
    const double LEGAL_CRITICALITY_ASPECT_RATIO = 3.0;
    const double LEGAL_CRITICALITY_UTILIZATION = 3.0;
    const double LEGAL_CRITICALITY_HOLE = 8.0;
    const double LEGAL_CRITICALITY_ONE_SHAPE = 8.0;

    const double UTILIZATION_CRITICALITY_WEIGHT = 2.5;
    const double ASPECT_RATIO_CRITICALITY_WEIGHT = 2.0;
    const double BORDER_CRITICALITY_WEIGHT = 4.0;
    const double SIZE_CRITICALITY_WEIGHT = 1.0;
    const double AREA_CRITICALITY_WEIGHT = 2.0;

    const int GROW_BLANK_SCLING_FACTOR = 1;
    const double UTILIZATION_TOLERATNCE_PCT_STEP1 = 0.90;

    const double VIOLENT_RATING_BLK_BASE_SCORE = 0.3;
    const double VIOLENT_RATING_LEGAL_SCORE = 0.2;

    const int GROW_VIOLENT_SCLING_FACTOR = 1;
    const double GROW_VIOLENT_MIN_PCT = 0.10;
    const double GROW_VIOLENT_MAX_PCT = 0.50;
    const double UTILIZATION_TOLERATNCE_PCT_STEP3 = 0.90;


    // non-linearly scales input (0 ~ 1)
    double scalingFunction(double input, int base) const;

public:
    LegaliseEngine(Floorplan *floorplan);


    void distributeOverlap() const;
    void huntFixAtypicalShapes() const;
    void fixTwoShapeRectilinear(Rectilinear *rect) const;
    void fixHoleRectilinear(Rectilinear *ret) const;

    void softRectilinearIllegalReport() const;
    double calculateRectilinearUrgency(Rectilinear *rt, Tile *candTile, double &cacheValue) const;

    bool legalise() const;

    double rateRectilinearCriticality(Rectilinear *rect) const;

    bool fillBoundingBox(Rectilinear *rect) const;
    void evaluateBlankGrowingDirection(Rectilinear *rect, std::vector<direction2D> &growOrder, std::vector<double> &growScore) const;
    double evalueBlankGrowingSingleDirection(Rectangle &probe, area_t probeArea) const;
    bool growBlank(Rectilinear *rect, direction2D direction, double rating, std::unordered_set<Rectilinear *> &updateUrgency) const;

    bool growWithinBoundingBox(Rectilinear *rt, std::unordered_set<Rectilinear *> &iterationLocked, 
    std::unordered_map<Rectilinear *, double> &allRectilinearUrgnecy, std::unordered_set<Rectilinear *> &updateUrgency, double urgencyMultiplier) const;

    void evaluateViolentGrowingDirection(Rectilinear *rect, std::vector<direction2D> &growOrder, std::vector<double> &growScore,
    std::unordered_map<Rectilinear *, double> &allRectilinearUrgency, std::unordered_set<Rectilinear *> &iterationLocked) const;

    double evalueViolentGrowingSingleDirection(Rectangle &probe, area_t probeArea,
    std::unordered_map<Rectilinear *, double> &allRectilinearUrgency, std::unordered_set<Rectilinear *> &iterationLocked) const;

    bool growViolent(Rectilinear *rect, direction2D direction, double rating, std::unordered_set<Rectilinear *> &updateUrgency, 
    std::unordered_map<Rectilinear *, double> &allRectilinearUrgency, std::unordered_set<Rectilinear *> &iterationLocked) const;

    bool forceFixAspectRatio(Rectilinear *rect) const;
    bool evaluateForceTrim(Rectilinear *rect, Rectangle &trimRect, area_t &areaAfterTrim) const;
    void executeTrim(Rectilinear *rect, Rectangle &trimRect) const;

    bool forceFixUtilization(Rectilinear *rect, std::unordered_set<Rectilinear *> &updateUrgency) const;

    bool forceGrowAlongBorder(Rectilinear *rect, std::unordered_set<Rectilinear *> &updateUrgency) const;


    double calculateLegalSuburgency(Rectilinear *rt, Tile *candTile) const;
    double calculateUtilizationSubUrgency(Rectilinear *rt) const;
    double calculateAspectRatioSubUrgency(Rectilinear *rt) const;
    double calculateOverlapRatioSubUrgency(Rectilinear *rt, Tile *candTile) const;
    double calculateBorderSubUrgency(Rectilinear *rt) const;
    double calcualteCentreSubUrgency(Rectilinear *rt) const;
    double calculateSizeSubUrgency(Rectilinear *rt) const;

    double rateRectilinearLegalCriticality(Rectilinear *rt) const;
    double rateRectilinearAreaCriticality(Rectilinear *rt) const;
    

};

#endif //__LEGALIZEENGINE_H__ 