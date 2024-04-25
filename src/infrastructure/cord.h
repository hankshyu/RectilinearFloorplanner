#ifndef __CORD_H__
#define __CORD_H__

#include <ostream>

#include "boost/polygon/polygon.hpp"
#include "units.h"

typedef boost::polygon::point_data<len_t> Cord;

// Implement hash function for map and set data structure
namespace std {
    template <>
    struct hash<Cord> {
        size_t operator()(const Cord &key) const;
    };
}  // namespace std

std::ostream &operator<<(std::ostream &os, const Cord &c);

inline len_t calL1Distance(Cord c1, Cord c2) {
    return boost::polygon::manhattan_distance<Cord, Cord>(c1, c2);
}

/* Floating Point coordinate system, only used for centre calculation, mind the rounding error !! */
typedef boost::polygon::point_data<flen_t> FCord;

namespace std {
    template <>
    struct hash<FCord> {
        size_t operator()(const FCord &key) const;
    };
} // namespace std

std::ostream &operator<<(std::ostream &os, const FCord &c);

inline len_t calL1Distance(FCord c1, FCord c2) {
    return boost::polygon::manhattan_distance<Cord, Cord>(c1, c2);
}

#endif  // #define __CORD_H__