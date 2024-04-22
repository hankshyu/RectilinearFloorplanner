#include "cord.h"

size_t std::hash<Cord>::operator()(const Cord &key) const {
    return (std::hash<len_t>()(key.x())) ^ (std::hash<len_t>()(key.y()));
}

std::ostream &operator<<(std::ostream &os, const Cord &c) {
    os << "(" << c.x() << ", " << c.y() << ")";
    return os;
}

size_t std::hash<FCord>::operator()(const FCord &key) const {
    return (std::hash<flen_t>()(key.x())) ^ (std::hash<flen_t>()(key.y()));
}

std::ostream &operator<<(std::ostream &os, const FCord &c) {
    os << "(" << c.x() << ", " << c.y() << ")";
    return os;
}