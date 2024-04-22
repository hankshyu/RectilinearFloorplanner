#ifndef __GLOBALRESULT_H__
#define __GLOBALRESULT_H__

#include <vector>
#include <string>
#include <iostream>

struct GlobalResultBlock{
    std::string name;
    std::string type;

    int legalArea;
    int llx, lly;
    int width, height;
};

struct GlobalResultConnection{
    std::vector<std::string> vertices;
    double weight;
};

class GlobalResult{
public:
    int blockCount;
    int connectionCount;
    int chipWidth, chipHeight;

    std::vector<GlobalResultBlock> blocks;
    std::vector<GlobalResultConnection> connections;

    GlobalResult();
    GlobalResult(std::string globalResultFile);
    GlobalResult(const GlobalResult &other);

    GlobalResult &operator = (const GlobalResult &other);
    bool operator == (const GlobalResult &other) const;
    friend std::ostream &operator << (std::ostream &os, const GlobalResult &gr);

    void readGlobalResult(std::ifstream &is);
    void readGlobalResult(std::string globalResultFile);
};

namespace std{
    template<>
    struct hash<GlobalResult>{
        size_t operator()(const GlobalResult &key) const;
    };
}

std::ostream &operator << (std::ostream &os, const GlobalResultBlock &grb);
std::ostream &operator << (std::ostream &os, const GlobalResultConnection &grc);
std::ostream &operator << (std::ostream &os, const GlobalResult &gr);

#endif // __GLOBALRESULT_H__