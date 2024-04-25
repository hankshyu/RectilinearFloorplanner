#ifndef _DFSLEDGE_H_
#define _DFSLEDGE_H_

#include <vector>

#include "Segment.h"

namespace DFSL{

enum class EDGETYPE : unsigned char { OB, BB, BW, WW, BAD_EDGE };

class DFSLEdge;
class MigrationEdge;

class DFSLEdge {
private:
    int mFromIndex;
    int mToIndex; 
    EDGETYPE mType; 
    std::vector<Segment> mTangentSegments;
public:
    DFSLEdge();
    DFSLEdge(int from, int to, EDGETYPE type);
    DFSLEdge(int from, int to, EDGETYPE type, std::vector<Segment>& tangentSegments);
    DFSLEdge(const DFSLEdge& other);
    int getFrom() const;
    int getTo() const;
    EDGETYPE getType() const;
    std::vector<Segment>& tangentSegments();
};

class MigrationEdge {
public:
    int fromIndex;
    int toIndex;
    EDGETYPE edgeType;
    Segment segment;
    Rectangle migratedArea; 
    double edgeCost;
    MigrationEdge();
    MigrationEdge(int from, int to, EDGETYPE type, Rectangle& area, Segment& seg, double cost);
};

std::ostream& operator<< (std::ostream& os, EDGETYPE type);

}

#endif