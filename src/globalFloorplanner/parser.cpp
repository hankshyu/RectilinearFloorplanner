#include "parser.h"


Parser::Parser() {
    this->softModuleNum = 0;
    this->fixedModuleNum = 0;
    this->moduleNum = 0;
    this->configExists = false;
    this->punishment = -1;
    this->max_aspect_ratio = -1;
}

Parser::~Parser() {
    for ( int i = 0; i < this->modules.size(); ++i ) {
        if ( this->modules[i] != nullptr ) {
            delete this->modules[i];
            this->modules[i] = nullptr;
        }
    }
    for ( int i = 0; i < this->connectionList.size(); ++i ) {
        if ( this->connectionList[i] != nullptr ) {
            delete this->connectionList[i];
            this->connectionList[i] = nullptr;
        }
    }
}

bool Parser::parseInput(std::string file_name,double globalAspectRatioMax, double punishmentArg, double maxAspectRatioRateArg) {
    std::ifstream istream(file_name);
    std::istringstream ss;
    std::string line;
    if ( istream.fail() ) {
        return false;
    }
    std::string s, ma, mb;
    int area, x, y, w, h, value;
    std::unordered_multiset<int> used_area;

    std::getline(istream, line);
    ss.str(line);
    ss >> s >> this->DieWidth >> this->DieHeight;

    // Read Soft Modules
    std::getline(istream, line);
    ss.clear();
    ss.str(line);
    ss >> s >> this->softModuleNum;

    for ( int i = 0; i < this->softModuleNum; i++ ) {
        std::getline(istream, line);
        ss.clear();
        ss.str(line);
        ss >> s >> area;
        GlobalModule *mod = new SoftModule(s, this->DieWidth / 2., this->DieHeight / 2., area);

        if ( used_area.count(area) > 0 ) {
            bool neg = used_area.count(area) % 2;
            int stretch = ( neg ) ? -( used_area.count(area) / 2 + 1 ) : used_area.count(area) / 2 + 1;
            int modifiedWidth = ( int ) std::ceil(std::sqrt(( double ) area)) + stretch;
            int modifiedHeight = ( int ) std::ceil(( double ) area / modifiedWidth);
            mod->width = modifiedWidth;
            mod->height = modifiedHeight;
            // std::cout << "modifiedWidth: " << modifiedWidth << " modifiedHeight: " << modifiedHeight << std::endl;
        }
        used_area.insert(area);

        this->modules.push_back(mod);
        // std::cout << "Reading Soft Module " << s << "..." << std::endl;
    }

    // Read Fixed Modules
    std::getline(istream, line);
    ss.clear();
    ss.str(line);
    ss >> s >> this->fixedModuleNum;

    for ( int i = 0; i < fixedModuleNum; i++ ) {
        std::getline(istream, line);
        ss.clear();
        ss.str(line);
        ss >> s >> x >> y >> w >> h;
        GlobalModule *mod = new FixedModule(s, x, y, w, h, w * h);
        this->modules.push_back(mod);
        // std::cout << "Reading Fixed Module " << s << "..." << std::endl;
    }

    this->moduleNum = this->softModuleNum + this->fixedModuleNum;

    std::getline(istream, line);
    ss.clear();
    ss.str(line);
    ss >> s >> this->connectionNum;

    for ( int i = 0; i < connectionNum; i++ ) {
        std::string mod;
        std::getline(istream, line);
        ss.clear();
        std::vector<std::string> mods;
        ss.str(line);

        while ( ss >> mod ) {
            mods.push_back(mod);
        }

        value = std::stoi(mods.back());
        mods.pop_back();

        ConnectionInfo *conn = new ConnectionInfo(mods, value);
        this->connectionList.push_back(conn);
        // std::cout << "Reading Connection " << mods[0] << "<->" << mods[1] << " : " << value << std::endl;
        // std::cout << "Reading Connection ";
        // for ( std::string mod : mods ) {
        //     std::cout << mod << " ";
        // }
        // std::cout << " : " << value << std::endl;
    }

    for ( int i = 0; i < connectionNum; i++ ) {
        ConnectionInfo *conn = this->connectionList[i];

        std::vector<GlobalModule *> connectedModules;

        for ( std::string &modName : conn->modules ) {
            for ( int j = 0; j < this->modules.size(); j++ ) {
                if ( this->modules[j]->name == modName ) {
                    connectedModules.push_back(this->modules[j]);
                }
            }
        }

        conn->modulePtrs = connectedModules;

        for ( int j = 0; j < connectedModules.size(); ++j ) {
            std::vector<GlobalModule *> connModules;
            connModules = connectedModules;
            connModules.erase(connModules.begin() + j);
            connectedModules[j]->addConnection(connModules, conn->value);
        }
    }

    istream.close();

    this->punishment = punishmentArg;
    if((maxAspectRatioRateArg <= 0) || (maxAspectRatioRateArg > 1)){
        std::cout << "[GlobalSolver] ERROR: maxAspectRatioRateArg shoudl >0 and <= 1.\n";
        abort();
    }
    this->max_aspect_ratio = maxAspectRatioRateArg * globalAspectRatioMax;

    return true;
}


