#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "legalResult.h"
#include "cSException.h"

LegalResult::LegalResult()
    : chipWidth(0), chipHeight(0), totalBlockCount(0), softBlockCount(0), fixedBlockCount(0), connectionCount(0) {
}

LegalResult::LegalResult(std::string legalResultFile){
   readLegalResult(legalResultFile); 
}

LegalResult::LegalResult(const LegalResult &other)
    : chipWidth(other.chipWidth), chipHeight(other.chipHeight), totalBlockCount(other.totalBlockCount),
    softBlockCount(other.softBlockCount), fixedBlockCount(other.fixedBlockCount), connectionCount(other.connectionCount) {
        this->softBlocks = std::vector<LegalResultBlock>(other.softBlocks);
        this->fixedBlocks = std::vector<LegalResultBlock>(other.fixedBlocks);
        this->connections = std::vector<LegalResultConnection>(other.connections);
}

LegalResult &LegalResult::operator = (const LegalResult &other){
    this->chipWidth = other.chipWidth;
    this->chipHeight = other.chipHeight;
    this->totalBlockCount = other.totalBlockCount;
    this->softBlockCount = other.softBlockCount;
    this->fixedBlockCount = other.fixedBlockCount;
    this->connectionCount = other.connectionCount;

    this->softBlocks = std::vector<LegalResultBlock>(other.softBlocks);
    this->fixedBlocks = std::vector<LegalResultBlock>(other.fixedBlocks);
    this->connections = std::vector<LegalResultConnection>(other.connections);

    return(*this);
}

bool LegalResult::operator == (const LegalResult &other) const {
    bool attributeEqual = (chipWidth == other.chipWidth) && (chipHeight == other.chipHeight) &&
            (totalBlockCount == other.totalBlockCount) && (softBlockCount == other.softBlockCount) && (fixedBlockCount == other.fixedBlockCount) &&
            (connectionCount == other.connectionCount);

    bool vecSizeEqual = (softBlocks.size() == other.softBlocks.size()) && (fixedBlocks.size() == other.fixedBlocks.size()) && (connections.size() == other.connections.size());

    if(!(attributeEqual && vecSizeEqual)) return false;

    for(int i = 0; i < softBlocks.size(); ++i){
        const LegalResultBlock *aBlk = &(this->softBlocks[i]);
        const LegalResultBlock *bBlk = &(other.softBlocks[i]);

        if((aBlk->name != bBlk->name) || (aBlk->legalArea != bBlk->legalArea) ||
             (aBlk->cornerCount != bBlk->cornerCount) || (aBlk->xVec != bBlk->xVec) || (aBlk->yVec != bBlk->yVec)){
                return false;
        }
    }

    for(int i = 0; i < fixedBlocks.size(); ++i){
        const LegalResultBlock *aBlk = &(this->fixedBlocks[i]);
        const LegalResultBlock *bBlk = &(other.fixedBlocks[i]);

        if((aBlk->name != bBlk->name) || (aBlk->legalArea != bBlk->legalArea) ||
            (aBlk->cornerCount != bBlk->cornerCount) || (aBlk->xVec != bBlk->xVec) || (aBlk->yVec != bBlk->yVec)){
                return false;
        }
    }

    for(int i = 0; i < connections.size(); ++i){
        const LegalResultConnection *aCnn = &(this->connections[i]);
        const LegalResultConnection *bCnn = &(other.connections[i]);

        if(aCnn->weight != bCnn->weight) return false;

        const std::vector<std::string> *aVts = &(aCnn->vertices);
        const std::vector<std::string> *bVts = &(bCnn->vertices);
        if(aVts->size() != bVts->size()) return false;

        for(int i = 0; i < aVts->size(); ++i){
            if(((*aVts)[i]) != ((*bVts)[i])) return false;
        }
    }

    return true;
}

void LegalResult::readLegalResult(std::string legalResultFile){
    std::ifstream ifs;
    ifs.open(legalResultFile);
    if(!ifs.is_open()){
        throw CSException("LEGALRESULT_01");
    }
    readLegalResult(ifs);
}

