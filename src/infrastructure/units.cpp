#include <math.h>
#include "units.h"

std::ostream &operator << (std::ostream &os, const direction1D &w){
    if(w == direction1D::CLOCKWISE) os << "direction1D::CLOCKWISE";
    else os << "direction1D::COUNTERCLOCKWISE";

    return os;
}

std::ostream &operator << (std::ostream &os, const orientation2D &o){
    if(o == orientation2D::HORIZONTAL) os << "orientation2D::HORIZONTAL";
    else os << "orientation2D::VERTICAL";

    return os;
}

std::ostream &operator << (std::ostream &os, const direction2D &d){
    switch (d){
    case direction2D::NORTH:
        os << "NORTH(UP)";
        break;
    case direction2D::NORTHEAST:
        os << "NORTHEAST";
        break;
    case direction2D::EAST:
        os << "EAST(RIGHT)";
        break;
    case direction2D::SOUTHEAST:
        os << "SOUTHEAST";
        break;
    case direction2D::SOUTH:
        os << "SOUTH(DOWN)";
        break;
    case direction2D::SOUTHWEST:
        os << "SOUTHWEST";
        break;
    case direction2D::WEST:
        os << "WEST(LEFT)";
        break;
    case direction2D::NORTHWEST:
        os << "NORTHWEST";
        break;
    case direction2D::CENTRE:
        os << "CENTRE";
        break;
    default:
        break;
    }
    
    return os;
}

std::ostream &operator << (std::ostream &os, const quadrant &q){
    switch (q){
        case quadrant::EMPTY:
            os << "quadrant::EMPTY";
            break;
        case quadrant::I:
            os << "Quadrant::I";
            break;
        case quadrant::II:
            os << "Quadrant::II";
            break;
        case quadrant::III:
            os << "Quadrant::III";
            break;
        case quadrant::IV:
            os << "quadrant::IV";
            break;
        default:
            break;
    }
    
    return os;
}

std::ostream &operator << (std::ostream &os, const sector &s){
    switch (s){
        case sector::EMPTY:
            os << "Sector::EMPTY";
            break;
        case sector::ONE:
            os << "Sector::ONE";
            break;
        case sector::TWO:
            os << "Sector::TWO";
            break;
        case sector::THREE:
            os << "Sector::THREE";
            break;
        case sector::FOUR:
            os << "Sector::FOUR";
            break;
        case sector::FIVE:
            os << "Sector::FIVE";
            break;
        case sector::SIX:
            os << "Sector::SIX";
            break;
        case sector::SEVEN:
            os << "Sector::SEVEN";
            break;
        case sector::EIGHT:
            os << "Sector::EIGHT";
            break;
        default:
            break;
    }
    
    return os;
}

angle_t flipAngle(angle_t angle){
    return (angle > 0)? (angle - M_PI) : (angle + M_PI);
}
quadrant translateAngleToQuadrant(angle_t angle){

	if(angle >= 0){
		if(angle <= (M_PI / 2.0)) return quadrant::I;
		else return quadrant::II;
	}else{
		if(angle >= (- M_PI / 2.0)) return quadrant::IV;
		else return quadrant::III;
	}
}

sector translateAngleToSector(angle_t angle){
    if(angle >= 0){
        if(angle <=(M_PI / 2.0)){
            if(angle <= (M_PI / 4.0)) return sector::ONE;
            else return sector::TWO;
        }else{
            if(angle <= (3.0 * M_PI / 4.0)) return sector::THREE;
            else return sector::FOUR;
        }

    }else{
        if(angle >= (- M_PI / 2.0)){
            if(angle >= (- M_PI / 4.0)) return sector::EIGHT;
            else return sector::SEVEN;
        }else{
            if(angle >= (-3.0 * M_PI / 4.0)) return sector::SIX;
            else return sector::FIVE;
        }

    }
}