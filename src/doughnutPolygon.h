#ifndef __DOUGHNUTPOLYGON_H__
#define __DOUGHNUTPOLYGON_H__

#include "boost/polygon/polygon.hpp"
#include "cord.h"
#include "units.h"
#include "rectangle.h"

typedef boost::polygon::polygon_90_with_holes_data<len_t> DoughnutPolygon;

std::ostream &operator << (std::ostream &os, const DoughnutPolygon &dp);

namespace dp{
    void acquireClockwiseWinding(const DoughnutPolygon &rectilinearShape, std::vector<Cord> &winding);
    
    void acquireBoundingBox(const DoughnutPolygon &rectilinearShape, Rectangle &boundingBox);
}

#endif  // #define __DOUGHNUTPOLYGON_H__