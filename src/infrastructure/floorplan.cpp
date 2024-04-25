#include <assert.h>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <map>
#include <set>
#include <utility>

#include "boost/polygon/polygon.hpp"
#include "floorplan.h"
#include "cSException.h"
#include "doughnutPolygon.h"
#include "doughnutPolygonSet.h"

#include "glpk.h"

Rectilinear *Floorplan::placeRectilinear(std::string name, rectilinearType type, Rectangle placement, area_t legalArea, double aspectRatioMin, double aspectRatioMax, double mUtilizationMin){
    if(!rec::isContained(mChipContour,placement)){
        throw CSException("FLOORPLAN_16");
    }

    // register the Rectilinear container into the floorplan data structure
    Rectilinear *newRect = new Rectilinear(mIDCounter++, name, type, placement, legalArea, aspectRatioMin, aspectRatioMax, mUtilizationMin); 
    switch (type){
    case rectilinearType::SOFT:
        this->allRectilinears.push_back(newRect);
        this->softRectilinears.push_back(newRect);
        break;
    case rectilinearType::PREPLACED:
        this->allRectilinears.push_back(newRect);
        this->preplacedRectilinears.push_back(newRect);
        break;
    case rectilinearType::PIN:
        this->pinRectilinears.push_back(newRect);
        return newRect;
        break;
    default:
        break;
    }

    std::vector<Tile *> lappingTiles;
    cs->enumerateDirectedArea(placement, lappingTiles);
    if(lappingTiles.empty()){
        // placement of the rectilinear is in an area whether no other tiles are present
        addBlockTile(placement, newRect);
    }else{

        using namespace boost::polygon::operators;
        DoughnutPolygonSet insertSet;
        insertSet += placement;

        // process all overlap parts with other tles
        for(int i = 0; i < lappingTiles.size(); ++i){
            Tile *lapTile = lappingTiles[i];

            DoughnutPolygonSet origTileSet;
            origTileSet += lapTile->getRectangle();

            DoughnutPolygonSet overlapSet;
            boost::polygon::assign(overlapSet, insertSet & origTileSet);

            if(boost::polygon::equivalence(origTileSet, overlapSet)){
                // the insertion of such piece would not cause new fragments
                increaseTileOverlap(lapTile, newRect);
            }else{
                // the insertion has overlap with some existing tiles
                // 1. remove the original whole block 
                // 2. place the intersection part
                // 3. place the rest part, dice into small rectangles if necessary

                tileType lapTileType = lapTile->getType();
                if(lapTileType == tileType::BLOCK){
                    Rectilinear *origPayload = this->blockTilePayload[lapTile];

                    deleteTile(lapTile);

                    // add the intersection part first
                    std::vector<Rectangle> intersectRect;
                    dps::diceIntoRectangles(overlapSet, intersectRect);
                    assert(intersectRect.size() == 1);
                    addOverlapTile(intersectRect[0], std::vector<Rectilinear *>({origPayload, newRect}));

                    // process the remains of the overlapSet
                    origTileSet -= overlapSet;
                    std::vector<Rectangle> restRect;
                    dps::diceIntoRectangles(origTileSet, restRect);
                    for(Rectangle const &rt : restRect){
                        addBlockTile(rt, origPayload);
                    }
                }else if(lapTileType == tileType::OVERLAP){
                    std::vector<Rectilinear *> origPaylaod = this->overlapTilePayload[lapTile];

                    deleteTile(lapTile);

                    // add the intersection part first
                    std::vector<Rectangle> intersectRect;
                    dps::diceIntoRectangles(overlapSet, intersectRect);
                    assert(intersectRect.size() == 1);
                    std::vector<Rectilinear *> newPayload(origPaylaod);
                    newPayload.push_back(newRect);
                    addOverlapTile(intersectRect[0], newPayload);

                    // process the remains of the overlapSet
                    origTileSet -= overlapSet;
                    std::vector<Rectangle> restRect;
                    dps::diceIntoRectangles(origTileSet, restRect);
                    for(Rectangle const &rt : restRect){
                        addOverlapTile(rt, origPaylaod);
                    }

                }else{
                    throw CSException("FLOORPLAN_16");
                }
                
            }

            // removed the processed part from insertSet
            insertSet -= overlapSet;
        }

        std::vector<Rectangle> remainInsertRect;
        dps::diceIntoRectangles(insertSet, remainInsertRect);
        for(const Rectangle &rt : remainInsertRect){
            addBlockTile(rt, newRect);
        }
    }

    return newRect;
}

// modified by ryan: 
// added initializers for all member functions
Floorplan::Floorplan()
    : mIDCounter(0), mChipContour(Rectangle(0, 0, 0, 0)) , mAllRectilinearCount(0), mSoftRectilinearCount(0), mPreplacedRectilinearCount(0), mConnectionCount(0),
    mGlobalAspectRatioMin(0.0), mGlobalAspectRatioMax(0.0), mGlobalUtilizationMin(0.0), cs(NULL) {
}

// modified by ryan: 
// added mIDCounter(0)
Floorplan::Floorplan(const GlobalResult &gr, double aspectRatioMin, double aspectRatioMax, double utilizationMin)
    : mIDCounter(0), mGlobalAspectRatioMin(aspectRatioMin), mGlobalAspectRatioMax(aspectRatioMax), mGlobalUtilizationMin(utilizationMin) {

    mChipContour = Rectangle(0, 0, gr.chipWidth, gr.chipHeight);
    mAllRectilinearCount = gr.blockCount;
    mConnectionCount = gr.connectionCount;

    cs = new CornerStitching(gr.chipWidth, gr.chipHeight);
    
    assert(mAllRectilinearCount == gr.blocks.size());
    assert(mConnectionCount == gr.connections.size());

    // map for create connections
    std::unordered_map<std::string, Rectilinear*> nameToRectilinear;

    // create Rectilinears
    // added by ryan: init soft & fixed count
    mSoftRectilinearCount = 0;
    mPreplacedRectilinearCount = 0;
    mPinRectilinearCount = 0;

    for(int i = 0; i < mAllRectilinearCount; ++i){
        GlobalResultBlock grb = gr.blocks[i];
        rectilinearType rType;
        if(grb.type == "SOFT"){
            rType = rectilinearType::SOFT;
            mSoftRectilinearCount++;
        }else if(grb.type == "FIXED"){
            if((grb.width == 0) && (grb.height == 0)){
                rType = rectilinearType::PIN;
                mPinRectilinearCount++;
            }else{
                rType = rectilinearType::PREPLACED;
                mPreplacedRectilinearCount++;
            }
        }else{
            throw CSException("FLOORPLAN_01");
        }

        Rectilinear *newRect = placeRectilinear(grb.name, rType, Rectangle(grb.llx, grb.lly, (grb.llx + grb.width), (grb.lly + grb.height)),
                        grb.legalArea, this->mGlobalAspectRatioMin, this->mGlobalAspectRatioMax, this->mGlobalUtilizationMin);

        nameToRectilinear[grb.name] = newRect;
    }
    // Reshape all Rectilinears after insertion complete, order of insertion causes excessive splitting
    for(Rectilinear* const &rect : this->allRectilinears){
        reshapeRectilinear(rect);
    }
    
    // create Connections
    // 2024/04/10 Hank's update: add connectonMap
    for(int i = 0; i < mConnectionCount; ++i){
        GlobalResultConnection grc = gr.connections[i];
        std::vector<Rectilinear *> connVertices;
        for(std::string const &str : grc.vertices){
            connVertices.push_back(nameToRectilinear[str]);
        }
        Connection *newConn = new Connection(connVertices, grc.weight);
        this->allConnections.push_back(newConn);
        for(Rectilinear *const &rt : connVertices){
            std::unordered_map<Rectilinear *, std::vector<Connection *>>::iterator it = connectionMap.find(rt);
            if(it == connectionMap.end()){
                connectionMap[rt] = {newConn};
            }else{
                connectionMap[rt].push_back(newConn);
            }
        }
    }
}

