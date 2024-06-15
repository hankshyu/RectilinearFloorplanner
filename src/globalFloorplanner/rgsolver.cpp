#include "rgsolver.h"


GlobalSolver::GlobalSolver() {
    std::srand(std::time(NULL));
    softModuleNum_ = 0;
    fixedModuleNum_ = 0;
    lr_ = 2e-4;
    punishment_ = 0.05;
    maxAspectRatio_ = 2.;
    sizeScalar_ = 0;
    timeStep_ = 0;
    toggle_ = 1;
}

GlobalSolver::~GlobalSolver() {
    // do nothing
    // dynamic memory will be freed by parser
}

void GlobalSolver::setOutline(int width, int height) {
    DieWidth_ = ( double ) width;
    DieHeight_ = ( double ) height;
    xMaxMovement_ = DieWidth_ / 4000.;
    yMaxMovement_ = DieHeight_ / 4000.;
}

void GlobalSolver::setSoftModuleNum(int num) {
    softModuleNum_ = num;
    moduleNum_ = softModuleNum_ + fixedModuleNum_;
    xGradient_.resize(moduleNum_);
    yGradient_.resize(moduleNum_);
    wGradient_.resize(moduleNum_);
    hGradient_.resize(moduleNum_);
    xFirstMoment_.resize(moduleNum_, 0.);
    yFirstMoment_.resize(moduleNum_, 0.);
    xSecondMoment_.resize(moduleNum_, 0.);
    ySecondMoment_.resize(moduleNum_, 0.);
}

void GlobalSolver::setFixedModuleNum(int num) {
    fixedModuleNum_ = num;
    moduleNum_ = softModuleNum_ + fixedModuleNum_;
    xGradient_.resize(moduleNum_);
    yGradient_.resize(moduleNum_);
    wGradient_.resize(moduleNum_);
    hGradient_.resize(moduleNum_);
    xFirstMoment_.resize(moduleNum_, 0.);
    yFirstMoment_.resize(moduleNum_, 0.);
    xSecondMoment_.resize(moduleNum_, 0.);
    ySecondMoment_.resize(moduleNum_, 0.);
}

void GlobalSolver::setConnectionNum(int num) {
    connectionNum_ = num;
}

void GlobalSolver::setLearningRate(double lr) {
    lr_ = lr;
}

void GlobalSolver::setPunishment(double force) {
    punishment_ = force;
}

void GlobalSolver::setMaxAspectRatio(double aspect_ratio) {
    maxAspectRatio_ = aspect_ratio;
    for ( GlobalModule *mod : modules_ ) {
        mod->setMaxAspectRatio(aspect_ratio);
    }
}

void GlobalSolver::setMaxMovement(double ratio) {
    xMaxMovement_ = DieWidth_ * ratio;
    yMaxMovement_ = DieHeight_ * ratio;
}

void GlobalSolver::addModule(GlobalModule *in_module) {
    modules_.push_back(in_module);
}

void GlobalSolver::addConnection(const std::vector<std::string> &in_modules, double in_value) {
    std::vector<GlobalModule *> connectedModules;

    for ( const std::string &modName : in_modules ) {
        for ( int i = 0; i < modules_.size(); i++ ) {
            if ( modules_[i]->name == modName ) {
                connectedModules.push_back(modules_[i]);
            }
        }
    }

    for ( int i = 0; i < connectedModules.size(); ++i ) {
        std::vector<GlobalModule *> connModules;
        connModules = connectedModules;
        connModules.erase(connModules.begin() + i);
        connectedModules[i]->addConnection(connModules, in_value);
    }
}

void GlobalSolver::readFloorplan(Parser &parser) {
    setOutline(parser.getDieWidth(), parser.getDieHeight());
    setSoftModuleNum(parser.getSoftModuleNum());
    setFixedModuleNum(parser.getFixedModuleNum());
    setConnectionNum(parser.getConnectionNum());
    for ( int i = 0; i < modules_.size(); i++ ) {
        delete modules_[i];
    }
    modules_.clear();
    for ( int i = 0; i < moduleNum_; i++ ) {
        GlobalModule *newModule = parser.getModule(i);
        module2index_[newModule] = i;
        modules_.push_back(newModule);
    }
    double scalar = -1.;
    for ( int i = 0; i < connectionNum_; i++ ) {
        ConnectionInfo *conn = parser.getConnection(i);
        if ( ( double ) conn->value > scalar ) {
            scalar = ( double ) conn->value;
        }
        connectionList_.push_back(conn);
    }
    connectNormalize_ = 1. / scalar;
}

void GlobalSolver::readConfig(Parser &parser) {
    // specify punishment
    if ( parser.getPunishment() != -1 ) {
        double configPunishment = parser.getPunishment();
        std::cout << "[GlobalSolver] Note: Punishment is set to " << configPunishment << std::endl;
        punishment_ = configPunishment;
    }
    else {
        std::cout << "[GlobalSolver] Note: Punishment is not set. Use default value 0.01" << std::endl;
        punishment_ = 0.01;
    }

    // specify maximum aspect ratio
    if ( parser.getMaxAspectRatio() > 0.99) {
        maxAspectRatio_ = parser.getMaxAspectRatio();
        std::cout << "[GlobalSolver] Note: Maximum Aspect Ratio is set to " << maxAspectRatio_ << std::endl;
    }
    else {
        std::cout << "[GlobalSolver] Note: Maximum Aspect Ratio is not set. Use default value 2" << std::endl;
        maxAspectRatio_ = 2.;
    }
    setMaxAspectRatio(maxAspectRatio_);

    // specify learning rate
    if ( parser.getLearnRate() > 0 ) {
        lr_ = parser.getLearnRate();
        std::cout << "[GlobalSolver] Note: Learning rate is set to " << lr_ << std::endl;
    }
    else {
        std::cout << "[GlobalSolver] Note: Learning rate is not set. Use default value 2e-4" << std::endl;
        lr_ = 2e-4;
    }

    // specify inflation property
    inflationRatio_ = parser.getInflationRatio();
    dumpInflation_ = parser.getDumpInflation();

    // specify extreme push scale
    if ( parser.getExtremePushScale() > 0 ) {
        extremePushScale_ = parser.getExtremePushScale();
    }
    else {
        extremePushScale_ = 3;
    }

    // specify shape constraint
    std::vector< std::vector<std::string> > shapeConstraintMods = parser.getShapeConstraints();
    for ( int i = 0; i < shapeConstraintMods.size(); ++i ) {
        std::cout << "[GlobalSolver] Note: Shape Constraint: ";
        sameShapeMods_.push_back(std::vector<GlobalModule *>());
        for ( int j = 0; j < shapeConstraintMods[i].size(); ++j ) {
            if ( j != 0 ) {
                std::cout << ", ";
            }
            std::string tarModName = shapeConstraintMods[i][j];
            std::cout << tarModName;
            for ( GlobalModule *mod : modules_ ) {
                if ( mod->name == tarModName ) {
                    sameShapeMods_.back().push_back(mod);
                    break;
                }
            }
        }
        std::cout << std::endl;
    }
    for ( int i = 0; i < sameShapeMods_.size(); ++i ) {
        double area = sameShapeMods_[i][0]->area;
        for ( GlobalModule *mod : sameShapeMods_[i] ) {
            if ( std::abs(mod->area - area) > 1e-5 ) {
                std::cout << "[GlobalSolver] Warning: Modules with shape constraint should have same area." << std::endl;
            }
        }
    }
}

