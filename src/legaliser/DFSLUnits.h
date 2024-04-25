#ifndef _DFSLUNITS_H_
#define _DFSLUNITS_H_

#include <boost/polygon/polygon.hpp>

#include "units.h"

namespace gtl = boost::polygon;
typedef gtl::rectangle_data<len_t> Rectangle;
typedef gtl::polygon_90_set_data<len_t> Polygon90Set;
typedef gtl::polygon_90_with_holes_data<len_t> Polygon90WithHoles;
typedef gtl::point_data<len_t> Point;

#endif