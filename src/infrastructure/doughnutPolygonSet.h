#ifndef __DOUGHNUTPOLYGONSET_H__
#define __DOUGHNUTPOLYGONSET_H__

#include <vector>

#include "boost/polygon/polygon.hpp"
#include "units.h"
#include "rectangle.h"
#include "doughnutPolygon.h"

typedef std::vector<DoughnutPolygon> DoughnutPolygonSet;

enum class doughnutPolygonSetIllegalType{
    DPS_LEGAL, DPS_AREA,  DPS_ASPECT_RATIO, DPS_UTILIZATION, DPS_HOLE, DPS_TWO_SHAPE, DPS_INNER_WIDTH
};

namespace dps{

    inline void diceIntoRectangles(const DoughnutPolygonSet &dpSet, std::vector<Rectangle> &fragments){
        boost::polygon::get_rectangles(fragments, dpSet);
    }

    inline area_t getArea(const DoughnutPolygonSet &dpSet){
        return area_t(boost::polygon::area(dpSet));
    }

    inline bool oneShape(const DoughnutPolygonSet &dpSet){
        return (dpSet.size() == 1);
    }

    inline bool noHole(const DoughnutPolygonSet &dpSet){

        for(int i = 0; i < dpSet.size(); ++i){
            DoughnutPolygon curSegment = dpSet[i];
            if(curSegment.begin_holes() != curSegment.end_holes()) return false;
        }

        return true;
    }

    bool innerWidthLegal(const DoughnutPolygonSet &dpSet);

    Rectangle calculateBoundingBox(const DoughnutPolygonSet &dpSet);
    
    inline double calculateUtilization(const DoughnutPolygonSet &dpSet){
        Rectangle boundingBox = calculateBoundingBox(dpSet);
        return double(dps::getArea(dpSet)) / double(rec::getArea(boundingBox));
    }

    inline bool checkIsLegalUtilization(const DoughnutPolygonSet &dpSet, double utilizationMin){
        Rectangle boundingBox = calculateBoundingBox(dpSet);
        double utilization = double(dps::getArea(dpSet)) / double(rec::getArea(boundingBox));
        return (utilization >= utilizationMin);
        
    }

    inline double calculateAspectRatio(const DoughnutPolygonSet &dpSet){
        Rectangle boundingBox = calculateBoundingBox(dpSet);
        return rec::calculateAspectRatio(boundingBox);
    }

    inline bool checkIsLegalAspectRatio(const DoughnutPolygonSet &dpSet, double aspectRatioMin, double aspectRatioMax){
        Rectangle boundingBox = calculateBoundingBox(dpSet);
        double aspectRatio = rec::calculateAspectRatio(boundingBox);
        return (aspectRatio >= aspectRatioMin) && (aspectRatio <= aspectRatioMax);

    }

    // if the doughnutPolygonSet is legal, return true, else return false and reason stored in illegalType
    bool checkIsLegal(const DoughnutPolygonSet &dpSet, doughnutPolygonSetIllegalType &illegalType, area_t legalArea, double aspectRatioMin, double aspectRatioMax, double utilizationMin);
    bool checkIsLegal(const DoughnutPolygonSet &dpSet, area_t legalArea, double aspectRatioMin, double aspectRatioMax, double utilizationMin);

}

std::ostream &operator << (std::ostream &os, const doughnutPolygonSetIllegalType &t);

#endif // __DOUGHNUTPOLYGONSET_H__