/* This is the old version for running ICCAD benchmarks */
void GlobalSolver::solve() {

    // specify gradient descent parameters
    this->setMaxMovement(0.001);
    int iteration = 1000;
    double punishment = punishment_;

    // apply gradient descent
    this->setPunishment(0);
    this->setSizeScalar(0.0001);
    for ( int phase = 1; phase <= 20; phase++ ) {
        // solver.setPunishment(punishment * phase / 10.);
        if ( phase > 1 ) {
            std::cout << "\r";
        }
        std::cout << "[GlobalSolver] Phase " << std::setw(2) << phase << " / 100" << std::flush;
        this->resetOptimizer();
        for ( int i = 0; i < iteration; i++ ) {
            this->calcGradient();
            this->gradientDescent(lr_);
        }
        this->checkShapeConstraint();
    }

    this->setPunishment(punishment);
    for ( int phase = 20; phase <= 100; phase++ ) {
        if ( phase > 1 ) {
            std::cout << "\r";
        }
        PunishmentScheme punishmentScheme = ( phase >= 98 ) ? SCALEPUSH : DEFAULT;
        std::cout << "[GlobalSolver] Phase " << std::setw(2) << phase << " / 100" << std::flush;
        this->setSizeScalar(phase * 0.01 * phase * 0.01, phase >= 80);
        this->resetOptimizer();
        for ( int i = 0; i < iteration; i++ ) {
            this->calcGradient(punishmentScheme);
            this->gradientDescent(lr_, phase >= 80);
        }
        this->checkShapeConstraint();
        if ( phase == 97 ) {
            std::cout << std::endl;
            this->markExtremeOverlap();
        }
        // this->currentPosition2txt("animation/" + std::to_string(phase) + ".txt");
    }
    std::cout << std::endl << std::endl;
}
// This is for Academic Benchmarks
/*
void GlobalSolver::solve() {
    // specify gradient descent parameters
    this->setMaxMovement(0.001);

    // apply gradient descent
    for ( int p = 1; p <= 50; p++ ) {
        // solver.setPunishment(punishment * phase);
        if ( p > 1 ) {
            std::cout << "\r";
            // solver.setPunishment(punishment);
        }
        std::cout << "[GlobalSolver] Phase " << std::setw(2) << p << " / 50" << std::flush;
        this->setSizeScalar(p * 0.02 * p * 0.02, p >= 40);
        this->resetOptimizer();
        for ( int i = 0; i < 1000; i++ ) {
            this->calcGradient();
            this->gradientDescent(lr_, p >= 40);
        }
        this->checkShapeConstraint();
        // solver.currentPosition2txt("animation/" + std::to_string(phase) + ".txt");
    }
}
*/
void GlobalSolver::inflate() {
    int iteration = 1000;
    // inflate soft modules
    std::cout << "[GlobalSolver] Note: Inflation Ratio is set to " << inflationRatio_ << std::endl;
    int inflationPhase = 10;
    for ( int p = 1; p <= inflationPhase; ++p ) {
        if ( p > 1 ) {
            std::cout << "\r";
        }
        std::cout << "[GlobalSolver] Inflation " << std::setw(2) << p << " / " << std::setw(2) << inflationPhase << std::flush;

        bool resolved = this->inflateModules();
        if ( resolved ) {
            break;
        }

        this->resetOptimizer();
        for ( int i = 0; i < iteration; ++i ) {
            this->calcGradient();
            this->gradientDescent(lr_, true);
        }
        this->checkShapeConstraint();
    }
}

void GlobalSolver::finetune() {
    int iteration = 1000;
    if ( inflationRatio_ > 0. ) {
        // resize soft modules to original sizes
        if ( !dumpInflation_ ) {
            this->setSizeScalar(1.);
        }
        this->roundToInteger();
    }
    else {
        // fine-tune after rounding to integer
        this->roundToInteger();
        this->resetOptimizer();
        // solver.setPunishment(punishment);
        for ( int i = 0; i < iteration; i++ ) {
            this->calcGradient(SCALEPUSH);
            this->gradientDescent(lr_);
        }
    }
    // solver.currentPosition2txt("animation/51.txt");
}

