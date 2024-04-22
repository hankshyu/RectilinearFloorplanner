#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <vector>

#include "rectilinear.h"

class Connection{
public:
    std::vector<Rectilinear *> vertices;
    double weight;

    Connection();
    Connection(const std::vector<Rectilinear *> &vertices, double weight);
    Connection(const Connection &other);

    Connection& operator = (const Connection &other);
    bool operator == (const Connection &comp) const;
    friend std::ostream &operator << (std::ostream &os, const Tile &t);
    
    double calculateCost() const;
};

namespace std{
    template<>
    struct hash<Connection>{
        size_t operator()(const Connection &key) const;
    };
}

std::ostream &operator << (std::ostream &os, const Connection &c);

#endif // __CONNECTION_H__