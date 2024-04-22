#include <iostream>

#include "tile.h"
#include "cSException.h"


Tile::Tile()
    : mType(tileType::BLANK), mRectangle(Rectangle(0, 0, 0, 0)), rt(nullptr), tr(nullptr), bl(nullptr), lb(nullptr) {
}

Tile::Tile(tileType t, Rectangle rect)
    : mType(t), mRectangle(rect), rt(nullptr), tr(nullptr), bl(nullptr), lb(nullptr) {
}

Tile::Tile(tileType t, Cord ll, len_t w, len_t h)
    : mType(t), mRectangle(Rectangle(ll.x(), ll.y(), (len_t)(ll.x() + w), (len_t)(ll.y() + h))), rt(nullptr), tr(nullptr), bl(nullptr), lb(nullptr) {
}

Tile::Tile(tileType t, Cord ll, Cord ur)
    : mType(t), mRectangle(Rectangle(ll.x(), ll.y(), ur.x(), ur.y())), rt(nullptr), tr(nullptr), bl(nullptr), lb(nullptr) {
}

Tile::Tile(const Tile &other)
    : mType(other.getType()), mRectangle(other.getRectangle()), rt(other.rt), tr(other.tr), bl(other.bl), lb(other.lb) {
}

Tile &Tile::operator = (const Tile &other) {
    if (this == &other) return (*this);

    this->mType = other.getType();
    this->mRectangle = other.getRectangle();

    this->rt = other.rt;
    this->tr = other.tr;
    this->bl = other.bl;
    this->lb = other.lb;

    return (*this);
}

bool Tile::operator == (const Tile &comp) const {
    return (mType == comp.getType()) && (mRectangle == comp.getRectangle()) &&
           ((rt == comp.rt) && (tr == comp.tr) && (bl == comp.bl) && (lb == comp.lb));
}

tileType Tile::getType() const {
    return this->mType;
}
Rectangle Tile::getRectangle() const {
    return this->mRectangle;
}

len_t Tile::getWidth() const {
    return rec::getWidth(this->mRectangle);
}
len_t Tile::getHeight() const {
    return rec::getHeight(this->mRectangle);
};

area_t Tile::getArea() const {
    return rec::getArea(this->mRectangle);
}

len_t Tile::getXLow() const {
    return rec::getXL(this->mRectangle);
};
len_t Tile::getXHigh() const {
    return rec::getXH(this->mRectangle);
};
len_t Tile::getYLow() const {
    return rec::getYL(this->mRectangle);
};
len_t Tile::getYHigh() const {
    return rec::getYH(this->mRectangle);
};

Cord Tile::getLowerLeft() const {
    return rec::getLL(this->mRectangle);
};
Cord Tile::getLowerRight() const {
    return rec::getLR(this->mRectangle);
};
Cord Tile::getUpperLeft() const {
    return rec::getUL(this->mRectangle);
};
Cord Tile::getUpperRight() const {
    return rec::getUR(this->mRectangle);
};

void Tile::setType(tileType type) {
    this->mType = type;
}

void Tile::setWidth(len_t width) {
    using namespace boost::polygon;
    this->mRectangle = Rectangle(xl(mRectangle), yl(mRectangle), (xl(mRectangle) + width), yh(mRectangle));
};
void Tile::setHeight(len_t height) {
    using namespace boost::polygon;
    this->mRectangle = Rectangle(xl(mRectangle), yl(mRectangle), xh(mRectangle), (yl(mRectangle) + height));
};

void Tile::setLowerLeft(Cord lowerLeft) {
    this->mRectangle = Rectangle(lowerLeft.x(), lowerLeft.y(), (lowerLeft.x() + getWidth()), (lowerLeft.y() + getHeight()));
};

double Tile::calAspectRatio() const {
    return rec::calculateAspectRatio(this->mRectangle);
};

bool Tile::checkCordInTile(const Cord& point) const{
    return  (point.x() >= getXLow()) &&
            (point.x() < getXHigh()) &&
            (point.y() >= getYLow()) &&
            (point.y() < getYHigh());
}

size_t std::hash<Tile>::operator()(const Tile &key) const {
    return (std::hash<int>()((int)key.getType())) ^ (std::hash<Rectangle>()(key.getRectangle()));
}

std::ostream &operator<<(std::ostream &os, const Tile &tile) {
    os << "T[";
    os << tile.mType << ", " << tile.mRectangle;
    os << "]";
    return os;
}

std::ostream &operator << (std::ostream &os, const enum tileType &t){
    switch (t){
        case tileType::BLOCK:
            os << "BLOCK";
            break;
        case tileType::BLANK:
            os << "BLANK";
            break;
        case tileType::OVERLAP:
            os << "OVERLAP";
            break;
        default:
            throw CSException("TILE_01");
    }
    return os;
}