Floorplan::Floorplan(const LegalResult &lr, double aspectRatioMin, double aspectRatioMax, double utilizationMin)
    : mGlobalAspectRatioMin(aspectRatioMin), mGlobalAspectRatioMax(aspectRatioMax), mGlobalUtilizationMin(utilizationMin) {

    mChipContour = Rectangle(0, 0, lr.chipWidth, lr.chipHeight);
    mAllRectilinearCount = lr.softBlockCount + lr.fixedBlockCount;
    mConnectionCount = lr.connectionCount;

    cs = new CornerStitching(lr.chipWidth, lr.chipHeight);

    // map for connection linking
    std::unordered_map<std::string, Rectilinear*> nameToRectilinear;

    // fill in rectiliners
    for(LegalResultBlock const &lrb : lr.softBlocks){
        Rectilinear *newRect = new Rectilinear(mIDCounter++, lrb.name, rectilinearType::SOFT, Rectangle(0, 0, 0, 0), lrb.legalArea,
                                                 this->mGlobalAspectRatioMin, this->mGlobalAspectRatioMax, this->mGlobalUtilizationMin);

        this->allRectilinears.push_back(newRect);
        this->softRectilinears.push_back(newRect);
        
        std::vector<Cord> cornerVec;
        for(int i = 0; i < lrb.cornerCount; ++i){
            cornerVec.push_back(Cord(lrb.xVec[i], lrb.yVec[i]));
        }
        using namespace boost::polygon::operators;
        DoughnutPolygon dp;
        boost::polygon::set_points(dp, cornerVec.begin(), cornerVec.end());
        DoughnutPolygonSet dpSet;
        dpSet += dp;

        std::vector<Rectangle> afterDicedRect;
        dps::diceIntoRectangles(dpSet, afterDicedRect);
        for(Rectangle const &drec : afterDicedRect){
            addBlockTile(drec, newRect);
        }

        nameToRectilinear[lrb.name] = newRect;
    }

    for(LegalResultBlock const &lrb : lr.fixedBlocks){
        Rectilinear *newRect = new Rectilinear(mIDCounter++, lrb.name, rectilinearType::PREPLACED, Rectangle(0, 0, 0, 0), lrb.legalArea,
                                                 this->mGlobalAspectRatioMin, this->mGlobalAspectRatioMax, this->mGlobalUtilizationMin);

        this->allRectilinears.push_back(newRect); 
        this->preplacedRectilinears.push_back(newRect);
        std::vector<Cord> cornerVec;
        for(int i = 0; i < lrb.cornerCount; ++i){
            cornerVec.push_back(Cord(lrb.xVec[i], lrb.yVec[i]));
        }
        using namespace boost::polygon::operators;
        DoughnutPolygon dp;
        boost::polygon::set_points(dp, cornerVec.begin(), cornerVec.end());
        DoughnutPolygonSet dpSet;
        dpSet += dp;

        std::vector<Rectangle> afterDicedRect;
        dps::diceIntoRectangles(dpSet, afterDicedRect);
        for(Rectangle const &drec : afterDicedRect){
            addBlockTile(drec, newRect);
        }

        nameToRectilinear[lrb.name] = newRect;
    }    

    // create Connections
    // 2024/04/10 update: add connectonMap
    for(int i = 0; i < mConnectionCount; ++i){
        LegalResultConnection grc = lr.connections[i];
        std::vector<Rectilinear *> connVertices;
        for(std::string const &str : grc.vertices){
            connVertices.push_back(nameToRectilinear[str]);
        }
        Connection *newConn = new Connection(connVertices, grc.weight);
        this->allConnections.push_back(newConn);
        for(Rectilinear *const &rt : connVertices){
            std::unordered_map<Rectilinear *, std::vector<Connection *>>::iterator it = connectionMap.find(rt);
            if(it == connectionMap.end()){
                connectionMap[rt] = {newConn};
            }else{
                connectionMap[rt].push_back(newConn);
            }
        }
    }

}

Floorplan::Floorplan(const Floorplan &other){
    
    // copy basic attributes
    this->mIDCounter = other.mIDCounter;
    this->mChipContour = Rectangle(other.mChipContour);
    this->mAllRectilinearCount = other.mAllRectilinearCount;
    this->mSoftRectilinearCount = other.mSoftRectilinearCount;
    this->mPreplacedRectilinearCount = other.mPreplacedRectilinearCount;
    this->mPinRectilinearCount = other.mPinRectilinearCount;

    this->mConnectionCount = other.mConnectionCount;

    this->mGlobalAspectRatioMin = other.mGlobalAspectRatioMin;
    this->mGlobalAspectRatioMax = other.mGlobalAspectRatioMax;
    this->mGlobalUtilizationMin = other.mGlobalUtilizationMin;

    this->cs = new CornerStitching(*other.cs);

    // build maps to assist copy
    std::unordered_map<Rectilinear *, Rectilinear*> rectMap;
    std::unordered_map<Tile *, Tile *> tileMap;

    for(Rectilinear *const &oldRect : other.allRectilinears){
        Rectilinear *nR = new Rectilinear(*oldRect);

        // re-consruct the block tiles pointers using the new CornerStitching System
        nR->blockTiles.clear();
        for(Tile *const &oldT : oldRect->blockTiles){
            Tile *newT = this->cs->findPoint(oldT->getLowerLeft());
            tileMap[oldT] = newT;
            nR->blockTiles.insert(newT);
        }

        // re-consruct the overlap tiles pointers using the new CornerStitching System
        nR->overlapTiles.clear();
        for(Tile *const &oldT : oldRect->overlapTiles){
            Tile *newT = this->cs->findPoint(oldT->getLowerLeft());
            tileMap[oldT] = newT;
            nR->overlapTiles.insert(newT);
        }

        rectMap[oldRect] = nR;
    }
    // rework pointers for rectilinear vectors to point to new CornerStitching System
    this->allRectilinears.clear();
    this->softRectilinears.clear();
    this->preplacedRectilinears.clear();

    for(Rectilinear *const &oldR : other.allRectilinears){
        Rectilinear *newR = rectMap[oldR];
        this->allRectilinears.push_back(rectMap[oldR]);
        
        // categorize types
        switch (newR->getType()){
        case rectilinearType::SOFT:
            this->softRectilinears.push_back(newR);
            break;
        case rectilinearType::PREPLACED:
            this->preplacedRectilinears.push_back(newR);
            break;
        default:
            break;
        }
    }

    // construct pinRectilinears;
    this->pinRectilinears.clear();
    for(Rectilinear *const &oldR : other.pinRectilinears){
        Rectilinear *newR = new Rectilinear(*oldR);
        this->pinRectilinears.push_back(newR);
        rectMap[oldR] = newR;
    }

    // rebuild connections
    this->allConnections.clear();
    for(Connection *const &cn : other.allConnections){
        Connection *newCN = new Connection(*cn);
        newCN->vertices.clear();
        for(Rectilinear *const &oldRT : cn->vertices){
            newCN->vertices.push_back(rectMap[oldRT]);
        }
        this->allConnections.push_back(newCN);
    }
    //rebuild connectionMap (added at 2024/04/10)
    this->connectionMap.clear();

    for(Connection *const &cn : this->allConnections){
        std::unordered_map <Rectilinear *, std::vector<Connection *>>::iterator it;
        for(Rectilinear *const &rt : cn->vertices){
            it = this->connectionMap.find(rt);
            if(it == connectionMap.end()){
                connectionMap[rt] = {cn};
            }else{
                connectionMap[rt].push_back(cn);
            }
        }
    }

    // rebuid Tile payloads section  
    this->blockTilePayload.clear();
    for(std::unordered_map<Tile *, Rectilinear *>::const_iterator it = other.blockTilePayload.begin(); it != other.blockTilePayload.end(); ++it){
        Tile *nT = tileMap[it->first];
        Rectilinear *nR = rectMap[it->second];

        this->blockTilePayload[nT] = nR;
    }

    this->overlapTilePayload.clear();
    for(std::unordered_map<Tile *, std::vector<Rectilinear *>>::const_iterator it = other.overlapTilePayload.begin(); it != other.overlapTilePayload.end(); ++it){
        Tile *nT = tileMap[it->first];
        std::vector<Rectilinear *> nRectVec;
        for(Rectilinear *const &oldR : it->second){
            nRectVec.push_back(rectMap[oldR]);
        }
        
        this->overlapTilePayload[nT] = nRectVec;
    }
    
}

Floorplan::~Floorplan(){
    for(Rectilinear *&rt : this->allRectilinears){
        delete(rt);
    }

    for(Rectilinear *&rt : this->pinRectilinears){
        delete(rt);
    }

    for(Connection *&cn : this->allConnections){
        delete(cn);
    }

    delete(cs);
}

