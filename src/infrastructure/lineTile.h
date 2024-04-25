#ifndef __LINETILE_H__
#define __LINETILE_H__

#include "line.h"
#include "tile.h"
#include "units.h"

class LineTile{
private:
    Line mLine;
    Tile *mTile;
    direction2D mDirection;

public:
    LineTile();
    LineTile(Line line, Tile *tile);
    LineTile(const LineTile &other);

    LineTile &operator = (const LineTile &other);
    bool operator == (const LineTile &comp) const;
    friend std::ostream &operator << (std::ostream &os, const LineTile &t);

    Line getLine() const;
    Tile *getTile() const;
    direction2D getDirection() const;

};

namespace std{
    template<>
    struct hash<LineTile>{
        size_t operator()(const LineTile &key) const;
    };
}

std::ostream &operator << (std::ostream &os, const LineTile &l);
#endif // __LINETILE_H__