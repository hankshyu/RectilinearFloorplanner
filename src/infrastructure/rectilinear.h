#ifndef __RECTILINEAR_H__
#define __RECTILINEAR_H__

#include <string.h>
#include <unordered_set>

#include "rectangle.h"
#include "tile.h"

enum class rectilinearType{
    EMPTY, SOFT, PREPLACED, PIN
};

enum class rectilinearIllegalType{
    LEGAL, OVERLAP, AREA, ASPECT_RATIO, UTILIZATION, HOLE, TWO_SHAPE, PREPLACE_FAIL, INNER_WIDTH
};

class Rectilinear{
private:
    int mId;
    std::string mName;
    rectilinearType mType;
    Rectangle mGlobalPlacement;

    area_t mLegalArea;

    double mAspectRatioMin;
    double mAspectRatioMax;

    double mUtilizationMin;

public: 

    std::unordered_set<Tile *> blockTiles;
    std::unordered_set<Tile *> overlapTiles;

    Rectilinear();
    Rectilinear(int id, std::string name, rectilinearType type, Rectangle globalPlacement,
                area_t legalArea, double aspectRatioMin, double aspectRatioMax, double utilizationMin);
    Rectilinear(const Rectilinear &other);

    Rectilinear &operator = (const Rectilinear &other);
    bool operator == (const Rectilinear &other) const;
    friend std::ostream &operator << (std::ostream &os, const Rectilinear &recti);

    int getId() const;
    std::string getName() const;
    rectilinearType getType() const;
    Rectangle getGlboalPlacement() const;
    area_t getLegalArea() const;
    double getAspectRatioMin() const;
    double getAspectRatioMax() const;
    double getUtilizationMin() const;


    void setId(int id);
    void setName(std::string name);
    void setType(rectilinearType type);
    void setGlobalPlacement(const Rectangle &globalPlacement);
    void setLegalArea(area_t legalArea);
    void setAspectRatioMin(double aspectRatioMin);
    void setAspectRatioMax(double aspectRatioMax);
    void setUtilizationMin(double utilizationMin);
    

    Rectangle calculateBoundingBox() const;
    area_t calculateActualArea() const;
    area_t calculateResidualArea() const;
    double calculateUtilization() const;

    // check if rectilinear contains enough area (larger than mLegalArea), return false if violated
    bool isLegalEnoughArea() const;

    // check if the aspect ratio of rectilinear is within mAspectRatioMin ~ mAspectRatioMax, return false if violated
    bool isLegalAspectRatio() const;

    // check if the utilization (acutal area within the bounding box) is greater than mUtilizationMin, return false if violated
    bool isLegalUtilization() const;

    // check if any of the tiles overlap each other, return false if violated
    bool isLegalNoOverlap() const;

    // check if rectilinear contains internal hole (become doughnut shape), return false if violated
    bool isLegalNoHole() const;

    // check if rectilinear is disconnected, return false if violated
    bool isLegalOneShape() const;
    
    // check if violate minimum inner width
    bool isLegalInnerWidth() const;

    // use all legal checking methods, if any violated, return false an error code passed through illegalCode
    bool isLegal(rectilinearIllegalType &illegalCode) const;

    // acquire the windings of rectiinear, may choose winding direction, points pass through vector "winding"
    void acquireWinding(std::vector<Cord> &winding,  direction1D wd) const;
    
};
namespace std{
    template<>
    struct hash<Rectilinear>{
        size_t operator()(const Rectilinear &key) const;
    };
}
std::ostream &operator << (std::ostream &os, const Rectilinear &t);
std::ostream &operator << (std::ostream &os, const rectilinearType &t);
std::ostream &operator << (std::ostream &os, const rectilinearIllegalType &t);

#endif // __RECTILINEAR_H__