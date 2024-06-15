#ifndef _DFSLEGALIZER_H_
#define _DFSLEGALIZER_H_

#include <string>
#include <cstdarg>
#include <vector>
#include <map>
#include <array>

#include "boost/polygon/polygon.hpp"
#include "boost/format.hpp"

#include "floorplan.h"

#include "DFSLConfig.h"
#include "DFSLNode.h"
#include "DFSLEdge.h"
#include "DFSLUnits.h"

#define EPSILON 0.0001
// #define UTIL_RULE 0.8
// #define ASPECT_RATIO_RULE 2.0 

namespace DFSL {

using namespace boost::polygon::operators;

struct MigrationEdge;

enum class RESULT : unsigned char { SUCCESS, OVERLAP_NOT_RESOLVED, AREA_CONSTRAINT_FAIL, OTHER_CONSTRAINT_FAIL };

class DFSLegalizer{
private:
    std::vector<DFSLNode> mAllNodes;
    double mBestCost;
    int mMigratingArea;
    int mResolvableArea;
    std::vector<MigrationEdge> mBestPath;
    std::vector<MigrationEdge> mCurrentPath;
    std::multimap<Tile*, int> mTilePtr2NodeIndex;
    Floorplan* mFP;
    int mFixedModuleNum;
    int mSoftModuleNum;
    int mOverlapNum;

    // initialize related functions
    void getSoftNeighbors(int nodeId);
    void addBlockNode(Rectilinear* tess, bool isFixed);
    void addOverlapInfo(Tile* tile);
    void addSingleOverlapInfo(Tile* tile, int overlapIdx1, int overlapIdx2);
    std::string toOverlapName(int tessIndex1, int tessIndex2);
    void findOBEdge(int fromIndex, int toIndex);

    // related to neighbor graph
    void updateGraph();
    void updateOverlapNode(DFSLNode& overlapNode);
    void checkNeighborDiscrepancies(DFSLNode& blockNode, std::set<int>& toUpdate);
    void updateBlockNodes(std::set<int>& toUpdate);

    // DFS path finding
    void dfs(DFSLEdge& edge, double currentCost);
    MigrationEdge getEdgeCost(DFSLEdge& edge);

    // functions that change physical layout
    bool migrateOverlap(int overlapIndex);
    bool splitOverlap(MigrationEdge& edge, std::vector<Tile*>& newTiles);
    void removeTileFromOverlap(Tile* overlapTile, DFSLNode& overlapNode,DFSLNode& toNode);
    bool splitSoftBlock(MigrationEdge& edge, std::vector<Tile*>& newTiles);
    bool placeInBlank(MigrationEdge& edge, std::vector<Tile*>& newTiles);
    void restructureRectis();

    // end of flow last-ditch efforts to legalize
    void lastDitchLegalize();
    void legalizeArea(DFSLNode& block, Rectilinear* recti);
    void legalizeAspectRatio(Rectilinear* recti, double aspectRatio);
    void legalizeUtil(Rectilinear* recti);
    void legalizeHole(Rectilinear* recti);
    void legalizeFragmented(Rectilinear* recti);
    // Added by Hank
    void legalizeInnerWidth(Rectilinear *recti);
    void fillBoundingBox(Rectilinear* recti, Rectangle& Bbox);
    RESULT checkLegal();

    // misc
    int overlapsRemaining();
    template<typename... Args>
    void DFSLPrint(int level, const std::string& fmt, const Args&... args)
    {
        if (config->getConfigValue<int>("OutputLevel") >= level){
            if ( level == DFSL_ERROR )
                printf( "[DFSL] ERROR  : " );
            else if ( level == DFSL_WARNING )
                printf( "[DFSL] WARNING: " );
            else if (level == DFSL_STANDARD){
                printf( "[DFSL] INFO   : ");
            }
            std::cout << (boost::format(fmt) % ... % args);
        }
    }

public:
    ConfigList* config;

    DFSLegalizer();
    ~DFSLegalizer();

    int getSoftBegin();
    int getSoftEnd();
    int getFixedBegin();
    int getFixedEnd();
    int getOverlapBegin();
    int getOverlapEnd();

    void initDFSLegalizer(Floorplan* floorplan, ConfigList* configList);
    void constructGraph();
    RESULT legalize(int mode);
    void setOutputLevel(int level);

    // io
    void printFloorplanStats();
    void printTiledFloorplan(const std::string& outputFileName, const std::string& floorplanName);
    void printCompleteFloorplan(const std::string& outputFileName, const std::string& floorplanName);
    void printAllNodesDebug();
};


static void DFSLNode2PolySet(DFSLNode& node, Polygon90Set& polySet);
static Segment findTangentSegment(Tile* from, Tile* to, int direction);

// [0]: area, [1]: util, [2]: aspect ratio
static std::array<double,3> getPolySetInfo(Polygon90Set& poly);

// from the direction of edge.segment, grow a rectangle equal to the area of goalArea
static Rectangle getRectFromEdge(MigrationEdge& edge, int goalArea, bool useCeil = true);

// from the direction of edge.segment, grow a rectangle equal to the area of goalArea, while also growing a
// unit-width/height rectangle at the end, such that the extra area is exactly equal to goal area
// [0]: rectangle calculated with getRectfromEdge(useCeil=false)
// [1]: rectangle with unit width/height whose height/width is equal to remainder area
static std::array<Rectangle,2> getRectFromEdgeExact(MigrationEdge& edge, int goalArea);

// find the rectangle that extends from segment,
// returns the smallest rectangle that is >= required area (if no overlaps)
// or returns the largest possible rectangle that doesn't overlap 
// with other tiles
static Rectangle extendSegment(Segment& seg, int requiredArea, Floorplan* fp);

}

#endif