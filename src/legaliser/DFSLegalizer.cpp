
#include <vector>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <sstream>
#include <fstream>
#include <ctime> 
#include <cstdarg>
#include <iostream>

#include "rectilinear.h"
#include "rectangle.h"

#include "DFSLegalizer.h"
#include "DFSLConfig.h"

namespace DFSL {

DFSLegalizer::DFSLegalizer(): config(NULL)
{
}

DFSLegalizer::~DFSLegalizer()
{
}


int DFSLegalizer::getSoftBegin(){
    return 0;
}
int DFSLegalizer::getSoftEnd(){
    return mSoftModuleNum;
}
int DFSLegalizer::getFixedBegin(){
    return mSoftModuleNum;
}
int DFSLegalizer::getFixedEnd(){
    return mSoftModuleNum + mFixedModuleNum;
}
int DFSLegalizer::getOverlapBegin(){
    return mSoftModuleNum + mFixedModuleNum;
}
int DFSLegalizer::getOverlapEnd(){
    return mSoftModuleNum + mFixedModuleNum + mOverlapNum;
}

// void DFSLegalizer::DFSLPrint(int level, const char* fmt, ...){
//     va_list args;
//     va_start( args, fmt );
//     if (config->getConfigValue<int>("OutputLevel") >= level){
//         if ( level == DFSL_ERROR )
//             printf( "[DFSL] ERROR  : " );
//         else if ( level == DFSL_WARNING )
//             printf( "[DFSL] WARNING: " );
//         else if (level == DFSL_STANDARD){
//             printf( "[DFSL] INFO   : ");
//         }
//         vprintf( fmt, args );
//         va_end( args );
//     }
// }

// note: relies on the fact that rectilinear shapes in floorplan are ordered in:
// soft blocks -> fixed blocks
void DFSLegalizer::initDFSLegalizer(Floorplan* floorplan, ConfigList* configList){
    mFP = floorplan;
    config = configList;
    constructGraph();
}

void DFSLegalizer::addBlockNode(Rectilinear* recti, bool isFixed){
    int index = mAllNodes.size();
    DFSLNodeType nodeType = isFixed ? DFSLNodeType::FIXED : DFSLNodeType::SOFT;
    mAllNodes.push_back(DFSLNode(recti->getName(), nodeType, index));
    mAllNodes.back().initBlockNode(recti);    
}

void DFSLegalizer::constructGraph(){
    mAllNodes.clear();
    // mTilePtr2NodeIndex.clear();
    mFixedModuleNum = mFP->getPreplacedRectilinearCount();
    mSoftModuleNum = mFP->getSoftRectilinearCount();

    // find fixed and soft tess
    for(int t = 0; t < mSoftModuleNum; t++){
        Rectilinear* recti = mFP->softRectilinears[t];
        addBlockNode(recti, false);
    }

    for(int t = 0; t < mFixedModuleNum; t++){
        Rectilinear* recti = mFP->preplacedRectilinears[t];
        addBlockNode(recti, true);
    }

    // find overlaps 
    for(int t = 0; t < mSoftModuleNum; t++){
        Rectilinear* recti = mFP->softRectilinears[t];
        for(Tile* overlap : recti->overlapTiles){
            addOverlapInfo(overlap);
        }
    }

    for(int t = 0; t < mFixedModuleNum; t++){
        Rectilinear* recti = mFP->preplacedRectilinears[t];
        for(Tile* overlap : recti->overlapTiles){
            addOverlapInfo(overlap);
        }
    }
    mOverlapNum = mAllNodes.size() - mFixedModuleNum - mSoftModuleNum;

    // find neighbors
    // overlap and block
    int overlapStartIndex = getOverlapBegin();
    int overlapEndIndex = getOverlapEnd();
    for (int from = overlapStartIndex; from < overlapEndIndex; from++){
        DFSLNode& overlap = mAllNodes[from];
        for (int to: overlap.overlaps){
            if (getSoftBegin() <= to && to < getSoftEnd()){
                findOBEdge(from, to);
            }
        }
    }

    // block and block, block and whitespace
    int softStartIndex = getSoftBegin();
    int softEndIndex = getSoftEnd();
    for (int from = softStartIndex; from < softEndIndex; from++){
        getSoftNeighbors(from);
    }

    printAllNodesDebug();
}

void DFSLegalizer::addOverlapInfo(Tile* tile){
    std::vector<int> allOverlaps;
    for (Rectilinear* recti: mFP->overlapTilePayload[tile]){
        allOverlaps.push_back(recti->getId());
    }

    for (int i = 0; i < allOverlaps.size(); i++){
        int tessIndex1 = allOverlaps[i];

        for (int j = i+1; j < allOverlaps.size(); j++){
            int tessIndex2 = allOverlaps[j];
            addSingleOverlapInfo(tile, tessIndex1, tessIndex2);
        }
    }
}

void DFSLegalizer::addSingleOverlapInfo(Tile* tile, int overlapIdx1, int overlapIdx2){
    bool found = false;
    for (int i = mFixedModuleNum + mSoftModuleNum; i < mAllNodes.size(); i++){
        DFSLNode& rectiNode = mAllNodes[i];
        if (rectiNode.overlaps.count(overlapIdx1) == 1 && rectiNode.overlaps.count(overlapIdx2) == 1){
            if (rectiNode.getOverlapTileList().count(tile) == 0){
                rectiNode.addOverlapTile(tile);
            }
            found = true;
            break;
        }
    }
    if (!found){
        int index = mAllNodes.size();
        DFSLNodeType type = DFSLNodeType::OVERLAP;
        std::string name = toOverlapName(overlapIdx1, overlapIdx2);
        mAllNodes.push_back(DFSLNode(name, type, index));
        mAllNodes.back().initOverlapNode(overlapIdx1, overlapIdx2);
        mAllNodes.back().addOverlapTile(tile);
    }
}

std::string DFSLegalizer::toOverlapName(int tessIndex1, int tessIndex2){
    std::string name1 = mAllNodes[tessIndex1].nodeName;
    std::string name2 = mAllNodes[tessIndex2].nodeName;
    return "OVERLAP_" + name1 + "_" + name2;
}

void DFSLegalizer::findOBEdge(int fromIndex, int toIndex){
    DFSLNode& fromNode = mAllNodes[fromIndex];
    DFSLNode& toNode = mAllNodes[toIndex];

    // this takes care of the case where a block is completely covered by overlap tiles,
    // no edge from overlap to block
    if (fromNode.getArea() == 0 || toNode.getArea() == 0){
        return;
    }

    // return if not a overlap -> block edge
    if (!(getOverlapBegin() <= fromIndex && fromIndex < getOverlapEnd()) 
        || !(getSoftBegin() <= toIndex && toIndex < getSoftEnd())){
        return;
    }

    std::vector<Segment> allTangentSegments;
    for (int dir = 0; dir < 4; dir++){
        // find Top, right, bottom, left neighbros
        std::vector<Segment> currentSegments;
        for (Tile* tile: fromNode.getOverlapTileList()){
            std::vector<Tile*> neighbors;
            switch (dir) {
            case 0:
                mFP->cs->findTopNeighbors(tile, neighbors);
                break;
            case 1:
                mFP->cs->findRightNeighbors(tile, neighbors);
                break;
            case 2:
                mFP->cs->findDownNeighbors(tile, neighbors);
                break;
            case 3:
                mFP->cs->findLeftNeighbors(tile, neighbors);
                break;
            }
            
            for (Tile* neighbor: neighbors){
                if (neighbor->getType() != tileType::BLOCK){
                    continue;
                }
                int nodeIndex = mFP->blockTilePayload[neighbor]->getId();
                if (nodeIndex == toIndex){
                    currentSegments.push_back(findTangentSegment(tile, neighbor, dir));
                }
            }
        }
        
        // check segment, splice touching segments together and add to allTangentSegments
        if (currentSegments.size() > 0){
            if (dir == 0 || dir == 2){
                std::sort(currentSegments.begin(), currentSegments.end(), compareXSegment);
            }
            else {
                std::sort(currentSegments.begin(), currentSegments.end(), compareYSegment);
            }
            Cord segBegin = currentSegments.front().getSegStart();
            for (int j = 1; j < currentSegments.size(); j++){
                if (currentSegments[j].getSegStart() != currentSegments[j-1].getSegEnd()){
                    Cord segEnd = currentSegments[j-1].getSegEnd();
                    Segment splicedSegment(segBegin, segEnd, currentSegments[j-1].getDirection());
                    allTangentSegments.push_back(splicedSegment);

                    segBegin = currentSegments[j].getSegStart();
                }
            }
            Cord segEnd = currentSegments.back().getSegEnd();
            Segment splicedSegment(segBegin, segEnd, currentSegments.back().getDirection());

            allTangentSegments.push_back(splicedSegment);
        }
    }

    // construct edge in graph
    fromNode.edgeList.push_back(DFSLEdge(fromIndex, toIndex, EDGETYPE::OB, allTangentSegments));
}

void DFSLegalizer::getSoftNeighbors(int fromId){
    DFSLNode& fromNode = mAllNodes[fromId];
    DFSLPrint(4, "Get Soft Neighbors: %s\n", fromNode.nodeName);
    fromNode.edgeList.clear();

    // this takes care of the case where a block is completely covered by overlap tiles,
    // no edge from this block
    if (fromNode.getArea() == 0){
        return;
    }

    // return if not a soft block
    if (!(getSoftBegin() <= fromId && fromId < getSoftEnd())){
        return;
    }

    // <nodeindex, tangentsegments>, stores all the tangent segments 
    // correpsonding to each neighboring node
    std::map<int,std::vector<Segment>> allTangentSegments;
    for (int dir = 0; dir < 4; dir++){
        // find Top, right, bottom, left neighbros
        std::map<int, std::vector<Segment>> allCurrentTangentSegments;
        for (Tile* tile: fromNode.getBlockTileList()){
            std::vector<Tile*> neighbors;
            switch (dir) {
            case 0:
                mFP->cs->findTopNeighbors(tile, neighbors);
                break;
            case 1:
                mFP->cs->findRightNeighbors(tile, neighbors);
                break;
            case 2:
                mFP->cs->findDownNeighbors(tile, neighbors);
                break;
            case 3:
                mFP->cs->findLeftNeighbors(tile, neighbors);
                break;
            }
            
            for (Tile* neighbor: neighbors){
                // skip if neighbor is not block or whitespace
                if (neighbor->getType() != tileType::BLOCK && neighbor->getType() != tileType::BLANK){
                    continue;
                }

                int neighborBelongsTo;
                if (neighbor->getType() == tileType::BLANK){
                    neighborBelongsTo = -1;
                }
                else{
                    neighborBelongsTo = mFP->blockTilePayload[neighbor]->getId();
                    int fixedBegin = getFixedBegin();
                    int fixedEnd = getFixedEnd();
                    // skip if neighbor belongs to fixed block
                    // or neighbor is from same rectilinear
                    if (neighborBelongsTo == fromId || (fixedBegin <= neighborBelongsTo && neighborBelongsTo < fixedEnd)){
                        continue;
                    }
                }

                int segmentBelongsTo = neighborBelongsTo;
                Segment currentSegment = findTangentSegment(tile, neighbor, dir);

                if (allCurrentTangentSegments.count(segmentBelongsTo) == 1){
                    std::vector<Segment>& neighborTangents = allCurrentTangentSegments[segmentBelongsTo];
                    neighborTangents.push_back(currentSegment);
                }
                else {
                    allCurrentTangentSegments.insert(std::pair<int,std::vector<Segment>>(segmentBelongsTo, std::vector<Segment>(1,currentSegment)));
                }
            }
        }
        
        // check segment, splice touching segments together and add to allTangentSegments
        for (auto& nodeSegments: allCurrentTangentSegments){
            int neighborIndex = nodeSegments.first;
            std::vector<Segment>& currentSegments = nodeSegments.second;
            std::vector<Segment> currentSplicedTangentSegments;

            if (dir == 0 || dir == 2){
                std::sort(currentSegments.begin(), currentSegments.end(), compareXSegment);
            }
            else {
                std::sort(currentSegments.begin(), currentSegments.end(), compareYSegment);
            }
            Cord segBegin = currentSegments.front().getSegStart();
            for (int j = 1; j < currentSegments.size(); j++){
                // iterate through segments, if a segment has
                // 1. different endpoint than the last segment, or
                // 2. points to different neighbor
                // then splice all stored segments into one segment
                if (currentSegments[j].getSegStart() != currentSegments[j-1].getSegEnd()){
                    Cord segEnd = currentSegments[j-1].getSegEnd();
                    Segment splicedSegment(segBegin, segEnd, currentSegments[j-1].getDirection());

                    currentSplicedTangentSegments.push_back(splicedSegment);
                    segBegin = currentSegments[j].getSegStart();
                }
            }
            Cord segEnd = currentSegments.back().getSegEnd();
            Segment splicedSegment(segBegin, segEnd, currentSegments.back().getDirection());

            currentSplicedTangentSegments.push_back(splicedSegment);

            // push all spliced segments to edgelist in graph
            bool found = false;
            for (DFSLEdge& edge: fromNode.edgeList){
                if (edge.getTo() == neighborIndex){
                    found = true;

                    for (Segment& segment: currentSplicedTangentSegments){
                        edge.tangentSegments().push_back(segment);
                    }
                    
                    break;
                }
            }
            // construct edge if edge does not exist 
            if (!found){
                EDGETYPE edgeType;
                if (neighborIndex == -1){
                    edgeType = EDGETYPE::BW;
                }
                else {
                    edgeType = EDGETYPE::BB; 
                }
                fromNode.edgeList.push_back(DFSLEdge(fromId, neighborIndex, edgeType, currentSplicedTangentSegments));
            }
        }
    }
}

void DFSLegalizer::printFloorplanStats(){
    DFSLPrint(2, "Remaining overlaps: %d\n", overlapsRemaining());
    long overlapArea = 0, physicalArea = 0, dieArea = gtl::area(mFP->getChipContour());
    int overlapStart = getOverlapBegin();
    int overlapEnd = getOverlapEnd();
    for (int i = overlapStart; i < overlapEnd; i++){
        DFSLNode& currentOverlap = mAllNodes[i];
        overlapArea += currentOverlap.getArea();
    }
    
    int softBegin = getSoftBegin();
    int softEnd = getSoftEnd();
    int fixedBegin = getFixedBegin();
    int fixedEnd = getFixedEnd();
    for (int i = softBegin; i < softEnd; i++){
        DFSLNode& currentBlock = mAllNodes[i];
        physicalArea += currentBlock.getArea();
    }
    for (int i = fixedBegin; i < fixedEnd; i++){
        DFSLNode& currentBlock = mAllNodes[i];
        physicalArea += currentBlock.getArea();
    }
    double overlapOverDie = (double) overlapArea / (double) dieArea;
    double overlapOverPhysical = (double) overlapArea / (double) physicalArea;
    DFSLPrint(2, "Total Overlap Area: %1$6d\t(o/d = %2$5.4f%%\to/p = %3$5.4f%%)\n", overlapArea, overlapOverDie * 100, overlapOverPhysical * 100);
}


// mode 0: resolve area big -> area small
// mode 1: resolve area small -> area big
// mode 2: resolve overlaps near center -> outer edge (descending order of euclidean distance)
// mode 3: same as mode 2 but using manhattan distance
// mode 4: completely random
RESULT DFSLegalizer::legalize(int mode){
    // todo: create backup (deep copy)
    RESULT result = RESULT::SUCCESS;
    int iteration = 0;
    std::srand(69);
    
    struct timespec start, end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    
    DFSLPrint(2, "Starting Legalization\n");
    while (1) {
        if (iteration % 10 == 0){
            printFloorplanStats();
        }
        if (iteration % 30 == 0 && iteration != 0){
            DFSLPrint(DFSL_VERBOSE, "Restructuring rectilinears\n");
            restructureRectis();
            // printTiledFloorplan("debug_DFSL.txt", "debug");
        }
        std::cout << std::flush;
        int overlapStart = getOverlapBegin();
        int overlapEnd = getOverlapEnd();

        // some overlaps may be 0 area
        int actualOverlaps = overlapsRemaining();
        if (actualOverlaps == 0){
            break;
        }

        bool overlapResolved = false;
        std::vector<bool> solveable(mAllNodes.size(), true);
        int resolvableOverlaps = actualOverlaps;
        
        while (!overlapResolved){
            int bestMetric;
            if (mode == 1 || mode == 2 || mode == 4){
                bestMetric = INT_MAX;
            }
            else {
                bestMetric = -INT_MAX;
            }

            int bestIndex = -1;
            int mode3RandomChoice = std::rand() % actualOverlaps;
            for (int i = overlapStart; i < overlapEnd; i++){
                DFSLNode& currentOverlap = mAllNodes[i];
                if (currentOverlap.getArea() == 0 || !solveable[i]){
                    continue;
                }
                switch (mode)
                {
                case 1:{
                    // area small -> big
                    if (currentOverlap.getArea() < bestMetric){
                        bestMetric = currentOverlap.getArea();
                        bestIndex = i;
                    }
                    break;
                }
                case 2:{
                    // from center -> outer (euclidean distance)
                    Cord chipCenter;
                    gtl::center(chipCenter, mFP->getChipContour());
                    
                    int min_x, max_x, min_y, max_y;
                    min_x = min_y = INT_MAX;
                    max_x = max_y = -INT_MAX;

                    for (Tile* tile: currentOverlap.getOverlapTileList()){
                        if (tile->getXLow() < min_x){
                            min_x = tile->getXLow();
                        }            
                        if (tile->getYLow() < min_y){
                            min_y = tile->getYLow();
                        }            
                        if (tile->getXHigh() > max_x){
                            max_x = tile->getXHigh();
                        }            
                        if (tile->getYHigh() > max_y){
                            max_y = tile->getYHigh();
                        }
                    }
                    int overlapCenterx = (min_x + max_x) / 2;
                    int overlapCentery = (min_y + max_y) / 2;
                    int distSquared = pow(overlapCenterx - chipCenter.x(), 2) + pow(overlapCentery - chipCenter.y(), 2);

                    if (distSquared < bestMetric){
                        bestMetric = distSquared;
                        bestIndex = i;
                    }

                    break;
                }
                case 3: {
                    // random choice
                    if (mode3RandomChoice == 0){
                        bestIndex = i;
                    }
                    else {
                        mode3RandomChoice--;
                    }
                    break;
                }
                case 4: {
                    // from center -> outer (manhattan distance)
                    Cord chipCenter;
                    gtl::center(chipCenter, mFP->getChipContour());
                    
                    int min_x, max_x, min_y, max_y;
                    min_x = min_y = INT_MAX;
                    max_x = max_y = -INT_MAX;

                    for (Tile* tile: currentOverlap.getOverlapTileList()){
                        if (tile->getXLow() < min_x){
                            min_x = tile->getXLow();
                        }            
                        if (tile->getYLow() < min_y){
                            min_y = tile->getYLow();
                        }            
                        if (tile->getXHigh() > max_x){
                            max_x = tile->getXHigh();
                        }            
                        if (tile->getYHigh() > max_y){
                            max_y = tile->getYHigh();
                        }
                    }
                    int overlapCenterx = (min_x + max_x) / 2;
                    int overlapCentery = (min_y + max_y) / 2;
                    int manDist = abs(overlapCenterx - chipCenter.x()) + abs(overlapCentery - chipCenter.y());

                    if (manDist < bestMetric){
                        bestMetric = manDist;
                        bestIndex = i;
                    }

                    break;
                }
                default:{
                    // area big -> small, default
                    
                    if (currentOverlap.getArea() > bestMetric){
                        bestMetric = currentOverlap.getArea();
                        bestIndex = i;
                    }
                    break;
                }
                }
            }

            solveable[bestIndex] = overlapResolved = migrateOverlap(bestIndex);
            if (!overlapResolved){
                resolvableOverlaps--;
            }

            if (resolvableOverlaps == 0){
                DFSLPrint(0, "Overlaps unable to resolve\n");
                result = RESULT::OVERLAP_NOT_RESOLVED;
                return result;
            }            
        }

        iteration++;


        if (this->config->getConfigValue<int>("OutputLevel") >= DFSL_DEBUG){
            printAllNodesDebug();
            DFSLPrint(4, "printing debug output floorplan");
            printTiledFloorplan("debug_DFSL.txt", "debug");
        }
        
    }

    // mLF->visualiseArtpiece("debug_DFSL_result.txt", true);
    
    // do end of legalization, simple area legalization
    lastDitchLegalize();

    result = checkLegal();
    return result;
}

void DFSLegalizer::lastDitchLegalize(){
    int softStart = getSoftBegin();
    int softEnd = getSoftEnd();

    // fragmented -> hole -> legalArea -> aspectRatio -> util
    for (int i = 0; i < 5; i++){
        for (int softIndex = softStart; softIndex < softEnd; softIndex++){
            DFSLNode& block = mAllNodes[softIndex];
            Rectilinear* recti = block.recti;

            if (recti->getLegalArea() == 0){
                continue;
            }

            switch (i)
            {
            case 0:
                if (!recti->isLegalOneShape()){
                    DFSLPrint(3, "Block %s has disjoint components\n", block.nodeName.c_str());
                    DFSLPrint(3, "Attempting Fix...\n");
                    legalizeFragmented(recti);
                }
                break;
            case 1:
                if (!recti->isLegalNoHole()){
                    DFSLPrint(3, "Block %s has holes\n", block.nodeName.c_str());
                    DFSLPrint(3, "Attempting Fix...\n");
                    legalizeHole(recti);
                }
                break;
            case 2:
                if (!recti->isLegalEnoughArea()){
                    DFSLPrint(3, "Required area for soft block %s fail (%d < %d)\n", block.nodeName.c_str(), recti->calculateActualArea(), recti->getLegalArea());
                    DFSLPrint(3, "Attempting Fix...\n");
                    legalizeArea(block, recti);
                }
                break;
            case 3:
                if (!recti->isLegalAspectRatio()){
                    double aspectRatio = rec::calculateAspectRatio(recti->calculateBoundingBox());
                    if (aspectRatio > this->mFP->getGlobalAspectRatioMax()){
                        DFSLPrint(3, "Aspect ratio for %1% fail (%2$3.2f > %3$3.2f)\n", block.nodeName.c_str(), aspectRatio, this->mFP->getGlobalAspectRatioMax());
                    }
                    else {
                        DFSLPrint(3, "Aspect ratio for %1% fail (%2$3.2f > %3$3.2f)\n", block.nodeName.c_str(), aspectRatio, this->mFP->getGlobalAspectRatioMin());
                    }
                    DFSLPrint(3, "Attempting Fix...\n");
                    legalizeAspectRatio(recti, aspectRatio);
                }
                break;
            case 4:
                if (!recti->isLegalUtilization()){
                    double util = recti->calculateUtilization();
                    DFSLPrint(3, "util for %1% fail (%2$4.3f < %3$4.3f)\n", block.nodeName.c_str(), util, this->mFP->getGlobalUtilizationMin());
                    DFSLPrint(3, "Attempting Fix...\n");
                    legalizeUtil(recti);
                }
                break;
            default:
                break;
            }
        }
    }
}

void DFSLegalizer::legalizeArea(DFSLNode& block, Rectilinear* recti){
    int missingArea = recti->getLegalArea() - recti->calculateActualArea();
    int BWEdgeIndex = -1;
    int i = 0;
    for (DFSLEdge& edge: block.edgeList){
        if (edge.getType() == EDGETYPE::BW){
            BWEdgeIndex = i;
            break; 
        }
        i++;
    }
    int requiredExpansionLength = 0;


    if (BWEdgeIndex == -1){
        int perimeterLength = 0;
        Polygon90Set rectiShape;
        for (Tile* tile: recti->blockTiles){
            rectiShape += tile->getRectangle();
        }
        std::vector<Polygon90WithHoles> polyContainer;
        rectiShape.get_polygons(polyContainer);
        for (Polygon90WithHoles& poly: polyContainer){
            perimeterLength += gtl::perimeter(poly);
        }
        requiredExpansionLength = missingArea % perimeterLength == 0 ?  missingArea/perimeterLength : (missingArea / perimeterLength) + 1;
    }
    else {
        DFSLEdge& BWEdge = block.edgeList[BWEdgeIndex];
        int contourLength = 0;
        for (Segment& BWSeg: BWEdge.tangentSegments()){
            contourLength += BWSeg.getLength();
        }
        requiredExpansionLength = missingArea % contourLength == 0 ? missingArea/contourLength : (missingArea / contourLength) + 1;
    }

    Rectangle expandedBbox = recti->calculateBoundingBox();
    gtl::bloat(expandedBbox, requiredExpansionLength);

    fillBoundingBox(recti, expandedBbox);
}

void DFSLegalizer::legalizeFragmented(Rectilinear* recti){
    DoughnutPolygonSet reshapePart;

    for(Tile *const &t : recti->blockTiles){
        reshapePart += t->getRectangle();
    }

    // only leave the pargest part, find the largest part and mark it
    area_t largestPart = boost::polygon::area(reshapePart[0]);
    int largestPartIdx = 0;
    for(int i = 1; i < reshapePart.size(); ++i){
        area_t partArea = boost::polygon::area(reshapePart[i]);
        if(partArea > largestPart){
            largestPart = partArea;
            largestPartIdx = i;
        }
    }
    
    // load all removal parts to array
    std::vector<DoughnutPolygon> toRemoveParts;
    for(int i = 0; i < reshapePart.size(); ++i){
        if(i != largestPartIdx) {
            toRemoveParts.push_back(reshapePart[i]);
            DFSLPrint(3, "Removing area of size %1%\n", gtl::area(reshapePart[i]));
        }
    }
    this->mFP->shrinkRectilinear(toRemoveParts, recti);
}

void DFSLegalizer::legalizeAspectRatio(Rectilinear* recti, double aspectRatio){
    Rectangle Bbox = recti->calculateBoundingBox();
    Polygon90Set rectiPolySet;
    for (Tile* tile: recti->blockTiles){
        rectiPolySet += tile->getRectangle();
    }

    std::vector<Polygon90WithHoles> toDiscard;
    if (aspectRatio > this->mFP->getGlobalAspectRatioMax()){
        // too flat
        int currentHeight = rec::getHeight(Bbox);
        int maxWidth = floor((double)currentHeight * this->mFP->getGlobalAspectRatioMax());

        Rectangle range1(Bbox), range2(Bbox), range3(Bbox);
        // range1: cut off right
        gtl::xh(range1, rec::getXL(Bbox) + maxWidth);
        // range2: cuh off left
        gtl::xl(range2, rec::getXH(Bbox) - maxWidth);
        // range3: stay in middle, cut off a bit of left and a bit of right
        int xMiddle = (rec::getXL(Bbox) + rec::getXH(Bbox)) / 2;
        gtl::xl(range3, xMiddle - maxWidth/2);
        gtl::xh(range3, xMiddle + maxWidth/2);

        Polygon90Set range1Discard = rectiPolySet - range1;
        Polygon90Set range2Discard = rectiPolySet - range2;
        Polygon90Set range3Discard = rectiPolySet - range3;
        int range1DiscardedArea = gtl::area(range1Discard);
        int range2DiscardedArea = gtl::area(range2Discard);
        int range3DiscardedArea = gtl::area(range3Discard);
        if (range1DiscardedArea <= range2DiscardedArea && range1DiscardedArea <= range2DiscardedArea){
            range1Discard.get_polygons(toDiscard);
        }
        else if (range2DiscardedArea <= range1DiscardedArea && range2DiscardedArea <= range3DiscardedArea){
            range2Discard.get_polygons(toDiscard);
        }
        else /*if (range3DiscardedArea <= range2DiscardedArea && range3DiscardedArea <= range1DiscardedArea)*/{
            range3Discard.get_polygons(toDiscard);
        }
    }
    else {
        // too tall
        int currentWidth = rec::getWidth(Bbox);
        int maxHeight = floor((double)currentWidth / this->mFP->getGlobalAspectRatioMin());

        Rectangle range1(Bbox), range2(Bbox), range3(Bbox);
        // range1: cut off top
        gtl::yh(range1, rec::getYL(Bbox) + maxHeight);
        // range2: cuh off bottom
        gtl::yl(range2, rec::getYH(Bbox) - maxHeight);
        // range3: stay in middle, cut off a bit of bottom and a bit of top
        int yMiddle = (rec::getYL(Bbox) + rec::getYH(Bbox)) / 2;
        gtl::yl(range3, yMiddle - maxHeight/2);
        gtl::yh(range3, yMiddle + maxHeight/2);

        Polygon90Set range1Discard = rectiPolySet - range1;
        Polygon90Set range2Discard = rectiPolySet - range2;
        Polygon90Set range3Discard = rectiPolySet - range3;
        int range1DiscardedArea = gtl::area(range1Discard);
        int range2DiscardedArea = gtl::area(range2Discard);
        int range3DiscardedArea = gtl::area(range3Discard);
        if (range1DiscardedArea <= range2DiscardedArea && range1DiscardedArea <= range2DiscardedArea){
            range1Discard.get_polygons(toDiscard);
        }
        else if (range2DiscardedArea <= range1DiscardedArea && range2DiscardedArea <= range3DiscardedArea){
            range2Discard.get_polygons(toDiscard);
        }
        else /*if (range3DiscardedArea <= range2DiscardedArea && range3DiscardedArea <= range1DiscardedArea)*/{
            range3Discard.get_polygons(toDiscard);
        }
    }

    for(int i = 0; i < toDiscard.size(); ++i){
        DFSLPrint(3, "Removing area of size %1%\n", gtl::area(toDiscard[i]));
    }
    this->mFP->shrinkRectilinear(toDiscard, recti);
}

void DFSLegalizer::legalizeUtil(Rectilinear* recti){
	Rectangle rectBB = recti->calculateBoundingBox();
    fillBoundingBox(recti, rectBB);
}

void DFSLegalizer::fillBoundingBox(Rectilinear* recti, Rectangle& rectBB){
	using namespace boost::polygon::operators;
	// This stores the growing shape of rect during the growing process
	DoughnutPolygonSet currentRectDPS;

	DoughnutPolygonSet petriDishDPS, blankDPS;
	petriDishDPS += rectBB;
	blankDPS += rectBB;

	// find all tiles that crosssect the area, find it and sieve those unfit
	std::vector<Tile *> tilesinPetriDish;	
	std::unordered_map<Rectilinear *, std::vector<Tile *>> invlovedTiles;
	this->mFP->cs->enumerateDirectedArea(rectBB, tilesinPetriDish);
	for(Tile *const &t : tilesinPetriDish){
		Rectilinear *tilesRectilinear = this->mFP->blockTilePayload[t];
		Rectangle tileRectangle = t->getRectangle();
		blankDPS -= tileRectangle;
		if(tilesRectilinear->getType() == rectilinearType::PREPLACED) continue;
		if(tilesRectilinear == recti){
			currentRectDPS += tileRectangle;
			continue;
		}

		std::unordered_map<Rectilinear *, std::vector<Tile *>>::iterator it = invlovedTiles.find(tilesRectilinear);
		if(it == invlovedTiles.end()){ // no entry found
			invlovedTiles[tilesRectilinear] = {t};			
		}else{
			invlovedTiles[tilesRectilinear].push_back(t);
		}
	}

	std::vector<Rectilinear *> involvedRectilinears;
	for(std::unordered_map<Rectilinear *, std::vector<Tile *>>::iterator it = invlovedTiles.begin(); it != invlovedTiles.end(); ++it){
		involvedRectilinears.push_back(it->first);
	}

	// start filling the candidates to grow	
	std::vector<DoughnutPolygon> growCandidates;
	std::vector<Rectilinear *> growCandidatesRectilinear;
	// start with the blank tiles 
	for(int i = 0; i < blankDPS.size(); ++i){
		growCandidates.push_back(blankDPS[i]);
		growCandidatesRectilinear.push_back(nullptr);
	}
	// then those Rectiliears
	for(int i = 0; i < involvedRectilinears.size(); ++i){
		Rectilinear *tgRect = involvedRectilinears[i];
		DoughnutPolygonSet rectDPS;
		for(Tile *const &t : invlovedTiles[tgRect]){
			rectDPS += t->getRectangle();
		}
		// trim those are out out of the bounding box
		rectDPS &= rectBB;
		int rectDPSSize = rectDPS.size();
		for(int j = 0; j < rectDPSSize; ++j){
			growCandidates.push_back(rectDPS[j]);
			growCandidatesRectilinear.push_back(tgRect);
		}
		// put the larger block at first place
		std::sort(growCandidates.end() - rectDPSSize, growCandidates.end(), 
			[&](DoughnutPolygon a, DoughnutPolygon b){return boost::polygon::size(a) > boost::polygon::size(b);});
		
	}	
	int growCandidateSize = growCandidates.size();
	// there exist no conadidate blocks, just return
	if(growCandidateSize == 0) return;


	std::unordered_map<Rectilinear *, DoughnutPolygonSet> rectCurrentShapeDPS;
	for(Rectilinear *const &ivr : involvedRectilinears){
		for(Tile *const &t : ivr->blockTiles){
			rectCurrentShapeDPS[ivr] += t->getRectangle();
		}
	}
	bool keepGrowing = true;		
	bool hasGrow = false;
	std::vector<bool> growCandidateSelect(growCandidateSize, false); // This records which candidate is selected

	while(keepGrowing){
		keepGrowing = false;
		for(int i = 0; i < growCandidateSize; ++i){
			if(growCandidateSelect[i]) continue;

			DoughnutPolygon markedDP = growCandidates[i];
			// test if installing this marked part would turn the current shape strange

			DoughnutPolygonSet xSelfDPS(currentRectDPS);
			xSelfDPS += markedDP;
			if(!(dps::oneShape(xSelfDPS) && dps::noHole(xSelfDPS))){
				// intrducing the markedDP would cause dpSet to generate strange shape, give up on the tile
				continue;
			}

			// test if pulling off the marked part would harm the victim too bad, skip if markedDP belongs to blank
			Rectilinear *victimRect = growCandidatesRectilinear[i];
			if(victimRect != nullptr){
				// markedDP belongs to other Rectilinear
				DoughnutPolygonSet xVictimDPS(rectCurrentShapeDPS[victimRect]);
				xVictimDPS -= markedDP;

				// if removing the piece makes the victim rectilinear illegal, quit	
				if(!dps::checkIsLegal(xVictimDPS, victimRect->getLegalArea(), this->mFP->getGlobalAspectRatioMin(), 
				this->mFP->getGlobalAspectRatioMax(), this->mFP->getGlobalUtilizationMin())){
					continue;
				}
			}

			// pass all tests, distribute the tile
			growCandidateSelect[i] = true;
			keepGrowing = true;
			hasGrow = true;
			currentRectDPS += markedDP;
			DoughnutPolygonSet markedDPS;
			markedDPS += markedDP;
			if(victimRect != nullptr){ // markedDP belongs to other Rectilienar
				rectCurrentShapeDPS[victimRect] -= markedDP;
				this->mFP->shrinkRectilinear(markedDPS, victimRect);
			}
			this->mFP->growRectilinear(markedDPS, recti);
		}
	}

	return;
}


RESULT DFSLegalizer::checkLegal(){
    RESULT result = RESULT::SUCCESS;
    int violations = 0;

    // test soft blocks for legality 
    int softStart = getSoftBegin();
    int softEnd = getSoftEnd();
    for (int i = softStart; i < softEnd; i++){
        DFSLNode& block = mAllNodes[i];
        Rectilinear* recti = block.recti;

        if (recti->getLegalArea() == 0){
            continue;
        }

        if (!recti->isLegalEnoughArea()){
            DFSLPrint(1, "Required area for soft block %s fail (%d < %d)\n", block.nodeName.c_str(), recti->calculateActualArea(), recti->getLegalArea());
            result = RESULT::AREA_CONSTRAINT_FAIL; 
            violations++;           
        } 
        
        if (!recti->isLegalUtilization()){
            double util = recti->calculateUtilization();
            DFSLPrint(1, "util for %1% fail (%2$4.3f < %3$4.3f)\n", block.nodeName.c_str(), util, this->mFP->getGlobalUtilizationMin());
            result = RESULT::OTHER_CONSTRAINT_FAIL;
            violations++;
        }

        if (!recti->isLegalAspectRatio()){
            double aspectRatio = rec::calculateAspectRatio(recti->calculateBoundingBox());
            if (aspectRatio > this->mFP->getGlobalAspectRatioMax()){
                DFSLPrint(1, "Aspect ratio for %1% fail (%2$3.2f > %3$3.2f)\n", block.nodeName.c_str(), aspectRatio, this->mFP->getGlobalAspectRatioMax());
            }
            else {
                DFSLPrint(1, "Aspect ratio for %1% fail (%2$3.2f > %3$3.2f)\n", block.nodeName.c_str(), aspectRatio, this->mFP->getGlobalAspectRatioMin());
            }
            result = RESULT::OTHER_CONSTRAINT_FAIL; 
            violations++;  
        }

        if (!recti->isLegalOneShape()){
            DFSLPrint(1, "Block %s has disjoint components\n", block.nodeName.c_str());
            result = RESULT::OTHER_CONSTRAINT_FAIL;
            violations++;            
        }

        if (!recti->isLegalNoHole()){
            DFSLPrint(1, "Block %s has holes\n", block.nodeName.c_str());
            result = RESULT::OTHER_CONSTRAINT_FAIL;
            violations++;
        }
    }

    // test fixed blocks for legalit
    int fixedStart = getFixedBegin();
    int fixedEnd = getFixedEnd();
    for (int i = fixedStart; i < fixedEnd; i++){
        DFSLNode& block = mAllNodes[i];
        Rectilinear* recti = block.recti;

        if (recti->getLegalArea() == 0){
            continue;
        }
        
        if (!recti->isLegalUtilization()){
            double util = recti->calculateUtilization();
            DFSLPrint(1, "util for %1% fail (%2$4.3f < %3$4.3f)\n", block.nodeName.c_str(), util, this->mFP->getGlobalUtilizationMin());
            result = RESULT::OTHER_CONSTRAINT_FAIL;
            violations++;
        }

        if (!recti->isLegalEnoughArea()){
            DFSLPrint(1, "Required area for fixed block %s fail (%d < %d)\n", block.nodeName.c_str(), recti->calculateActualArea(), recti->getLegalArea());
            result = RESULT::OTHER_CONSTRAINT_FAIL; 
            violations++;           
        } 

        if (!recti->isLegalOneShape()){
            DFSLPrint(1, "Block %s has disjoint components\n", block.nodeName.c_str());
            result = RESULT::OTHER_CONSTRAINT_FAIL;
            violations++;            
        }

        if (!recti->isLegalNoHole()){
            DFSLPrint(1, "Block %s has holes\n", block.nodeName.c_str());
            result = RESULT::OTHER_CONSTRAINT_FAIL;
            violations++;
        }
    }

    DFSLPrint(2, "Total Violations: %d\n", violations);
    return result;
}

void DFSLegalizer::legalizeHole(Rectilinear* recti){
    area_t largestArea = AREA_T_MIN;
    Tile *largestTile;
    std::vector<Tile *>allTiles; 
    for(Tile *const &t : recti->blockTiles){
        allTiles.push_back(t);
        area_t tileArea = t->getArea();
        if(tileArea > largestArea){
            largestArea = tileArea;
            largestTile = t;
        }
    }
    // leave only the largest tile, remove all remaining
    for(Tile *const &t : allTiles){
        if(t != largestTile){
            this->mFP->decreaseTileOverlap(t, recti);
        }
    }
}

bool DFSLegalizer::migrateOverlap(int overlapIndex){
    mBestPath.clear();
    mCurrentPath.clear();
    mBestCost = (double) INT_MAX;
    mMigratingArea = mAllNodes[overlapIndex].getArea();

    DFSLPrint(3, "\nMigrating Overlap: %s\n", mAllNodes[overlapIndex].nodeName.c_str());
    for (DFSLEdge& edge: mAllNodes[overlapIndex].edgeList){
        // todo: add dijkstra's
        dfs(edge, 0.0);
    }
    DFSLPrint(3, "Path cost: %f\n", mBestCost);

    if (mBestPath.size() == 0){
        DFSLPrint(3, "Path not found. Layout unchanged\n");
        return false;
    }

    std::ostringstream messageStream;
    messageStream << "Path: " << mAllNodes[overlapIndex].nodeName << " ";
    for (MigrationEdge& edge: mBestPath){
        if (edge.toIndex == -1){
            std::string direction;
            switch (edge.segment.getDirection())
            {
            case DIRECTION::TOP:
                direction = "above";
                break;
            case DIRECTION::RIGHT:
                direction = "right of";
                break;
            case DIRECTION::DOWN:
                direction = "below";
                break;
            case DIRECTION::LEFT:
                direction = "left of";
                break;
            default:
                direction = "error";
                break;
            }
            messageStream << "→ Whitespace " << direction << ' ' << mAllNodes[edge.fromIndex].nodeName 
                        << " (tangent: " << edge.segment << ") ";
        }
        else {
            messageStream << "→ " << mAllNodes[edge.toIndex].nodeName << ' ';
        }
    }
    DFSLPrint(3, messageStream.str().c_str());

    // go through path, find maximum resolvable area
    mResolvableArea = mMigratingArea;
    for (MigrationEdge& edge: mBestPath){
        if (edge.edgeType == EDGETYPE::BB){
            if (gtl::area(edge.migratedArea) < mResolvableArea){
                mResolvableArea = gtl::area(edge.migratedArea);
            }
        }
        else if (edge.edgeType == EDGETYPE::BW){
            if (gtl::area(edge.migratedArea) < mResolvableArea){
                mResolvableArea = gtl::area(edge.migratedArea);
            }
        }
    }
    DFSLPrint(3, "Resolvable Area: %d\n", mResolvableArea);

    // start changing physical layout
    for (MigrationEdge& edge: mBestPath){
        DFSLNode& fromNode = mAllNodes[edge.fromIndex];
        std::string toNodeName;
        if (edge.edgeType == EDGETYPE::BW){
            toNodeName = "BLANK";
        }
        else {
            toNodeName = mAllNodes[edge.toIndex].nodeName;
        }
        
        DFSLPrint(3, "* %s -> %s:\n", fromNode.nodeName.c_str(), toNodeName.c_str());

        if (edge.edgeType == EDGETYPE::OB){
            DFSLNode& toNode = mAllNodes[edge.toIndex];
            if (mResolvableArea < mMigratingArea){
                // create transient overlap area
                std::vector<Tile*> newTiles;
                bool result = splitOverlap(edge, newTiles);

                DFSLPrint(3, "Overlap not completely resolvable (overlap area: %d, resolvable area: %d)\n", mMigratingArea, mResolvableArea);
                if (result){
                    std::ostringstream messageStream;

                    int actualAreaCount = 0;
                    for (Tile* tile: newTiles){
                        messageStream << "\t" << *tile << '\n';
                        actualAreaCount += tile->getArea();
                    }
                    if (actualAreaCount != mResolvableArea && config->getConfigValue<bool>("ExactAreaMigration")){
                        DFSLPrint(1, "Migration area mismatch (actual: %d)\n", actualAreaCount);
                    }
                    DFSLPrint(3, "Splitting overlap tile. New tile: \n%s", messageStream.str().c_str());
                }
                else {
                    DFSLPrint(3, "Split overlap failed.\n");
                }
            }
            else {
                DFSLPrint(3, "Removing %s attribute from %d tiles:\n", toNode.nodeName.c_str(), fromNode.getOverlapTileList().size());
                std::set<Tile*> overlapTilesCopy = fromNode.getOverlapTileList();
                for (Tile* overlap: overlapTilesCopy){
                    DFSLPrint(3, "\t%1%\n", *(overlap));
                    removeTileFromOverlap(overlap, fromNode, toNode);
                }
            }
        }
        else if (edge.edgeType == EDGETYPE::BB){
            DFSLNode& toNode = mAllNodes[edge.toIndex];
            if (edge.segment.getDirection() == DIRECTION::NONE) {
                DFSLPrint(0, "BB edge ( %s -> %s ) must have DIRECTION\n", fromNode.nodeName.c_str(), toNode.nodeName.c_str());
                return false;
            }

            std::vector<Tile*> newTiles;
            bool result = splitSoftBlock(edge, newTiles);
            if (!result){
                DFSLPrint(0, "BB flow ( %s -> %s ) FAILED.\n", fromNode.nodeName.c_str(), toNode.nodeName.c_str());
                return false;
            }
            else {
                std::ostringstream messageStream;

                int actualAreaCount = 0;
                for (Tile* tile: newTiles){
                    messageStream << "\t" << *tile << '\n';
                    actualAreaCount += tile->getArea();
                }
                if (actualAreaCount != mResolvableArea && config->getConfigValue<bool>("ExactAreaMigration")){
                    DFSLPrint(1, "Migration area mismatch (actual: %d)\n", actualAreaCount);
                }
                DFSLPrint(3, "Splitting tiles. New %s tile: \n%s", fromNode.nodeName.c_str(), messageStream.str().c_str());
            }

        }
        else if (edge.edgeType == EDGETYPE::BW){
            if (edge.segment.getDirection() == DIRECTION::NONE) {
                DFSLPrint(0, "BW edge ( %s -> %s ) must have DIRECTION\n", fromNode.nodeName.c_str(), messageStream.str().c_str());
                return false;
            }
            
            std::vector<Tile*> newTiles;
            bool result = placeInBlank(edge, newTiles);
            if (!result){
                DFSLPrint(0, "BW flow ( %s -> BLANK ) FAILED.\n", fromNode.nodeName.c_str());
                return false;
            }
            else {
                std::ostringstream messageStream;
                int actualAreaCount = 0;
                for (Tile* tile: newTiles){
                    messageStream << "\t" << *tile << '\n';
                    actualAreaCount += tile->getArea();
                }
                if (actualAreaCount != mResolvableArea && config->getConfigValue<bool>("ExactAreaMigration")){
                    DFSLPrint(1, "Migration area mismatch (actual: %d)\n", actualAreaCount);
                }
                DFSLPrint(3, "Placing tiles. New %s tiles: \n%s", fromNode.nodeName.c_str(), messageStream.str().c_str());
            }

        }
        else {  
            if (edge.toIndex == -1){
                DFSLPrint(1, "Doing nothing for migrating path: %s -> BLANK\n", fromNode.nodeName.c_str());
            }
            else {
                DFSLNode& toNode = mAllNodes[edge.toIndex];
                DFSLPrint(1, "Doing nothing for migrating path: %s -> %s\n", fromNode.nodeName.c_str(),  toNode.nodeName.c_str());
            }
        }
    }

    // update graph
    updateGraph();
    return true;
}

// April 27: Bug Fix by Ryan 
void DFSLegalizer::updateGraph(){
    DFSLPrint(4, "Updating graph\n");
    // go along mBestPath, (the path just traversed), 
    // update overlapNode, and
    // find all updated neighbors of each block, push to toUpdate
    // and update all nodes in toUpdate

    // update overlapNode
    DFSLNode& overlapNode = mAllNodes[mBestPath[0].fromIndex];
    DFSLPrint(4, "update %s\n", overlapNode.nodeName);
    updateOverlapNode(overlapNode);

    std::set<int> toUpdate;
    std::set<int> alreadyUpdated;
    for (int i = 0; i < mBestPath.size(); i++){
        MigrationEdge& edge = mBestPath[i];
        DFSLNode& fromNode = mAllNodes[edge.fromIndex];
        if (fromNode.nodeType == DFSLNodeType::OVERLAP){
            int nextIndex = edge.toIndex;
            int otherIndex; 
            for (int to: fromNode.overlaps){
                if (to != nextIndex){
                    otherIndex = to;
                    break;
                }
            }
            
            DFSLNode& otherNode = mAllNodes[otherIndex];
            if (otherNode.nodeType == DFSLNodeType::SOFT){
                checkNeighborDiscrepancies(otherNode, toUpdate);
                alreadyUpdated.insert(otherNode.index);
            }
        }
        else if (fromNode.nodeType == DFSLNodeType::SOFT){
            checkNeighborDiscrepancies(fromNode, toUpdate);
            alreadyUpdated.insert(fromNode.index);
        }
    }

    for (int alreadyUpdatedIndex: alreadyUpdated){
        toUpdate.erase(alreadyUpdatedIndex);
    }

    updateBlockNodes(toUpdate);
}

void DFSLegalizer::updateOverlapNode(DFSLNode& overlapNode){
    if (overlapNode.getOverlapTileList().empty()){
        overlapNode.edgeList.clear();
    }
    else {
        overlapNode.edgeList.clear();
        for (int to: overlapNode.overlaps){
            if (getSoftBegin() <= to && to < getSoftEnd()){
                findOBEdge(overlapNode.index, to);
            }
        }
    }
}

// April 27, Bug fix by Ryan Lin
void DFSLegalizer::updateBlockNodes(std::set<int>& toUpdate){
    for (int nodeIndex: toUpdate){
        DFSLNode& node = mAllNodes[nodeIndex];
        DFSLPrint(4, "update %s\n", node.nodeName);
        getSoftNeighbors(nodeIndex);
    }
}

// April 27, Bug fix by Ryan Lin
void DFSLegalizer::checkNeighborDiscrepancies(DFSLNode& blockNode, std::set<int>& toUpdate){
    // re-find neighbors of block
    std::vector<DFSLEdge> oldEdgeListCopy = blockNode.edgeList;
    std::vector<bool> oldEdgeExists(oldEdgeListCopy.size(), false);
    getSoftNeighbors(blockNode.index);

    // get all neighbors of A
    // for each edge A->B, look at the old version of A->B
    // if the edge segments of A->B and A->B are different, then there is a discrepency 
    // B will be pushed to toUpdate
    for (DFSLEdge& newEdge: blockNode.edgeList){
        if (newEdge.getTo() == -1){
            continue;
        }
        
        bool discrepancy = true;
        for (int i = 0; i < oldEdgeExists.size(); i++){
            DFSLEdge& oldEdge = oldEdgeListCopy[i];
            // find same edge
            if (oldEdge.getTo() == newEdge.getTo()){
                oldEdgeExists[i] = true;
                discrepancy = false;

                if (oldEdge.tangentSegments().size() != newEdge.tangentSegments().size()){
                    discrepancy = true;
                }
                else {
                    // if two edges are the same, then in theory the ordering of each tangent segment
                    // should be the same too
                    for (int i = 0; i < oldEdge.tangentSegments().size(); i++){
                        Segment& oldSegment = oldEdge.tangentSegments()[i];
                        Segment& newSegment = newEdge.tangentSegments()[i];
                        if (oldSegment.getSegStart() != newSegment.getSegStart() || oldSegment.getSegEnd() != newSegment.getSegEnd()){
                            discrepancy = true;
                            break;
                        }
                    } 
                }
                break;
            }
        }

        // three cases for discrepencies:
        // edge has been updated
        // edge is completely new
        // edge has been removed
        if (discrepancy){
            toUpdate.insert(newEdge.getTo());
        }
    }
    // edge has been removed
    for (int i = 0; i < oldEdgeExists.size(); i++){
        if (!oldEdgeExists[i]){
            DFSLEdge& oldEdge = oldEdgeListCopy[i];
            int toIndex = oldEdge.getTo();
            if (toIndex == -1){
                continue;
            }
            toUpdate.insert(toIndex);
        }
    }
}

void DFSLegalizer::dfs(DFSLEdge& edge, double currentCost){
    int toIndex = edge.getTo();
    MigrationEdge edgeResult = getEdgeCost(edge);
    double edgeCost = edgeResult.edgeCost;
    currentCost += edgeCost;
    mCurrentPath.push_back(edgeResult);
    if (toIndex == -1){
        if (currentCost < mBestCost){
            mBestPath = mCurrentPath;
            mBestCost = currentCost;
            
            DFSLPrint(4, "Best path found (cost = %f): ", mBestCost);
            for (MigrationEdge& edge: mBestPath){
                std::string fromNodeName = mAllNodes[edge.fromIndex].nodeName;
                std::string toNodeName = edge.toIndex == -1 ? "BLANK" : mAllNodes[edge.toIndex].nodeName;
                DFSLPrint(4, "%1% -> %2%, ", fromNodeName, toNodeName);
            }
            DFSLPrint(4,"\n");
        }
    }
    else if (currentCost < config->getConfigValue<double>("MaxCostCutoff") && currentCost < mBestCost){
        for (DFSLEdge& nextEdge: mAllNodes[toIndex].edgeList){
            bool skip = false;
            for (MigrationEdge& edge: mCurrentPath){
                if (edge.fromIndex == nextEdge.getTo() || edge.toIndex == nextEdge.getTo()) {
                    skip = true;
                    break;
                }
            }
            if (skip){
                continue;
            }
            else {
                dfs(nextEdge, currentCost);
            }
        }
    }

    mCurrentPath.pop_back();
}


MigrationEdge DFSLegalizer::getEdgeCost(DFSLEdge& edge){
    // [0]: area, [1]: util, [2]: aspect ratio
    typedef std::array<double,3> RectiInfo;

    EDGETYPE edgeType = edge.getType(); 
    DFSLNode& fromNode = mAllNodes[edge.getFrom()];
    double edgeCost = 0.0;
    Rectangle returnRectangle(0,0,1,1);
    Segment bestSegment;

    std::string toNodeName = edge.getTo() == -1 ? "Blank" : mAllNodes[edge.getTo()].nodeName;
    DFSLPrint(4, "getEdgeCost of (%s -> %s)\n", fromNode.nodeName.c_str(), toNodeName.c_str());

    switch (edgeType)
    {
    case EDGETYPE::OB:
    {
        DFSLNode& toNode = mAllNodes[edge.getTo()];
        Rectilinear* toRecti = toNode.recti;

        Polygon90Set overlap;
        Polygon90Set withoutOverlap;
        Polygon90Set withOverlap;
        overlap.clear();
        withoutOverlap.clear();
        withOverlap.clear();

        for (Tile* tile: toRecti->blockTiles){
            withoutOverlap += tile->getRectangle();
            withOverlap += tile->getRectangle();
        }
        for (Tile* allOverlapTile: toRecti->overlapTiles){
            withoutOverlap += allOverlapTile->getRectangle();
            withOverlap += allOverlapTile->getRectangle();
        }

        for (Tile* overlapTile: fromNode.getOverlapTileList()){
            overlap += overlapTile->getRectangle();
            withoutOverlap -= overlapTile->getRectangle();
        }

        RectiInfo overlapInfo = getPolySetInfo(overlap);
        RectiInfo withOverlapInfo = getPolySetInfo(withOverlap);
        RectiInfo withoutOverlapInfo = getPolySetInfo(withoutOverlap);

        // get area
        double overlapArea = overlapInfo[0];
        double blockArea = withoutOverlapInfo[0];
        double areaWeight = (overlapArea / blockArea) * config->getConfigValue<double>("OBAreaWeight");

        // get util
        double withOverlapUtil = withOverlapInfo[1];
        double withoutOverlapUtil = withoutOverlapInfo[1];
        // positive reinforcement if util is improved
        double utilCost = (withOverlapUtil < withoutOverlapUtil) ? (withoutOverlapUtil - withOverlapUtil) * config->getConfigValue<double>("OBUtilPosRein") :  
                                                        (1.0 - withoutOverlapUtil) * config->getConfigValue<double>("OBUtilWeight");


        // get value of long/short side without overlap
        double aspectRatio = withoutOverlapInfo[2];
        aspectRatio = aspectRatio > 1.0 ? aspectRatio : 1.0/aspectRatio;
        double aspectCost = pow(aspectRatio - 1.0, 4) * config->getConfigValue<double>("OBAspWeight");

        edgeCost += areaWeight + utilCost + aspectCost;

        break;
    }

    case EDGETYPE::BB:
    {
        DFSLNode& toNode = mAllNodes[edge.getTo()];
        Polygon90Set fromBlock, toBlock;
        fromBlock.clear();
        toBlock.clear();

        DFSLNode2PolySet(fromNode, fromBlock);
        DFSLNode2PolySet(toNode, toBlock);

        RectiInfo oldFromBlockInfo = getPolySetInfo(fromBlock);
        RectiInfo oldToBlockInfo = getPolySetInfo(toBlock);

        // predict new rectangle
        double lowestCost = (double) LONG_MAX;
        Rectangle bestRectangle;
        int bestSegmentIndex = -1;
        std::vector<Segment>& tangentSegments = edge.tangentSegments();
        for (int s = 0; s < tangentSegments.size(); s++){
            Segment seg = tangentSegments[s];
            Segment wall = FindNearestOverlappingInterval(seg, toBlock);
            len_t Blx, Bly;
            int width;
            int height;
            DFSLPrint(4, ">> SEG: %1%\t WALL: %2%\n", seg, wall);
            if (seg.getDirection() == DIRECTION::TOP){
                width = seg.getLength();
                int requiredHeight = ceil((double) mMigratingArea / (double) width);
                int availableHeight = wall.getSegStart().y() - seg.getSegStart().y();
                assert(availableHeight > 0);
                height = availableHeight > requiredHeight ? requiredHeight : availableHeight;
                Blx = seg.getSegStart().x();
                Bly = seg.getSegStart().y();
            }
            else if (seg.getDirection() == DIRECTION::RIGHT){
                height = seg.getLength();
                int requiredWidth = ceil((double) mMigratingArea / (double) height);
                int availableWidth = wall.getSegStart().x() - seg.getSegStart().x();
                assert(availableWidth > 0);
                width = availableWidth > requiredWidth ? requiredWidth : availableWidth;
                Blx = seg.getSegStart().x();
                Bly = seg.getSegStart().y();
            }
            else if (seg.getDirection() == DIRECTION::DOWN){
                width = seg.getLength();
                int requiredHeight = ceil((double) mMigratingArea / (double) width);
                int availableHeight = seg.getSegStart().y() - wall.getSegStart().y();
                assert(availableHeight > 0);
                height = availableHeight > requiredHeight ? requiredHeight : availableHeight;
                Blx = seg.getSegStart().x();
                Bly = seg.getSegStart().y() - height;
            }
            else if (seg.getDirection() == DIRECTION::LEFT) {
                height = seg.getLength();
                int requiredWidth = ceil((double) mMigratingArea / (double) height);
                int availableWidth = seg.getSegStart().x() - wall.getSegStart().x();
                assert(availableWidth > 0);
                width = availableWidth > requiredWidth ? requiredWidth : availableWidth;
                Blx = seg.getSegStart().x() - width;
                Bly = seg.getSegStart().y();
            }
            else {
                DFSLPrint(0, "BB edge ( %s -> %s ) has no DIRECTION\n", fromNode.nodeName.c_str(),  toNode.nodeName.c_str());
                return MigrationEdge();
            }

            Rectangle newArea(Blx, Bly, Blx+width, Bly+height);
            Polygon90Set newFromBlock = fromBlock + newArea;
            Polygon90Set newToBlock = toBlock - newArea;

            RectiInfo newFromBlockInfo = getPolySetInfo(newFromBlock);
            RectiInfo newToBlockInfo = getPolySetInfo(newToBlock);

            // area cost
            double areaCost = (oldFromBlockInfo[0] / oldToBlockInfo[0]) * config->getConfigValue<double>("BBAreaWeight");

            // get util
            double oldFromBlockUtil = oldFromBlockInfo[1];
            double newFromBlockUtil = newFromBlockInfo[1];
            double oldToBlockUtil = oldToBlockInfo[1];
            double newToBlockUtil = newToBlockInfo[1];
            // positive reinforcement if util is improved
            double fromUtilCost, toUtilCost;
            fromUtilCost = (oldFromBlockUtil < newFromBlockUtil) ? (newFromBlockUtil - oldFromBlockUtil) * config->getConfigValue<double>("BBFromUtilPosRein") :  
                                                        (1.0 - newFromBlockUtil) * config->getConfigValue<double>("BBFromUtilWeight");
            toUtilCost = (oldToBlockUtil < newToBlockUtil) ? (newToBlockUtil - oldToBlockUtil) * config->getConfigValue<double>("BBToUtilPosRein") :  
                                                        pow(1.0 - newToBlockUtil, 2) * config->getConfigValue<double>("BBToUtilWeight");
            
            // get aspect ratio with new area
            double aspectRatio = newFromBlockInfo[2];
            aspectRatio = aspectRatio > 1.0 ? aspectRatio : 1.0/aspectRatio;
            double arCost;
            arCost = (aspectRatio - 1.0) * config->getConfigValue<double>("BBAspWeight");

            // to block might be fragemented after, if fragmented then add giant cost
            double legalCost = 0.0;
            std::vector<Polygon90WithHoles> polySet;
            newToBlock.get_polygons(polySet);
            if (polySet.size() > 1){
                legalCost = 100000.0;
            }


            double cost = areaCost + fromUtilCost + toUtilCost + arCost + legalCost;
                            
            if (cost < lowestCost){
                lowestCost = cost;
                bestSegmentIndex = s;
                bestRectangle = newArea;
            }
        }

        bestSegment = tangentSegments[bestSegmentIndex];
        returnRectangle = bestRectangle;
        edgeCost += lowestCost + config->getConfigValue<double>("BBFlatCost");

        break;
    }
    case EDGETYPE::BW:
    {
        Polygon90Set oldBlock;
        DFSLNode2PolySet(fromNode, oldBlock);

        RectiInfo oldBlockInfo = getPolySetInfo(oldBlock);

        // predict new tile
        double lowestCost = (double) LONG_MAX;
        Rectangle bestRectangle;
        int bestSegmentIndex = -1;
        std::vector<Segment>& tangentSegments = edge.tangentSegments();
        for (int s = 0; s < tangentSegments.size(); s++){
            Segment seg = tangentSegments[s];

            Rectangle predictNewArea = extendSegment(seg, mMigratingArea, mFP);
            assert(rec::getArea(predictNewArea) > 0);

            Polygon90Set newBlock = oldBlock + predictNewArea;
            RectiInfo newBlockInfo = getPolySetInfo(newBlock);

            // get util
            double oldBlockUtil = oldBlockInfo[1];
            double newBlockUtil = newBlockInfo[1];
            // positive reinforcement if util is improved
            double utilCost;
            utilCost = (oldBlockUtil < newBlockUtil) ? (newBlockUtil - oldBlockUtil) * config->getConfigValue<double>("BWUtilPosRein") :  
                                                        (1.0 - newBlockUtil) * config->getConfigValue<double>("BWUtilWeight");

            // get aspect ratio with new area
            double aspectRatio = newBlockInfo[2];
            aspectRatio = aspectRatio > 1.0 ? aspectRatio : 1.0/aspectRatio;
            double arCost;
            arCost = pow(aspectRatio - 1.0, 4.0) * config->getConfigValue<double>("BWAspWeight");

            double cost = utilCost + arCost;

            DFSLPrint(4, ">> SEG: %1%\tCost: %2%\n", seg, cost);

            if (cost < lowestCost){
                lowestCost = cost;
                bestSegmentIndex = s;
                bestRectangle = predictNewArea;
            }
        }

        bestSegment = tangentSegments[bestSegmentIndex];
        returnRectangle = bestRectangle;
        edgeCost += lowestCost; 

        break;
    }
    default:
        edgeCost += config->getConfigValue<double>("MaxCostCutoff") * 100;
        break;
    }

    DFSLPrint(4, ">cost: %f\n", edgeCost);

    // note: returnTile & bestSegment are not initialized if OB edge
    return MigrationEdge(edge.getFrom(), edge.getTo(), edge.getType(), returnRectangle, bestSegment, edgeCost);
}


bool DFSLegalizer::splitOverlap(MigrationEdge& edge, std::vector<Tile*>& newTiles){
    if (edge.fromIndex == -1 || edge.toIndex == -1){
        DFSLPrint(0, "splitOverlap can only be used on overlap->soft edge.\n");
        return false;
    }
    DFSLNode& fromNode = mAllNodes[edge.fromIndex];
    DFSLNode& toNode = mAllNodes[edge.toIndex];
    if (!(fromNode.nodeType == DFSLNodeType::OVERLAP && toNode.nodeType == DFSLNodeType::SOFT)){
        DFSLPrint(0, "splitOverlap can only be used on overlap->soft edge.\n");
        return false;
    }
    
    DFSLEdge* fullEdge = NULL;
    for (DFSLEdge& fromEdge: fromNode.edgeList){
        if (fromEdge.getFrom() == edge.fromIndex && fromEdge.getTo() == edge.toIndex){
            fullEdge = &fromEdge;
            break;
        }
    }
    if (fullEdge == NULL){
        DFSLPrint(0, "OB edge not found.\n");
        return false;
    }

    // find segment directions
    // if overlap flows x area to A, then x amount of area should be assigned to B in overlap
    std::vector<bool> sideOccupied(4, false);
    for (Segment& segment: fullEdge->tangentSegments()){
        switch (segment.getDirection()){
        case DIRECTION::TOP:
            sideOccupied[0] = true;
            break;
        case DIRECTION::RIGHT:
            sideOccupied[1] = true;
            break;
        case DIRECTION::DOWN:
            sideOccupied[2] = true;
            break;
        case DIRECTION::LEFT:
            sideOccupied[3] = true;
            break;
        default:
            DFSLPrint(1, "OB edge segment has no direction\n");
            break;
        }
    }


    // From ascending order of area, resolve each overlap tile individually 
    std::set<Tile*>& overlapTileList = fromNode.getOverlapTileList();
    std::vector<bool> tileChosen(overlapTileList.size(), false);
    int remainingMigrateArea = mResolvableArea;
    Tile* smallestTile = NULL;
    while (true){
        // find Tile with smallest area
        int smallestArea = INT_MAX;
        int index = 0;
        for (Tile* tile: overlapTileList){
            int currentArea = tile->getArea();
            if (currentArea < smallestArea && !tileChosen[index]){
                smallestTile = tile;
                smallestArea = currentArea;
            }
            index++;
        }

        // if Tile area is smaller than mresolvable area, directly resolve that tile
        if (smallestTile->getArea() <= remainingMigrateArea){
            removeTileFromOverlap(smallestTile, fromNode, toNode);
            
            remainingMigrateArea -= smallestTile->getArea();
            newTiles.push_back(smallestTile);
        }
        // otherwise, find the largest rectangle within that rectangle whose area
        // is smaller than the remainingMigrateArea
        else {
            // for each unoccupied side of the rectangle,
            // calculate the area of the rectangle extending from B
            // keep the one with the area closest to the actual resolvableArea
            int closestArea = -1;
            DIRECTION bestDirection = DIRECTION::TOP;
            Rectangle bestRectangle;
            if (!sideOccupied[0]){
                // grow from top side
                int width = smallestTile->getWidth();
                int height = (int) floor((double) remainingMigrateArea / (double) width);
                int thisArea = width * height;
                if (thisArea > closestArea){
                    closestArea = thisArea;
                    bestDirection = DIRECTION::TOP;

                    int newYl = smallestTile->getYHigh() - height;
                    bestRectangle = smallestTile->getRectangle();
                    gtl::yl(bestRectangle, newYl);
                }
            }
            if (!sideOccupied[1]){
                // grow from right side
                int height = smallestTile->getHeight();
                int width = (int) floor((double) remainingMigrateArea / (double) height);
                int thisArea = width * height;
                if (thisArea > closestArea){
                    closestArea = thisArea;
                    bestDirection = DIRECTION::RIGHT;

                    int newXl = smallestTile->getXHigh() - width;
                    bestRectangle = smallestTile->getRectangle();
                    gtl::xl(bestRectangle, newXl);
                }
            }
            if (!sideOccupied[2]){
                // grow from bottom side
                int width = smallestTile->getWidth();
                int height = (int) floor((double) remainingMigrateArea / (double) width);
                int thisArea = width * height;
                if (thisArea > closestArea){
                    closestArea = thisArea;
                    bestDirection = DIRECTION::DOWN;

                    int newYh = smallestTile->getYLow() + height;
                    bestRectangle = smallestTile->getRectangle();
                    gtl::yh(bestRectangle, newYh);
                }
            }
            if (!sideOccupied[3]){
                // grow from left side
                int height = smallestTile->getHeight();
                int width = (int) floor((double) remainingMigrateArea / (double) height);
                int thisArea = width * height;
                if (thisArea > closestArea){
                    closestArea = thisArea;
                    bestDirection = DIRECTION::LEFT;

                    int newXh = smallestTile->getXLow() + width;
                    bestRectangle = smallestTile->getRectangle();
                    gtl::xh(bestRectangle, newXh);
                }
            }
            
            if (closestArea > 0){
                Tile* newTile = NULL;
                switch (bestDirection){
                    case DIRECTION::TOP:{
                        // split tiles
                        Tile* newTopOverlap = mFP->generalSplitTile(smallestTile, bestRectangle);
                        Tile* newBottomOverlap = newTopOverlap->lb;
                        // manually update overlapTileList
                        fromNode.getOverlapTileList().insert(newBottomOverlap);

                        removeTileFromOverlap(newTopOverlap, fromNode, toNode);
                        newTile = newTopOverlap;
                        break;
                    }
                    case DIRECTION::RIGHT:{
                        // split tiles
                        Tile* newRightOverlap = mFP->generalSplitTile(smallestTile, bestRectangle);
                        Tile* newLeftOverlap = newRightOverlap->bl;
                        // manually update overlapTileList
                        fromNode.getOverlapTileList().insert(newLeftOverlap);

                        removeTileFromOverlap(newRightOverlap, fromNode, toNode);
                        newTile = newRightOverlap;
                        break;
                    }
                    case DIRECTION::DOWN:{
                        // split tiles
                        Tile* newBottomOverlap = mFP->generalSplitTile(smallestTile, bestRectangle);
                        Tile* newTopOverlap = newBottomOverlap->rt;
                        // manually update overlapTileList
                        fromNode.getOverlapTileList().insert(newTopOverlap);

                        removeTileFromOverlap(newBottomOverlap, fromNode, toNode);
                        newTile = newBottomOverlap;
                        break;
                    }
                    case DIRECTION::LEFT:{
                        // split tiles
                        Tile* newLeftOverlap = mFP->generalSplitTile(smallestTile, bestRectangle);
                        Tile* newRightOverlap = newLeftOverlap->tr;
                        // manually update overlapTileList
                        fromNode.getOverlapTileList().insert(newRightOverlap);

                        removeTileFromOverlap(newLeftOverlap, fromNode, toNode);
                        newTile = newLeftOverlap;
                        break;
                    }
                    default:
                    break;
                }

                newTiles.push_back(newTile);

                remainingMigrateArea -= newTile->getArea();
            }

            // TODO: exact area migration for new 
            // // if exact area migration, find remainder 
            // // and split old overlap tile
            // if (config->getConfigValue<bool>("ExactAreaMigration") && remainingMigrateArea > 0){
            //     int height, width;
            //     Rectangle remainderRectangle;
            //     switch (bestDirection){
            //     case DIRECTION::TOP:
            //         // grow from top side
            //         height = 1;
            //         width = remainingMigrateArea;
            //         remainderRectangle = tile2Rectangle(smallestTile);
            //         gtl::xh(remainderRectangle, gtl::xl(remainderRectangle) + width);
            //         gtl::yl(remainderRectangle, gtl::yh(remainderRectangle) - height);
                    
            //         break;
            //     case DIRECTION::RIGHT:
            //         // grow from right side
            //         height = remainingMigrateArea;
            //         width = 1;
            //         remainderRectangle = tile2Rectangle(smallestTile);
            //         gtl::xl(remainderRectangle, gtl::xh(remainderRectangle) - width);
            //         gtl::yh(remainderRectangle, gtl::yl(remainderRectangle) + height);

            //         break;
            //     case DIRECTION::DOWN:
            //         // grow from down side
            //         height = 1;
            //         width = remainingMigrateArea;
            //         remainderRectangle = tile2Rectangle(smallestTile);
            //         gtl::xh(remainderRectangle, gtl::xl(remainderRectangle) + width);
            //         gtl::yh(remainderRectangle, gtl::yl(remainderRectangle) + height);

            //         break;
            //     case DIRECTION::LEFT:
            //         // grow from left side
            //         height = remainingMigrateArea;
            //         width = 1;
            //         remainderRectangle = tile2Rectangle(smallestTile);
            //         gtl::xh(remainderRectangle, gtl::xl(remainderRectangle) + width);
            //         gtl::yh(remainderRectangle, gtl::yl(remainderRectangle) + height);

            //         break;
            //     default:
            //         break;
            //     }
            //     // split tiles
            //     Tile* remainderTile = mFP->splitTile(smallestTile, remainderRectangle);
            //     if (remainderTile == NULL){
            //         DFSLPrint(0, "Split tile failed (in exact area migration).\n");
            //         return false;
            //     }
            //     else {
            //         int indexToRemove = edge.toIndex;
            //         removeIndexFromOverlap(indexToRemove, remainderTile);
            //     }
            //     newTiles.push_back(remainderTile);
            // }

            break;
        }
    }

    return true;
}

void DFSLegalizer::removeTileFromOverlap(Tile* tile, DFSLNode& overlapNode, DFSLNode& toNode){
    std::vector<Rectilinear*> belongsToOverlaps = mFP->overlapTilePayload[tile];
    if (belongsToOverlaps.size() == 2){
        std::set<Tile*>& overlapList = overlapNode.getOverlapTileList();
        auto iter = overlapList.find(tile);
        assert(iter != overlapList.end());
        overlapList.erase(iter);
    }
    else {
        // bandaid solution
        // todo: make this better
        std::vector<int> overlapsToRemoveFrom;
        for (Rectilinear* recti: belongsToOverlaps){
            if (recti->getId() != toNode.index){
                overlapsToRemoveFrom.push_back(recti->getId());
            }
        }
        int overlapStart = getOverlapBegin();
        int overlapEnd = getOverlapEnd();
        int overlapIndex1 = toNode.index;
        int overlapsRemaining = overlapsToRemoveFrom.size();
        for (int i = overlapStart; i < overlapEnd; i++){
            DFSLNode& overlapNode = mAllNodes[i];
            for (int overlapIndex2: overlapsToRemoveFrom){
                if (overlapNode.overlaps.count(overlapIndex1) == 1 && overlapNode.overlaps.count(overlapIndex2) == 1){
                    std::set<Tile*>& overlapList = overlapNode.getOverlapTileList();
                    auto iter = overlapList.find(tile);
                    assert(iter != overlapList.end());
                    overlapList.erase(iter);
                    overlapsRemaining--;
                    continue;
                }
                if (overlapsRemaining == 0){
                    break; 
                }
            }
        }
    }

    Rectilinear* rectiToRemove = toNode.recti;
    mFP->decreaseTileOverlap(tile, rectiToRemove);
}

bool DFSLegalizer::splitSoftBlock(MigrationEdge& edge, std::vector<Tile*>& newTiles){
    DFSLNode& fromNode = mAllNodes[edge.fromIndex];
    DFSLNode& toNode = mAllNodes[edge.toIndex];

    if (!(fromNode.nodeType == DFSLNodeType::SOFT && toNode.nodeType == DFSLNodeType::SOFT)){
        DFSLPrint(0, "splitSoftBlock can only be used on soft->soft edge.\n");
        return false;
    }

    Rectangle migratedRect, remainderRect(0,0,0,0);
    if (config->getConfigValue<bool>("ExactAreaMigration")){
        std::array<Rectangle, 2> rectangles = getRectFromEdgeExact(edge, mResolvableArea);
        migratedRect = rectangles[0];
        remainderRect = rectangles[1];
    }
    else {
        migratedRect = getRectFromEdge(edge, mResolvableArea, true);
    }

    if (gtl::area(migratedRect) > 0){
        // when splitting tiles, the blockTilelist may be changed and
        // the range-based for loop may be invalidated
        // solution: make a copy of old tiles that stores old tiles
        std::vector<Tile*> originalTiles;
        for (Tile* tile: toNode.getBlockTileList()){
            originalTiles.push_back(tile);
        }
        
        for (Tile* tile: originalTiles){
            Rectangle tileRect = tile->getRectangle();

            // known boost polygon issue: intersect does not pass the value of considerTouch to intersects
            bool intersects = gtl::intersects(tileRect, migratedRect, false);
            if (intersects){
                gtl::intersect(tileRect, migratedRect, false);
                // two conditions
                Tile* newTile = mFP->generalSplitTile(tile, tileRect);

                if (newTile == NULL){
                    DFSLPrint(0, "BB Split tile failed.\n");
                    return false;
                }
                else {
                    mFP->moveTileParent(newTile, toNode.recti, fromNode.recti);
                }
                newTiles.push_back(newTile);
            }
        }
    }

    /*
    TODO: rework exact area migration
    if (gtl::area(remainderRect) > 0 && config->getConfigValue<bool>("ExactAreaMigration")){
        std::vector<Tile*> updatedTileList = mFP->softTesserae[edge.toIndex - mFixedModuleNum]->TileArr;
        for (Tile* tile: updatedTileList){
            Rectangle tileRect = tile2Rectangle(tile);

            // known boost polygon issue: intersect does not pass the value of considerTouch to intersects
            bool intersects = gtl::intersects(tileRect, remainderRect, false);
            if (intersects){
                gtl::intersect(tileRect, remainderRect, false);
                Tile* newTile = mFP->splitTile(tile, tileRect);

                if (newTile == NULL){
                    DFSLPrint(0, " BB Split tile failed\n");
                    return false;
                }
                else {
                    Tessera* fromTess = mFP->softTesserae[edge.fromIndex - mFixedModuleNum];
                    Tessera* toTess = mFP->softTesserae[edge.toIndex - mFixedModuleNum];
                    
                    bool removed = removeFromVector(newTile, toTess->TileArr);
                    if (!removed){
                        DFSLPrint(0, "In splitSoftBlock, tile not found in toTess\n");
                        return false;
                    }
                    fromTess->TileArr.push_back(newTile);
                }
                newTiles.push_back(newTile);
            }
        }
    }
    */
    return true;
}


bool DFSLegalizer::placeInBlank(MigrationEdge& edge, std::vector<Tile*>& newTiles){
    DFSLNode& fromNode = mAllNodes[edge.fromIndex];

    Rectangle newRect, remainderRect(0,0,0,0);
    if (config->getConfigValue<bool>("ExactAreaMigration")){
        std::array<Rectangle, 2> rectangles = getRectFromEdgeExact(edge, mResolvableArea);
        newRect = rectangles[0];
        remainderRect = rectangles[1];
    }
    else {
        newRect = getRectFromEdge(edge, mResolvableArea, true);
    }

    if (gtl::area(newRect) > 0){
        Tile* newTile = mFP->addBlockTile(newRect, fromNode.recti);
        // add to tess, place in physical layout
        newTiles.push_back(newTile);
    }

    if (config->getConfigValue<bool>("ExactAreaMigration") && gtl::area(remainderRect) > 0){
        Tile* remainderTile = mFP->addBlockTile(remainderRect, fromNode.recti);
        // add to tess, place in physical layout
        newTiles.push_back(remainderTile);
    }
    return true;
}

void DFSLegalizer::restructureRectis(){
    // test soft blocks for legality 
    int softStart = getSoftBegin();
    int softEnd = getSoftEnd();
    for (int i = softStart; i < softEnd; i++){
        DFSLNode& block = mAllNodes[i];
        Rectilinear* recti = block.recti;
        mFP->reshapeRectilinear(recti);
    }

    // test fixed blocks for legality
    int fixedStart = getFixedBegin();
    int fixedEnd = getFixedEnd();
    for (int i = fixedStart; i < fixedEnd; i++){
        DFSLNode& block = mAllNodes[i];
        if (block.getArea() == 0){
            continue;
        }
        Rectilinear* recti = block.recti;
        mFP->reshapeRectilinear(recti);
    }
}

int DFSLegalizer::overlapsRemaining(){
    int overlapBegin = getOverlapBegin();
    int overlapEnd = getOverlapEnd();
    int n = 0;
    for (int i = overlapBegin; i < overlapEnd; i++){
        DFSLNode& overlap = mAllNodes[i];
        if (overlap.getArea() > 0){
            n++;
        }
    }
    return n;
}

void DFSLegalizer::printAllNodesDebug(){
    if (config->getConfigValue<int>("OutputLevel") < DFSL_DEBUG){
        return;
    }
    for (DFSLNode& node: mAllNodes){
        DFSLPrint(4, "%d: %s\n", node.index, node.nodeName.c_str());
        DFSLPrint(4, "Area: %d\n", node.getArea());
        switch (node.nodeType)
        {
        case DFSLNodeType::OVERLAP:
            DFSLPrint(4, "OVERLAP\n");
            for (Tile* tile: node.getOverlapTileList()){
                DFSLPrint(4, "\t%1%\n", *(tile));
            }
            break;
        case DFSLNodeType::FIXED:
            DFSLPrint(4, "FIXED");
            for (Tile* tile: node.getBlockTileList()){
                DFSLPrint(4, "\t%1%\n", *(tile));
            }
            break;
        case DFSLNodeType::SOFT:
            DFSLPrint(4, "SOFT");
            for (Tile* tile: node.getBlockTileList()){
                DFSLPrint(4, "\t%1%\n", *(tile));
            }
            break;
        
        default:
            break;
        }
        DFSLPrint(4, "Edges:\n");
        for (DFSLEdge& edge: node.edgeList){
            DFSLPrint(4, ">(%d, %d), ", edge.getFrom(), edge.getTo());
            if (edge.getType() == EDGETYPE::OB){
                DFSLPrint(4, "OB, \n");
            }
            else if (edge.getType() == EDGETYPE::BB){
                DFSLPrint(4, "BB, \n");
            }
            else if (edge.getType() == EDGETYPE::BW){
                DFSLPrint(4, "BW, \n");
            }
            else {
                DFSLPrint(4, "OTHER, \n");
            }
            for (Segment& seg: edge.tangentSegments()){
                DFSLPrint(4, ">>%1%\n", seg);
            }
        }
        DFSLPrint(4, "\n");
    }
}

static void DFSLNode2PolySet(DFSLNode& node, Polygon90Set& polySet){
    if (node.nodeType == DFSLNodeType::OVERLAP){
        for (Tile* tile: node.getOverlapTileList()){
            polySet += tile->getRectangle();
        }
    }
    else if (node.nodeType == DFSLNodeType::SOFT || node.nodeType == DFSLNodeType::FIXED){
        for (Tile* tile: node.getBlockTileList()){
            polySet += tile->getRectangle();
        }
    }
}

// given two tiles, find their tangent segment
static Segment findTangentSegment(Tile* from, Tile* to, int direction){
    // find tangent segment 
    Cord segStart, segEnd;
    DIRECTION dir;
    Cord fromStart, fromEnd, toStart, toEnd;
    if (direction == 0){
        // to on top of from 
        fromStart = from->getUpperLeft();
        fromEnd = from->getUpperRight();
        toStart = to->getLowerLeft(); 
        toEnd = to->getLowerRight();
        dir = DIRECTION::TOP;
    }
    else if (direction == 1){
        // to is right of from 
        fromStart = from->getLowerRight();
        fromEnd = from->getUpperRight();
        toStart = to->getLowerLeft(); 
        toEnd = to->getUpperLeft();
        dir = DIRECTION::RIGHT;
    }
    else if (direction == 2){
        // to is bottom of from 
        fromStart = from->getLowerLeft();
        fromEnd = from->getLowerRight();
        toStart = to->getUpperLeft(); 
        toEnd = to->getUpperRight();
        dir = DIRECTION::DOWN;
    }
    else {
        // to is left of from 
        fromStart = from->getLowerLeft();
        fromEnd = from->getUpperLeft();
        toStart = to->getLowerRight(); 
        toEnd = to->getUpperRight();
        dir = DIRECTION::LEFT;
    }
    segStart = fromStart <= toStart ? toStart : fromStart;
    segEnd = fromEnd <= toEnd ? fromEnd : toEnd;
    return Segment(segStart, segEnd, dir);
}

static std::array<double,3> getPolySetInfo(Polygon90Set& poly){
    double actualArea = (double) gtl::area(poly);
    Rectangle bbox;
    gtl::extents(bbox, poly);

    double bboxArea = (double) gtl::area(bbox);
    double util = actualArea / bboxArea;

    // rectangle.h
    double aspectRatio = rec::calculateAspectRatio(bbox);

    return {actualArea, util, aspectRatio};
}

// find actual rectangle, equal to resolvable area
Rectangle getRectFromEdge(MigrationEdge& edge, int goalArea, bool useCeil){
    int xl = 0, xh = 0, yl = 0, yh = 0;
    if (edge.segment.getDirection() == DIRECTION::TOP){
        xl = edge.segment.getSegStart().x();
        xh = edge.segment.getSegEnd().x();
        yl = edge.segment.getSegStart().y();
        int width = xh - xl;
        int requiredHeight = useCeil ? ceil((double) goalArea / (double) width) : floor((double) goalArea / (double) width);
        yh = yl + requiredHeight;
    }
    else if (edge.segment.getDirection() == DIRECTION::RIGHT){
        yl = edge.segment.getSegStart().y();
        yh = edge.segment.getSegEnd().y();
        xl = edge.segment.getSegStart().x();
        int height = yh - yl;
        int requiredWidth = useCeil ? ceil((double) goalArea / (double) height) : floor((double) goalArea / (double) height);
        xh = xl + requiredWidth;
    }
    else if (edge.segment.getDirection() == DIRECTION::DOWN){
        xl = edge.segment.getSegStart().x();
        xh = edge.segment.getSegEnd().x();
        yh = edge.segment.getSegStart().y();
        int width = xh - xl;
        int requiredHeight = useCeil ? ceil((double) goalArea / (double) width) : floor((double) goalArea / (double) width);
        yl = yh - requiredHeight;
    }
    else if (edge.segment.getDirection() == DIRECTION::LEFT) {
        yl = edge.segment.getSegStart().y();
        yh = edge.segment.getSegEnd().y();
        xh = edge.segment.getSegStart().x();
        int height = yh - yl;
        int requiredWidth = useCeil ? ceil((double) goalArea / (double) height) : floor((double) goalArea / (double) height);
        xl = xh - requiredWidth;
    }
    else {
        std::cerr << "getRectFromEdge: Edge (" << edge.fromIndex << " -> " << edge.toIndex << ") has no DIRECTION\n";
    }
    

    return Rectangle(xl,yl,xh,yh);
}

static std::array<Rectangle,2> getRectFromEdgeExact(MigrationEdge& edge, int goalArea){
    int xl = 0, xh = 0, yl = 0, yh = 0;
    int rxl = 0, rxh = 0, ryl = 0, ryh = 0;
    Rectangle bigRect = getRectFromEdge(edge, goalArea, false);
    Rectangle remainderRect = Rectangle(0,0,0,0);
    int remainingArea = goalArea - gtl::area(bigRect);
    xl = rec::getXL(bigRect);
    xh = rec::getXH(bigRect);
    yl = rec::getYL(bigRect);
    yh = rec::getYH(bigRect);

    
    if (remainingArea > 0){    
        if (edge.segment.getDirection() == DIRECTION::TOP){
            rxl = xl;
            rxh = rxl + remainingArea;
            ryl = yh;
            ryh = ryl + 1;
        }
        else if (edge.segment.getDirection() == DIRECTION::RIGHT){
            rxl = xh;
            rxh = rxl + 1;
            ryl = yl;
            ryh = ryl + remainingArea;
        }
        else if (edge.segment.getDirection() == DIRECTION::DOWN){
            rxl = xl;
            rxh = rxl + remainingArea;
            ryh = yl;
            ryl = ryh - 1;
        }
        else if (edge.segment.getDirection() == DIRECTION::LEFT) {
            rxh = xl;
            rxl = rxh - 1;
            ryl = yl;
            ryh = ryl + remainingArea;
        }
        else {
            std::cerr << "getRectFromEdgeExact: Edge (" << edge.fromIndex << " -> " << edge.toIndex << ") has no DIRECTION\n";
        }
        remainderRect = Rectangle(rxl, ryl, rxh, ryh);
    }

    return {bigRect, remainderRect};
}

static Rectangle extendSegment(Segment& seg, int requiredArea, Floorplan* fp){
    int xl, xh, yl, yh;
    Rectangle resultRectangle;

    if (seg.getDirection() == DIRECTION::TOP){
        xl = seg.getSegStart().x();
        xh = seg.getSegEnd().x();
        yl = seg.getSegStart().y();
        int requiredHeight = ceil((double) requiredArea / (double) (xh-xl));
        yh = yl + requiredHeight;
        
        yh = yh > rec::getYH(fp->getChipContour()) ? rec::getYH(fp->getChipContour()) : yh;

        Rectangle goalRect(xl,yl,xh,yh);
        std::vector<Tile*> obstacles;
        fp->cs->enumerateDirectedArea(goalRect, obstacles);

        int lowestObstacle = yh;
        for (Tile* tile: obstacles){
            if (tile->getYLow() < lowestObstacle){
                lowestObstacle = tile->getYLow();
            }
        }

        resultRectangle = Rectangle(xl,yl,xh,lowestObstacle);
    }
    else if (seg.getDirection() == DIRECTION::RIGHT){
        yl = seg.getSegStart().y();
        yh = seg.getSegEnd().y();
        xl = seg.getSegStart().x();
        int requiredWidth = ceil((double) requiredArea / (double) (yh-yl));
        xh = xl + requiredWidth;

        xh = xh > rec::getXH(fp->getChipContour()) ? rec::getXH(fp->getChipContour()) : xh;

        Rectangle goalRect(xl,yl,xh,yh);
        std::vector<Tile*> obstacles;
        fp->cs->enumerateDirectedArea(goalRect, obstacles);

        int leftmostObstacle = xh;
        for (Tile* tile: obstacles){
            if (tile->getXLow() < leftmostObstacle){
                leftmostObstacle = tile->getXLow();
            }
        }

        resultRectangle = Rectangle(xl,yl,leftmostObstacle,yh);
    }
    else if (seg.getDirection() == DIRECTION::DOWN){
        xl = seg.getSegStart().x();
        xh = seg.getSegEnd().x();
        yh = seg.getSegStart().y();
        int requiredHeight = ceil((double) requiredArea / (double) (xh-xl));
        yl = yh - requiredHeight;

        yl = yl < 0 ? 0 : yl;

        Rectangle goalRect(xl,yl,xh,yh);
        std::vector<Tile*> obstacles;
        fp->cs->enumerateDirectedArea(goalRect, obstacles);

        int highestObstacle = yl;
        for (Tile* tile: obstacles){
            if (tile->getYHigh() > highestObstacle){
                highestObstacle = tile->getYHigh();
            }
        }

        resultRectangle = Rectangle(xl,highestObstacle,xh,yh);
    }
    else if (seg.getDirection() == DIRECTION::LEFT) {
        yl = seg.getSegStart().y();
        yh = seg.getSegEnd().y();
        xh = seg.getSegStart().x();
        int requiredWidth = ceil((double) requiredArea / (double) (yh-yl));
        xl = xh - requiredWidth;

        xl = xl < 0 ? 0 : xl;

        Rectangle goalRect(xl,yl,xh,yh);
        std::vector<Tile*> obstacles;
        fp->cs->enumerateDirectedArea(goalRect, obstacles);

        int rightmostObstacle = xl;
        for (Tile* tile: obstacles){
            if (tile->getXHigh() > rightmostObstacle){
                rightmostObstacle = tile->getXHigh();
            }
        }

        resultRectangle = Rectangle(rightmostObstacle,yl,xh,yh);
    }
    else {
        std::cerr << "extendSegment: edge has no DIRECTION\n";
    }

    return resultRectangle;
}

// 0 : Errors
// 1 : Warnings
// 2 : Standard info
// 3 : Verbose info
// 4 : debug info
void DFSLegalizer::setOutputLevel(int level){
    config->setConfigValue<int>("OutputLevel",level);
}

void DFSLegalizer::printTiledFloorplan(const std::string& outputFileName, const std::string& floorplanName){
    DFSLPrint(2, "Print floorplan (with tile info) to \"%s\"\n", outputFileName);
    this->mFP->visualizeTiledFloorplan(outputFileName);
    std::ofstream fout(outputFileName, std::ios::out|std::ios::app);
        
    if (!fout.is_open()){
        std::cerr << "Filename " << outputFileName << " not opened\n";
        return;
    }

    // append name of floorplan to output file
    fout << floorplanName << std::endl;

    fout.close();
}

void DFSLegalizer::printCompleteFloorplan(const std::string& outputFileName, const std::string& floorplanName){
    DFSLPrint(2, "Print floorplan to \"%s\"\n", outputFileName);
    this->mFP->visualiseRectiFloorplan(outputFileName);
    std::ofstream fout(outputFileName, std::ios::out|std::ios::app);
    
    if (!fout.is_open()){
        std::cerr << "Filename " << outputFileName << " not opened\n";
        return;
    }

    // append name of floorplan to output file
    fout << floorplanName << std::endl;

    fout.close();
}


}