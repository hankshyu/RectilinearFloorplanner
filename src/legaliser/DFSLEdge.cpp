#include "DFSLEdge.h"

namespace DFSL {

DFSLEdge::DFSLEdge(): mFromIndex(-1), mToIndex(-1), mType(EDGETYPE::BAD_EDGE){ ; }

DFSLEdge::DFSLEdge(int from, int to, EDGETYPE type): mFromIndex(from), mToIndex(to), mType(type){ ; }

DFSLEdge::DFSLEdge(int from, int to, EDGETYPE type, std::vector<Segment>& tangentSegments):
mFromIndex(from), mToIndex(to), mType(type), mTangentSegments(tangentSegments) { ; }

DFSLEdge::DFSLEdge(const DFSLEdge& other){
    this->mFromIndex = other.mFromIndex;
    this->mToIndex = other.mToIndex;
    this->mType = other.mType;
    this->mTangentSegments = other.mTangentSegments;
}

int DFSLEdge::getFrom() const{
    return mFromIndex;
}

int DFSLEdge::getTo() const{
    return mToIndex;
}

EDGETYPE DFSLEdge::getType() const{
    return mType;
}

std::vector<Segment>& DFSLEdge::tangentSegments(){
    return mTangentSegments;
}

MigrationEdge::MigrationEdge(int from, int to, EDGETYPE type, Rectangle& area, Segment& seg, double cost):
    fromIndex(from), toIndex(to), edgeType(type), segment(seg), migratedArea(area), edgeCost(cost){ ; }

MigrationEdge::MigrationEdge():
    fromIndex(-1), toIndex(-1), edgeType(EDGETYPE::BAD_EDGE), segment(Segment()), migratedArea(0,0,1,1), edgeCost(0.0) { ; }

std::ostream& operator<< (std::ostream& os, EDGETYPE type){   
    switch (type)
    {
    case EDGETYPE::OB:
        os << "OB ";
        break;
    case EDGETYPE::BB:
        os << "BB ";
        break;
    case EDGETYPE::BW:
        os << "BW ";
        break;
    default:
        os << "OTHER ";
        break;
    }
    return os;
}

}