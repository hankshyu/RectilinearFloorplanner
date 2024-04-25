#ifndef __TILE_H__
#define __TILE_H__

#include "boost/polygon/polygon.hpp"
#include "units.h"
#include "cord.h"
#include "rectangle.h"

namespace gtl = boost::polygon;

enum class tileType{
    BLANK, BLOCK, OVERLAP
};

class Tile{
private:
    tileType mType;
    Rectangle mRectangle;

public:
    Tile *rt, *tr, *bl, *lb;

    Tile();
    Tile(tileType t, Rectangle rect);
    Tile(tileType t, Cord ll, len_t w, len_t h);
    Tile(tileType t, Cord ll, Cord ur);
    Tile(const Tile &other);

    Tile& operator = (const Tile &other);
    bool operator == (const Tile &comp) const;
    friend std::ostream &operator << (std::ostream &os, const Tile &t);
    
    tileType getType() const;
    Rectangle getRectangle() const;

    len_t getWidth() const;
    len_t getHeight() const;
    area_t getArea() const;

    len_t getXLow() const;
    len_t getXHigh() const; 
    len_t getYLow() const;
    len_t getYHigh() const;
     
    Cord getLowerLeft() const;
    Cord getUpperLeft() const;
    Cord getLowerRight() const;
    Cord getUpperRight() const;
    
    void setType(tileType type);
    void setWidth(len_t width);
    void setHeight(len_t height);
    void setLowerLeft(Cord lowerLeft);

    double calAspectRatio() const;
    
    // added by ryan
    // checks if point is contained within this tile
    bool checkCordInTile(const Cord& point) const;
};

// Implement hash function for map and set data structure
namespace std{
    template<>
    struct hash<Tile>{
        size_t operator()(const Tile &key) const;
    };
}

std::ostream &operator << (std::ostream &os, const Tile &t);
std::ostream &operator << (std::ostream &os, const enum tileType &t);

#endif // __TILE_H__