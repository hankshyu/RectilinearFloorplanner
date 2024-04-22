#include "eVector.h"
#include "math.h"

EVector::EVector()
    : ex(0), ey(0){
}

EVector::EVector(len_t ai, len_t bi)
    : ex(ai), ey(bi){
}

EVector::EVector(Cord from, Cord to)
    : ex(to.x() - from.x()), ey(to.y() - from.y()){
}

EVector::EVector(FCord from, Cord to)
    :ex(to.x() - len_t(from.x())), ey(to.y() - len_t(from.y())){
}

EVector::EVector(Cord from, FCord to)
    :ex(len_t(to.x()) - from.x()), ey(len_t(to.y()) - from.y()){
}
    
EVector::EVector(FCord from, FCord to)
    :ex(len_t(to.x()) - len_t(from.x())), ey(len_t(to.y()) - len_t(from.y())){
}

EVector::EVector(const EVector &other)
    : ex(other.ex), ey(other.ey){
}

EVector& EVector::operator = (const EVector &other){
    if (this == &other) return (*this);

    this->ex = other.ex;
    this->ey = other.ey;

    return (*this);
}

bool EVector::operator == (const EVector &comp) const{
    return (this->ex == comp.ex) && (this->ey == comp.ey);
}

len_t EVector::calculateL1Magnitude() const {
    return std::abs(this->ex) + std::abs(this->ey);
}

flen_t EVector::calculateL2Magnitude() const {
    return std::sqrt((this->ex * this->ex) + (this->ey * this->ey));
}


angle_t EVector::calculateDirection() const {
    return std::atan2(double(this->ey), double(this->ex));
}


angle_t vec::calculateAngle(const EVector &v1, const EVector &v2){
    
    len_t dotProduct = v1.ex * v2.ex + v1.ey * v2.ey;
    return std::acos( dotProduct/(v1.calculateL2Magnitude() * v2.calculateL2Magnitude()));
}

size_t std::hash<EVector>::operator()(const EVector &key) const {
    return (std::hash<len_t>()(key.ex)) ^ (std::hash<len_t>()(key.ey));
}

std::ostream &operator << (std::ostream &os, const EVector &v){
    os << "V[" << v.ex << ", " << v.ey << "]";
    return os;
}



