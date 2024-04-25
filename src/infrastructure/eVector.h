#ifndef __EVECTOR_H__
#define __EVECTOR_H__

#include <ostream>
#include <cmath>

#include "units.h"
#include "cord.h"

class EVector{
public:
    len_t ex;
    len_t ey;

    EVector();
    EVector(len_t ai, len_t bi);
    EVector(Cord from, Cord to);
    EVector(FCord from, Cord to);
    EVector(Cord from, FCord to);
    EVector(FCord from, FCord to);
    EVector(const EVector &other);

    EVector& operator = (const EVector &other);
    bool operator == (const EVector &comp) const;
    friend std::ostream &operator << (std::ostream &os, const EVector &v);

    len_t calculateL1Magnitude() const;
    flen_t calculateL2Magnitude() const;

    angle_t calculateDirection() const;

};

namespace std{
    template<>
    struct hash<EVector>{
        size_t operator()(const EVector &key) const;
    };
}

std::ostream &operator << (std::ostream &os, const EVector &v);

namespace vec{
    angle_t calculateAngle(const EVector &v1, const EVector &v2);
}


#endif // __EVECTOR_H__