void GlobalSolver::calcGradient(PunishmentScheme punishmentScheme) {
    GlobalModule *curModule;
    for ( int i = 0; i < moduleNum_; i++ ) {
        if ( modules_[i]->fixed == true ) {
            continue;
        }

        curModule = modules_[i];
        double x_grad = 0;
        double y_grad = 0;
        double w_grad = 0;
        double h_grad = 0;

        // gradient for HPWL
        for ( int j = 0; j < curModule->connections.size(); j++ ) {
            double pullValue = curModule->connections[j]->value * connectNormalize_;
            if ( curModule->connections[j]->modules.size() > 1 ) {
                int x_sign = 0, y_sign = 0;
                bool x_is_max = true;
                bool x_is_min = true;
                bool y_is_max = true;
                bool y_is_min = true;
                for ( GlobalModule *pullModule : curModule->connections[j]->modules ) {
                    if ( pullModule->centerX > curModule->centerX ) {
                        x_is_max = false;
                    }
                    if ( pullModule->centerX < curModule->centerX ) {
                        x_is_min = false;
                    }
                    if ( pullModule->centerY > curModule->centerY ) {
                        y_is_max = false;
                    }
                    if ( pullModule->centerY < curModule->centerY ) {
                        y_is_min = false;
                    }
                }
                if ( x_is_max ) {
                    ++x_sign;
                }
                if ( x_is_min ) {
                    --x_sign;
                }
                if ( y_is_max ) {
                    ++y_sign;
                }
                if ( y_is_min ) {
                    --y_sign;
                }
                x_grad += pullValue * x_sign;
                y_grad += pullValue * y_sign;
            }
            else {
                GlobalModule *pullModule = curModule->connections[j]->modules[0];
                double x_diff, y_diff;

                x_diff = curModule->centerX - pullModule->centerX;
                y_diff = curModule->centerY - pullModule->centerY;
                if ( x_diff == 0 && y_diff == 0 ) {
                    continue;
                }

                double x_sign = ( x_diff == 0 ) ? 0. : ( x_diff > 0 ) ? 1. : -1.;
                double y_sign = ( y_diff == 0 ) ? 0. : ( y_diff > 0 ) ? 1. : -1.;
                x_grad += pullValue * x_sign;
                y_grad += pullValue * y_sign;
            }

        }

        // gradient for overlapped area
        double pushScale = 1;
        for ( int j = 0; j < moduleNum_; j++ ) {
            if ( j == i ) {
                continue;
            }
            GlobalModule *pushModule = modules_[j];

            // Calculate the movement gradient
            double overlappedWidth, overlappedHeight, x_diff, y_diff;

            x_diff = curModule->centerX - pushModule->centerX;
            y_diff = curModule->centerY - pushModule->centerY;
            if ( x_diff == 0 && y_diff == 0 ) {
                continue;
            }

            double curWidth = curModule->width;
            double pushWidth = pushModule->width;
            double curHeight = curModule->height;
            double pushHeight = pushModule->height;

            double max_xl = std::max(curModule->centerX - curWidth / 2., pushModule->centerX - pushWidth / 2.);
            double min_xr = std::min(curModule->centerX + curWidth / 2., pushModule->centerX + pushWidth / 2.);
            double max_yd = std::max(curModule->centerY - curHeight / 2., pushModule->centerY - pushHeight / 2.);
            double min_yu = std::min(curModule->centerY + curHeight / 2., pushModule->centerY + pushHeight / 2.);

            overlappedWidth = min_xr - max_xl;
            overlappedHeight = min_yu - max_yd;
            if ( overlappedWidth <= 0. || overlappedHeight <= 0. ) {
                continue;
            }

            bool width_cover = ( overlappedWidth >= curWidth || overlappedWidth >= pushWidth );
            bool height_cover = ( overlappedHeight >= curHeight || overlappedHeight >= pushHeight );

            if ( width_cover && !height_cover ) {
                overlappedHeight = 0;
            }
            else if ( height_cover && !width_cover ) {
                overlappedWidth = 0;
            }

            double x_sign = ( x_diff == 0 ) ? 0 : ( x_diff > 0 ) ? 1. : -1.;
            double y_sign = ( y_diff == 0 ) ? 0 : ( y_diff > 0 ) ? 1. : -1.;

            if ( punishmentScheme == SCALEPUSH && ( bigOverlapModules_.count(curModule) || bigOverlapModules_.count(pushModule) ) ) {
                pushScale = extremePushScale_;
            }

            x_grad += -pushScale * punishment_ * x_sign * overlappedHeight / DieHeight_ * DieWidth_;
            y_grad += -pushScale * punishment_ * y_sign * overlappedWidth;
            // double x_unitPush = x_sign * overlappedHeight / ( overlappedWidth * overlappedWidth + overlappedHeight * overlappedHeight );
            // double y_unitPush = y_sign * overlappedWidth / ( overlappedWidth * overlappedWidth + overlappedHeight * overlappedHeight );
            // x_grad += -punishment * x_unitPush;
            // y_grad += -punishment * y_unitPush;

            // Calculate the dimension gradient
            w_grad += pushScale * overlappedHeight / DieHeight_ * DieWidth_;
            h_grad += pushScale * overlappedWidth;
        }

        xGradient_[i] = x_grad;
        yGradient_[i] = y_grad;
        wGradient_[i] = w_grad;
        hGradient_[i] = h_grad;
    }

    // for ( int i = 0; i < moduleNum_; i++ ) {
    //    std::cout << modules_[i]->name << ": " << xGradient_[i] << " " << yGradient_[i] << std::endl;
    // }
}

void GlobalSolver::gradientDescent(double lr, bool squeeze) {
    timeStep_ += 1;
    // move soft modules
    GlobalModule *curModule;
    for ( int i = 0; i < moduleNum_; i++ ) {
        if ( modules_[i]->fixed ) {
            continue;
        }

        curModule = modules_[i];

        xFirstMoment_[i] = 0.9 * xFirstMoment_[i] + 0.1 * xGradient_[i];
        yFirstMoment_[i] = 0.9 * yFirstMoment_[i] + 0.1 * yGradient_[i];

        xSecondMoment_[i] = 0.999 * xSecondMoment_[i] + 0.001 * xGradient_[i] * xGradient_[i];
        ySecondMoment_[i] = 0.999 * ySecondMoment_[i] + 0.001 * yGradient_[i] * yGradient_[i];

        double xFirstMomentHat = xFirstMoment_[i] / ( 1 - std::pow(0.9, timeStep_) );
        double yFirstMomentHat = yFirstMoment_[i] / ( 1 - std::pow(0.9, timeStep_) );

        double xSecondMomentHat = xSecondMoment_[i] / ( 1 - std::pow(0.999, timeStep_) );
        double ySecondMomentHat = ySecondMoment_[i] / ( 1 - std::pow(0.999, timeStep_) );

        double xMovement = ( lr * xFirstMomentHat / ( std::sqrt(xSecondMomentHat) + 1e-8 ) ) * DieWidth_;
        double yMovement = ( lr * yFirstMomentHat / ( std::sqrt(ySecondMomentHat) + 1e-8 ) ) * DieHeight_;

        if ( std::abs(xMovement) > xMaxMovement_ ) {
            xMovement = ( xMovement > 0 ) ? xMaxMovement_ : -xMaxMovement_;
        }
        if ( std::abs(yMovement) > yMaxMovement_ ) {
            yMovement = ( yMovement > 0 ) ? yMaxMovement_ : -yMaxMovement_;
        }

        curModule->centerX -= xMovement;
        curModule->centerY -= yMovement;

        if ( curModule->centerX < curModule->width / 2. ) {
            curModule->centerX = curModule->width / 2.;
            wGradient_[i] += curModule->height * 0.5;
        }
        if ( curModule->centerY < curModule->height / 2. ) {
            curModule->centerY = curModule->height / 2.;
            hGradient_[i] += curModule->width * 0.5;
        }
        if ( curModule->centerX > DieWidth_ - curModule->width / 2. ) {
            curModule->centerX = DieWidth_ - curModule->width / 2.;
            wGradient_[i] += curModule->height * 0.5;
        }
        if ( curModule->centerY > DieHeight_ - curModule->height / 2. ) {
            curModule->centerY = DieHeight_ - curModule->height / 2.;
            hGradient_[i] += curModule->width * 0.5;
        }
    }

    // handle shape constraint
    for ( int i = 0; i < sameShapeMods_.size(); ++i ) {
        double sharedWidthGrad = 0, sharedHeightGrad = 0;
        for ( GlobalModule *mod : sameShapeMods_[i] ) {
            sharedWidthGrad += wGradient_[module2index_[mod]];
            sharedHeightGrad += hGradient_[module2index_[mod]];
        }
        sharedWidthGrad /= ( double ) sameShapeMods_[i].size();
        sharedHeightGrad /= ( double ) sameShapeMods_[i].size();
        for ( GlobalModule *mod : sameShapeMods_[i] ) {
            wGradient_[module2index_[mod]] = sharedWidthGrad;
            hGradient_[module2index_[mod]] = sharedHeightGrad;
        }
    }

    if ( squeeze ) {
        for ( int i = 0; i < moduleNum_; i++ ) {
            if ( modules_[i]->fixed ) {
                continue;
            }
            curModule = modules_[i];
            if ( toggle_ ) {
                curModule->setHeight(curModule->height - lr * hGradient_[i]);
            }
            else {
                curModule->setWidth(curModule->width - lr * wGradient_[i]);
            }
        }
    }
    toggle_ = !toggle_;
}

