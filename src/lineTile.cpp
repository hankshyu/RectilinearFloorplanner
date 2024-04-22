#include "lineTile.h"
#include "cSException.h"

LineTile::LineTile()
    :mLine(Line()), mTile(nullptr), mDirection(direction2D::CENTRE){
}

LineTile::LineTile(Line line, Tile *tile){
    if(tile == nullptr) throw CSException("LINETILE_01");

    if(line.getOrient() == orientation2D::HORIZONTAL){
        
        // check if the line is well fit with the tile
        if(tile->getXHigh() < line.getHigh().x()) throw CSException("LINETILE_02");
        if(tile->getXLow() > line.getLow().x()) throw CSException("LINETILE_03");
        
        len_t lineY = line.getLow().y();
        len_t tileYHigh = tile->getYHigh();
        len_t tileYLow = tile->getYLow();
        if(lineY > tileYHigh) throw CSException("LINETILE_04");
        if(lineY < tileYLow) throw CSException("LINETILE_05");

        if(lineY == tileYHigh) mDirection = direction2D::DOWN;
        else if(lineY == tileYLow) mDirection = direction2D::UP;
        else mDirection = direction2D::CENTRE;

        mLine = line;
        mTile = tile;

    }else{ //orientation2D::VERTICAL

        // check if the lie is wel fit with the tile
        if(tile->getYHigh() < line.getHigh().y()) throw CSException("LINETILE_06");
        if(tile->getYLow() > line.getLow().y()) throw CSException("LINETILE_07");

        len_t lineX = line.getLow().x();
        len_t tileXHigh = tile->getXHigh();
        len_t tileXLow = tile->getXLow();
        if(lineX > tileXHigh) throw CSException("LINETILE_08");
        if(lineX < tileXLow) throw CSException("LINETILE_09");

        if(lineX == tileXHigh) mDirection = direction2D::LEFT;
        else if(lineX == tileXLow) mDirection = direction2D::RIGHT;
        else mDirection = direction2D::CENTRE;

        mLine = line;
        mTile = tile;
    }
}

LineTile::LineTile(const LineTile &other)
    : mLine(other.mLine), mTile(other.mTile), mDirection(other.mDirection) {
}


LineTile &LineTile::operator = (const LineTile &other) {
    if (this == &other) return (*this);

    this->mLine = other.mLine;
    this->mTile = other.mTile;
    this->mDirection = other.mDirection;

    return (*this);
}

bool LineTile::operator == (const LineTile &comp) const {
    return (mLine == comp.mLine) && (mTile == comp.mTile);

}

Line LineTile::getLine() const {
    return this->mLine;
}

Tile *LineTile::getTile() const {
    return this->mTile;
}

direction2D LineTile::getDirection() const {
    return this->mDirection;
}

size_t std::hash<LineTile>::operator()(const LineTile &key) const {
    return (std::hash<Line>()(key.getLine())) ^ (std::hash<Tile*>()(key.getTile()));
}

std::ostream &operator<<(std::ostream &os, const LineTile &tile) {
    os << "LT[";
    os << tile.getLine() << " " << *(tile.getTile()) << " " << tile.getDirection();
    os << "]";
    return os;
}