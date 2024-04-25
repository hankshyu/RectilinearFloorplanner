#ifndef __GLOBMODULE_H__
#define __GLOBMODULE_H__

#include <string>
#include <vector>
#include <iostream>
#include <cmath>

class GlobalModule;
namespace gb{
    class Connection {
    public:
        std::vector<GlobalModule *> modules;
        double value;
    };
};

struct ConnectionInfo {
    std::vector<std::string> modules;
    std::vector<GlobalModule *> modulePtrs;
    int value;
    ConnectionInfo(const std::vector<std::string> &modules, int value) {
        this->modules = modules;
        this->value = value;
    }
};

class GlobalModule {
public:
    std::string name;
    double centerX, centerY;
    double x, y;
    int area;
    double currentArea;
    double width, height;
    double maxAspectRatio;
    bool fixed;
    std::vector<gb::Connection *> connections;

    // Constructor
    GlobalModule();
    ~GlobalModule();

    // Member function
    void addConnection(const std::vector<GlobalModule *> &in_modules, double in_value);
    void updateCord(int DieWidth, int DieHeight);
    virtual void setWidth(double width);
    virtual void setHeight(double height);
    virtual void setMaxAspectRatio(double aspect_ratio);
    virtual void growWidth(double width_to_grow);
    virtual void growHeight(double height_to_grow);
    virtual void setArea(double area);
    virtual void scaleSize(double ratio);
};

class SoftModule : public GlobalModule {
public:
    // Constructor
    SoftModule();
    // for soft modules with specified width and height
    SoftModule(std::string name, double centerX, double centerY, int width, int height, int area);
    // for general soft modules
    SoftModule(std::string name, double centerX, double centerY, int area);
    ~SoftModule();

    // Member function
    void setWidth(double width) override;
    void setHeight(double height) override;
    void setMaxAspectRatio(double aspect_ratio) override;
    void growWidth(double width_to_grow) override;
    void growHeight(double height_to_grow) override;
    void setArea(double area) override;
    void scaleSize(double ratio) override;
};

class FixedModule : public GlobalModule {
public:
    // Constructor
    FixedModule();
    // for fixed modules
    FixedModule(std::string name, int x, int y, int width, int height, int area);
    ~FixedModule();
};


#endif