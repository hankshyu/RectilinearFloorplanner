#ifndef __CORNERSTITCHING_H__
#define __CORNERSTITCHING_H__

#include <unordered_map>
#include <unordered_set>

#include "units.h"
#include "cord.h"
#include "tile.h"
#include "lineTile.h"

class CornerStitching {
// private:
public:
    
    len_t mCanvasWidth, mCanvasHeight;
    Tile *mCanvasSizeBlankTile;

    std::unordered_map <Cord, Tile*> mAllNonBlankTilesMap;
    // return true if the object is inside the canvas, else return false
    bool checkPointInCanvas(const Cord &point) const;
    bool checkRectangleInCanvas(const Rectangle &rect) const;

    // Pick-up all tiles(include BLANK) inside the data structure 
    void collectAllTiles(std::unordered_set<Tile *> &allTiles) const;
    void collectAllTilesDFS(Tile *tile, std::unordered_set<Tile *> &allTiles) const;

    // Helper function of enumerateDirectArea
    void enumerateDirectedAreaRProcedure(Rectangle box, std::vector <Tile *> &allTiles, Tile *targetTile) const;

    // Helper functions of findLineTile
    void findLineTileHorizontalPositive(Tile *initTile, Line line, std::vector<LineTile> &positiveSide);
    void findLineTileHorizontalNegative(Tile *initTile, Line line, std::vector<LineTile> &negativeSide);
    void findLineTileVerticalPositive(Tile *initTile, Line line, std::vector<LineTile> &positiveSide);
    void findLineTileVerticalNegative(Tile *initTile, Line line, std::vector<LineTile> &negativeSide);


// public:
    CornerStitching();
    CornerStitching(len_t chipWidth, len_t chipHeight);
    CornerStitching(const CornerStitching &other);
    ~CornerStitching();
 
    bool operator == (const CornerStitching &comp) const;

    len_t getCanvasWidth() const;
    len_t getCanvasHeight() const;

    // Given a Cord, find the tile (could be balnk or block) that includes it.
    Tile *findPoint(const Cord &key) const;
    
    // Given a line segment, search for all tiles attatched to the line
    // If the line is horizontal:
    //  - positive side returns all tiles that the line tangents(marked UP/NORTH) or cross(marked CENTRE) on the upper portion of line
    //  - negative side returns all tiles that the line tangents(marked DOWN/SOUTH) or cross(marked CENTRE) on the lower portion of line
    // If the line is vertical:
    //  - positive side returns all tiles that the line tangents(marked RIGHT/EAST) or cross(marked CENTRE) on the right portion of line
    //  - negative side returns all tiles that the line tangents(marked LEFT/WEST) or cross(marked CENTRE) on the left portion of line
    // Note that if a the line crosses the 
    void findLineTile(Line &line, std::vector<LineTile> &positiveSide, std::vector<LineTile> &negativeSide);
    
    // Pushes all top neighbors of Tile "centre" to vector "neighbors", the first would be centre->tr, then to left (<--)
    void findTopNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const;
    // Pushes all down neighbors of Tile "centre" to vector "neighbors", the first would be centre->lb, then to right (-->) 
    void findDownNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const;
    // Pushes all left neighbors of Tile "centre" to vector "neighbors", the first would be centre->bl, then up (^)
    void findLeftNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const;
    // Pushes all right neighbors of Tile "centre" to vector "neighbors", the first would be centre->tr, then down (v)
    void findRightNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const;
    // Pushes all neighbors (in the order of top, left, down, right) of Tile "centre" to vector "neighbors"
    // Short hand for continuoutsly call, find Top, Left, Down, Right neighbors
    void findAllNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const;    

    // Determine if there is any nonblank tiles in the box area
    bool searchArea(Rectangle box) const;
    // Determine if there is any nonblank tiles in the box area, return any tile in the region if found (via pointer)
    bool searchArea(Rectangle box, Tile *someTile) const;

    // Enumerates all nonblank tiles in a given box area, each tile is visited only after all the tiles above and to its left does
    void enumerateDirectedArea(Rectangle box, std::vector <Tile *> &allTiles) const;

    // Input a tile "prototype" to insert into the corner stitching system, returns the actual pointer to the tile in the system
    Tile *insertTile(const Tile &tile);

    // Removes the tile within system, retunrs false if Tile not within cornerStitching System. true if succes
    void removeTile(Tile *tile);

    // Pass in a victim tile through origTop, it will split the victim into two pieces:
    // 1. origTop represents the top portion of the split, with height (origTop.height - newDownHeight)
    // 2. newDown represents the lower portion of the split, with height newDownHeight, is the return value
    Tile *cutTileHorizontally(Tile *origTop, len_t newDownHeight);

    // Pass in a victim tile through origRight, it will stplit the victim into two pieces:
    // 1. origRight represents the right portion of the split, with width (origRight.width - newLeftWidth)
    // 2. newLeft represents the left portion of the split, with width newLeftWidth, is the return value
    Tile *cutTileVertically(Tile *origRight, len_t newLeftWidth);

    // Merges two tiles (up, down), mergeDown is merged into mergeUp, mergeDown is deleted and the new merged tile is the return value
    Tile *mergeTilesVertically(Tile *mergeUp, Tile *mergeDown);

    // Merges two tiles (left, right), mergeRight is merged into mergeLeft, mergeRight is deleted and the new merged tile is the return value
    Tile *mergeTilesHorizontally(Tile *mergeLeft, Tile *mergeRight);

    // write cornerStitching class (composed of Tiles) for presenting software (renderCornerStitching.py)
    void visualiseCornerStitching(const std::string &ouputFileName) const;
    
    // Check globally if any two tiles is mergable, returns true if no fails found, else return false and
    // two unmergable tiles is returned through tile1 and tile2
    bool debugBlankMerged(Tile *&tile1, Tile *&tile2) const;

    // Check globally if two pointer-linked tiles share the same edge
    // two detatched tiles is returned through tile1 and tile2
    bool debugPointerAttatched(Tile *&tile1, Tile *&tile2) const;

    // self checking function, wrapper of debugBlankMerged(...) and debugPointerAttatched(...)
    bool conductSelfTest()const;

    // added by ryan
    // given a tile orignalTile, carves a tile at newArea while spliting the corner stitching
    // of the tile 
    // The tile representing newArea is returned (in fact it is the same pointer as originalTile),
    // and the newly constrcuted neighbors are added to newNeighbors
    Tile* generalSplitTile(Tile* originalTile, Rectangle newArea, std::vector<Tile*>& newNeighbors);
};
#endif // __CORNERSTITCHING_H__