void LegalResult::readLegalResult(std::ifstream &ifs){
    if(!ifs.is_open()){
        throw CSException("LEGALRESULT_02");
    }

    std::string rawLine;
    
    std::string tword;

    // process the possible casexx as the first line
    while(tword != "BLOCK"){
        ifs >> tword;
    }
    ifs >> this->totalBlockCount >> this->chipWidth >> this->chipHeight;
    
    // read softblocks
    ifs >> tword >> this->softBlockCount;
    for(int i = 0; i < this->softBlockCount; ++i){
        LegalResultBlock lrb;
        int numCorners;
        int tInt;
        ifs >> lrb.name >> lrb.legalArea >> numCorners;
        if(numCorners < 4){
            throw CSException("LEGALRESULT_03");
        }

        lrb.cornerCount = numCorners;
        for(int j = 0; j < numCorners; ++j){
           ifs >> tInt; 
           lrb.xVec.push_back(tInt);
           ifs >> tInt; 
           lrb.yVec.push_back(tInt);
        }

        this->softBlocks.push_back(lrb);
    }

    // read fixedblocks
    ifs >> tword >> this->fixedBlockCount;
    for(int i = 0; i < this->fixedBlockCount; ++i){
        LegalResultBlock lrb;
        int numCorners;
        int tInt;
        ifs >> lrb.name >> lrb.legalArea >> numCorners;
        if(numCorners < 4){
            throw CSException("LEGALRESULT_04");
        }

        lrb.cornerCount = numCorners;
        for(int j = 0; j < numCorners; ++j){
           ifs >> tInt; 
           lrb.xVec.push_back(tInt);
           ifs >> tInt; 
           lrb.yVec.push_back(tInt);
        }

        this->fixedBlocks.push_back(lrb);
    }

    // read connections
    ifs >> tword >> this->connectionCount;
    std::getline(ifs, rawLine);
    std::stringstream ss;

    for(int i = 0; i < this->connectionCount; ++i){
        LegalResultConnection grc;
        std::getline(ifs, rawLine);
        ss = std::stringstream(rawLine);
        while(ss >> tword){
            grc.vertices.push_back(tword);
        }
        grc.weight = std::stod(grc.vertices.back());
        grc.vertices.pop_back();
        if(grc.vertices.size() < 2){
            throw CSException("LEGALRESULT_05");
        }
        this->connections.push_back(grc);
    }
}


size_t std::hash<LegalResult>::operator()(const LegalResult &key) const {
    return std::hash<int>()(key.softBlockCount) ^ std::hash<int>()(key.fixedBlockCount) ^ std::hash<int>()(key.connectionCount) ^ std::hash<int>()(key.chipWidth) ^ std::hash<int>()(key.chipHeight);
}

std::ostream &operator << (std::ostream &os, const LegalResultBlock &lrb){
    os << lrb.name << " " << lrb.legalArea << " " << lrb.cornerCount;
    for(int i = 0; i < lrb.cornerCount; ++i){
        os << std::endl << lrb.xVec[i] << " " << lrb.yVec[i];
    }

    return os;
}

std::ostream &operator << (std::ostream &os, const LegalResultConnection &lrc){
    for(std::string const &vertex : lrc.vertices){
        os << vertex << " ";
    }
    os << lrc.weight;

    return os;
}

std::ostream &operator << (std::ostream &os, const LegalResult &lr){
    os << "BLOCK" << " " << lr.totalBlockCount << std::endl;
    os << lr.chipWidth << " " << lr.chipHeight << std::endl;

    os << "SOFTBLOCKS" << " " << lr.softBlockCount << std::endl;
    for(LegalResultBlock const &lrb : lr.softBlocks){
        os << lrb << std::endl;
    }
    
    os << "FIXEDBLOCKS" << " " << lr.fixedBlockCount << std::endl;
    for(LegalResultBlock const &lrb : lr.fixedBlocks){
        os << lrb << std::endl;
    }

    os << "CONNECTION" << " " << lr.connectionCount << std::endl;
    const LegalResultConnection *grc;
    int connectionNum = lr.connectionCount;
    for(int i = 0; i < connectionNum; ++i){
        grc = &(lr.connections[i]);
        os << *grc;
        if(i != (connectionNum - 1)) os << std::endl;
    }

    return os;
}