void GlobalSolver::setSizeScalar(double sizeScalar, bool overlap_aware) {
    // scalar: new sizeScalar
    if ( sizeScalar <= sizeScalar_ || !overlap_aware ) {
        for ( GlobalModule *mod : modules_ ) {
            mod->scaleSize(sizeScalar);
        }
        sizeScalar_ = sizeScalar;
        return;
    }

    std::vector<double> areaToGrow(moduleNum_);
    std::vector<std::vector<double>> direcGrad(moduleNum_);
    for ( int i = 0; i < moduleNum_; ++i ) {
        areaToGrow[i] = modules_[i]->area * ( sizeScalar - sizeScalar_ );
        direcGrad[i].resize(4); // 0: top, 1: right, 2: bottom, 3: left
    }

    int iterationNum = 8;
    for ( int it = 0; it < iterationNum; ++it ) {
        // calculate gradient
        for ( int i = 0; i < moduleNum_; ++i ) {
            if ( modules_[i]->fixed ) {
                continue;
            }
            GlobalModule *curModule = modules_[i];
            // gradient for overlapped area
            for ( int j = 0; j < moduleNum_; j++ ) {
                if ( j == i ) {
                    continue;
                }
                GlobalModule *pushModule = modules_[j];

                // Calculate the dimension gradient
                double overlappedWidth, overlappedHeight;

                double curWidth = curModule->width;
                double pushWidth = pushModule->width;
                double curHeight = curModule->height;
                double pushHeight = pushModule->height;

                double max_xl = std::max(curModule->centerX - curWidth / 2., pushModule->centerX - pushWidth / 2.);
                double min_xr = std::min(curModule->centerX + curWidth / 2., pushModule->centerX + pushWidth / 2.);
                double max_yd = std::max(curModule->centerY - curHeight / 2., pushModule->centerY - pushHeight / 2.);
                double min_yu = std::min(curModule->centerY + curHeight / 2., pushModule->centerY + pushHeight / 2.);

                overlappedWidth = min_xr - max_xl;
                overlappedHeight = min_yu - max_yd;
                if ( overlappedWidth <= 0. || overlappedHeight <= 0. ) {
                    continue;
                }

                // bool width_cover = ( overlappedWidth >= curWidth || overlappedWidth >= pushWidth );
                // bool height_cover = ( overlappedHeight >= curHeight || overlappedHeight >= pushHeight );

                // if ( width_cover && !height_cover ) {
                //     overlappedHeight = 0;
                // }
                // else if ( height_cover && !width_cover ) {
                //     overlappedWidth = 0;
                // }

                if ( curModule->centerY > pushModule->centerY ) {        // bottom
                    direcGrad[i][2] += overlappedWidth;
                }
                else if ( curModule->centerY < pushModule->centerY ) {   // top
                    direcGrad[i][0] += overlappedWidth;
                }
                if ( curModule->centerX > pushModule->centerX ) {        // left
                    direcGrad[i][3] += overlappedHeight;
                }
                else if ( curModule->centerX < pushModule->centerX ) {   // right
                    direcGrad[i][1] += overlappedHeight;
                }
            }
        }
        // handle shape constraint
        for ( int i = 0; i < sameShapeMods_.size(); ++i ) {
            double sharedBottomGrad = 0, sharedTopGrad = 0, sharedLeftGrad = 0, sharedRightGrad = 0;
            for ( GlobalModule *mod : sameShapeMods_[i] ) {
                sharedTopGrad += direcGrad[module2index_[mod]][0];
                sharedRightGrad += direcGrad[module2index_[mod]][1];
                sharedBottomGrad += direcGrad[module2index_[mod]][2];
                sharedLeftGrad += direcGrad[module2index_[mod]][3];
            }
            sharedTopGrad /= ( double ) sameShapeMods_[i].size();
            sharedRightGrad /= ( double ) sameShapeMods_[i].size();
            sharedBottomGrad /= ( double ) sameShapeMods_[i].size();
            sharedLeftGrad /= ( double ) sameShapeMods_[i].size();
            for ( GlobalModule *mod : sameShapeMods_[i] ) {
                direcGrad[module2index_[mod]][0] = sharedTopGrad;
                direcGrad[module2index_[mod]][1] = sharedRightGrad;
                direcGrad[module2index_[mod]][2] = sharedBottomGrad;
                direcGrad[module2index_[mod]][3] = sharedLeftGrad;
            }
        }
        // grow soft modules
        for ( int i = 0; i < moduleNum_; ++i ) {
            if ( modules_[i]->fixed ) {
                continue;
            }
            GlobalModule *curModule = modules_[i];
            double curAreaToGrow = areaToGrow[i] / iterationNum;
            if ( direcGrad[i][0] < 1e-5 && direcGrad[i][1] < 1e-5 && direcGrad[i][2] < 1e-5 && direcGrad[i][3] < 1e-5 ) {
                // grow evenly
                curModule->setArea(curModule->area + curAreaToGrow);
            }
            else {
                double max_val = std::max({ direcGrad[i][0], direcGrad[i][1], direcGrad[i][2], direcGrad[i][3] });
                direcGrad[i][0] /= ( max_val / 5 );
                direcGrad[i][1] /= ( max_val / 5 );
                direcGrad[i][2] /= ( max_val / 5 );
                direcGrad[i][3] /= ( max_val / 5 );
                double denom = std::exp(-direcGrad[i][0]) + std::exp(-direcGrad[i][1]) + std::exp(-direcGrad[i][2]) + std::exp(-direcGrad[i][3]);
                // std::cerr << direcGrad[i][0] << " " << direcGrad[i][1] << " " << direcGrad[i][2] << " " << direcGrad[i][3] << " " << denom << std::endl;
                double topGrowRatio = std::exp(-direcGrad[i][0]) / denom;
                double rightGrowRatio = std::exp(-direcGrad[i][1]) / denom;
                double bottomGrowRatio = std::exp(-direcGrad[i][2]) / denom;
                double leftGrowRatio = std::exp(-direcGrad[i][3]) / denom;
                // grow top
                double topGrowthHeight = curAreaToGrow * topGrowRatio / curModule->width;
                curModule->growHeight(topGrowthHeight);
                curModule->centerY += topGrowthHeight / 2;
                // grow right
                double rightGrowthWidth = curAreaToGrow * rightGrowRatio / curModule->height;
                curModule->growWidth(rightGrowthWidth);
                curModule->centerX += rightGrowthWidth / 2;
                // grow bottom
                double bottomGrowthHeight = curAreaToGrow * bottomGrowRatio / curModule->width;
                curModule->growHeight(bottomGrowthHeight);
                curModule->centerY -= bottomGrowthHeight / 2;
                // grow left
                double leftGrowthWidth = curAreaToGrow * leftGrowRatio / curModule->height;
                curModule->growWidth(leftGrowthWidth);
                curModule->centerX -= leftGrowthWidth / 2;
            }
        }
    }

    // minimize error
    for ( GlobalModule *mod : modules_ ) {
        mod->scaleSize(sizeScalar);
    }
    sizeScalar_ = sizeScalar;
}