Floorplan &Floorplan::operator = (const Floorplan &other){
    // copy basic attributes
    this->mChipContour = Rectangle(other.mChipContour);
    this->mAllRectilinearCount = other.mAllRectilinearCount;
    this->mSoftRectilinearCount = other.mSoftRectilinearCount;
    this->mPreplacedRectilinearCount = other.mPreplacedRectilinearCount;
    this->mPinRectilinearCount = other.mPinRectilinearCount;

    this->mConnectionCount = other.mConnectionCount;

    this->mGlobalAspectRatioMin = other.mGlobalAspectRatioMin;
    this->mGlobalAspectRatioMax = other.mGlobalAspectRatioMax;
    this->mGlobalUtilizationMin = other.mGlobalUtilizationMin;

    this->cs = new CornerStitching(*other.cs);

    // build maps to assist copy
    std::unordered_map<Rectilinear *, Rectilinear*> rectMap;
    std::unordered_map<Tile *, Tile *> tileMap;

    for(Rectilinear *const &oldRect : other.allRectilinears){
        Rectilinear *nR = new Rectilinear(*oldRect);

        // re-consruct the block tiles pointers using the new CornerStitching System
        nR->blockTiles.clear();
        for(Tile *const &oldT : oldRect->blockTiles){
            Tile *newT = this->cs->findPoint(oldT->getLowerLeft());
            tileMap[oldT] = newT;
            nR->blockTiles.insert(newT);
        }

        // re-consruct the overlap tiles pointers using the new CornerStitching System
        nR->overlapTiles.clear();
        for(Tile *const &oldT : oldRect->overlapTiles){
            Tile *newT = this->cs->findPoint(oldT->getLowerLeft());
            tileMap[oldT] = newT;
            nR->overlapTiles.insert(newT);
        }

        rectMap[oldRect] = nR;
    }
    // rework pointers for rectilinear vectors to point to new CornerStitching System
    this->allRectilinears.clear();
    this->softRectilinears.clear();
    this->preplacedRectilinears.clear();

    for(Rectilinear *const &oldR : other.allRectilinears){
        Rectilinear *newR = rectMap[oldR];
        this->allRectilinears.push_back(rectMap[oldR]);
        
        // categorize types
        switch (newR->getType()){
        case rectilinearType::SOFT:
            this->softRectilinears.push_back(newR);
            break;
        case rectilinearType::PREPLACED:
            this->preplacedRectilinears.push_back(newR);
            break;
        default:
            break;
        }
    }

    // construct pinRectilinears;
    this->pinRectilinears.clear();
    for(Rectilinear *const &oldR : other.pinRectilinears){
        Rectilinear *newR = new Rectilinear(*oldR);
        this->pinRectilinears.push_back(newR);
        rectMap[oldR] = newR;
    }

    // rebuild connections
    this->allConnections.clear();
    for(Connection *const &cn : other.allConnections){
        Connection *newCN = new Connection(*cn);
        newCN->vertices.clear();
        for(Rectilinear *const &oldRT : cn->vertices){
            newCN->vertices.push_back(rectMap[oldRT]);
        }
        this->allConnections.push_back(newCN);
    }
    //rebuild connectionMap (added at 2024/04/10)
    this->connectionMap.clear();

    for(Connection *const &cn : this->allConnections){
        std::unordered_map <Rectilinear *, std::vector<Connection *>>::iterator it;
        for(Rectilinear *const &rt : cn->vertices){
            it = this->connectionMap.find(rt);
            if(it == connectionMap.end()){
                connectionMap[rt] = {cn};
            }else{
                connectionMap[rt].push_back(cn);
            }
        }
    }

    // rebuid Tile payloads section  
    this->blockTilePayload.clear();
    for(std::unordered_map<Tile *, Rectilinear *>::const_iterator it = other.blockTilePayload.begin(); it != other.blockTilePayload.end(); ++it){
        Tile *nT = tileMap[it->first];
        Rectilinear *nR = rectMap[it->second];

        this->blockTilePayload[nT] = nR;
    }

    this->overlapTilePayload.clear();
    for(std::unordered_map<Tile *, std::vector<Rectilinear *>>::const_iterator it = other.overlapTilePayload.begin(); it != other.overlapTilePayload.end(); ++it){
        Tile *nT = tileMap[it->first];
        std::vector<Rectilinear *> nRectVec;
        for(Rectilinear *const &oldR : it->second){
            nRectVec.push_back(rectMap[oldR]);
        }
        
        this->overlapTilePayload[nT] = nRectVec;
    }

    return (*this);
}

Rectangle Floorplan::getChipContour() const {
    return this->mChipContour;
}

int Floorplan::getAllRectilinearCount() const {
    return this->mAllRectilinearCount;
}

int Floorplan::getSoftRectilinearCount() const {
    return this->mSoftRectilinearCount;
}

int Floorplan::getPreplacedRectilinearCount() const {
    return this->mPreplacedRectilinearCount;
}

int Floorplan::getPinRectilinearCount() const {
    return this->mPinRectilinearCount;
}

int Floorplan::getConnectionCount() const {
    return this->mConnectionCount;
}


double Floorplan::getGlobalAspectRatioMin() const {
    return this->mGlobalAspectRatioMin;
}
double Floorplan::getGlobalAspectRatioMax() const {
    return this->mGlobalAspectRatioMax;
}
double Floorplan::getGlobalUtilizationMin() const {
    return this->mGlobalUtilizationMin;
}

void Floorplan::setGlobalAspectRatioMin(double globalAspectRatioMin){
    this->mGlobalAspectRatioMin = globalAspectRatioMin;
}
void Floorplan::setGlobalAspectRatioMax(double globalAspectRatioMax){
    this->mGlobalAspectRatioMax = globalAspectRatioMax;

}
void Floorplan::setGlobalUtilizationMin(double globalUtilizationMin){
    this->mGlobalUtilizationMin = globalUtilizationMin;
}

Tile *Floorplan::addBlockTile(const Rectangle &tilePosition, Rectilinear *rt){
    if(rt->getType() == rectilinearType::PIN){
        throw CSException("FLOORPLAN_24");
    }
    
    if(!rec::isContained(this->mChipContour, tilePosition)){
        throw CSException("FLOORPLAN_02");
    }

    if(std::find(allRectilinears.begin(), allRectilinears.end(), rt) == allRectilinears.end()){
        throw CSException("FLOORPLAN_03");
    }

    // use the prototype to insert the tile onto the cornerstitching system, receive the actual pointer as return
    Tile tilePrototype(tileType::BLOCK, tilePosition);
    Tile *newTile = cs->insertTile(tilePrototype);
    // register the pointer to the rectilienar system 
    rt->blockTiles.insert(newTile);
    // connect tile's payload as the rectilinear on the floorplan system 
    this->blockTilePayload[newTile] = rt;

    return newTile;
}

Tile *Floorplan::addOverlapTile(const Rectangle &tilePosition, const std::vector<Rectilinear*> &payload){
    if(!rec::isContained(this->mChipContour, tilePosition)){
        throw CSException("FLOORPLAN_04");
    }

    for(Rectilinear *const &rt : payload){
        if(rt->getType() == rectilinearType::PIN){
            throw CSException("FLOORPLAN_25");
        }
        if(std::find(allRectilinears.begin(), allRectilinears.end(), rt) == allRectilinears.end()){
            throw CSException("FLOORPLAN_05");
        }
    }

    // use the prototype to insert the tile onto the cornerstitching system, receive the actual pointer as return
    Tile tilePrototype(tileType::OVERLAP, tilePosition);
    Tile *newTile = cs->insertTile(tilePrototype);
    // register the pointer to all Rectilinears
    for(Rectilinear *const rt : payload){
        rt->overlapTiles.insert(newTile);
    }

    // connect tile's payload 
    this->overlapTilePayload[newTile] = std::vector<Rectilinear *>(payload);

    return newTile;
}

void Floorplan::deleteTile(Tile *tile){
    tileType toDeleteType = tile->getType();
    if(!((toDeleteType == tileType::BLOCK) || (toDeleteType == tileType::OVERLAP))){
        throw CSException("FLOORPLAN_06");
    }
    // clean-up the payload information stored inside the floorplan system
    if(toDeleteType == tileType::BLOCK){
        std::unordered_map<Tile *, Rectilinear *>::iterator blockIt= this->blockTilePayload.find(tile);
        if(blockIt == this->blockTilePayload.end()){
            throw CSException("FLOORPLAN_07");
        }
        // erase the tile from the rectilinear structure
        Rectilinear *rt = blockIt->second;
        rt->blockTiles.erase(tile);
        // erase the payload from the floorplan structure
        this->blockTilePayload.erase(tile);
    }else{
        std::unordered_map<Tile *, std::vector<Rectilinear *>>::iterator overlapIt = this->overlapTilePayload.find(tile);
        if(overlapIt == this->overlapTilePayload.end()){
            throw CSException("FLOORPLAN_08");
        }

        // erase the tiles from the rectiliear structures
        for(Rectilinear *const rt : overlapIt->second){
            rt->overlapTiles.erase(tile);
        }

        // erase the payload from the floorplan structure
        this->overlapTilePayload.erase(tile);
    }

    // remove the tile from the cornerStitching structure
    cs->removeTile(tile);
}