int Parser::getDieWidth() {
    return this->DieWidth;
}

int Parser::getDieHeight() {
    return this->DieHeight;
}

int Parser::getSoftModuleNum() {
    return this->softModuleNum;
}

int Parser::getFixedModuleNum() {
    return this->fixedModuleNum;
}

int Parser::getModuleNum() {
    return this->moduleNum;
}

int Parser::getConnectionNum() {
    return this->connectionNum;
}

GlobalModule *Parser::getModule(int index) {
    return this->modules[index];
}

ConnectionInfo *Parser::getConnection(int index) {
    ConnectionInfo *newConn = new ConnectionInfo(*( this->connectionList[index] ));
    return newConn;
}


// change by Hank, will not use this funciton anymore
// ============================
//      Parse Config File
// ============================

// bool Parser::parseConfig(std::string file_name) {
//     std::ifstream istream(file_name);
//     std::istringstream ss;
//     std::string line, header;
//     if ( istream.fail() ) {
//         return false;
//     }

//     while ( std::getline(istream, line) ) {
//         ss.clear();
//         ss.str(line);
//         ss >> header;

//         if ( header == "punishment" ) {
//             std::string p;
//             ss >> p;
//             if ( this->punishment == "" ) {
//                 this->punishment = p;
//             }
//             // std::cout << "punishment " << this->punishment << std::endl;
//         }
//         else if ( header == "max_aspect_ratio" ) {
//             double mar;
//             ss >> mar;
//             if ( this->max_aspect_ratio == -1 ) {
//                 this->max_aspect_ratio = mar;
//             }
//             // std::cout << "max_aspect_ratio " << this->max_aspect_ratio << std::endl;
//         }
//         else if ( header == "lr" ) {
//             ss >> this->lr;
//             // std::cout << "lr " << this->lr << std::endl;
//         }
//         else if ( header == "inflation" ) {
//             ss >> this->inflationRatio;
//             std::string dump;
//             ss >> dump;
//             this->dumpInflation = ( dump == "--dump" );
//         }
//         else if ( header == "extreme_push_scale" ) {
//             ss >> this->extremePushScale;
//         }
//         else if ( header == "shape_constraint" ) {
//             int num;
//             ss >> num;
//             while ( num-- > 0 ) {
//                 std::getline(istream, line);
//                 ss.clear();
//                 ss.str(line);
//                 std::string mod;
//                 this->ShapeConstraintMods.push_back(std::vector<std::string>());
//                 while ( ss >> mod ) {
//                     this->ShapeConstraintMods.back().push_back(mod);
//                 }
//             }
//             // for ( int i = 0; i < ShapeConstraintMods.size(); ++i ) {
//             //     for ( int j = 0; j < ShapeConstraintMods[i].size(); ++j ) {
//             //         std::cout << ShapeConstraintMods[i][j] << " ";
//             //     }
//             //     std::cout << std::endl;
//             // }
//         }
//         else {
//             std::cout << "[GlobalSolver] ERROR: Invalid syntax in config file." << std::endl;
//             return false;
//         }
//     }

//     istream.close();
//     this->configExists = true;
//     return true;
// }

double Parser::getPunishment() {
    return this->punishment;
}

double Parser::getMaxAspectRatio() {
    return this->max_aspect_ratio;
}

double Parser::getLearnRate() {
    if ( this->configExists ) {
        return this->lr;
    }
    return -1;
}

double Parser::getInflationRatio() {
    if ( this->configExists ) {
        return this->inflationRatio;
    }
    return -1;
}

bool Parser::getDumpInflation() {
    if ( this->configExists ) {
        return this->dumpInflation;
    }
    return false;
}

double Parser::getExtremePushScale() {
    if ( this->configExists ) {
        return this->extremePushScale;
    }
    return -1;
}

std::vector< std::vector<std::string> > Parser::getShapeConstraints() {
    if ( this->configExists ) {
        return this->ShapeConstraintMods;
    }
    else {
        return std::vector< std::vector<std::string> >();
    }
}