bool GlobalSolver::inflateModules() {
    // Initialization
    std::vector<double> areaToGrow(moduleNum_, 0);
    std::vector<double> overlapToSolve(moduleNum_, 0);
    std::vector<std::vector<double>> direcGrad(moduleNum_);
    for ( int i = 0; i < moduleNum_; ++i ) {
        direcGrad[i].resize(4); // 0: top, 1: right, 2: bottom, 3: left
    }

    // calculate overlaps
    for ( int i = 0; i < moduleNum_; ++i ) {
        for ( int j = i + 1; j < moduleNum_; ++j ) {
            GlobalModule *mod1 = modules_[i];
            GlobalModule *mod2 = modules_[j];

            // Calculate the dimension gradient
            double overlappedWidth, overlappedHeight;

            double curWidth = mod1->width;
            double pushWidth = mod2->width;
            double curHeight = mod1->height;
            double pushHeight = mod2->height;

            double max_xl = std::max(mod1->centerX - curWidth / 2., mod2->centerX - pushWidth / 2.);
            double min_xr = std::min(mod1->centerX + curWidth / 2., mod2->centerX + pushWidth / 2.);
            double max_yd = std::max(mod1->centerY - curHeight / 2., mod2->centerY - pushHeight / 2.);
            double min_yu = std::min(mod1->centerY + curHeight / 2., mod2->centerY + pushHeight / 2.);

            overlappedWidth = min_xr - max_xl;
            overlappedHeight = min_yu - max_yd;
            if ( overlappedWidth <= 0. || overlappedHeight <= 0. ) {
                continue;
            }

            if ( modules_[i]->fixed ) {
                overlapToSolve[j] += overlappedWidth * overlappedHeight;
            }
            else if ( modules_[j]->fixed ) {
                overlapToSolve[i] += overlappedWidth * overlappedHeight;
            }
            else {
                overlapToSolve[i] += overlappedWidth * overlappedHeight / 2.;
                overlapToSolve[j] += overlappedWidth * overlappedHeight / 2.;
            }
        }
    }

    bool resolved = true;
    for ( int i = 0; i < moduleNum_; ++i ) {
        if ( modules_[i]->fixed ) {
            continue;
        }
        if ( overlapToSolve[i] > modules_[i]->currentArea - modules_[i]->area ) {
            areaToGrow[i] = overlapToSolve[i] - ( modules_[i]->currentArea - modules_[i]->area );
        }
        if ( areaToGrow[i] / modules_[i]->area >= 0.02 ) {
            areaToGrow[i] = modules_[i]->area * 0.02;
        }
        if ( modules_[i]->currentArea + areaToGrow[i] >= ( 1 + inflationRatio_ ) * modules_[i]->area ) {
            areaToGrow[i] = 0;
        }
        if ( areaToGrow[i] > 0 ) {
            resolved = false;
            // std::cout << modules_[i]->name << ": " << areaToGrow[i] << std::endl;
        }
    }

    if ( resolved ) {
        return true;
    }

    int iterationNum = 8;
    for ( int it = 0; it < iterationNum; ++it ) {
        // calculate gradient
        for ( int i = 0; i < moduleNum_; ++i ) {
            if ( modules_[i]->fixed ) {
                continue;
            }
            GlobalModule *curModule = modules_[i];
            // gradient for overlapped area
            for ( int j = 0; j < moduleNum_; j++ ) {
                if ( j == i ) {
                    continue;
                }
                GlobalModule *pushModule = modules_[j];

                // Calculate the dimension gradient
                double overlappedWidth, overlappedHeight;

                double curWidth = curModule->width;
                double pushWidth = pushModule->width;
                double curHeight = curModule->height;
                double pushHeight = pushModule->height;

                double max_xl = std::max(curModule->centerX - curWidth / 2., pushModule->centerX - pushWidth / 2.);
                double min_xr = std::min(curModule->centerX + curWidth / 2., pushModule->centerX + pushWidth / 2.);
                double max_yd = std::max(curModule->centerY - curHeight / 2., pushModule->centerY - pushHeight / 2.);
                double min_yu = std::min(curModule->centerY + curHeight / 2., pushModule->centerY + pushHeight / 2.);

                overlappedWidth = min_xr - max_xl;
                overlappedHeight = min_yu - max_yd;
                if ( overlappedWidth <= 0. || overlappedHeight <= 0. ) {
                    continue;
                }

                bool width_cover = ( overlappedWidth >= curWidth || overlappedWidth >= pushWidth );
                bool height_cover = ( overlappedHeight >= curHeight || overlappedHeight >= pushHeight );

                if ( width_cover && !height_cover ) {
                    overlappedHeight = 0;
                }
                else if ( height_cover && !width_cover ) {
                    overlappedWidth = 0;
                }

                if ( curModule->centerY > pushModule->centerY ) {        // bottom
                    direcGrad[i][2] += overlappedWidth;
                }
                else if ( curModule->centerY < pushModule->centerY ) {   // top
                    direcGrad[i][0] += overlappedWidth;
                }
                if ( curModule->centerX > pushModule->centerX ) {        // left
                    direcGrad[i][3] += overlappedHeight;
                }
                else if ( curModule->centerX < pushModule->centerX ) {   // right
                    direcGrad[i][1] += overlappedHeight;
                }
            }
        }
        // handle shape constraint
        for ( int i = 0; i < sameShapeMods_.size(); ++i ) {
            double sharedBottomGrad = 0, sharedTopGrad = 0, sharedLeftGrad = 0, sharedRightGrad = 0;
            for ( GlobalModule *mod : sameShapeMods_[i] ) {
                sharedTopGrad += direcGrad[module2index_[mod]][0];
                sharedRightGrad += direcGrad[module2index_[mod]][1];
                sharedBottomGrad += direcGrad[module2index_[mod]][2];
                sharedLeftGrad += direcGrad[module2index_[mod]][3];
            }
            sharedTopGrad /= ( double ) sameShapeMods_[i].size();
            sharedRightGrad /= ( double ) sameShapeMods_[i].size();
            sharedBottomGrad /= ( double ) sameShapeMods_[i].size();
            sharedLeftGrad /= ( double ) sameShapeMods_[i].size();
            for ( GlobalModule *mod : sameShapeMods_[i] ) {
                direcGrad[module2index_[mod]][0] = sharedTopGrad;
                direcGrad[module2index_[mod]][1] = sharedRightGrad;
                direcGrad[module2index_[mod]][2] = sharedBottomGrad;
                direcGrad[module2index_[mod]][3] = sharedLeftGrad;
            }
        }
        // grow soft modules
        for ( int i = 0; i < moduleNum_; ++i ) {
            if ( modules_[i]->fixed ) {
                continue;
            }
            GlobalModule *curModule = modules_[i];
            double curAreaToGrow = areaToGrow[i] / iterationNum;
            if ( direcGrad[i][0] < 1e-5 && direcGrad[i][1] < 1e-5 && direcGrad[i][2] < 1e-5 && direcGrad[i][3] < 1e-5 ) {
                // grow evenly
                curModule->setArea(curModule->area + curAreaToGrow);
            }
            else {
                double max_val = std::max({ direcGrad[i][0], direcGrad[i][1], direcGrad[i][2], direcGrad[i][3] });
                direcGrad[i][0] /= ( max_val / 5 );
                direcGrad[i][1] /= ( max_val / 5 );
                direcGrad[i][2] /= ( max_val / 5 );
                direcGrad[i][3] /= ( max_val / 5 );
                double denom = std::exp(-direcGrad[i][0]) + std::exp(-direcGrad[i][1]) + std::exp(-direcGrad[i][2]) + std::exp(-direcGrad[i][3]);
                // std::cerr << direcGrad[i][0] << " " << direcGrad[i][1] << " " << direcGrad[i][2] << " " << direcGrad[i][3] << " " << denom << std::endl;
                double topGrowRatio = std::exp(-direcGrad[i][0]) / denom;
                double rightGrowRatio = std::exp(-direcGrad[i][1]) / denom;
                double bottomGrowRatio = std::exp(-direcGrad[i][2]) / denom;
                double leftGrowRatio = std::exp(-direcGrad[i][3]) / denom;
                // grow top
                double topGrowthHeight = curAreaToGrow * topGrowRatio / curModule->width;
                curModule->growHeight(topGrowthHeight);
                curModule->centerY += topGrowthHeight / 2;
                // grow right
                double rightGrowthWidth = curAreaToGrow * rightGrowRatio / curModule->height;
                curModule->growWidth(rightGrowthWidth);
                curModule->centerX += rightGrowthWidth / 2;
                // grow bottom
                double bottomGrowthHeight = curAreaToGrow * bottomGrowRatio / curModule->width;
                curModule->growHeight(bottomGrowthHeight);
                curModule->centerY -= bottomGrowthHeight / 2;
                // grow left
                double leftGrowthWidth = curAreaToGrow * leftGrowRatio / curModule->height;
                curModule->growWidth(leftGrowthWidth);
                curModule->centerX -= leftGrowthWidth / 2;
            }
        }
    }
    return false;
}