void Floorplan::increaseTileOverlap(Tile *tile, Rectilinear *newRect){
    if(newRect->getType() == rectilinearType::PIN){
        throw CSException("FLOORPLAN_26");
    }  
    tileType increaseTileType = tile->getType();

    if(increaseTileType == tileType::BLOCK){
        // check if the tile is present in the floorplan structure
        std::unordered_map<Tile *, Rectilinear *>::iterator blockIt = this->blockTilePayload.find(tile);
        if(blockIt == this->blockTilePayload.end()){
            throw CSException("FLOORPLAN_09");
        }

        if(blockIt->second == newRect){
            throw CSException("FLOORPLAN_20");
        }

        Rectilinear *oldRect = blockIt->second;
        // erase record at floorplan system and rectilinear system
        this->blockTilePayload.erase(tile);
        oldRect->blockTiles.erase(tile);

        // change the tile's type attribute 
        tile->setType(tileType::OVERLAP);

        // refill correct information for floorplan system and rectilinear
        this->overlapTilePayload[tile] = {oldRect, newRect};
        oldRect->overlapTiles.insert(tile);
        newRect->overlapTiles.insert(tile);

    }else if(increaseTileType == tileType::OVERLAP){
        std::unordered_map<Tile *, std::vector<Rectilinear *>>::iterator overlapIt = this->overlapTilePayload.find(tile);
        if(overlapIt == this->overlapTilePayload.end()){
            throw CSException("FLOORPLAN_10");
        }

        if(std::find(overlapIt->second.begin(), overlapIt->second.end(), newRect) != overlapIt->second.end()){
            throw CSException("FLOORPLAN_20");
        }
        
        // update newRect's rectilienar structure and register newRect as tile's payload at floorplan system
        newRect->overlapTiles.insert(tile);
        this->overlapTilePayload[tile].push_back(newRect);

    }else{
        throw CSException("FLOORPLAN_11");
    }
}

void Floorplan::decreaseTileOverlap(Tile *tile, Rectilinear *removeRect){
    if(tile->getType() != tileType::OVERLAP){
        throw CSException("FLOORPLAN_12");
    }

    if(this->overlapTilePayload.find(tile) == this->overlapTilePayload.end()){
        throw CSException("FLOORPLAN_13");
    }

    std::vector<Rectilinear *> *oldPayload = &(this->overlapTilePayload[tile]);
    if(std::find(oldPayload->begin(), oldPayload->end(), removeRect) == oldPayload->end()){
        throw CSException("FLOORPLAN_14");
    }
    int oldPayloadSize = oldPayload->size();
    if(oldPayloadSize == 2){
        // ready to change tile's type to tileType::BLOCK
        Rectilinear *solePayload = (((*oldPayload)[0]) == removeRect)? ((*oldPayload)[1]) : ((*oldPayload)[0]);
        // remove tile payload from floorplan structure
        this->overlapTilePayload.erase(tile);

        // remove from rectilinear structure
        removeRect->overlapTiles.erase(tile);
        solePayload->overlapTiles.erase(tile);

        // change type of the tile
        tile->setType(tileType::BLOCK);

        solePayload->blockTiles.insert(tile);
        this->blockTilePayload[tile] = solePayload;
    }else if(oldPayloadSize > 2){
        // the tile has 2 or more rectilinear after removal, keep type as tileType::OVERLAP

        std::vector<Rectilinear *> *toChange = &(this->overlapTilePayload[tile]);
        // apply erase-remove idiom to delete removeRect from the payload from floorplan 
        toChange->erase(std::remove(toChange->begin(), toChange->end(), removeRect));

        removeRect->overlapTiles.erase(tile);

    }else{
        throw CSException("FLOORPLAN_15");
    }
}

void Floorplan::reshapeRectilinear(Rectilinear *rt){
    if((rt->blockTiles.empty()) && (rt->overlapTiles.empty())) return;
    using namespace boost::polygon::operators;
    DoughnutPolygonSet reshapePart;
    std::unordered_map<Rectangle, Tile*> rectanglesToDelete;

    for(Tile *const &blockTile : rt->blockTiles){
        Rectangle blockRectangle = blockTile->getRectangle();
        rectanglesToDelete[blockRectangle] = blockTile;
        reshapePart += blockRectangle;
    }
    
    std::vector<Rectangle> diceResult;
    dps::diceIntoRectangles(reshapePart, diceResult);
    std::unordered_set<Rectangle> rectanglesToAdd;

    // extract those rectangles that should be deleted and those that should stay
    for(Rectangle const &diceRect : diceResult){
        if(rectanglesToDelete.find(diceRect) != rectanglesToDelete.end()){
            // old Rectilinear already exist such tile
            rectanglesToDelete.erase(diceRect); 
        }else{
            // new tile for the rectilinear
            rectanglesToAdd.insert(diceRect);
        }
    }

    for(std::unordered_map<Rectangle, Tile*>::iterator it = rectanglesToDelete.begin(); it != rectanglesToDelete.end(); ++it){
        deleteTile(it->second);
    }

    for(Rectangle const &toAddRect : rectanglesToAdd){
        addBlockTile(toAddRect, rt);
    }

}

void Floorplan::growRectilinear(std::vector<DoughnutPolygon> &toGrow, Rectilinear *rect){
    using namespace boost::polygon::operators;
    DoughnutPolygonSet growPart;
    std::unordered_map<Rectangle, Tile*> rectanglesToDelete;

    // the original part of the Rectilinear
    for(Tile *const &blockTile :rect->blockTiles){
        Rectangle blockRectangle = blockTile->getRectangle();
        rectanglesToDelete[blockRectangle] = blockTile;
        growPart += blockRectangle;
    }
    // the added part
    for(DoughnutPolygon const &dps : toGrow){
        growPart += dps;
    }
    

    std::vector<Rectangle> diceResult;
    dps::diceIntoRectangles(growPart, diceResult);
    std::unordered_set<Rectangle> rectanglesToAdd;

    // extract those rectangles that should be deleted and tose that should stay
    for(Rectangle const &diceRect : diceResult){
        if(rectanglesToDelete.find(diceRect) != rectanglesToDelete.end()){
            // old Rectilinear already exist such tile
            rectanglesToDelete.erase(diceRect); 
        }else{
            // new tile for the rectilinear
            rectanglesToAdd.insert(diceRect);
        }
    }

    for(std::unordered_map<Rectangle, Tile*>::iterator it = rectanglesToDelete.begin(); it != rectanglesToDelete.end(); ++it){
        deleteTile(it->second);
    }

    for(Rectangle const &toAddRect : rectanglesToAdd){
        addBlockTile(toAddRect, rect);
    }

}

void Floorplan::shrinkRectilinear(std::vector<DoughnutPolygon> &toShrink, Rectilinear *rect){
    using namespace boost::polygon::operators;
    DoughnutPolygonSet growPart;
    std::unordered_map<Rectangle, Tile*> rectanglesToDelete;

    // the original part of the Rectilinear
    for(Tile *const &blockTile :rect->blockTiles){
        Rectangle blockRectangle = blockTile->getRectangle();
        rectanglesToDelete[blockRectangle] = blockTile;
        growPart += blockRectangle;
    }
    // the added part
    for(DoughnutPolygon const &dps : toShrink){
        growPart -= dps;
    }
    

    std::vector<Rectangle> diceResult;
    dps::diceIntoRectangles(growPart, diceResult);
    std::unordered_set<Rectangle> rectanglesToAdd;

    // extract those rectangles that should be deleted and tose that should stay
    for(Rectangle const &diceRect : diceResult){
        if(rectanglesToDelete.find(diceRect) != rectanglesToDelete.end()){
            // old Rectilinear already exist such tile
            rectanglesToDelete.erase(diceRect); 
        }else{
            // new tile for the rectilinear
            rectanglesToAdd.insert(diceRect);
        }
    }

    for(std::unordered_map<Rectangle, Tile*>::iterator it = rectanglesToDelete.begin(); it != rectanglesToDelete.end(); ++it){
        deleteTile(it->second);
    }

    for(Rectangle const &toAddRect : rectanglesToAdd){
        addBlockTile(toAddRect, rect);
    }

}

Tile *Floorplan::divideTileHorizontally(Tile *origTop, len_t newDownHeight){
    switch (origTop->getType()){
    case tileType::BLOCK:{
        Rectilinear *origTopBelongRect = this->blockTilePayload[origTop];
        Tile *newDown = cs->cutTileHorizontally(origTop, newDownHeight);
        origTopBelongRect->blockTiles.insert(newDown);
        this->blockTilePayload[newDown] = origTopBelongRect;
        return newDown;
        break;
    }
    case tileType::OVERLAP:{
        std::vector<Rectilinear *> origTopContainedRect(this->overlapTilePayload[origTop]);
        Tile *newDown = cs->cutTileHorizontally(origTop, newDownHeight);
        for(Rectilinear *const &rect : origTopContainedRect){
            rect->overlapTiles.insert(newDown);
        }
        this->overlapTilePayload[newDown] = origTopContainedRect;
        return newDown;
        break;
    }
    default:
        throw CSException("FLOORPLAN_18");
        break;
    }
}

