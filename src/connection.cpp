#include "connection.h"
#include "cSException.h"

Connection::Connection()
    : weight(0) {
}

Connection::Connection(const std::vector<Rectilinear *> &vertices, double weight)
    :  vertices(vertices), weight(weight) {
}

Connection::Connection(const Connection &other)
    : vertices(other.vertices), weight(other.weight) {
}

Connection &Connection::operator = (const Connection &other){

    if(this == &other) return (*this);

    this->vertices = std::vector<Rectilinear *>(other.vertices);
    this->weight = other.weight;

    return (*this);
}

bool Connection::operator == (const Connection &comp) const {
    return (this->weight == comp.weight) && (this->vertices == comp.vertices);
}

double Connection::calculateCost() const {
    if(this->vertices.size() < 2){
        throw CSException("CONNECTION_01");
    }
  
    Rectangle boundingBox = this->vertices[0]->calculateBoundingBox();
    
    double boundingBoxCentreX, boundingBoxCentreY;
    
    rec::calculateCentre(boundingBox, boundingBoxCentreX, boundingBoxCentreY);
    
    double minX, maxX, minY, maxY;
    minX = maxX = boundingBoxCentreX;
    minY = maxY = boundingBoxCentreY;

    for(int i = 1; i < vertices.size(); ++i){
        boundingBox = this->vertices[i]->calculateBoundingBox();
        rec::calculateCentre(boundingBox, boundingBoxCentreX, boundingBoxCentreY);

        if(boundingBoxCentreX < minX) minX = boundingBoxCentreX;
        if(boundingBoxCentreX > maxX) maxX = boundingBoxCentreX;
        if(boundingBoxCentreY < minY) minY = boundingBoxCentreY;
        if(boundingBoxCentreY > maxY) maxY = boundingBoxCentreY;
    }

    return this->weight * ((maxX - minX)+(maxY - minY));
}

size_t std::hash<Connection>::operator()(const Connection &key) const {
    
    size_t hashValue = std::hash<double>()(key.weight);
    for(Rectilinear *r : key.vertices){
        hashValue ^= std::hash<Rectilinear *>()(r);
    }

    return hashValue;
}

std::ostream &operator << (std::ostream &os, const Connection &c){
    os << "CONN[ x" << c.weight << ", ";
    Rectilinear *r;

    for(int i = 0; i < c.vertices.size() - 1; ++i){
        os << c.vertices[i]->getName() << ", ";
    }
    os << c.vertices[c.vertices.size() - 1]->getName() << " = " << c.calculateCost() << "]";

    return os;
}
