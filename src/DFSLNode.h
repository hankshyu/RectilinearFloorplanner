#ifndef _DFSLNODE_H_
#define _DFSLNODE_H_

#include <vector>
#include <set>
#include <unordered_set>
#include <string>

#include "tile.h"
#include "rectilinear.h"

#include "DFSLEdge.h"

namespace DFSL {

enum class DFSLNodeType : unsigned char { OVERLAP, FIXED, SOFT, BLANK };

class DFSLNode;

class DFSLNode {
private:
    // tileListUnion mTileListPtr;
    std::unordered_set<Tile *>* blockSet;
    std::set<Tile *> overlapTiles;
public:
    std::string nodeName;
    DFSLNodeType nodeType;
    int index;
    // std::vector<Tile*> tileList; 
    std::vector<DFSLEdge> edgeList;
    std::set<int> overlaps;
    Rectilinear* recti;

    DFSLNode();
    DFSLNode(std::string name, DFSLNodeType nodeType, int index);
    DFSLNode(const DFSLNode& other);
    ~DFSLNode();

    std::unordered_set<Tile *>& getBlockTileList();
    std::set<Tile *>& getOverlapTileList();

    void addOverlapTile(Tile* newTile);
    // void removeTile(Tile* deleteTile);
    void initOverlapNode(int overlapIndex1, int overlapIndex2);
    void initBlockNode(Rectilinear* recti);
    int getArea();
};

}
#endif