Tile *Floorplan::divideTileVertically(Tile *origRight, len_t newLeftWidth){
    switch (origRight->getType()){
    case tileType::BLOCK:{
        Rectilinear *origTopBelongRect = this->blockTilePayload[origRight];
        Tile *newDown = cs->cutTileVertically(origRight, newLeftWidth);
        origTopBelongRect->blockTiles.insert(newDown);
        this->blockTilePayload[newDown] = origTopBelongRect;
        return newDown;
        break;
    }
    case tileType::OVERLAP:{
        std::vector<Rectilinear *> origTopContainedRect(this->overlapTilePayload[origRight]);
        Tile *newDown = cs->cutTileVertically(origRight, newLeftWidth);
        for(Rectilinear *const &rect : origTopContainedRect){
            rect->overlapTiles.insert(newDown);
        }
        this->overlapTilePayload[newDown] = origTopContainedRect;
        return newDown;
        break;
    }
    default:
        throw CSException("FLOORPLAN_19");
        break;
    }

}

double Floorplan::calculateHPWL(){
    double floorplanHPWL = 0;
    for(Connection *const &c : this->allConnections){
        floorplanHPWL += c->calculateCost();
    }
    
    return floorplanHPWL;
}

void Floorplan::displayHPWL(){
    for(std::unordered_map<Rectilinear *, std::vector<Connection *>>::iterator it = connectionMap.begin(); it != connectionMap.end(); ++it){
        Rectilinear *rt = it->first;
        if(rt->getType() == rectilinearType::PREPLACED) continue;
        Rectangle rtBB = rt->calculateBoundingBox();
        double rtbbx, rtbby;
        rec::calculateCentre(rtBB, rtbbx, rtbby);
        std::cout << it->first->getName() <<", Centre = (" << rtbbx << ", " << rtbby << "), optimal centre = " << calculateOptimalCentre(rt) << " ";
        double totalCost = 0;
        for(Connection * const &c : it->second){
            totalCost += c->calculateCost();
        }
        std::cout << "Link cost = " << totalCost << std::endl;

    }
}

Cord Floorplan::calculateOptimalCentre(Rectilinear *rect) const {
    if(rect->getType() != rectilinearType::SOFT){
        throw CSException("FLOORPLAN_27");
    }
    std::unordered_map<Rectilinear *, std::vector<Connection *>>::const_iterator it = connectionMap.find(rect);
    if(it == connectionMap.end()){
        return Cord(-1, -1);
    }

    flen_t accOptCentreX = 0, accOptCentreY = 0;
    double accWeight = 0;

    for(Connection *const &conn : it->second){
        double connWeight = conn->weight;
        flen_t accX = 0, accY = 0;
        int totalVetices = 0;
        for(Rectilinear *const &rt : conn->vertices){
            if(rt != rect){
                FCord bb;
                Rectangle bbR = rt->calculateBoundingBox();
                flen_t centreX, centreY;
                rec::calculateCentre(bbR, centreX, centreY);
                bb = FCord(centreX, centreY);

                accX += bb.x();
                accY += bb.y();
                totalVetices++;
            }
        }

        accX = (accX * connWeight) / double(totalVetices);   
        accY = (accY * connWeight) / double(totalVetices);

        accOptCentreX += accX;
        accOptCentreY += accY;
        accWeight += connWeight; 

    }    
    flen_t trueX = accOptCentreX / accWeight;
    flen_t trueY = accOptCentreY / accWeight;

    // round positive float number by ... trick
    return Cord(len_t(trueX + 0.5), len_t(trueY + 0.5));
    
}

void Floorplan::calculateAllOptimalCentre(std::unordered_map<Rectilinear *, Cord> &optCentre) const {
    // calculate all centres and cache them
    std::unordered_map<Rectilinear *, FCord>allCentre;

    // fill the optCentre with all soft Rectilinears
    std::unordered_map<Rectilinear *, FCord> tmpOptCentre;
    std::unordered_map<Rectilinear *, double> accWeight;
    for(Rectilinear *const &rt : this->softRectilinears){
        tmpOptCentre[rt] = FCord(-1, -1);
        accWeight[rt] = 0;
    }


    for(Connection *const &conn : this->allConnections){
		double connWeight = conn->weight;
        flen_t accX = 0, accY = 0;
		int totalVertices = 0;
        for(Rectilinear *const &rt : conn->vertices){
            // find the Rectilinear in all Centre Cache, if not, find and fill it
            FCord bb;
            std::unordered_map<Rectilinear *, FCord>::const_iterator it = allCentre.find(rt);
            if(it != allCentre.end()){
                bb = it->second;
            }else{
                Rectangle bbR = rt->calculateBoundingBox();
                flen_t centreX, centreY;
                rec::calculateCentre(bbR, centreX, centreY);
                bb = FCord(centreX, centreY);
                allCentre[rt] = bb;
            }
            accX += bb.x();
            accY += bb.y();
			totalVertices++;
        }
        for(Rectilinear *const &rt : conn->vertices){
            if(rt->getType() == rectilinearType::SOFT){
                FCord bb = allCentre[rt];
                flen_t newX = accX - bb.x();
				flen_t newY = accY - bb.y();
                FCord record = tmpOptCentre[rt]; 
                newX = (newX * connWeight) / (totalVertices - 1); 
                newY = (newY * connWeight) / (totalVertices - 1); 
				if(record != FCord(-1, -1)){
                    newX += record.x();
                    newY += record.y();
				}
				tmpOptCentre[rt] = FCord(newX, newY);
				accWeight[rt] += conn->weight;
            }
        }
    }

	for(Rectilinear *const &rt : this->softRectilinears){
		FCord accOptCentre = tmpOptCentre[rt];
		double divWeight = accWeight[rt];
		if(accOptCentre == FCord(-1, -1)){
			optCentre[rt] = Cord(-1, -1);
		}else{
			// divide the weight and round
			flen_t trueX = accOptCentre.x() / divWeight;
			flen_t trueY = accOptCentre.y() / divWeight;

			// round positive float number by ... trick
			optCentre[rt] = Cord(int(trueX + 0.5), int(trueY + 0.5));
		}
	}
}

