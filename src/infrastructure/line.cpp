#include "line.h"
#include "cSException.h"

Line::Line()
    : mLow(Cord(0, 0)), mHigh(Cord(LEN_T_MAX, 0)), mOrient(orientation2D::HORIZONTAL){
}

Line::Line(Cord low, Cord high) {
    if(low.x() == high.x()){
        mOrient = orientation2D::VERTICAL;
        if(low.y() < high.y()){
            mLow = low;
            mHigh = high;
        }else if(low.y() > high.y()){
            mLow = high;
            mHigh = low;
        }else{
            throw CSException("LINE_01");
        }
    }else if(low.y() == high.y()){
        mOrient = orientation2D::HORIZONTAL;
        if(low.x() < high.x()){
            mLow = low;
            mHigh = high;
        }else if(low.x() > high.x()){
            mLow = high;
            mHigh = low;
        }else{
            throw CSException("LINE_01");
        }
    }else{
        throw CSException("LINE_02");
    }

}

Line::Line(const Line &other)
    : mLow(other.mLow), mHigh(other.mHigh), mOrient(other.mOrient) {
}

Line &Line::operator = (const Line &other) {
    if(this == &other) return (*this);

    this->mLow = other.getLow();
    this->mHigh = other.getHigh();
    this->mOrient = other.getOrient();

    return (*this);
}

bool Line::operator == (const Line &comp) const {
    return (this->mLow == comp.getLow()) && (this->mHigh == comp.getHigh());
}

Cord Line::getLow() const {
    return this->mLow;
}

Cord Line::getHigh() const {
    return this->mHigh;
}

orientation2D Line::getOrient() const {
    return this->mOrient;
}

void Line::setLow(Cord newLow) {
    if(newLow == mHigh) throw CSException("LINE_03");

    if(newLow.x() == mHigh.x()){
        mOrient = orientation2D::VERTICAL;
        if(newLow.y() < mHigh.y()){
            mLow = newLow;
        }else{
            mLow = mHigh;
            mHigh = newLow;
        }
    }else if(newLow.y() == mHigh.y()){
        mOrient = orientation2D::HORIZONTAL;
        if(newLow.x() < mHigh.x()){
            mLow = newLow;
        }else{
            mLow = mHigh;
            mHigh = newLow;
        }
    }else{
        throw CSException("LINE_04");
    }
}

void Line::setHigh(Cord newHigh) {
    if(newHigh == mHigh) throw CSException("LINE_05");

    if(newHigh.x() == mLow.x()){
        mOrient = orientation2D::VERTICAL;
        if(newHigh.y() > mLow.y()){
            mHigh = newHigh;
        }else{
            mHigh = mLow;
            mLow = newHigh;
        }
    }else if(newHigh.y() == mLow.y()){
        mOrient = orientation2D::HORIZONTAL;
        if(newHigh.x() > mLow.x()){
            mHigh = newHigh;
        }else{
            mHigh = mLow;
            mLow = newHigh;
        }
    }else{
        throw CSException("LINE_06");
    }
}

len_t Line::calculateLength() const {
    if(this->mOrient == orientation2D::HORIZONTAL){
        return mHigh.x() - mLow.x();
    }else{
        return mHigh.y() - mLow.y();
    }
}

bool Line::inTile(Tile *tile) const{
    if(mOrient == orientation2D::HORIZONTAL){
        len_t lineY = mLow.y(); 
		len_t lineXLow = mLow.x();
		len_t lineXHigh = mHigh.x();

		len_t tileXLow = tile->getXLow();
		len_t tileXHigh = tile->getXHigh();
		len_t tileYLow = tile->getYLow();
		len_t tileYHigh = tile->getYHigh();

        bool yDownInRange = (lineY >= tileYLow);
        bool yUpInRange = (lineY <= tileYHigh);
        bool yCordInTile = (yDownInRange && yUpInRange);

        bool lineToTileLeft = (lineXHigh <= tileXLow);
        bool lineToTileRight =(lineXLow >= tileXHigh);

        return yCordInTile && (!(lineToTileLeft || lineToTileRight));

    }else{ // orientation2D::VERTICAL
        len_t lineX = mLow.x();
        len_t lineYLow = mLow.y();
        len_t lineYHigh = mHigh.y();

		len_t tileXLow = tile->getXLow();
		len_t tileXHigh = tile->getXHigh();
		len_t tileYLow = tile->getYLow();
		len_t tileYHigh = tile->getYHigh();

        bool xDownInRange = (lineX >= tileXLow);
        bool xUpInRange = (lineX <= tileXHigh);
        bool xCordInTile = (xDownInRange && xUpInRange);

        bool linetoTileDown = (lineYHigh <= tileYLow);
        bool linetoTileUp = (lineYLow >= tileYHigh);

        return xCordInTile && (!(linetoTileDown || linetoTileUp));
    }

}

size_t std::hash<Line>::operator()(const Line &key) const {
    return (std::hash<Cord>()(key.getLow())) ^ (std::hash<Cord>()(key.getHigh()));
}

std::ostream &operator<<(std::ostream &os, const Line &line) {
    os << "L[";
    os << line.mLow << " --- " << ((line.mOrient == orientation2D::HORIZONTAL)? "H" : "V") << " --- " << line.mHigh;
    os << "]";
    
    return os;
}