void GlobalSolver::markExtremeOverlap() {
    for ( GlobalModule *curModule : modules_ ) {
        if ( curModule->fixed ) {
            continue;
        }
        double totalOverlapArea = 0;
        for ( GlobalModule *pushModule : modules_ ) {
            if ( pushModule == curModule ) {
                continue;
            }

            double overlappedWidth, overlappedHeight;

            double curWidth = curModule->width;
            double pushWidth = pushModule->width;
            double curHeight = curModule->height;
            double pushHeight = pushModule->height;

            double max_xl = std::max(curModule->centerX - curWidth / 2., pushModule->centerX - pushWidth / 2.);
            double min_xr = std::min(curModule->centerX + curWidth / 2., pushModule->centerX + pushWidth / 2.);
            double max_yd = std::max(curModule->centerY - curHeight / 2., pushModule->centerY - pushHeight / 2.);
            double min_yu = std::min(curModule->centerY + curHeight / 2., pushModule->centerY + pushHeight / 2.);

            overlappedWidth = min_xr - max_xl;
            overlappedHeight = min_yu - max_yd;

            if ( overlappedWidth > 0. && overlappedHeight > 0. ) {
                totalOverlapArea += overlappedWidth * overlappedHeight;
            }
        }
        // utilization: ( curModule->currentArea - 0.5 * totalOverlapArea ) / ( curModule->currentArea + 0.5 * totalOverlapArea )
        if ( ( curModule->currentArea - 0.5 * totalOverlapArea ) / ( curModule->currentArea + 0.5 * totalOverlapArea ) < 0.8 ) {
            std::cout << "[GlobalSolver] Note: Found extremely overlapping module: " << curModule->name << std::endl;
            bigOverlapModules_.emplace(curModule);
        }
    }
}

void GlobalSolver::checkShapeConstraint() {
    for ( int i = 0; i < sameShapeMods_.size(); ++i ) {
        double sharedWidth = 0, sharedHeight = 0;
        for ( GlobalModule *mod : sameShapeMods_[i] ) {
            sharedWidth += mod->width;
            sharedHeight += mod->height;
        }
        sharedWidth /= ( double ) sameShapeMods_[i].size();
        sharedHeight /= ( double ) sameShapeMods_[i].size();
        for ( GlobalModule *mod : sameShapeMods_[i] ) {
            mod->width = sharedWidth;
            mod->height = sharedHeight;
        }
    }
}

void GlobalSolver::roundToInteger() {
    for ( GlobalModule *mod : modules_ ) {
        if ( mod->fixed ) {
            continue;
        }
        double modArea = ( mod->area > mod->currentArea ) ? ( double ) mod->area : mod->currentArea;
        if ( DieWidth_ > DieHeight_ ) {
            mod->width = std::round(mod->width);
            mod->height = std::ceil(modArea / mod->width);
            if ( mod->height / mod->width > maxAspectRatio_ ) {
                mod->height -= 1.;
                mod->width = std::ceil(modArea / mod->height);
            }
            else if ( mod->height / mod->width < 1 / maxAspectRatio_ ) {
                mod->width -= 1.;
                mod->height = std::ceil(modArea / mod->width);
            }
        }
        else {
            mod->height = std::round(mod->height);
            mod->width = std::ceil(modArea / mod->height);
            if ( mod->height / mod->width > maxAspectRatio_ ) {
                mod->height -= 1.;
                mod->width = std::ceil(modArea / mod->height);
            }
            else if ( mod->height / mod->width < 1 / maxAspectRatio_ ) {
                mod->width -= 1.;
                mod->height = std::ceil(modArea / mod->width);
            }
        }
    }
}