void Floorplan::removePrimitiveOvelaps(bool verbose){

    /* 
        Primitive Overlap Removal STEP 0: 
        cherry pick those overlap with removal potentials, rectilinear that has residual area >= overlap area
    */
    std::map<Tile *, std::set<Rectilinear *>> tiletoRect;
    std::map<Rectilinear *, std::vector<Tile *>> recttoTile;
    for(std::unordered_map<Tile*, std::vector<Rectilinear*>>::const_iterator it = overlapTilePayload.begin(); it != overlapTilePayload.end(); ++it){
        bool tileQualify = false;
        Tile *overlapTile = it->first;
        area_t overlapArea = overlapTile->getArea();

        for(Rectilinear* const &rect : overlapTilePayload[overlapTile]){
            area_t residualArea = (rect->calculateActualArea() - rect->getLegalArea());
            Rectilinear testRect = Rectilinear(*rect);
            testRect.overlapTiles.erase(overlapTile);
            rectilinearIllegalType rectIllegalCode;
            bool legalAfterRemoval = testRect.isLegal(rectIllegalCode);
            if((residualArea >= overlapArea) && legalAfterRemoval){
                if(!tileQualify){
                    tiletoRect[overlapTile] = {rect};
                    tileQualify = true;
                }else{
                    tiletoRect[overlapTile].insert(rect);
                }
                std::map<Rectilinear *, std::vector<Tile *>>::iterator rttIt = recttoTile.find(rect);
                if(rttIt == recttoTile.end()){
                    recttoTile[rect] = {overlapTile};
                }else{
                    recttoTile[rect].push_back(overlapTile);
                }
            } 
        }
    }

    /* 
        Primitive Overlap Removal STEP 1: 
        For every Rectilinear, if there's only one available overlap to remove, do so. Iterate until no overlap solved
    */
    std::vector<Rectilinear *> allRectCandidates;
    for(std::map<Rectilinear *, std::vector<Tile *>>::const_iterator it = recttoTile.begin(); it != recttoTile.end(); it++){
        allRectCandidates.push_back(it->first);
    }

    bool newOverlapRemoved;
    do{
        newOverlapRemoved = false;
		for(int i = 0; i < allRectCandidates.size(); ++i){
			Rectilinear *checkRt = allRectCandidates[i];
			if(recttoTile[checkRt].size() == 1){
				Tile *overlapTile = recttoTile[checkRt][0];

                area_t residualArea = checkRt->calculateActualArea() - checkRt->getLegalArea();
                if(verbose) std::cout << "[PrimitiveOverlapRemoval(1/2)] " << "Remove " << *overlapTile << " with " << checkRt->getName();
                if(verbose) std::cout << " (" << residualArea << " -> " << residualArea - overlapTile->getArea() << ")" << std::endl;

                decreaseTileOverlap(overlapTile, checkRt);

                for(Rectilinear *const &rt : tiletoRect[overlapTile]){
                    if(recttoTile[rt].size() == 1){
                        // the rectilinear would have empty overlap afterwards, remove entry
                        recttoTile.erase(rt);
                        allRectCandidates.erase(std::find(allRectCandidates.begin(), allRectCandidates.end(), checkRt));
                    }else{
                        recttoTile[rt].erase(std::find(recttoTile[rt].begin(), recttoTile[rt].end(), overlapTile));
                    }
                }
                tiletoRect.erase(overlapTile);
                newOverlapRemoved = true;

			}
		}
    
    }while(newOverlapRemoved);

// debugging info
    // std::cout << "Tile to Rect debug print: " << tiletoRect.size() << std::endl;
    // for(auto it = tiletoRect.begin(); it != tiletoRect.end(); ++it){
    //     std::cout << *(it->first) << " - > ";
    //     for(Rectilinear * rt : it->second){
    //         std::cout << rt->getName() << " [" << rt->calculateActualArea() - rt->getLegalArea() << "], ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << "Rect to Tile debug pirnt" << recttoTile.size() << std::endl;
    // for(auto it = recttoTile.begin(); it != recttoTile.end(); ++it){
    //     std::cout << it->first->getName() << " - > ";
    //     for(Tile *tt : it->second){
    //         std::cout << *tt << "(" << tt->getArea() << "), ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << "Print all rect candidate: ";
    // for(Rectilinear *it : allRectCandidates){
    //     std::cout << it->getName() << ", ";
    // }
    // std::cout << std::endl;



    // If there's no candidates to remove, terminate function
    if(allRectCandidates.empty()) return;

    /* 
        Primitive Overlap Removal STEP 2: 
        use binary ILP (BILP) to solve the problem
    */   

    // Prepare BILP parameters
    int totalVarCount = 0;
    std::vector<std::pair<Rectilinear *, Tile *>> ilpVarVec;
    std::vector<double>ilpVarWeight;
    std::unordered_map<Tile *, double> tileWeightStore;
    std::vector<Rectilinear *> involvedRect;
    std::vector<std::vector<int>> sameRectVar;
    std::unordered_map<Tile *, std::vector<int>> sameTileVar;

    for(std::map<Rectilinear *, std::vector<Tile *>>::const_iterator it = recttoTile.begin(); it != recttoTile.end(); it++){
        Rectilinear *rect = it->first;
        involvedRect.push_back(rect);
        sameRectVar.push_back(std::vector<int>());

        for(Tile *const &tt : it->second){
            if(tileWeightStore.find(tt) == tileWeightStore.end()){
                // the tile wieght is never calculated;
                double tileWeight = 0;
                for(Rectilinear *const &motherRT : overlapTilePayload[tt]){
                    area_t overlapArea = 0;
                    area_t blockArea = 0;
                    for(Tile * const &t : motherRT->blockTiles){
                        blockArea += t->getArea();
                    }
                    for(Tile * const &t : motherRT->overlapTiles){
                        overlapArea += t->getArea();
                    }
                    double overlapPercentage = double(overlapArea) / double(blockArea + overlapArea);
                    tileWeight += std::pow(2, overlapPercentage);
                }
                tileWeight *= overlapTilePayload[tt].size();

                tileWeightStore[tt] = tileWeight;
                ilpVarWeight.push_back(tileWeight);

                sameTileVar[tt] = {totalVarCount};
            }else{
                ilpVarWeight.push_back(tileWeightStore[tt]);
                sameTileVar[tt].push_back(totalVarCount);
            }
            
            ilpVarVec.push_back(std::pair<Rectilinear *, Tile*>(rect, tt));
            sameRectVar.back().push_back(totalVarCount);
            totalVarCount++;
        }
    }

    int sameRectVarSize = sameRectVar.size();
    int sameTileVarSize = sameTileVar.size();

    // Initialize ILP slover:
    glp_prob *lp;
    lp = glp_create_prob();
    glp_term_out(GLP_OFF);

    // auxiliary_variables_rows: 
    // 1. Sum of overlaps solved in a Rectilinear <= Rectilinear resudiual area

    glp_add_rows(lp, sameRectVarSize + sameTileVarSize);
    for(int i = 0; i < sameRectVarSize; ++i){
        Rectilinear *rect = involvedRect[i];
        area_t upperBound = rect->calculateActualArea() - rect->getLegalArea();
        glp_set_row_bnds(lp, i+1, GLP_DB, 0.0, double(upperBound));
    }
    // 2. Same overlap should be removed (overlap Rectilinear count - 1) times
    for(int i = sameRectVarSize; i < (sameRectVarSize + sameTileVarSize); ++i){
        glp_set_row_bnds(lp, i+1, GLP_DB, 0.0, 1.0);
    }

    // variables_cloumns:
    int ilpTotalVarCount = ilpVarVec.size();
    glp_add_cols(lp, ilpTotalVarCount);
    for(int i = 0; i < ilpTotalVarCount; ++i){
        Tile *tl = ilpVarVec[i].second;
        double upperBound = double(overlapTilePayload[tl].size() - 1);
        glp_set_col_bnds(lp, i+1, GLP_DB, 0.0, upperBound);
        glp_set_col_kind(lp, i+1, GLP_IV);
    }
    // ilp opmitization target:
    glp_set_obj_dir(lp, GLP_MAX);
    for(int i = 0; i < ilpVarWeight.size(); ++i){
        glp_set_obj_coef(lp, i+1, ilpVarWeight[i]);
    }

    // constant_matrix
    int constCount = ilpVarVec.size() + 1;
    for(std::unordered_map<Tile *, std::vector<int>>::iterator it = sameTileVar.begin(); it != sameTileVar.end(); ++it ){
        constCount += it->second.size();
    }
    int ia[constCount], ja[constCount];
    double ar[constCount];
    int fillConstIdx = 1;
    for(int i = 0; i < sameRectVarSize; ++i){
        for(int j = 0; j < sameRectVar[i].size(); ++j){
            int varIdx = sameRectVar[i][j];
            Tile *targetTile = ilpVarVec[varIdx].second;
            ia[fillConstIdx] = i + 1;
            ja[fillConstIdx] = varIdx + 1;
            ar[fillConstIdx] = targetTile->getArea();

            fillConstIdx++;
        }
    }

    int iaCount = sameRectVarSize + 1;
    for(std::unordered_map<Tile *, std::vector<int>>::iterator it = sameTileVar.begin(); it != sameTileVar.end(); ++it ){
        for(int const &it : it->second){
            ia[fillConstIdx] = iaCount;
            ja[fillConstIdx] = it + 1;
            ar[fillConstIdx] = 1;
            fillConstIdx++;
        }
        iaCount++;
    }
    glp_load_matrix(lp, constCount-1, ia, ja, ar);

    // calculate:
    glp_simplex(lp, NULL);
    glp_intopt(lp, NULL);


    //use bilp answer to conduct removal
    for(int i = 1 ; i < ilpVarVec.size() + 1; ++i){
        int varAns = glp_mip_col_val(lp, i);
        // std::cout << "Var " << i << " " << varAns << std::endl;
        if(varAns != 0){
            std::pair<Rectilinear *, Tile*> extractedPair = ilpVarVec[i-1];
            Rectilinear *removeRT = extractedPair.first;
            Tile *removeTile = extractedPair.second;

            Rectilinear testRect = Rectilinear(*removeRT);
            testRect.overlapTiles.erase(removeTile);
            rectilinearIllegalType rectIllegalCode;
            bool legalAfterRemoval = testRect.isLegal(rectIllegalCode);

            if(legalAfterRemoval){
                area_t removeTileArea = removeTile->getArea();
                area_t beforeArea = removeRT->calculateActualArea() - removeRT->getLegalArea();
                if(verbose) std::cout << "[PrimitiveOverlapRemoval(2/2)] Remove " << *removeTile << " with " << removeRT->getName();
                if(verbose) std::cout << " (" << beforeArea << " -> " << beforeArea - removeTileArea << ")" << std::endl;
                decreaseTileOverlap(removeTile, removeRT);
                if(removeTile->getType() == tileType::BLOCK){
                    reshapeRectilinear(this->blockTilePayload[removeTile]);
                }
            }

        }
    }

    // clean_up
    glp_delete_prob(lp);
    
}

