#include <utility>

#include "cord.h"

#include "Segment.h"

namespace DFSL{

bool compareXSegment(Segment a, Segment b){
    return a.segStart.y() == b.segStart.y() ? a.segStart.x() < b.segStart.x() : a.segStart.y() < b.segStart.y() ;
}

bool compareYSegment(Segment a, Segment b){
    return a.segStart.x() == b.segStart.x() ? a.segStart.y() < b.segStart.y() : a.segStart.x() < b.segStart.x() ;
}

Segment::Segment(Cord begin, Cord end, DIRECTION dir):
    segStart(begin), segEnd(end), direction(dir) { ; }

Segment::Segment(Cord begin, Cord end):
    segStart(begin), segEnd(end), direction(DIRECTION::NONE) { ; }

Segment::Segment():
    segStart(Cord(0,0)), segEnd(Cord(0,0)), direction(DIRECTION::NONE) { ; }

Segment::Segment(const Segment& other){
    this->segStart = other.segStart;
    this->segEnd = other.segEnd;
    this->direction = other.direction;
}

int Segment::getLength() const {
    return gtl::euclidean_distance(segStart, segEnd);
}

// always returns the leftmost/downmost point
Cord Segment::getSegStart() const {
    return segStart < segEnd ? segStart : segEnd;
}

// always returns the rightmost/upmost point
Cord Segment::getSegEnd() const {
    return segStart < segEnd ? segEnd : segStart;
}

DIRECTION Segment::getDirection() const {
    return direction;
}

void Segment::setSegStart(Cord start){
    segStart = start;
}

void Segment::setSegEnd(Cord end){
    segEnd = end;
}

void Segment::setDirection(DIRECTION dir){
    direction = dir;
}

std::ostream& operator<<(std::ostream& out, const Segment& seg){
    switch (seg.getDirection())
    {
    case DIRECTION::TOP:
        out << "┷: ";
        break;
    
    case DIRECTION::RIGHT:
        out << "┠: ";
        break;
    
    case DIRECTION::DOWN:
        out << "┯: ";
        break;
    
    case DIRECTION::LEFT:
        out << "┨: ";
        break;
    default:
        out << "?: ";
        break;
    }
    Cord start = seg.getSegStart();
    Cord end = seg.getSegEnd();
    out << "(" << start.x() <<", "<< start.y() << ")→(" << end.x() << ", " << end.y() << ")";
    return out;
    
}

// find the segment, such that when an area is extended in the normal direction of seg will 
// collide with this segment.
Segment FindNearestOverlappingInterval(Segment& seg, Polygon90Set& poly){
    int segmentOrientation; // 0 = segments are X direction, 1 = Y direction
    Segment closestSegment = seg;
    int closestDistance = INT_MAX;
    if (seg.getDirection() == DIRECTION::DOWN || seg.getDirection() == DIRECTION::TOP){
        segmentOrientation = 0;
    }
    else if (seg.getDirection() == DIRECTION::RIGHT || seg.getDirection() == DIRECTION::LEFT){
        segmentOrientation = 1;
    }
    else {
        return seg;
    }

    std::vector<Polygon90WithHoles> polyContainer;
    // int currentDepth, currentStart, currentEnd;
    poly.get_polygons(polyContainer);
    for (Polygon90WithHoles poly: polyContainer) {
        Point firstPoint, prevPoint, currentPoint(0,0);
        int counter = 0;
        // iterate through all edges of polygon
        for (auto it = poly.begin(); it != poly.end(); it++){
            // std::cout << counter << ": (" << (*it).x() << ","<< (*it).y() << ")\n";
            prevPoint = currentPoint;
            currentPoint = *it;

            if (counter++ == 0){
                firstPoint = currentPoint;
                continue; 
            }

            Segment currentSegment(prevPoint, currentPoint);
            int currentOrientation = prevPoint.y() == currentPoint.y() ? 0 : 1;

            if (segmentOrientation == currentOrientation){
                switch (seg.getDirection())
                {
                case DIRECTION::TOP:
                {
                    // test if y coord is larger
                    bool overlaps = ((currentSegment.getSegStart().x() < seg.getSegEnd().x()) && (seg.getSegStart().x() < currentSegment.getSegEnd().x()));
                    bool isAbove = currentSegment.getSegStart().y() > seg.getSegStart().y();
                    if (overlaps && isAbove){
                        int distance = currentSegment.getSegStart().y() - seg.getSegStart().y();
                        if (distance < closestDistance){
                            closestDistance = distance;
                            closestSegment = currentSegment;
                        }
                    }
                    break;
                }
                case DIRECTION::RIGHT:
                {
                    // test if x coord is larger
                    bool overlaps = ((currentSegment.getSegStart().y() < seg.getSegEnd().y()) && (seg.getSegStart().y() < currentSegment.getSegEnd().y()));
                    bool isRight = currentSegment.getSegStart().x() > seg.getSegStart().x();
                    if (overlaps && isRight){
                        int distance = currentSegment.getSegStart().x() - seg.getSegStart().x();
                        if (distance < closestDistance){
                            closestDistance = distance;
                            closestSegment = currentSegment;
                        }
                    }
                    break;
                }
                case DIRECTION::DOWN:
                {
                    // test if y coord is smaller
                    bool overlaps = ((currentSegment.getSegStart().x() < seg.getSegEnd().x()) && (seg.getSegStart().x() < currentSegment.getSegEnd().x()));
                    bool isBelow = currentSegment.getSegStart().y() < seg.getSegStart().y();
                    if (overlaps && isBelow){
                        int distance = seg.getSegStart().y() -currentSegment.getSegStart().y();
                        if (distance < closestDistance){
                            closestDistance = distance;
                            closestSegment = currentSegment;
                        }
                    }
                    break;
                }
                case DIRECTION::LEFT:
                {
                    // test if x coord is smaller
                    bool overlaps = ((currentSegment.getSegStart().y() < seg.getSegEnd().y()) && (seg.getSegStart().y() < currentSegment.getSegEnd().y()));
                    bool isLeft = currentSegment.getSegStart().x() < seg.getSegStart().x();
                    if (overlaps && isLeft){
                        int distance = seg.getSegStart().x() - currentSegment.getSegStart().x();
                        if (distance < closestDistance){
                            closestDistance = distance;
                            closestSegment = currentSegment;
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
            else {
                continue;
            }
            
        }
        //compare to first point again
        Segment lastSegment(currentPoint, firstPoint);
        int lastOrientation = currentPoint.y() == firstPoint.y() ? 0 : 1;  
                 
        if (segmentOrientation == lastOrientation){
            switch (seg.getDirection())
            {
            case DIRECTION::TOP:
            {
                // test if y coord is larger
                bool overlaps = ((lastSegment.getSegStart().x() < seg.getSegEnd().x()) && (seg.getSegStart().x() < lastSegment.getSegEnd().x()));
                bool isAbove = lastSegment.getSegStart().y() > seg.getSegStart().y();
                if (overlaps && isAbove){
                    int distance = lastSegment.getSegStart().y() - seg.getSegStart().y();
                    if (distance < closestDistance){
                        closestDistance = distance;
                        closestSegment = lastSegment;
                    }
                }
                break;
            }
            case DIRECTION::RIGHT:
            {
                // test if x coord is larger
                bool overlaps = ((lastSegment.getSegStart().y() < seg.getSegEnd().y()) && (seg.getSegStart().y() < lastSegment.getSegEnd().y()));
                bool isRight = lastSegment.getSegStart().x() > seg.getSegStart().x();
                if (overlaps && isRight){
                    int distance = lastSegment.getSegStart().x() - seg.getSegStart().x();
                    if (distance < closestDistance){
                        closestDistance = distance;
                        closestSegment = lastSegment;
                    }
                }
                break;
            }
            case DIRECTION::DOWN:
            {
                // test if y coord is smaller
                bool overlaps = ((lastSegment.getSegStart().x() < seg.getSegEnd().x()) && (seg.getSegStart().x() < lastSegment.getSegEnd().x()));
                bool isBelow = lastSegment.getSegStart().y() < seg.getSegStart().y();
                if (overlaps && isBelow){
                    int distance = seg.getSegStart().y() -lastSegment.getSegStart().y();
                    if (distance < closestDistance){
                        closestDistance = distance;
                        closestSegment = lastSegment;
                    }
                }
                break;
            }
            case DIRECTION::LEFT:
            {
                // test if x coord is smaller
                bool overlaps = ((lastSegment.getSegStart().y() < seg.getSegEnd().y()) && (seg.getSegStart().y() < lastSegment.getSegEnd().y()));
                bool isLeft = lastSegment.getSegStart().x() < seg.getSegStart().x();
                if (overlaps && isLeft){
                    int distance = seg.getSegStart().x() - lastSegment.getSegStart().x();
                    if (distance < closestDistance){
                        closestDistance = distance;
                        closestSegment = lastSegment;
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    }   

    return closestSegment;
}

}