void GlobalSolver::generateCluster() {
    std::cout << "[GlobalSolver] Note: Generating clusters..." << std::endl;
    Cluster cluster(connectionList_);
    cluster.louvain();
    std::vector<std::vector<GlobalModule *>> moduleCluster = cluster.getCluster();
    GlobalModule *mod1, *mod2;
    for ( int i = 0; i < moduleCluster.size(); ++i ) {
        for ( int j = 0; j < moduleCluster[i].size() - 1; ++j ) {
            for ( int k = j + 1; k < moduleCluster[i].size(); ++k ) {
                mod1 = moduleCluster[i][j];
                mod2 = moduleCluster[i][k];
                mod1->addConnection(std::vector<GlobalModule *> { mod2 }, 0.3 / connectNormalize_);
                mod2->addConnection(std::vector<GlobalModule *> { mod1 }, 0.3 / connectNormalize_);
            }
        }
    }
}

void GlobalSolver::resetOptimizer() {
    xFirstMoment_.resize(moduleNum_, 0.);
    yFirstMoment_.resize(moduleNum_, 0.);
    xSecondMoment_.resize(moduleNum_, 0.);
    ySecondMoment_.resize(moduleNum_, 0.);
    timeStep_ = 0;
}

void GlobalSolver::currentPosition2txt(std::string file_name) {
    for ( GlobalModule *mod : modules_ ) {
        mod->updateCord(( int ) DieWidth_, ( int ) DieHeight_);
    }
    std::ofstream ostream(file_name);
    ostream << "BLOCK " << moduleNum_ << " CONNECTION " << connectionNum_ << std::endl;
    ostream << DieWidth_ << " " << DieHeight_ << std::endl;
    for ( GlobalModule *mod : modules_ ) {
        ostream << mod->name << " ";
        ostream << ( ( mod->fixed ) ? "FIXED" : "SOFT" ) << " ";
        ostream << mod->area << " ";
        ostream << mod->x << " " << mod->y << " ";
        ostream << mod->width << " " << mod->height << std::endl;

    }
    for ( int i = 0; i < connectionNum_; i++ ) {
        ConnectionInfo *conn = connectionList_[i];
        for ( int j = 0; j < conn->modules.size(); ++j ) {
            ostream << conn->modules[j] << " ";
        }
        ostream << conn->value << std::endl;
    }
    ostream.close();
}

double GlobalSolver::calcDeadspace() {
    double dieArea = DieWidth_ * DieHeight_;
    double moduleArea = 0;
    for ( int i = 0; i < moduleNum_; i++ ) {
        moduleArea += modules_[i]->area;
    }
    return 1. - moduleArea / dieArea;
}

double GlobalSolver::calcEstimatedHPWL() {
    double HPWL = 0;
    for ( int i = 0; i < connectionNum_; ++i ) {
        ConnectionInfo *conn = connectionList_[i];
        double maxX = 0., maxY = 0.;
        double minX = 1e10, minY = 1e10;
        for ( GlobalModule *mod : conn->modulePtrs ) {
            maxX = ( mod->centerX > maxX ) ? mod->centerX : maxX;
            minX = ( mod->centerX < minX ) ? mod->centerX : minX;
            maxY = ( mod->centerY > maxY ) ? mod->centerY : maxY;
            minY = ( mod->centerY < minY ) ? mod->centerY : minY;
        }
        HPWL += ( maxX - minX + maxY - minY ) * conn->value;
    }
    return HPWL;
}

double GlobalSolver::calcOverlapRatio() {
 double totalOverlapArea = 0;
    double totalArea = 0;

    for ( int i = 0; i < moduleNum_ - 1; i++ ) {
        for ( int j = i + 1; j < moduleNum_; j++ ) {
            GlobalModule *mod1 = modules_[i];
            GlobalModule *mod2 = modules_[j];

            if ( mod1->fixed && mod2->fixed ) {
                continue;
            }

            double overlappedWidth, overlappedHeight;

            double mod1Width = mod1->width;
            double mod2Width = mod2->width;
            double mod1Height = mod1->height;
            double mod2Height = mod2->height;

            double max_xl = std::max(mod1->centerX - mod1Width / 2., mod2->centerX - mod2Width / 2.);
            double min_xr = std::min(mod1->centerX + mod1Width / 2., mod2->centerX + mod2Width / 2.);
            double max_yd = std::max(mod1->centerY - mod1Height / 2., mod2->centerY - mod2Height / 2.);
            double min_yu = std::min(mod1->centerY + mod1Height / 2., mod2->centerY + mod2Height / 2.);

            overlappedWidth = min_xr - max_xl;
            overlappedHeight = min_yu - max_yd;

            if ( overlappedWidth > 0. && overlappedHeight > 0. ) {
                // std::cout << mod1->name << " & " << mod2->name << "\t: " << overlappedWidth * overlappedHeight << std::endl;
                // std::cout << "Mod1 Width: " << mod1Width << " Height: " << mod1Height << std::endl;
                // std::cout << "Mod2 Width: " << mod2Width << " Height: " << mod2Height << std::endl;
                // std::cout << std::endl;
                totalOverlapArea += overlappedWidth * overlappedHeight;
            }
        }
    }

    for ( GlobalModule *&mod : modules_ ) {
        if ( !mod->fixed ) {
            totalArea += ( double ) mod->area;
        }
    }

    return totalOverlapArea / totalArea;
}

bool GlobalSolver::isAreaLegal() {
    for ( GlobalModule *mod : modules_ ) {
        if ( mod->fixed ) {
            continue;
        }
        if ( mod->width * mod->height < mod->area ) {
            std::cout << "[GlobalSolver] " << mod->name << ": ";
            std::cout << mod->width << " x " << mod->height << " < " << mod->area << std::endl;
            return false;
        }
    }
    return true;
}

bool GlobalSolver::isAspectRatioLegal() {
    double maxAR = ( maxAspectRatio_ > 2 ) ? maxAspectRatio_ : 2;
    for ( GlobalModule *mod : modules_ ) {
        if ( mod->fixed ) {
            continue;
        }
        if ( mod->height / mod->width > maxAR ) {
            std::cout << "[GlobalSolver] " << mod->name << ": ";
            std::cout << mod->height << " / " << mod->width << " > " << maxAR << std::endl;
            return false;
        }
        else if ( mod->height / mod->width < 1. / maxAR ) {
            std::cout << "[GlobalSolver] " << mod->name << ": ";
            std::cout << mod->height << " / " << mod->width << " < " << 1. / maxAR << std::endl;
            return false;
        }
    }
    return true;
}

