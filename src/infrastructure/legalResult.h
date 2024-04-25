#ifndef __LEGALRESULT_H__
#define __LEGALRESULT_H__

#include <vector>
#include <string>
#include <iostream>

struct LegalResultBlock{
    std::string name;

    int legalArea;
    int cornerCount;
    
    std::vector<int> xVec;
    std::vector<int> yVec;
};

struct LegalResultConnection{
    std::vector<std::string> vertices;
    double weight;
};

class LegalResult{
public:
    int chipWidth, chipHeight;

    int totalBlockCount;
    int softBlockCount;
    int fixedBlockCount;

    int connectionCount;

    std::vector<LegalResultBlock> softBlocks;
    std::vector<LegalResultBlock> fixedBlocks;

    std::vector<LegalResultConnection> connections;

    LegalResult();
    LegalResult(std::string legalResultFile);
    LegalResult(const LegalResult &other);

    LegalResult &operator = (const LegalResult &other);
    bool operator == (const LegalResult &other) const;
    friend std::ostream &operator << (std::ostream &os, const LegalResult &lr);

    void readLegalResult(std::string globalResultFile);
    void readLegalResult(std::ifstream &is);

};

namespace std{
    template<>
    struct hash<LegalResult>{
        size_t operator()(const LegalResult &key) const;
    };
}

std::ostream &operator << (std::ostream &os, const LegalResultBlock &lrb);
std::ostream &operator << (std::ostream &os, const LegalResultConnection &lrc);
std::ostream &operator << (std::ostream &os, const LegalResult &lr);

#endif // __LEGALRESULT_H__