#include <iostream>
#include <cassert>

#include "rectilinear.h"

#include "DFSLNode.h"

namespace DFSL{

DFSLNode::DFSLNode():
    blockSet(NULL),
    nodeName(""), nodeType(DFSLNodeType::BLANK), index(-1), recti(NULL)
{ ; }

DFSLNode::DFSLNode(std::string name, DFSLNodeType type, int id):
    blockSet(NULL),
    nodeName(name), nodeType(type), index(id), recti(NULL)
{ ; }

DFSLNode::DFSLNode(const DFSLNode& other){
    this->blockSet = other.blockSet;
    this->overlapTiles = other.overlapTiles;

    this->nodeName = other.nodeName;
    this->nodeType = other.nodeType;
    this->index = other.index;
    this->edgeList = other.edgeList;
    this->overlaps = other.overlaps;
    this->recti = other.recti;
}

DFSLNode::~DFSLNode(){
}

void DFSLNode::initOverlapNode(int overlapIndex1, int overlapIndex2){
    overlaps.insert(overlapIndex1);
    overlaps.insert(overlapIndex2);
    overlapTiles.clear();
}

void DFSLNode::initBlockNode(Rectilinear* recti){
    blockSet = &(recti->blockTiles);
    this->recti = recti;
}

int DFSLNode::getArea(){
    int area = 0;
    if (nodeType == DFSLNodeType::OVERLAP){
        for (Tile* tile: getOverlapTileList()){
            area += tile->getArea();
        }
    }
    else if (nodeType == DFSLNodeType::FIXED){
        for (Tile* tile: getBlockTileList()){
            area += tile->getArea();
        }
    }
    else if (nodeType == DFSLNodeType::SOFT){
        for (Tile* tile: getBlockTileList()){
            area += tile->getArea();
        }
    }
    return area;
}

std::unordered_set<Tile *>& DFSLNode::getBlockTileList(){
    return *(blockSet);
}

std::set<Tile *>& DFSLNode::getOverlapTileList(){
    return overlapTiles;
}

void DFSLNode::addOverlapTile(Tile* newTile){
    if (nodeType != DFSLNodeType::OVERLAP){
        std::cerr << "addOverlapTile can only be used on overlap tiles" << std::endl;
        return;
    }
    overlapTiles.insert(newTile);
}

}