void GlobalSolver::reportOverlap() {
    double totalOverlapArea = 0;
    double totalArea = 0;
    printf("╔═════════ Overlap Report ═════════╗\n");
    printf("║   Module Name   │      Area      ║\n");
    printf("╟─────────────────┼────────────────╢\n");

    for ( int i = 0; i < moduleNum_ - 1; i++ ) {
        for ( int j = i + 1; j < moduleNum_; j++ ) {
            GlobalModule *mod1 = modules_[i];
            GlobalModule *mod2 = modules_[j];

            if ( mod1->fixed && mod2->fixed ) {
                continue;
            }

            double overlappedWidth, overlappedHeight;

            double mod1Width = mod1->width;
            double mod2Width = mod2->width;
            double mod1Height = mod1->height;
            double mod2Height = mod2->height;

            double max_xl = std::max(mod1->centerX - mod1Width / 2., mod2->centerX - mod2Width / 2.);
            double min_xr = std::min(mod1->centerX + mod1Width / 2., mod2->centerX + mod2Width / 2.);
            double max_yd = std::max(mod1->centerY - mod1Height / 2., mod2->centerY - mod2Height / 2.);
            double min_yu = std::min(mod1->centerY + mod1Height / 2., mod2->centerY + mod2Height / 2.);

            overlappedWidth = min_xr - max_xl;
            overlappedHeight = min_yu - max_yd;

            if ( overlappedWidth > 0. && overlappedHeight > 0. ) {
                // std::cout << mod1->name << " & " << mod2->name << "\t: " << overlappedWidth * overlappedHeight << std::endl;
                printf("║ %6s & %-6s │ %14.2lf ║\n", mod1->name.c_str(), mod2->name.c_str(), overlappedWidth * overlappedHeight);
                // std::cout << "Mod1 Width: " << mod1Width << " Height: " << mod1Height << std::endl;
                // std::cout << "Mod2 Width: " << mod2Width << " Height: " << mod2Height << std::endl;
                // std::cout << std::endl;
                totalOverlapArea += overlappedWidth * overlappedHeight;
            }
        }
    }

    for ( GlobalModule *&mod : modules_ ) {
        if ( !mod->fixed ) {
            totalArea += ( double ) mod->area;
        }
    }

    printf("╠═════════════════╪════════════════╣\n");
    printf("║  Soft Mod Area  │ %14.2lf ║\n", totalArea);
    printf("║  Overlap Area   │ %14.2lf ║\n", totalOverlapArea);
    printf("║  Overlap Ratio  │      %8.5lf%% ║\n", totalOverlapArea / totalArea * 100);
    printf("╚═════════════════╧════════════════╝\n");
}

void GlobalSolver::reportDeadSpace() {
    double totalArea = DieWidth_ * DieHeight_;
    double moduleArea = 0;
    for ( int i = 0; i < moduleNum_; i++ ) {
        moduleArea += modules_[i]->area;
    }
    printf("╔═══════ Dead Space Report ════════╗\n");
    printf("║   Chip Area     │ %14.0lf ║\n", totalArea);
    printf("║   Module Area   │ %14.0lf ║\n", moduleArea);
    printf("║   Dead Space    │      %8.5lf%% ║\n", ( 1. - moduleArea / totalArea ) * 100);
    printf("╚═════════════════╧════════════════╝\n");
}

void GlobalSolver::reportInflation() {
    std::cout << "=========== Inflation Report ===========" << std::endl;
    std::cout << std::fixed;
    double totalInflation = 0, totalOverlap = 0;
    for ( int i = 0; i < moduleNum_; i++ ) {
        double overlapArea = 0;
        for ( int j = 0; j < moduleNum_; j++ ) {
            if ( j == i ) {
                continue;
            }

            GlobalModule *mod1 = modules_[i];
            GlobalModule *mod2 = modules_[j];

            double overlappedWidth, overlappedHeight;

            double mod1Width = mod1->width;
            double mod2Width = mod2->width;
            double mod1Height = mod1->height;
            double mod2Height = mod2->height;

            double max_xl = std::max(mod1->centerX - mod1Width / 2., mod2->centerX - mod2Width / 2.);
            double min_xr = std::min(mod1->centerX + mod1Width / 2., mod2->centerX + mod2Width / 2.);
            double max_yd = std::max(mod1->centerY - mod1Height / 2., mod2->centerY - mod2Height / 2.);
            double min_yu = std::min(mod1->centerY + mod1Height / 2., mod2->centerY + mod2Height / 2.);

            overlappedWidth = min_xr - max_xl;
            overlappedHeight = min_yu - max_yd;

            if ( overlappedWidth > 0. && overlappedHeight > 0. ) {
                overlapArea += overlappedWidth * overlappedHeight;
            }
        }
        if ( modules_[i]->fixed ) {
            continue;
        }
        double inflatedArea = modules_[i]->currentArea - modules_[i]->area;
        std::cout << modules_[i]->name << ":\t";
        printf("inflation = %10.2lf(%3.2lf%%),\toverlap = %10.2lf", inflatedArea, inflatedArea / modules_[i]->area, overlapArea);
        std::cout << "\t==> ";
        if ( inflatedArea >= overlapArea / 2 ) {
            std::cout << "REMOVABLE" << std::endl;
        }
        else {
            std::cout << "UNREMOVABLE" << std::endl;
        }
        totalInflation += inflatedArea;
        totalOverlap += overlapArea;
    }
    totalOverlap /= 2;
    std::cout << "total inflation = " << totalInflation << std::endl;
    std::cout << "total overlap =   " << totalOverlap << std::endl;
    if ( totalInflation > totalOverlap ) {
        std::cout << "==> REMOVABLE" << std::endl;
    }
    else {
        std::cout << "==> UNREMOVABLE" << std::endl;
    }
}

bool GlobalSolver::exportGlobalFloorplan(GlobalResult &result) {
    /*
    int blockCount;
    int connectionCount;
    int chipWidth, chipHeight;

    std::vector<GlobalResultBlock> blocks;
    std::vector<GlobalResultConnection> connections;

    struct GlobalResultBlock{
        std::string name;
        std::string type;

        int legalArea;
        int llx, lly;
        int width, height;
    };

    struct GlobalResultConnection{
        std::vector<std::string> vertices;
        double weight;
    };
    */
    for ( GlobalModule *mod : modules_ ) {
        mod->updateCord(( int ) DieWidth_, ( int ) DieHeight_);
    }
    result.blockCount = moduleNum_;
    result.connectionCount = connectionNum_;
    result.chipWidth = DieWidth_;
    result.chipHeight = DieHeight_;

    GlobalResultBlock tmp;
    for ( GlobalModule *mod : modules_ ) {
        tmp.name = mod->name;
        tmp.type = ( mod->fixed ) ? "FIXED" : "SOFT";
        tmp.llx = (int) mod->x;
        tmp.lly = (int) mod->y;
        tmp.legalArea = mod->area;
        tmp.width = mod->width;
        tmp.height = mod->height;
        result.blocks.push_back(tmp);
    }

    for ( ConnectionInfo *conn : connectionList_ ) {
        GlobalResultConnection tmpC;
        for ( std::string name : conn->modules ) {
            tmpC.vertices.push_back(name);
        }
        tmpC.weight = conn->value;
        result.connections.push_back(tmpC);
    }

    return (isAreaLegal() && isAspectRatioLegal());
}