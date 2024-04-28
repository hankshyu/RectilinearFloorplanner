#ifndef __CSEXCEPTION_H__
#define __CSEXCEPTION_H__

#include <exception>
#include <unordered_map>
#include <string>

#define CSEX_EMPTY "CSEX: Empty Excpeiton"

/* Look Up Table for all exceptin messages */
inline std::unordered_map<std::string, const char*> CSEXCEPTION_LUT = {
    {"TILE_01", "operator <<: tileType out of Range"},

    {"GLOBALPHASEADAPTER_01", "globalPhaseAdapter(): file stream not open"},
    {"GLOBALPHASEADAPTER_02", "realGlobalResult(): Module not marked as SOFT or FIXED"},
    
    {"GLOBALRESULT_01", "GlobalResult(std::string globalResultFile): file stream not open"},
    {"GLOBALRESULT_02", "GlobalResult(const std::ifstream &ifs): file stream not open"},
    {"GLOBALRESULT_03", "readGlobalResult(std::ifstream &ifs): block inital placement is out of chip contour even after adjustments"},
    {"GLOBALRESULT_04", "readGlobalResult(std::ifstream &ifs): read connectin with <1 vertices"},

    {"CORNERSTITCHING_01", "findPoint(): function's input key, the finding target, is out of canvas"},
    {"CORNERSTITCHING_02", "searchArea(): function's input box, the searching area, is out of canvas"},
    {"CORNERSTITCHING_03", "enumerateDirectArea(): function's input box, the searching area, is out of canvas"},
    {"CORNERSTITCHING_04", "CornerStitching(): input chipWidth or chipHeight should > 0"},
    {"CORNERSTITCHING_05", "visualiseCornerStitching(): outputFileName invalid, cannot open file"},
    {"CORNERSTITCHING_06", "insertTile(): input tile prototype is out of canvas"},
    {"CORNERSTITCHING_07", "insertTile(): input tile's position already exist another tile on canvas"},
    {"CORNERSTITCHING_08", "cutTileHorizontally(): there is no enough height for newDownHeight to cut from origTop (victim tile)"},
    {"CORNERSTITCHING_09", "cutTileVertically(): there is no enough width for newLeftHeight to cut from origRight (victim tile)"},
    {"CORNERSTITCHING_10", "mergeTilesHorizontally(): input mergeUp is not above input mergeDown"},
    {"CORNERSTITCHING_11", "mergeTilesHorizontally(): two input tiles (mergeUp and mergeDown) are not mergable"},
    {"CORNERSTITCHING_12", "insertTile(): input tile's type shall not be tileType::BLANK, blank tile is prohibited from insertion"},
    {"CORNERSTITCHING_13", "mergeTilesVertically(): input mergeLeft is not at the left of input mergeRight"},
    {"CORNERSTITCHING_14", "mergeTilesVertically(): two input tiles (mergeLeft and mergeRight) are not mergable"},
    {"CORNERSTITCHING_15", "cutTileVertically(): victim tile"},
    {"CORNERSTITCHING_17", "removeTile(Tile *tile): input tile is not in the cornerStitching system"},
    {"CORNERSTITCHING_18", "removeTile(Tile *tile): cornerStitching exist a tile that has same LowerLeft Cord with input Tile* Tile"},
    {"CORNERSTITCHING_19", "findLineTile(...): the line is out of canvas"},
    {"CORNERSTITCHING_20", "findLineTileHorizontalPositive(...): the input Tile *initTile is nullptr"},
    {"CORNERSTITCHING_21", "findLineTileHorizontalNegative(...): the input Tile *initTile is nullptr"},
    {"CORNERSTITCHING_22", "findLineTileVerticalPositive(...): the input Tile *initTile is nullptr"},
    {"CORNERSTITCHING_23", "findLineTileVerticalNegative(...): the input Tile *initTile is nullptr"},

    {"RECTILINEAR_01", "calculateBoundingBox(): there exist no tile in rectilinear"},
    {"RECTILINEAR_02", "acquireWinding(): there exist no tile in rectilinear"},
    {"RECTILINEAR_03", "acquireWinding(): the rectilinear is not in one shape"},
    {"RECTILINEAR_04", "acquireWinding(): there exist hole in the rectilinear structure"},
    {"RECTILINEAR_05", "operator << (std::ostream &os, const rectilinearType &t): rectilinearType does not exist"},
    {"RECTILINEAR_06", "operator << (std::ostream &os, const rectilinearIllegalType &t): rectilinearIllegalType does not exist"},
    {"RECTILINEAR_07", "operator << (std::ostream &os, const windingDirection &t): windingDirection does not exist"},


    {"CONNECTION_01", "calculateCost(): Connection has < 2 vertices"},

    {"FLOORPLAN_01", "Floorplan(GlobalResult gr): undefined gr.type"},
    {"FLOORPLAN_02", "addBlockTile(const Rectangle &tilePosition, Rectilinear *rt): input tilePosition is not within the chip"},
    {"FLOORPLAN_03", "addBlockTile(const Rectangle &tilePosition, Rectilinear *rt): input Rectilinear *rt is not present in floorplan data structure"},
    {"FLOORPLAN_04", "addOverlapTile(...): input tilePosition is not within the chip"},
    {"FLOORPLAN_05", "addOverlapTile(...): payload containes some Rectilinear* that is not registered in the floorplan system"},
    {"FLOORPLAN_06", "deleteTile(Tile *tile): tile type should be tileType::BLOCK or tileType::OVERLAP"},
    {"FLOORPLAN_07", "deleteTile(Tile *tile): tile type is tile::BLOCK but not entry found in floorplan->blockTilePayload"},
    {"FLOORPLAN_08", "deleteTile(Tile *tile): tile type is tile::OVERLAP but not entry found in floorplan->overlapTilePayload"},
    {"FLOORPLAN_09", "increaseTileOverlap(Tile *tile, Rectilinear *newRect): tile has tileType::BLOCK but not logged in blocTilePayload, tile is unregistered"},
    {"FLOORPLAN_10", "increaseTileOverlap(Tile *tile, Rectilinear *newRect): tile has tileType::OVERLAP but not logged in overlapTilePayload, tile is unregistered"},
    {"FLOORPLAN_11", "increaseTileOverlap(Tile *tile, Rectilinear *newRect): tile's type is neither tileType::BLOCK nor tileType::OVERLAP"},
    {"FLOORPLAN_12", "decreaseTileOverlap(Tile *tile, Rectilinear *removeRect): tile's type is not tileType::OVERLAP"},
    {"FLOORPLAN_13", "decreaseTileOverlap(Tile *tile, Rectilinear *removeRect): tile has tileType::OVERLAP but not logged in overlapTilePayload, tile is unregistered"},
    {"FLOORPLAN_14", "decreaseTileOverlap(Tile *tile, Rectilinear *removeRect): the tile's payload (overlapTilePayload[tile]) does not include the removeal target, removeRect"},
    {"FLOORPLAN_15", "decreaseTileOverlap(Tile *tile, Rectilinear *removeRect): tile's type is tileType::OVERLAP but the payload in floorplan system has entry < 2"},
    {"FLOORPLAN_16", "placeRectilinear(...): input placement is not within the chip"},
    {"FLOORPLAN_17", "placeRectilinear(...): return tiles from cornerStitching.enumerateDirectArea() tile.type could only be tileType::BLOCK or tileType::OVERLAP"},
    {"FLOORPLAN_18", "divideTileHorizontally(Tile *origTop, len_t newDownHeight): Tile origTop cannot be tileType::BLANK type, only tileType::BLOCK or tileType::OVERLAP is accepted"},
    {"FLOORPLAN_19", "divideTileVertically(Tile *origRight, len_t newLeftWidth): Tile origRight cannot be tileType::BLANK type, only tileType::BLOCK or tileType::OVERLAP is accepted"},
    {"FLOORPLAN_20", "increaseTileOverlap(Tile *tile, Rectilinear *newRect): tile payload already contained such Rectilinear"},
    {"FLOORPLAN_21", "visualiseFloorplan(...): outputFileName invalid, cannot open file"},
    {"FLOORPLAN_22", "visualiseFloorplan(...): some Rectilinear in the system contains overlap tiles, cannot output"},
    {"FLOORPLAN_23", "visualiseFloorplan(...): some Rectilinear is not in one piece"},
    {"FLOORPLAN_24", "addBlockTile(...): rectilinearType::PIN cannot perform block tile adding"},
    {"FLOORPLAN_25", "addOverlapTile(...): rectilinearType::PIN cannot perform block tile adding"},
    {"FLOORPLAN_26", "increaseTileOverlap(Tile *tile, Rectilinear *newRect): newRect cannot be rectilinearType::PIN"},
    {"FLOORPLAN_27", "calculateOptimalCentre(Rectilinear *rect): input Rectilinear *rect is not rectilinear::SOFT type"},

    {"LEGALRESULT_01", "readLegalResult(std::string legalResultFile): file stream not open"},
    {"LEGALRESULT_02", "readLegalResult(const std::ifstream &ifs): file stream not open"},
    {"LEGALRESULT_03", "readLegalResult(const std::ifstream &ifs): read soft block that has <4 corners"},
    {"LEGALRESULT_04", "readLegalResult(const std::ifstream &ifs): read fixed block that has <4 corners"},
    {"LEGALRESULT_05", "readLegalResult(const std::ifstream &ifs): read connection with <1 vetices"},

    {"LINE_01", "Line(Cord low, Cord height): low and height cannot be the same coordinate"},
    {"LINE_02", "Line(Cord low, Cord height): low and height must be in horizontal/veritcal line"},
    {"LINE_03", "setLow(Cord newLow): low and height cannot be the same coordinate"},
    {"LINE_04", "setLow(Cord newLow): low and height must be in horizontal/veritcal line"},
    {"LINE_05", "setHigh(Cord newHigh): low and height cannot be the same coordinate"},
    {"LINE_06", "setHigh(Cord newHigh): low and height must be in horizontal/veritcal line"},


    {"LINETILE_01", "LineTile(Line line, Tile tile): tile is nullptr, cannot operate"},
    {"LINETILE_02", "LineTile(Line line, Tile tile): HORIZONTAL mode, line's high x not fit in tile"},
    {"LINETILE_03", "LineTile(Line line, Tile tile): HORIZONTAL mode, line's low x not fit in tile"},
    {"LINETILE_04", "LineTile(Line line, Tile tile): HORIZONTAL mode, line's y coordinate over tile's y range"},
    {"LINETILE_05", "LineTile(Line line, Tile tile): HORIZONTAL mode, line's y coordinate under tile's y range"},
    {"LINETILE_06", "LineTile(Line line, Tile tile): VERTICAL mode, line's high y not fit in tile"},
    {"LINETILE_07", "LineTile(Line line, Tile tile): VERTICAL mode, line's low y not fit in tile"},
    {"LINETILE_08", "LineTile(Line line, Tile tile): VERTICAL mode, line's x coordinate over tile's x range"},
    {"LINETILE_09", "LineTile(Line line, Tile tile): VERTICAL mode, line's x coordinate under tile's x range"},

    {"LEGALISEENGINE_01", "legalise(): the growing direction is not within direction2D::UP, LEFT, DOWN, RIGHT"},

    {"DOUGHNUTPOLYGONSET_01", "operator <<: doughnutPolygonSetIllegalType &t is not in switch type"},
    {"DOUGHNUTPOLYGONSET_02", "calculateBoundingBox(...): the input dpSet is empty"},

    {"REFINEENGINE_01", "refineBytrimming(...): input direction should be either NORTH, SOUTH, EAST, WEST, others are porhibited"},
    {"REFINEENGINE_02", "refineByTrimming(...): rectTowardSector sould be within sector::ONE ~ sector::EIGHT"},
    {"REFINEENGINE_03", "refineByGrowing(...): rectTowardSector sould be within sector::ONE ~ sector::EIGHT"},
    {"REFINEENGINE_04", "trialGrow(...): input growDirection should be either NORTH, SOUTH, EAST, WEST, others are prohibited"},

};

class CSException : public std::exception{
private:
    std::string m_Exception_MSG;

public :
    CSException() = delete;
    CSException(std::string excpetion_msg);
    const char* what(); 
};

#endif // __CSEXCEPTION_H__