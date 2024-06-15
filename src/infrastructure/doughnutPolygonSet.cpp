#include "doughnutPolygonSet.h"
#include "cSException.h"

bool dps::innerWidthLegal(const DoughnutPolygonSet &dpSet){
    
    if (dpSet.empty()) return true;
    const len_t minInnerWidth = 30;
    // const len_t minInnerWidth = 5;
    using namespace boost::polygon::operators;
    
    // dice the rectangle vertically and measure the height
    std::vector<Rectangle> verticalFragments;
    boost::polygon::get_rectangles(verticalFragments, dpSet, orientation2D::VERTICAL);
    for(Rectangle &vrec : verticalFragments){
        if(rec::getHeight(vrec) < minInnerWidth) return false;
    }

    // dice the rectangle horizontally and measure the width
    std::vector<Rectangle> horizontalFragments;
    boost::polygon::get_rectangles(horizontalFragments, dpSet, orientation2D::HORIZONTAL);
    for(Rectangle &hrec : horizontalFragments){
        if(rec::getWidth(hrec) < minInnerWidth) return false;
    }

    return true;
}

Rectangle dps::calculateBoundingBox(const DoughnutPolygonSet &dpSet){
    std::vector<Rectangle> fragments;
    boost::polygon::get_rectangles(fragments, dpSet);
    if(fragments.empty()){
        throw CSException("DOUGHNUTPOLYGONSET_02");
    }

    len_t xl = LEN_T_MAX;
    len_t yl = LEN_T_MAX;
    len_t xh = LEN_T_MIN;
    len_t yh = LEN_T_MIN;

    for(Rectangle &r : fragments){
        len_t rxl = rec::getXL(r);
        len_t ryl = rec::getYL(r);
        len_t rxh = rec::getXH(r);
        len_t ryh = rec::getYH(r);

        if(rxl < xl) xl = rxl;
        if(ryl < yl) yl = ryl;
        if(rxh > xh) xh = rxh;
        if(ryh > yh) yh = ryh;
    }

    return Rectangle(xl, yl, xh, yh);
}


bool dps::checkIsLegal(const DoughnutPolygonSet &dpSet, doughnutPolygonSetIllegalType &illegalType, area_t legalArea, double aspectRatioMin, double aspectRatioMax, double utilizationMin){
    int dpSetSize = dpSet.size();
    if(dpSetSize != 1){
        illegalType = doughnutPolygonSetIllegalType::DPS_TWO_SHAPE;
        return false;
    }

    DoughnutPolygon curSegment = dpSet[0];
    if(curSegment.begin_holes() != curSegment.end_holes()){
        illegalType = doughnutPolygonSetIllegalType::DPS_HOLE;
        return false;
    }

    area_t dpSetArea = dps::getArea(dpSet);
    if(dpSetArea < legalArea){
        illegalType = doughnutPolygonSetIllegalType::DPS_AREA;
        return false;
    }

    Rectangle boundingBox = dps::calculateBoundingBox(dpSet);
    double boundingBoxAspectRatio = rec::calculateAspectRatio(boundingBox);
    
    if((boundingBoxAspectRatio < aspectRatioMin) || (boundingBoxAspectRatio > aspectRatioMax)){
        illegalType = doughnutPolygonSetIllegalType::DPS_ASPECT_RATIO;
        return false;
    }

    double dpSetUtilization = double(dpSetArea) / double(rec::getArea(boundingBox));
    if(dpSetUtilization < utilizationMin){
        illegalType = doughnutPolygonSetIllegalType::DPS_UTILIZATION;
        return false;
    }

    if(!innerWidthLegal(dpSet)){
        illegalType = doughnutPolygonSetIllegalType::DPS_INNER_WIDTH;
        return false;
    }

    // pass all test
    illegalType = doughnutPolygonSetIllegalType::DPS_LEGAL;
    return true;
}

bool dps::checkIsLegal(const DoughnutPolygonSet &dpSet, area_t legalArea, double aspectRatioMin, double aspectRatioMax, double utilizationMin){
    int dpSetSize = dpSet.size();
    if(dpSetSize != 1){
        return false;
    }

    DoughnutPolygon curSegment = dpSet[0];
    if(curSegment.begin_holes() != curSegment.end_holes()){
        return false;
    }

    area_t dpSetArea = dps::getArea(dpSet);
    if(dpSetArea < legalArea){
        return false;
    }

    Rectangle boundingBox = dps::calculateBoundingBox(dpSet);
    double boundingBoxAspectRatio = rec::calculateAspectRatio(boundingBox);
    
    if((boundingBoxAspectRatio < aspectRatioMin) || (boundingBoxAspectRatio > aspectRatioMax)){
        return false;
    }

    double dpSetUtilization = double(dpSetArea) / double(rec::getArea(boundingBox));
    if(dpSetUtilization < utilizationMin){
        return false;
    }

    if(!innerWidthLegal(dpSet)){
        return false;
    }

    // pass all test
    return true;

}
std::ostream &operator << (std::ostream &os, const doughnutPolygonSetIllegalType &t){
    switch (t)
    {
    case doughnutPolygonSetIllegalType::DPS_LEGAL:
        os << "DPS_LEGAL";
        break;
    case doughnutPolygonSetIllegalType::DPS_AREA:
        os << "DPS_AREA";
        break;
    case doughnutPolygonSetIllegalType::DPS_ASPECT_RATIO:
        os << "DPS_ASPECT_RATIO";
        break;
    case doughnutPolygonSetIllegalType::DPS_UTILIZATION:
        os << "DPS_UTILIZATION";
        break;
    case doughnutPolygonSetIllegalType::DPS_HOLE:
        os << "DPS_HOLE";
        break;
    case doughnutPolygonSetIllegalType::DPS_TWO_SHAPE:
        os << "DPS_TWO_SHAPE";
        break;
    case doughnutPolygonSetIllegalType::DPS_INNER_WIDTH:
        os << "DPS_INNER_WIDTH";
        break;
    default:
        throw CSException("DOUGHNUTPOLYGONSET_01");
        break;
    }
    
    return os;
}
