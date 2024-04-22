#ifndef __LINE_H__
#define __LINE_H__

#include <ostream>

#include "boost/polygon/polygon.hpp"
#include "units.h"
#include "cord.h"
#include "tile.h"

class Line{
private:
    Cord mLow;
    Cord mHigh;
    orientation2D mOrient;

public:

    Line();
    Line(Cord low, Cord high);
    Line(const Line &other);

    Line &operator = (const Line &other);
    bool operator == (const Line &comp) const;
    friend std::ostream &operator << (std::ostream &os, const Line &t);

    Cord getLow() const;
    Cord getHigh() const;
    orientation2D getOrient() const;

    void setLow(Cord newLow);
    void setHigh(Cord newHigh);

    len_t calculateLength() const;

    // if the line "touches the tile", return yes even if it touches the right/top border
    bool inTile(Tile *tile) const;
};

namespace std{
    template<>
    struct hash<Line>{
        size_t operator()(const Line &key) const;
    };
}

std::ostream &operator << (std::ostream &os, const Line &l);

#endif  // #define __LINE_H__