#ifndef __FLOORPLAN_H__
#define __FLOORPLAN_H__

#include <string.h>
#include <unordered_map>

#include "tile.h"
#include "lineTile.h"
#include "rectangle.h"
#include "rectilinear.h"
#include "connection.h"
#include "cornerStitching.h"
#include "globalResult.h"
#include "legalResult.h"
#include "doughnutPolygonSet.h"
#include "eVector.h"

class Floorplan{
private:
    int mIDCounter;

    Rectangle mChipContour;

    int mAllRectilinearCount;
    int mSoftRectilinearCount;
    int mPreplacedRectilinearCount;
    int mPinRectilinearCount;

    int mConnectionCount;

    double mGlobalAspectRatioMin;
    double mGlobalAspectRatioMax;
    double mGlobalUtilizationMin;
    
    // function that places a rectilinear into the floorplan system. It automatically resolves overlaps by splittng and divide existing tiles
    Rectilinear *placeRectilinear(std::string name, rectilinearType type, Rectangle placement, area_t legalArea, double aspectRatioMin, double aspectRatioMax, double mUtilizationMin);

public:
    CornerStitching *cs;

    // allRectilinears is softRectilinears + preplacedRectilinears
    std::vector<Rectilinear *> allRectilinears;
    std::vector<Rectilinear *> softRectilinears;
    std::vector<Rectilinear *> preplacedRectilinears;

    // pinRectilinears should only be used for HPWL calculation
    std::vector<Rectilinear *> pinRectilinears;

    std::vector<Connection *> allConnections;
    std::unordered_map<Rectilinear *, std::vector<Connection *>> connectionMap;
    
    std::unordered_map<Tile *, Rectilinear *> blockTilePayload;
    std::unordered_map<Tile *, std::vector<Rectilinear *>> overlapTilePayload;


    Floorplan();
    Floorplan(const GlobalResult &gr, double aspectRatioMin, double aspectRatioMax, double utilizationMin);
    Floorplan(const LegalResult &lr, double aspectRatioMin, double aspectRatioMax, double utilizationMin);
    Floorplan(const Floorplan &other);
    ~Floorplan();

    Floorplan& operator = (const Floorplan &other);

    Rectangle getChipContour() const;
    int getAllRectilinearCount() const;
    int getSoftRectilinearCount() const;
    int getPreplacedRectilinearCount() const;
    int getPinRectilinearCount() const;
    int getConnectionCount() const;
    double getGlobalAspectRatioMin() const;
    double getGlobalAspectRatioMax() const;
    double getGlobalUtilizationMin() const;

    void setGlobalAspectRatioMin(double globalAspectRatioMin);
    void setGlobalAspectRatioMax(double globalAspectRatioMax);
    void setGlobalUtilizationMin(double globalUtilizationMin);

    // insert a tleType::BLOCK tile at tilePosition into cornerStitching & rectilinear (*rt) system,
    // record rt as it's payload into floorplan system (into blockTilePayload) and return new tile's pointer
    Tile *addBlockTile(const Rectangle &tilePosition, Rectilinear *rt);

    // insert a tleType::OVERLAP tile at tilePosition into cornerStitching & rectilinear (•rt) system,
    // record payload as it's payload into floorplan system (into overlapTilePayload) and return new tile's pointer
    Tile *addOverlapTile(const Rectangle &tilePosition, const std::vector<Rectilinear*> &payload);

    // remove tile data payload at floorplan system, the rectilienar that records it and lastly remove from cornerStitching,
    // only tile.type == tileType::BLOCK or tileType::OVERLAP is accepted
    void deleteTile(Tile *tile);

    // log newRect as it's overlap tiles and update Rectilinear structure & floorplan paylaod, upgrade tile to tile::OVERLAP if necessary 
    void increaseTileOverlap(Tile *tile, Rectilinear *newRect);

    // remove removeRect as tile's overlap and update Rectilinear structure & floorplan payload, make tile tile::BLOCK if necessary
    void decreaseTileOverlap(Tile *tile, Rectilinear *removeRect);

    // collect all blocks within a rectilinear, reDice them into Rectangles. (potentially reduce Tile count)
    void reshapeRectilinear(Rectilinear *rt);

    // grow the shape toGrow to the Rectilinear
    void growRectilinear(std::vector<DoughnutPolygon> &toGrow, Rectilinear *rect);

    // take the shape toShrink off the Rectilinear
    void shrinkRectilinear(std::vector<DoughnutPolygon> &toShrink, Rectilinear *rect);

    // Pass in the victim tile pointer through origTop, it will split the tile into two pieces:
    // 1. origTop represents the top portion of the split, with height (origTop.height - newDownHeight)
    // 2. newDown represents the lower portion of the split, with height newDownHeight, is the return value
    Tile *divideTileHorizontally(Tile *origTop, len_t newDownHeight);

    // Pass in the victim tile pointer through origRight, it will stplit the tile into two pieces:
    // 1. origRight represents the right portion of the split, with width (origRight.width - newLeftWidth)
    // 2. newLeft represents the left portion of the split, with width newLeftWidth, is the return value
    Tile *divideTileVertically(Tile *origRight, len_t newLeftWidth);

    // calculate the HPWL (cost) of the floorplan system, using the connections information stored inside "allConnections"
    double calculateHPWL() const;

    // calculate the optimal centre of a soft rectilnear, return Cord(-1, -1) if no connection is present
    Cord calculateOptimalCentre(Rectilinear *rect) const;
    void calculateAllOptimalCentre(std::unordered_map<Rectilinear *, Cord> &optCentre) const;

    bool calculateRectilinearGradient(Rectilinear *rect, EVector &gradient) const;
    
    double calculateOverlapRatio() const;

    // use area rounding residuals to remove certain easy to remove overlaps
    // void removePrimitiveOvelaps(bool verbose);

    // check if the floorplan is legal, return nullptr if floorplan legal, otherwise return first met faulty Rectilinear
    Rectilinear *checkFloorplanLegal(rectilinearIllegalType &illegalType) const;

    bool debugFloorplanLegal() const;

    // added by ryan
    // visuailseLegalFloorplan with 2 modifications
    // 1. can handle case with fragmented modules 
    // 2. draws overlapTiles
    void visualiseRectiFloorplan(const std::string &outputFileName) const;
    
    // added by ryan
    // visualizes each tile in the floorplan (for use in utils/draw_tile_layout.py)
    void visualizeTiledFloorplan(const std::string& outputFileName) const;

    // write Floorplan class for presenting software (renderFloorplan.py)
    void visualiseFloorplan(const std::string &outputFileName) const;

    void visualiseICCADFormat(const std::string &outputFileName) const;

    // added by ryan
    // given a tile that belongs in fromRect, remove that tile from fromRect, and place it in toRect
    // updates payloads 
    void moveTileParent(Tile* tile, Rectilinear* fromRect, Rectilinear* toRect);

    // added by ryan
    // given a tile orignalTile, carves a tile at newArea while spliting the corner stitching
    // of the tile 
    // The tile representing newArea is returned (in fact it is the same pointer as originalTile),
    // and payload is updated
    Tile* generalSplitTile(Tile* tile, Rectangle newArea);
};

namespace std{
    template<>
    struct hash<Floorplan>{
        size_t operator()(const Floorplan &key) const;
    };
}

#endif // #define __FLOORPLAN_H__