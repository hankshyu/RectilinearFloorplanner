#ifndef _DFSLSEGMENT_H_
#define _DFSLSEGMENT_H_

#include "cord.h"

#include "DFSLUnits.h"

namespace DFSL {

struct Segment;
enum class DIRECTION : unsigned char { TOP, RIGHT, DOWN, LEFT, NONE };

bool compareXSegment(Segment a, Segment b);
bool compareYSegment(Segment a, Segment b);

// note: use ONLY on vertical and horizontal segments 
class Segment {
private:
    Cord segStart;
    Cord segEnd;
    DIRECTION direction; // direction of the normal vector of this segment

public:
    Segment(Cord start, Cord end, DIRECTION dir);
    Segment(Cord start, Cord end);
    Segment(const Segment& other);
    Segment();
    int getLength() const ;
    Cord getSegStart() const ;
    Cord getSegEnd() const ;   
    DIRECTION getDirection() const ;

    void setSegStart(Cord start);
    void setSegEnd(Cord end);
    void setDirection(DIRECTION dir);

    friend bool compareXSegment(Segment a, Segment b);
    friend bool compareYSegment(Segment a, Segment b);
};

// in the direction of seg.direction,
// find the segment in poly of the same orientation that has the shortest distance
Segment FindNearestOverlappingInterval(Segment& seg, Polygon90Set& poly);
std::ostream &operator << (std::ostream &os, const Segment &seg);

}

#endif