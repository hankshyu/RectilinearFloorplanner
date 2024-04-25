#include "globmodule.h"


GlobalModule::GlobalModule() {
    // Empty
}

GlobalModule::~GlobalModule() {
    for ( int i = 0; i < connections.size(); i++ ) {
        delete connections[i];
    }
}

void GlobalModule::addConnection(const std::vector<GlobalModule *> &in_modules, double in_value) {
    gb::Connection *nc = new gb::Connection;
    nc->modules = in_modules;
    nc->value = in_value;
    this->connections.push_back(nc);
}

void GlobalModule::updateCord(int DieWidth, int DieHeight) {
    if ( this->fixed ) {
        return;
    }

    this->x = centerX - width / 2.;
    this->y = centerY - height / 2.;

    if ( this->x < 0 ) {
        this->x = 0;
    }
    else if ( this->x > DieWidth - this->width ) {
        this->x = DieWidth - this->width;
    }
    if ( this->y < 0 ) {
        this->y = 0;
    }
    else if ( this->y > DieHeight - this->height ) {
        this->y = DieHeight - this->height;
    }

    // std::cout << this->name << " " << this->width << " , " << this->height << std::endl;
    // std::cout << this->centerX << " , " << this->centerY << std::endl;
}

void GlobalModule::setWidth(double width) {
    // Empty
}
void GlobalModule::setHeight(double height) {
    // Empty
}
void GlobalModule::setMaxAspectRatio(double aspect_ratio) {
    // Empty
}
void GlobalModule::growWidth(double width_to_grow) {
    // Empty
}
void GlobalModule::growHeight(double height_to_grow) {
    // Empty
}
void GlobalModule::setArea(double area) {
    // Empty
}
void GlobalModule::scaleSize(double ratio) {
    // Empty
}


SoftModule::SoftModule() {
    // Empty
}

SoftModule::SoftModule(std::string name, double centerX, double centerY, int width, int height, int area) {
    this->name = name;
    this->centerX = centerX;
    this->centerY = centerY;
    this->width = width;
    this->height = height;
    this->area = area;
    this->fixed = false;
    this->maxAspectRatio = 2.;
}

SoftModule::SoftModule(std::string name, double centerX, double centerY, int area) {
    this->name = name;
    this->centerX = centerX;
    this->centerY = centerY;
    this->area = area;
    this->width = std::ceil(std::sqrt(( double ) area));
    this->height = std::ceil(std::sqrt(( double ) area));
    this->fixed = false;
    this->maxAspectRatio = 2.;
}

SoftModule::~SoftModule() {
    // Empty
}

void SoftModule::setWidth(double width) {
    double maxAR = this->maxAspectRatio;
    this->width = width;
    this->height = this->currentArea / this->width;
    double aspectRatio = this->height / this->width;
    if ( aspectRatio > maxAR ) {
        this->width = std::sqrt(this->currentArea / maxAR);
        this->height = this->currentArea / this->width;
    }
    else if ( aspectRatio < 1. / maxAR ) {
        this->width = std::sqrt(this->currentArea * maxAR);
        this->height = this->currentArea / this->width;
    }
}

void SoftModule::setHeight(double height) {
    double maxAR = this->maxAspectRatio;
    this->height = height;
    this->width = this->currentArea / this->height;
    double aspectRatio = this->height / this->width;
    if ( aspectRatio > maxAR ) {
        this->width = std::sqrt(this->currentArea / maxAR);
        this->height = this->currentArea / this->width;
    }
    else if ( aspectRatio < 1. / maxAR ) {
        this->width = std::sqrt(this->currentArea * maxAR);
        this->height = this->currentArea / this->width;
    }
}

void SoftModule::setMaxAspectRatio(double aspect_ratio) {
    this->maxAspectRatio = aspect_ratio;
}

void SoftModule::growWidth(double width_to_grow) {
    double maxAR = this->maxAspectRatio;
    this->width += width_to_grow;
    this->currentArea = this->width * this->height;
    double aspectRatio = this->height / this->width;
    if ( aspectRatio < 1. / maxAR ) {
        this->width = std::sqrt(this->currentArea * maxAR);
        this->height = this->currentArea / this->width;
    }
}

void SoftModule::growHeight(double height_to_grow) {
    double maxAR = this->maxAspectRatio;
    this->height += height_to_grow;
    this->currentArea = this->width * this->height;
    double aspectRatio = this->height / this->width;
    if ( aspectRatio > maxAR ) {
        this->width = std::sqrt(this->currentArea / maxAR);
        this->height = this->currentArea / this->width;
    }
}

void SoftModule::setArea(double area) {
    this->currentArea = area;
    this->width = std::sqrt(area / ( this->height / this->width ));
    this->height = area / this->width;
}

void SoftModule::scaleSize(double ratio) {
    this->currentArea = this->area * ratio;
    this->width = std::sqrt(this->currentArea / ( this->height / this->width ));
    this->height = this->currentArea / this->width;
}


FixedModule::FixedModule() {
    // Empty
}

FixedModule::FixedModule(std::string name, int x, int y, int width, int height, int area) {
    this->name = name;
    this->x = ( double ) x;
    this->y = ( double ) y;
    this->width = width;
    this->height = height;
    this->area = area;
    this->centerX = x + width / 2.;
    this->centerY = y + height / 2.;
    this->fixed = true;
    // std::cout << "Fixed Module " << name << " " << x << " , " << y << " , " << width << " , " << height << std::endl;
}

FixedModule::~FixedModule() {
    // Empty
}
