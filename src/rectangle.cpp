#include "rectangle.h"

size_t std::hash<Rectangle>::operator()(const Rectangle &key) const {
    namespace gtl = boost::polygon;
    return (std::hash<len_t>()(gtl::xl(key))) ^ (std::hash<len_t>()(gtl::yl(key)));
}

std::ostream &operator<<(std::ostream &os, const Rectangle &c) {
    os << "R[" << boost::polygon::ll(c) << ", ";
    os << rec::getWidth(c) << ", " << rec::getHeight(c) <<", ";
    os << boost::polygon::ur(c) << "]";
    return os;
}