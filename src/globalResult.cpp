#include <fstream>
#include <iostream>
#include <cmath>
#include <sstream>

#include "globalResult.h"
#include "cSException.h"

GlobalResult::GlobalResult()
    : blockCount(0), connectionCount(0), chipWidth(0), chipHeight(0) {
}

GlobalResult::GlobalResult(std::string globalResultFile){
    readGlobalResult(globalResultFile);
}

GlobalResult::GlobalResult(const GlobalResult &other)
    : blockCount(other.blockCount), connectionCount(other.connectionCount), chipWidth(other.chipWidth), chipHeight(other.chipHeight){
    this->blocks = std::vector<GlobalResultBlock>(other.blocks);
    this->connections = std::vector<GlobalResultConnection>(other.connections);
}

GlobalResult &GlobalResult::operator = (const GlobalResult &other){
    this->blockCount = other.blockCount;
    this->connectionCount = other.connectionCount;
    this->chipWidth = other.chipWidth;
    this->chipHeight = other.chipHeight;

    this->blocks = std::vector<GlobalResultBlock>(other.blocks);
    this->connections = std::vector<GlobalResultConnection>(other.connections);

    return (*this);
}

bool GlobalResult::operator == (const GlobalResult &other) const {
    bool attributeEqual = ((blockCount == other.blockCount) && (connectionCount == other.connectionCount) &&
            (chipWidth == other.chipWidth) && (chipHeight == other.chipHeight)); 

    bool vecSizeEqual = ((blocks.size() == other.blocks.size()) && (connections.size() == other.connections.size()));

    if(!(attributeEqual && vecSizeEqual)) return false;

    for(int i = 0; i < blocks.size(); ++i){
        GlobalResultBlock aBlk = this->blocks[i];
        GlobalResultBlock bBlk = other.blocks[i];
        if((aBlk.name != bBlk.name) || (aBlk.type != bBlk.type) || (aBlk.llx != bBlk.llx) || (aBlk.lly != bBlk.lly) || (aBlk.width != bBlk.width) || (aBlk.height != bBlk.height)){
            return false;
        }
    }

    for(int i = 0; i < connections.size(); ++i){
        const GlobalResultConnection *aCnn = &(this->connections[i]);
        const GlobalResultConnection *bCnn = &(other.connections[i]);

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

void GlobalResult::readGlobalResult(std::string globalResultFile){
    std::ifstream ifs;
    ifs.open(globalResultFile);
    if(!ifs.is_open()){
        throw CSException("GLOBALRESULT_01");
    }
    readGlobalResult(ifs);
}

void GlobalResult::readGlobalResult(std::ifstream &ifs){
    if(!ifs.is_open()){
        throw CSException("GLOBALRESULT_02");
    }

    std::string rawLine;

    int blockNum = 0;
    std::string tword;
    int tnum;

    ifs >> tword >> this->blockCount >> tword >> this->connectionCount;
    ifs >> this->chipWidth >> this->chipHeight;

    for(int i = 0; i < this->blockCount; ++i){
        GlobalResultBlock grb;
        double initX, initY, initW, initH;
        ifs >> grb.name >> grb.type >> grb.legalArea >> initX >> initY >> initW >> initH;
        initX = round(initX);
        initY = round(initY);

        // adjust the placement if block is out of chip contour 
        if(initX < 0) initX = 0;
        if(initY < 0) initY = 0;

        if((initX + initW) > this->chipWidth) initX = (this->chipWidth - initW);
        if((initY + initH) > this->chipHeight) initY = (this->chipHeight - initH);

        if((initX < 0) || (initY < 0) || ((initX + initW) > this->chipWidth) || ((initY + initH) > this->chipHeight)){
            throw CSException("GLOBALRESULT_03");
        }

        grb.llx = initX;
        grb.lly = initY;
        grb.width = initW;
        grb.height = initH;

        this->blocks.push_back(grb);
    }
    std::getline(ifs, rawLine);
    std::stringstream ss; 

    for(int i = 0; i < this->connectionCount; ++i){
        GlobalResultConnection grc;
        std::getline(ifs, rawLine);
        ss = std::stringstream(rawLine);
        while(ss >> tword){
            grc.vertices.push_back(tword);
        }
        grc.weight = std::stod(grc.vertices.back());
        grc.vertices.pop_back();
        if(grc.vertices.size() < 2){
            throw CSException("GLOBALRESULT_04");
        }
        this->connections.push_back(grc);
    }

}

size_t std::hash<GlobalResult>::operator()(const GlobalResult &key) const {
    return std::hash<int>()(key.blockCount) ^ std::hash<int>()(key.connectionCount) ^ std::hash<int>()(key.chipWidth) ^ std::hash<int>()(key.chipHeight);
}

std::ostream &operator << (std::ostream &os, const GlobalResultBlock &grb){
    os << grb.name << " " << grb.type << " " << grb.llx << " " << grb.lly << " " << grb.width << " " << grb.height;
    return os;

}
std::ostream &operator << (std::ostream &os, const GlobalResultConnection &grc){
    for(std::string const &vertex : grc.vertices){
        os << vertex << " ";
    }
    os << grc.weight;

    return os;
}

std::ostream &operator << (std::ostream &os, const GlobalResult &gr){
    os << "BLOCK " << gr.blockCount << " CONNECTION " << gr.connectionCount << std::endl;
    os << gr.chipWidth << " " << gr.chipHeight << std::endl;

    for(GlobalResultBlock const &grb : gr.blocks){
        os << grb << std::endl;
    }

    const GlobalResultConnection *grc;
    int connectionNum = gr.connections.size();
    for(int i = 0; i < connectionNum; ++i){
        grc = &(gr.connections[i]);
        os << *grc;
        if(i != (connectionNum - 1)) os << std::endl;
    }

    return os;
}