void Floorplan::debugPrint(){
    std::cout << "OverlapTilePayload = " << overlapTilePayload.size() << std::endl;
    for(std::unordered_map<Tile *, std::vector<Rectilinear *>>::iterator it = overlapTilePayload.begin(); it != overlapTilePayload.end(); ++it){
        std::cout << *(it->first) << " (" << it->second.size() << ")";
        for(Rectilinear *rt : it->second){
            std::cout << rt->getName() << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << "TilePayload = " << blockTilePayload.size() << std::endl;
    for(std::unordered_map<Tile *, Rectilinear *>::iterator it = blockTilePayload.begin(); it != blockTilePayload.end(); ++it){
        std::cout << *(it->first) << " -> " << it->second->getName();
        std::cout << std::endl;
    }
    
}

Rectilinear *Floorplan::checkFloorplanLegal(rectilinearIllegalType &illegalType) const {
    // check if there is overlap tile
    if(!overlapTilePayload.empty()){
        illegalType = rectilinearIllegalType::OVERLAP;
        return overlapTilePayload.begin()->second.at(0);
    }

    using namespace boost::polygon::operators;
    // check if preplacedRecilinears are all legal
    for(Rectilinear *const &rt : preplacedRectilinears){
        DoughnutPolygonSet currentShape;
        for(Tile *const &fragment : rt->blockTiles){
            currentShape += fragment->getRectangle();
        }
        std::vector<Rectangle> diceResult;
        dps::diceIntoRectangles(currentShape, diceResult);
        if (diceResult.size() == 1){
            if(diceResult[0] != rt->getGlboalPlacement()){
                illegalType = rectilinearIllegalType::PREPLACE_FAIL;
                return rt;
            }
        }else{
            illegalType = rectilinearIllegalType::PREPLACE_FAIL;
            return rt;
        }
    }

    // check if softRectilinears are all legal
    for(Rectilinear *const &rt : softRectilinears){
        if(!(rt->isLegal(illegalType))) return rt;
    }

    illegalType = rectilinearIllegalType::LEGAL;
    return nullptr;

}

// two modifications compared to visualiseLegalFloorplan()
// 1. removed error_22, change so it outputs all polygons in set
// this is so that it is easier to visualize and debug if polygons are fragmented
// 2. changed to include all overlapTiles along with blockTiles
// this is so the floorplan shape can be accurately shown at any point 
void Floorplan::visualiseRectiFloorplan(const std::string &outputFileName) const {
    using namespace boost::polygon::operators;
	std::ofstream ofs;
	ofs.open(outputFileName, std::fstream::out);
	if(!ofs.is_open()){
        // modified by ryan
        std::cerr << "file not found" << std::endl;
        return;
        // throw(CSException("CORNERSTITCHING_21"));
    } 

    ofs << "BLOCK " << mAllRectilinearCount << std::endl;
    ofs << rec::getWidth(mChipContour) << " " << rec::getHeight(mChipContour) << std::endl;

    ofs << "SOFTBLOCK " << mSoftRectilinearCount << std::endl;
    for(Rectilinear *const &rect : softRectilinears){
        DoughnutPolygonSet dps;
        if(!rect->overlapTiles.empty()){
            throw(CSException("CORNERSTITCHING_22"));
        }
        for(Tile *const &t : rect->blockTiles){
            dps += t->getRectangle();
        }


        // modified by ryan
        // remove error, change to output all polygons
        // bugfix: space between x and y
        for (DoughnutPolygon& dp: dps){

            boost::polygon::direction_1d direction = boost::polygon::winding(dp);
            ofs << rect->getName() << " " << dp.size() << std::endl;  
            
            if(direction == boost::polygon::direction_1d_enum::CLOCKWISE){
                for(auto it = dp.begin(); it != dp.end(); ++it){
                    ofs << (*it).x() << ' ' << (*it).y() << std::endl;
                }
            }else{
                std::vector<Cord> buffer;
                for(auto it = dp.begin(); it != dp.end(); ++it){
                    buffer.push_back(*it);
                }
                for(std::vector<Cord>::reverse_iterator it = buffer.rbegin(); it != buffer.rend(); ++it){
                    ofs << (*it).x() << ' ' << (*it).y() << std::endl;
                }
            }
        }
        // if(dps.size() != 1){
        //     throw(CSException("CORNERSTITCHING_23"));
        // }
        // DoughnutPolygon dp = dps[0];

        // boost::polygon::direction_1d direction = boost::polygon::winding(dp);
        // ofs << rect->getName() << " " << dp.size() << std::endl;  
        
        // if(direction == boost::polygon::direction_1d_enum::CLOCKWISE){
        //     for(auto it = dp.begin(); it != dp.end(); ++it){
        //         ofs << (*it).x() << (*it).y() << std::endl;
        //     }
        // }else{
        //     std::vector<Cord> buffer;
        //     for(auto it = dp.begin(); it != dp.end(); ++it){
        //         buffer.push_back(*it);
        //     }
        //     for(std::vector<Cord>::reverse_iterator it = buffer.rbegin(); it != buffer.rend(); ++it){
        //         ofs << (*it).x() << (*it).y() << std::endl;
        //     }
        // }
    }

    ofs << "PREPLACEDBLOCK " << mPreplacedRectilinearCount << std::endl;
    for(Rectilinear *const &rect : preplacedRectilinears){
        DoughnutPolygonSet dps;
        if(!rect->overlapTiles.empty()){
            throw(CSException("CORNERSTITCHING_22"));
        }

        // added by ryan
        // case where zero area rectilinear
        if (rect->getLegalArea() == 0){
            ofs << rect->getName() << " 4" << std::endl;  
            int x = rec::getXL(rect->getGlboalPlacement());
            int y = rec::getYL(rect->getGlboalPlacement());
            ofs << x << ' ' << y << std::endl;
            ofs << x << ' ' << y << std::endl;
            ofs << x << ' ' << y << std::endl;
            ofs << x << ' ' << y << std::endl;
            continue;
        }
        

        for(Tile *const &t : rect->blockTiles){
            dps += t->getRectangle();
        }

        // modified by ryan
        // remove error, change to output all polygons
        for (DoughnutPolygon& dp: dps){

            boost::polygon::direction_1d direction = boost::polygon::winding(dp);
            ofs << rect->getName() << " " << dp.size() << std::endl;  
            
            if(direction == boost::polygon::direction_1d_enum::CLOCKWISE){
                for(auto it = dp.begin(); it != dp.end(); ++it){
                    ofs << (*it).x() << ' ' << (*it).y() << std::endl;
                }
            }else{
                std::vector<Cord> buffer;
                for(auto it = dp.begin(); it != dp.end(); ++it){
                    buffer.push_back(*it);
                }
                for(std::vector<Cord>::reverse_iterator it = buffer.rbegin(); it != buffer.rend(); ++it){
                    ofs << (*it).x() << ' ' <<(*it).y() << std::endl;
                }
            }
        }

        // if(dps.size() != 1){
        //     throw(CSException("CORNERSTITCHING_23"));
        // }
        // DoughnutPolygon dp = dps[0];

        // boost::polygon::direction_1d direction = boost::polygon::winding(dp);
        // ofs << rect->getName() << " " << dp.size() << std::endl;  
        
        // if(direction == boost::polygon::direction_1d_enum::CLOCKWISE){
        //     for(auto it = dp.begin(); it != dp.end(); ++it){
        //         ofs << (*it).x() << (*it).y() << std::endl;
        //     }
        // }else{
        //     std::vector<Cord> buffer;
        //     for(auto it = dp.begin(); it != dp.end(); ++it){
        //         buffer.push_back(*it);
        //     }
        //     for(std::vector<Cord>::reverse_iterator it = buffer.rbegin(); it != buffer.rend(); ++it){
        //         ofs << (*it).x() << (*it).y() << std::endl;
        //     }
        // }
    }

    ofs << "CONNECTION " << mConnectionCount << std::endl;
    for(Connection* const &conn : allConnections){
        for(Rectilinear *const &rect : conn->vertices){
            ofs << rect->getName() << " ";
        }
        ofs << conn->weight << std::endl; 
    }
}

void Floorplan::visualizeTiledFloorplan(const std::string& outputFileName){

    std::ofstream ofs(outputFileName);
	if (!ofs.is_open()){
		std::cerr << "File " << outputFileName << " not opened\n";
        return;
	}

    ofs << "BLOCK " << mAllRectilinearCount << std::endl;
    ofs << rec::getWidth(mChipContour) << " " << rec::getHeight(mChipContour) << std::endl;

    for(Rectilinear* const& recti : softRectilinears){
        Rectangle bbox = recti->calculateBoundingBox();
        ofs << recti->getName() << " " << recti->getLegalArea() << " ";
        ofs << rec::getXL(bbox) << " " << rec::getYL(bbox) << " ";
        ofs << rec::getWidth(bbox) << " " << rec::getHeight(bbox) << " " << "SOFT_BLOCK" << std::endl;
        ofs << recti->blockTiles.size() << " " << recti->overlapTiles.size() << std::endl;
        for(Tile *t : recti->blockTiles){
            ofs << t->getXLow() << " " << t->getYLow() << " " << t->getWidth() << " " << t->getHeight() << " BLOCK" << std::endl;
        }
        for(Tile *t : recti->overlapTiles){
            ofs << t->getXLow() << " " << t->getYLow() << " " << t->getWidth() << " " << t->getHeight() << " OVERLAP" <<std::endl;
        }
    }

    for(Rectilinear* const& recti : preplacedRectilinears){
        Rectangle bbox = recti->calculateBoundingBox();
        ofs << recti->getName() << " " << recti->getLegalArea() << " ";
        ofs << rec::getXL(bbox) << " " << rec::getYL(bbox) << " ";
        ofs << rec::getWidth(bbox) << " " << rec::getHeight(bbox) << " " << "HARD_BLOCK" << std::endl;
        ofs << recti->blockTiles.size() << " " << recti->overlapTiles.size() << std::endl;
        for(Tile *t : recti->blockTiles){
            ofs << t->getXLow() << " " << t->getYLow() << " " << t->getWidth() << " " << t->getHeight() << " BLOCK" << std::endl;
        }
        for(Tile *t : recti->overlapTiles){
            ofs << t->getXLow() << " " << t->getYLow() << " " << t->getWidth() << " " << t->getHeight() << " OVERLAP" <<std::endl;
        }
    }

    ofs << "CONNECTION " << mConnectionCount << std::endl;
    for(Connection *const &conn : allConnections){
        for(Rectilinear *const &rect : conn->vertices){
            ofs << rect->getName() << " ";
        }
        ofs << conn->weight << std::endl; 
    }

    ofs.close();
}

void Floorplan::visualiseFloorplan(const std::string &outputFileName) const {
    using namespace boost::polygon::operators;
	std::ofstream ofs;
	ofs.open(outputFileName, std::fstream::out);
	if(!ofs.is_open()){
        throw(CSException("CORNERSTITCHING_21"));
    } 

    ofs << "CHIP " << rec::getWidth(mChipContour) << " " << rec::getHeight(mChipContour) << std::endl;

    ofs << "SOFTBLOCK " << softRectilinears.size() << std::endl;
    for(Rectilinear *const &rect : softRectilinears){
        
        ofs << rect->getName() << " " << rect->calculateActualArea() - rect->getLegalArea() << std::endl;
        Rectangle boundingBox = rect->calculateBoundingBox();
        Cord bbCords [4];
        bbCords[0] = rec::getLL(boundingBox);
        bbCords[1] = rec::getUL(boundingBox);
        bbCords[2] = rec::getUR(boundingBox);
        bbCords[3] = rec::getLR(boundingBox);
        for(int i = 0; i < 4; ++i){
            ofs << bbCords[i].x() << " " << bbCords[i].y() << std::endl;
        }

        double BBCentreX, BBCentreY;
        rec::calculateCentre(boundingBox, BBCentreX, BBCentreY); 
        Cord optCentre = calculateOptimalCentre(rect);

        ofs << int(BBCentreX + 0.5) << " " << int(BBCentreY + 0.5) << " " << optCentre.x() << " " << optCentre.y() << std::endl;

        DoughnutPolygonSet dps;

        // if(!rect->overlapTiles.empty()){
        //     throw(CSException("CORNERSTITCHING_22"));
        // }

        for(Tile *const &t : rect->blockTiles){
            dps += t->getRectangle();
        }

        for(Tile *const &t : rect->overlapTiles){
            dps += t->getRectangle();
        }

        if(dps.size() != 1){
            throw(CSException("CORNERSTITCHING_23"));
        }
        DoughnutPolygon dp = dps[0];

        boost::polygon::direction_1d direction = boost::polygon::winding(dp);
        ofs << dp.size() << std::endl;  
        
        if(direction == boost::polygon::direction_1d_enum::CLOCKWISE){
            for(auto it = dp.begin(); it != dp.end(); ++it){
                ofs << (*it).x() << " " << (*it).y() << std::endl;
            }
        }else{
            std::vector<Cord> buffer;
            for(auto it = dp.begin(); it != dp.end(); ++it){
                buffer.push_back(*it);
            }
            for(std::vector<Cord>::reverse_iterator it = buffer.rbegin(); it != buffer.rend(); ++it){
                ofs << (*it).x() << " " << (*it).y() << std::endl;
            }
        }
    }

    ofs << "PREPLACEDBLOCK " << preplacedRectilinears.size() << std::endl;
    for(Rectilinear *const &rect : preplacedRectilinears){
        DoughnutPolygonSet dps;
        // if(!rect->overlapTiles.empty()){
        //     throw(CSException("CORNERSTITCHING_22"));
        // }
        for(Tile *const &t : rect->blockTiles){
            dps += t->getRectangle();
        }

        for(Tile *const &t : rect->overlapTiles){
            dps += t->getRectangle();
        }

        if(dps.size() != 1){
            throw(CSException("CORNERSTITCHING_23"));
        }
        DoughnutPolygon dp = dps[0];

        boost::polygon::direction_1d direction = boost::polygon::winding(dp);
        ofs << rect->getName() << std::endl;
        ofs << dp.size() << std::endl;  
        
        if(direction == boost::polygon::direction_1d_enum::CLOCKWISE){
            for(auto it = dp.begin(); it != dp.end(); ++it){
                ofs << (*it).x() << " " <<  (*it).y() << std::endl;
            }
        }else{
            std::vector<Cord> buffer;
            for(auto it = dp.begin(); it != dp.end(); ++it){
                buffer.push_back(*it);
            }
            for(std::vector<Cord>::reverse_iterator it = buffer.rbegin(); it != buffer.rend(); ++it){
                ofs << (*it).x() << " " << (*it).y() << std::endl;
            }
        }
    }

    ofs << "PIN " << pinRectilinears.size() << std::endl;
    for(Rectilinear *const &rect : pinRectilinears){
        Rectangle bb = rect->calculateBoundingBox();
        Cord pinLoc = rec::getLL(bb);
        ofs << rect->getName() << " " << pinLoc.x() << " " << pinLoc.y() << std::endl;
    }

    ofs << "CONNECTION " << allConnections.size() << std::endl;
    for(Connection *const &conn : allConnections){
        ofs << conn->vertices.size() << " ";
        for(Rectilinear *const &rect : conn->vertices){
            ofs << rect->getName() << " ";
        }
        ofs << conn->weight << std::endl; 
    }

    ofs.close();
}

void Floorplan::moveTileParent(Tile* tile, Rectilinear* fromRect, Rectilinear* toRect){
    std::unordered_map<Tile *, Rectilinear *>::iterator blockIt= this->blockTilePayload.find(tile);
    if(blockIt == this->blockTilePayload.end() || blockIt->second != fromRect){
        std::cerr << "Error: in moveTileParent, tile not found in original parent\n";
    }

    // remove from fromRect's blockTiles
    fromRect->blockTiles.erase(tile);
    // change tile's payload to new rectilinear parent
    this->blockTilePayload[tile] = toRect;
    // add to toRect's blockTiles
    toRect->blockTiles.insert(tile);
}

// added by ryan
Tile* Floorplan::generalSplitTile(Tile* tile, Rectangle newArea){
    std::vector<Tile*> newNeighbors;
    Tile* centerTile;
    
    switch (tile->getType()){
        case tileType::BLOCK:{
            Rectilinear *originalBelongRecti = this->blockTilePayload[tile];
            centerTile = this->cs->generalSplitTile(tile, newArea, newNeighbors);

            for (Tile* newTile: newNeighbors){
                originalBelongRecti->blockTiles.insert(newTile);
                this->blockTilePayload[newTile] = originalBelongRecti;
            }

            return centerTile;
        }
        case tileType::OVERLAP:{
            std::vector<Rectilinear *> orignalBelongRectis(this->overlapTilePayload[tile]);
            centerTile = this->cs->generalSplitTile(tile, newArea, newNeighbors);

            for (Tile* newTile: newNeighbors){
                for (Rectilinear* const& rect: orignalBelongRectis){
                    rect->overlapTiles.insert(newTile);
                }
                this->overlapTilePayload[newTile] = orignalBelongRectis;
            }
            
            return centerTile;
            break;
        } 
        default:{
            std::cerr << "ERROR: generalsplittile used on unknown block\n";
            return NULL;
            break;
        }
    }
}

size_t std::hash<Floorplan>::operator()(const Floorplan &key) const {
    return std::hash<Rectangle>()(key.getChipContour()) ^ std::hash<int>()(key.getAllRectilinearCount()) ^ std::hash<int>()(key.getConnectionCount());
}
