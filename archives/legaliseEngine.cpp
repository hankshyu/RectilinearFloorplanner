#include <vector>
#include <unordered_map>

#include "legaliseEngine.h"
#include "cSException.h"
#include "rectilinear.h"
#include "colours.h"


double LegaliseEngine::scalingFunction(double input, int base) const {
    if(input >= 1) return 1;
    else if(input <= 0) return 0;
    return (base <= 1)? input : (pow(base, input) - 1) / double(base - 1);
}

LegaliseEngine::LegaliseEngine(Floorplan *floorplan){
    this->fp = floorplan;
	minSoftRectilinearRequiredArea = AREA_T_MAX;
	maxSoftRectilinearRequiredArea = AREA_T_MIN;

	for(Rectilinear *const &rt : floorplan->softRectilinears){
		area_t legalArea = rt->getLegalArea();
		if(legalArea > maxSoftRectilinearRequiredArea) maxSoftRectilinearRequiredArea = legalArea;
		if(legalArea < minSoftRectilinearRequiredArea) minSoftRectilinearRequiredArea = legalArea;
	}

    this->criticalitySum = (LEGAL_CRITICALITY_WEIGHT + UTILIZATION_CRITICALITY_WEIGHT + ASPECT_RATIO_CRITICALITY_WEIGHT +
                            BORDER_CRITICALITY_WEIGHT + SIZE_CRITICALITY_WEIGHT + AREA_CRITICALITY_WEIGHT);
}

void LegaliseEngine::distributeOverlap() const {
    if(fp->overlapTilePayload.empty()) return;

    // If the overlap belongs to preplaced Rectilinear, we must distribute the overlap to preplaced moduele
    for(Rectilinear *const &rt : fp->preplacedRectilinears){
        if(rt->overlapTiles.empty()) continue;

        // the overlap tile must belong to the preplaced rectilinear
        std::vector<Tile *> overlapTiles (rt->overlapTiles.begin(), rt->overlapTiles.end());
        for(Tile *const &overlapTile : overlapTiles){

            std::vector<Rectilinear *> sacrifiseRects (fp->overlapTilePayload[overlapTile].begin(), fp->overlapTilePayload[overlapTile].end());
            for(Rectilinear *const &sacrificeRT : sacrifiseRects){
                if(sacrificeRT != rt) {
                    fp->decreaseTileOverlap(overlapTile, sacrificeRT);
                }
            }
        }
        fp->reshapeRectilinear(rt);
    }

    // overlap distribution
    while(!fp->overlapTilePayload.empty()){

        std::unordered_map<Rectilinear *, double> rectToUrgency;

        double maxUrgency = 0;
        Tile *maxUrgencyTile;
        
        for(std::unordered_map<Tile *, std::vector<Rectilinear *>>::iterator it = fp->overlapTilePayload.begin(); it != fp->overlapTilePayload.end(); ++it){
            double tileCriticality = 0;
            Tile *overlapTile = it->first;
            for(Rectilinear *const &rt : it->second){
                std::unordered_map<Rectilinear *, double>::iterator rit = rectToUrgency.find(rt);
                double urgency;
                if(rit == rectToUrgency.end()){ // result not cached
                    double cacheValue;
                    urgency = calculateRectilinearUrgency(rt, overlapTile, cacheValue);
                    rectToUrgency[rt] = cacheValue;

                }else{ // cache hit
                    double cachedValue = rit->second;

                    double legalSubUrgency = calculateLegalSuburgency(rt, overlapTile);
                    urgency = legalSubUrgency * LEGAL_URGENCY_WEIGHT;

                    double overlapRatioSubUrgency = calculateOverlapRatioSubUrgency(rt, overlapTile);
                    overlapRatioSubUrgency = scalingFunction(overlapRatioSubUrgency, OVERLAPRATIO_SCALING_FACTOR);

                    urgency += overlapRatioSubUrgency*OVERLAPRATIO_URGENCY_WEIGHT;
                }

                tileCriticality += urgency;
                // std::cout << "Judging OverlapTile: " << *overlapTile << " on Rectilinear = " << rt->getName() << std::endl;
                // rectilinearIllegalType ril;
                // std::cout << "Legal: " << calculateLegalSuburgency(rt, overlapTile ,ril) << " ,Reason = " << ril << std::endl;
                // std::cout << "Utilization: " << calculateUtilizationSubUrgency(rt) << std::endl;
                // std::cout << "Aspect Ratio: " << calculateAspectRatioSubUrgency(rt) << std::endl;
                // std::cout << "Overlap Ratio: " << calculateOverlapRatioSubUrgency(rt, overlapTile) << std::endl;
                // std::cout << "Border Situation: " << calculateBorderSubUrgency(rt) << std::endl;
                // std::cout << "Centre: " << calcualteCentreSubUrgency(rt) << std::endl;
                // std::cout << "Size: " << calculateSizeSubUrgency(rt) << std::endl;
                // std::cout << "--------------------" << std::endl;
                // std::cout << "Total Evaluation: " << calculateRectilinearUrgency(rt, overlapTile) << std::endl;
                // std::cout << "--------------------" << std::endl << std::endl;
            }
            if(tileCriticality > maxUrgency){
                maxUrgency = tileCriticality;
                maxUrgencyTile = overlapTile;
            }
        }

        // know which tile to remove, decide which rectilinear gets the overlap tile
        double maxRectUrgency = 0;
        Rectilinear *candRect;
        for(Rectilinear *const &crt : fp->overlapTilePayload[maxUrgencyTile]){
            double cachedValue = rectToUrgency[crt];

            double legalSubUrgency = calculateLegalSuburgency(crt, maxUrgencyTile);
            double urgency = legalSubUrgency * LEGAL_URGENCY_WEIGHT;

            double overlapRatioSubUrgency = calculateOverlapRatioSubUrgency(crt, maxUrgencyTile);
            overlapRatioSubUrgency = scalingFunction(overlapRatioSubUrgency, OVERLAPRATIO_SCALING_FACTOR);

            urgency += overlapRatioSubUrgency*OVERLAPRATIO_URGENCY_WEIGHT; 
            if(urgency > maxRectUrgency){
                candRect = crt;
                maxRectUrgency = urgency;
            } 
        }

        // distribute the overlap tile to the the most critical Rectilinear
        std::vector<Rectilinear *> toRemoveRects (fp->overlapTilePayload[maxUrgencyTile]);
        
        for(Rectilinear *const &crt : toRemoveRects){
            if(crt != candRect){
                fp->decreaseTileOverlap(maxUrgencyTile, crt);
            } 
        }
        fp->reshapeRectilinear(candRect);
    }

}

void LegaliseEngine::huntFixAtypicalShapes() const {
    for(Rectilinear *const rt : fp->softRectilinears){
        if(!rt->isLegalOneShape()){
            std::cout << "[HuntFixAtypicalShape] " << rt->getName() << ": ONE_SHAPE issue deteced and fixed" << std::endl;
            fixTwoShapeRectilinear(rt);
        } 
        if(!rt->isLegalNoHole()){
            std::cout << "[HuntFixAtypicalShape] " << rt->getName() << ": HOLE issue deteced and fixed" << std::endl;
            fixHoleRectilinear(rt);
        } 
    }
}
 
void LegaliseEngine::fixTwoShapeRectilinear(Rectilinear *rect) const {

    using namespace boost::polygon::operators;
    DoughnutPolygonSet reshapePart;

    for(Tile *const &t : rect->blockTiles){
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
        if(i != largestPartIdx) toRemoveParts.push_back(reshapePart[i]);
    }
    fp->shrinkRectilinear(toRemoveParts, rect);
}

void LegaliseEngine::fixHoleRectilinear(Rectilinear *rect) const {
    area_t largestArea = AREA_T_MIN;
    Tile *largestTile;
    std::vector<Tile *>allTiles; 
    for(Tile *const &t : rect->blockTiles){
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
            fp->decreaseTileOverlap(t, rect);
        }
    }
    
    
}

void LegaliseEngine::softRectilinearIllegalReport() const {
    std::cout << "Soft Rectilinear illegal Report: " << std::endl;
    for(Rectilinear *const rt : fp->softRectilinears){
        std::cout << rt->getName() << ", residual area = " <<  rt->calculateActualArea() - rt->getLegalArea() << " ";
        std::cout << ", Legal situation = ";
        bool nohole = rt->isLegalNoHole();
        bool twoshape = rt->isLegalOneShape();
        bool utilization = rt->isLegalUtilization();
        bool aspectRatio = rt->isLegalAspectRatio();
        bool overlap = rt->isLegalNoOverlap();
        bool area = rt->isLegalEnoughArea();
        if(!nohole) std::cout << RED << "HOLE_ERROR, " << COLORRST;
        if(!twoshape) std::cout << RED << "TWO_SHPAE, " << COLORRST;
        if(!overlap) std::cout << RED << "OVERLAP, " << COLORRST;
        if(!utilization) std::cout <<  CYAN << "UTILIZATION, " << COLORRST;
        if(!aspectRatio) std::cout <<  CYAN << "ASPECT_RATIO, " << COLORRST;
        if(!area) std::cout <<  CYAN << "AREA, " << COLORRST;
        
        rectilinearIllegalType rtill;
        if(rt->isLegal(rtill)){
            std::cout << GREEN << "LEGAL !!" << COLORRST;
        }
        std::cout<< std::endl;

    }
}

double LegaliseEngine::calculateRectilinearUrgency(Rectilinear *rect, Tile *candTile, double &cacheValue) const {

	double legalSubUrgency = calculateLegalSuburgency(rect, candTile);

    double overlapRatioSubUrgency = calculateOverlapRatioSubUrgency(rect, candTile);
    overlapRatioSubUrgency = scalingFunction(overlapRatioSubUrgency, OVERLAPRATIO_SCALING_FACTOR);

    double aspectRatioSubUrgency = calculateAspectRatioSubUrgency(rect);
    aspectRatioSubUrgency = scalingFunction(aspectRatioSubUrgency, ASPECTRATIO_SCALING_FACTOR);

    double borderSubUrgency = calculateBorderSubUrgency(rect);
    borderSubUrgency = scalingFunction(borderSubUrgency, BORDER_SCALING_FACTOR);

    double centreSubUrgency = calcualteCentreSubUrgency(rect);
    centreSubUrgency = scalingFunction(centreSubUrgency, CENTRE_SCALING_FACTOR);

    double sizeSubUrgency = calculateSizeSubUrgency(rect);
    sizeSubUrgency = scalingFunction(sizeSubUrgency, SIZE_SCALING_FACTOR);

    cacheValue = (aspectRatioSubUrgency * ASPECTRATIO_URGENCY_WEIGHT) + (borderSubUrgency * BORDER_URGENCY_WEIGHT) 
		+ (centreSubUrgency * CENTRE_URGENCY_WEIGHT) + (sizeSubUrgency * SIZE_URGENCY_WEIGHT);

    return (legalSubUrgency * LEGAL_URGENCY_WEIGHT) + (overlapRatioSubUrgency * OVERLAPRATIO_URGENCY_WEIGHT) + cacheValue;
}

bool LegaliseEngine::legalise() const {
    int highTmpIterations = int(MAX_LEGALIZE_ITERATION * HIGH_TMP_RATIO);
    int lowTmpIteration = int(MAX_LEGALIZE_ITERATION * (1 - LOW_TMP_RATIO));
    int legaliseIteration = 0;
    while ((legaliseIteration++) < MAX_LEGALIZE_ITERATION){
        std::unordered_set<Rectilinear*> illegalTargets;
        for(Rectilinear *const &rt : fp->softRectilinears){
            rectilinearIllegalType rit;
            if(!rt->isLegal(rit)) illegalTargets.insert(rt);
        }
        // rate all Targets's urgency
        std::unordered_set<Rectilinear*>iterationLocked; 
        std::unordered_map<Rectilinear *, double> allRectilinearUrgency;
        // rate all Rectilinear's criticality
        for(Rectilinear *const &rect : fp->softRectilinears){
            allRectilinearUrgency[rect] = rateRectilinearCriticality(rect);
        }

        while(!illegalTargets.empty()){
            // pick the candidate target Rectilinear to legalise
            Rectilinear *targetRect = *(illegalTargets.begin());
            double maxUrgency = allRectilinearUrgency[targetRect];
            for(Rectilinear *const &rect : illegalTargets){
                double rectUrgency = allRectilinearUrgency[rect];
				// std::cout << "Seleting: " << rect->getName() << " has urgency" << rectUrgency << std::endl;
                if(rectUrgency > maxUrgency){
                    maxUrgency = rectUrgency;
                    targetRect = rect;
                }
            }
            // Start Legalise the Rectilinear
            // std::cout << "Chosen target = " << targetRect->getName() << std::endl;
            std::unordered_set<Rectilinear *> updateUrgency;

            /* STEP 1: grow the blank tiles around the bounding box */
            bool keepMoving = true;
            area_t targetArea = targetRect->getLegalArea();
			double utilizationTolerMinStep1 = fp->getGlobalUtilizationMin() * UTILIZATION_TOLERATNCE_PCT_STEP1;
			
            while((targetRect->calculateActualArea() < targetArea) && keepMoving && (targetRect->calculateUtilization() >= utilizationTolerMinStep1)){
                keepMoving = false;
                std::vector<direction2D> validGrowDirections;
                std::vector<double> growScore;
                evaluateBlankGrowingDirection(targetRect, validGrowDirections, growScore);

                while(!validGrowDirections.empty()){
					// std::cout << "Attemp to grow blank at: " << validGrowDirections[0] << " direction ";
                    bool gorwSuccess = growBlank(targetRect, validGrowDirections[0], growScore[0], updateUrgency);
					// if(!gorwSuccess) std::cout << " failed " << std::endl;
					// else{
					// 	std::cout << GREEN << "Success !" << COLORRST << std::endl;;
					// }

					if(gorwSuccess){
						keepMoving = true;
						// check if area is enough
						if(targetRect->calculateActualArea() >= targetArea) break;
					} 
					
					validGrowDirections.erase(validGrowDirections.begin());
					growScore.erase(growScore.begin());
                }
            }
            // update affected REctilinear's criticality
            for(Rectilinear *const &rt : updateUrgency){
                allRectilinearUrgency[rt] = rateRectilinearCriticality(rt);
            }
			updateUrgency.clear();

			// check if the targetRect is Legal, it it is, hop on to the next Rectilinear target
            rectilinearIllegalType stepOneRIT;
			if(targetRect->isLegal(stepOneRIT)){
				illegalTargets.erase(targetRect);
				iterationLocked.insert(targetRect);
				continue;
			}


			/* STEP2 : grow the tiles within the boundinb box */
			bool stepTwoResult = growWithinBoundingBox(targetRect, iterationLocked, allRectilinearUrgency, updateUrgency, 1.0);
			// if(stepTwoResult){
			// 	std::cout << GREEN << "STEP TWO SUCCESS" << COLORRST << std::endl;
			// }else{
			// 	std::cout << RED << "STEP TWO FAIL" << COLORRST << std::endl;

			// }
            // update affected REctilinear's criticality 
            for(Rectilinear *const &rt : updateUrgency){
                allRectilinearUrgency[rt] = rateRectilinearCriticality(rt);
            }
			updateUrgency.clear();

			// check if the targetRect is Legal, it it is, hop on to the next Rectilinear target
            rectilinearIllegalType stepTwoRIT;
			if(targetRect->isLegal(stepTwoRIT)){
                illegalTargets.erase(targetRect);
                iterationLocked.insert(targetRect);
				continue;
			}

            /* STEP3 : grow on all directions if possible */
            double utilizationTolerMinStep3 = fp->getGlobalUtilizationMin() * UTILIZATION_TOLERATNCE_PCT_STEP3; 
            keepMoving = true;
            while((targetRect->calculateActualArea() < targetArea) && keepMoving && (targetRect->calculateUtilization() >= utilizationTolerMinStep3)){
                keepMoving = false;
                std::vector<direction2D> validGrowDirections;
                std::vector<double> growScore;
                evaluateViolentGrowingDirection(targetRect, validGrowDirections, growScore, allRectilinearUrgency, iterationLocked);
                // std::cout << "Done evaluating Violent Growing (" << validGrowDirections.size() << ")" << std::endl;
                // for(int i = 0; i < validGrowDirections.size(); ++i){
                //     std::cout << validGrowDirections[i] << " -> " << growScore[i] << std::endl;
                // }
                // std::cout << *targetRect << "aspect Ratio = " << rec::calculateAspectRatio(targetRect->calculateBoundingBox()) << std::endl;

                while(!validGrowDirections.empty()){
                    bool growSuccess = growViolent(targetRect, validGrowDirections[0], growScore[0], updateUrgency, allRectilinearUrgency, iterationLocked);
                    // if(growSuccess){
                    //     std::cout << GREEN << "STEP Three SUCCESS" << COLORRST << std::endl;
                    // }else{
                    //     std::cout << RED << "STEP Three FAIL" << COLORRST << std::endl;

                    // }
                    if(growSuccess){
                        keepMoving = true;
                        if(targetRect->calculateActualArea() >= targetArea) break;
                    }
                    validGrowDirections.erase(validGrowDirections.begin());
                    growScore.erase(growScore.begin());
                }
                
            }

            // update affected REctilinear's criticality
            for(Rectilinear *const &rt : updateUrgency){
                allRectilinearUrgency[rt] = rateRectilinearCriticality(rt);
            }
			updateUrgency.clear();

			// check if the targetRect is Legal, it it is, hop on to the next Rectilinear target
            rectilinearIllegalType stepThreeRIT;
			if(targetRect->isLegal(stepThreeRIT)){
				illegalTargets.erase(targetRect);
				iterationLocked.insert(targetRect);
				continue;
			}

            
            // /* STEP 4: IF ASPECTRAIO is BAD, Directly trim off the bad part, only inititate at lower temperature */
            // if(legaliseIteration > lowTmpIteration){
            //     if(!targetRect->isLegalAspectRatio()){               
            //         forceFixAspectRatio(targetRect, updateUrgency);
                    
            //         // update affected REctilinear's criticality
            //         for(Rectilinear *const &rt : updateUrgency){
            //             allRectilinearUrgency[rt] = rateRectilinearCriticality(rt);
            //         }
            //         updateUrgency.clear();

            //         // check if the targetRect is Legal, it it is, hop on to the next Rectilinear target
            //         rectilinearIllegalType stepThreeRIT;
            //         if(targetRect->isLegal(stepThreeRIT)){
            //             illegalTargets.erase(targetRect);
            //             iterationLocked.insert(targetRect);
            //             continue;
            //         }
            //     }
            // }

            // /* STEP 5: IF Utilization is BAD, Directly tirm off the bad part, only inititate at lower temperature */
            // if(legaliseIteration > lowTmpIteration){
            //     if(!targetRect->isLegalUtilization()){
            //         forceFixUtilization(targetRect, updateUrgency);
            //         // update affected REctilinear's criticality
            //         for(Rectilinear *const &rt : updateUrgency){
            //             allRectilinearUrgency[rt] = rateRectilinearCriticality(rt);
            //         }
            //         updateUrgency.clear();

            //         // check if the targetRect is Legal, it it is, hop on to the next Rectilinear target
            //         rectilinearIllegalType stepThreeRIT;
            //         if(targetRect->isLegal(stepThreeRIT)){
            //             illegalTargets.erase(targetRect);
            //             iterationLocked.insert(targetRect);
            //             continue;
            //         }
            //     }
            // }

            // /* STEP 6: */
            // forceGrowAlongBorder(targetRect, updateUrgency);

            

            illegalTargets.erase(targetRect);
            iterationLocked.insert(targetRect);
        }

    }

    // fail to legalise
    return false;    
}

double LegaliseEngine::rateRectilinearCriticality(Rectilinear *rect) const {
    double legalCriticality = rateRectilinearLegalCriticality(rect);
    
    double utilizationCriticality = calculateUtilizationSubUrgency(rect);
    utilizationCriticality = scalingFunction(utilizationCriticality, UTILIZATION_SCALING_FACTOR);

    double aspectRatioCriticality = calculateAspectRatioSubUrgency(rect);
    aspectRatioCriticality = scalingFunction(aspectRatioCriticality, ASPECTRATIO_SCALING_FACTOR);

    double borderCriticality = calculateBorderSubUrgency(rect);
    borderCriticality = scalingFunction(borderCriticality, BORDER_SCALING_FACTOR);

    double sizeCriticality = calculateSizeSubUrgency(rect);
    sizeCriticality = scalingFunction(sizeCriticality, SIZE_SCALING_FACTOR);

    double areaCriticality = rateRectilinearAreaCriticality(rect);
    
    return (legalCriticality * LEGAL_CRITICALITY_WEIGHT) + (utilizationCriticality * UTILIZATION_CRITICALITY_WEIGHT) 
        + (aspectRatioCriticality * ASPECT_RATIO_CRITICALITY_WEIGHT) + (borderCriticality * BORDER_CRITICALITY_WEIGHT)
        + (sizeCriticality * SIZE_CRITICALITY_WEIGHT) + (areaCriticality * AREA_CRITICALITY_WEIGHT);
}

bool LegaliseEngine::fillBoundingBox(Rectilinear *rect) const {
    using namespace boost::polygon::operators;
    area_t residualArea = rect->getLegalArea() - rect->calculateActualArea();
    Rectangle boundingBox = rect->calculateBoundingBox();
    DoughnutPolygonSet blankPart;
    blankPart += boundingBox;

    std::vector<Tile *>edaAllTiles;
    fp->cs->enumerateDirectedArea(boundingBox, edaAllTiles);

    for(Tile *const &tile : edaAllTiles){
        blankPart -= tile->getRectangle();
    }

    // if no empty space, return
    if(boost::polygon::empty(blankPart)) return false;
    
    // dps is now the blank spaces within the bounding box;
    DoughnutPolygonSet rectOrig; 
    std::vector<DoughnutPolygon> toAdd;
    for(Tile *const &tile : rect->blockTiles){
        rectOrig += tile->getRectangle();
    }

    for(int i = 0; i< blankPart.size(); ++i){
        DoughnutPolygon blankSegment = blankPart[i];
        toAdd.push_back(blankSegment);
        rectOrig += blankSegment;
    }
    
    // if adding the would not cause strange shapes, execute 
    if(dps::noHole(rectOrig) && dps::oneShape(rectOrig)){
        for(int i = 0; i < blankPart.size(); ++i){
            fp->growRectilinear(toAdd, rect);
        }
        return true;
    }else{
        return false;
    }
    
}

void LegaliseEngine::evaluateBlankGrowingDirection(Rectilinear *rect, std::vector<direction2D> &growOrder, std::vector<double> &growScore) const {
    // [0, 1, 2, 3] -> [UP, RIGHT, DOWN, LEFT]
    double growRating[4];
    Rectangle boundingBox = rect->calculateBoundingBox();
    len_t bbWidth = rec::getWidth(boundingBox);
    len_t bbHeight = rec::getHeight(boundingBox);
    

    len_t BBXL = rec::getXL(boundingBox);
    len_t BBXH = rec::getXH(boundingBox);
    len_t BBYL = rec::getYL(boundingBox);
    len_t BBYH = rec::getYH(boundingBox);


    double aspectRatioMax = fp->getGlobalAspectRatioMax();
    bool verticalGrowValid = (double(bbHeight + 1) / double(bbWidth)) < aspectRatioMax; 
    bool horizontalGrowValid = (double(bbWidth + 1) / double(bbHeight)) < aspectRatioMax;

    bool growUpValid = (BBYH != fp->cs->getCanvasHeight()) && verticalGrowValid; 
    bool growRightValid = (BBXH != fp->cs->getCanvasWidth()) && horizontalGrowValid; 
    bool growDownValid = (BBYL != 0) && verticalGrowValid;
    bool growLeftValid = (BBXL != 0) && horizontalGrowValid;

    if(growUpValid){
        Rectangle upProbe =  Rectangle(BBXL, BBYH, BBXH, BBYH + 1);
        growRating[0] = evalueBlankGrowingSingleDirection(upProbe, area_t(bbWidth));
    }else{
        growRating[0] = 0;
    }

    if(growRightValid){
        Rectangle rightProbe = Rectangle(BBXH, BBYL, BBXH + 1, BBYH);
        growRating[1] = evalueBlankGrowingSingleDirection(rightProbe, area_t(bbHeight));
    }else{
        growRating[1] = 0;
    }

    if(growDownValid){
        Rectangle downProbe = Rectangle(BBXL, BBYL - 1, BBXH, BBYL);
        growRating[2] = evalueBlankGrowingSingleDirection(downProbe, area_t(bbWidth));
    }else{
        growRating[2] = 0;
    }

    if(growLeftValid){
        Rectangle leftProbe = Rectangle(BBXL - 1, BBYL, BBXL, BBYH);
        growRating[3] = evalueBlankGrowingSingleDirection(leftProbe, area_t(bbHeight));
    }else{
        growRating[3] = 0;
    }
    // Rectangle rectself = rect->calculateBoundingBox();
    // std::cout << "Rectilinear " << rect->getName() << "BB W, H = " << rec::getWidth(rectself) << " " << rec::getHeight(rectself) << " ";
    // printf("Rating (up, right, down, left) = (%f, %f, %f, %f\n", growRating[0], growRating[1], growRating[2], growRating[3]);

    while (!((growRating[0] == 0) && (growRating[1] == 0) && (growRating[2] == 0) && (growRating[3] == 0))){
        int maxIdx = 0;
        double maxRating = growRating[0];
        for(int i = 1; i < 4; ++i){
            double rating = growRating[i];
            if(rating > maxRating){
                maxIdx = i;
                maxRating = rating;
            }
        }

        if(maxRating != 0){
            growScore.push_back(maxRating);

            if(maxIdx == 0) growOrder.push_back(direction2D::UP);
            else if(maxIdx == 1) growOrder.push_back(direction2D::RIGHT);
            else if(maxIdx == 2) growOrder.push_back(direction2D::DOWN);
            else growOrder.push_back(direction2D::LEFT);

            growRating[maxIdx] = 0;
        }
    }

}

bool LegaliseEngine::growBlank(Rectilinear *rect, direction2D direction, double rating, std::unordered_set<Rectilinear *> &updateUrgency) const {
	area_t residualArea = rect->getLegalArea() - rect->calculateActualArea();
    Rectangle boundingBox = rect->calculateBoundingBox();
	len_t boundingBoxXL = rec::getXL(boundingBox);
	len_t boundingBoxXH = rec::getXH(boundingBox);
	len_t boundingBoxYL = rec::getYL(boundingBox);
	len_t boundingBoxYH = rec::getYH(boundingBox);
    len_t boundingBoxWidth = rec::getWidth(boundingBox);
    len_t boundingBoxHeight = rec::getHeight(boundingBox);
    double aspectRatioMax = fp->getGlobalAspectRatioMax();
	double utilizationMin = fp->getGlobalUtilizationMin();

	Rectangle canvasProto = fp->getChipContour();
	len_t canvasXH = rec::getXH(canvasProto);
	len_t canvasYH = rec::getYH(canvasProto);

    // decide how the growing area should look like
	Rectangle bluePrintRect;
    if((direction == direction2D::UP) || (direction == direction2D::DOWN)){
		len_t bluePrintWidth = boundingBoxWidth;
		len_t bluePrintHeight;
		len_t heightUpperLimit = len_t((double(boundingBoxWidth) * aspectRatioMax) - boundingBoxHeight);
		if(heightUpperLimit <= 0) return false;
		assert(heightUpperLimit > 0);
		
		if(rating >= utilizationMin){
			double dHeight = double(residualArea) / double(boundingBoxWidth * rating);
			bluePrintHeight = len_t(dHeight);

			if((dHeight - bluePrintHeight) > 0) bluePrintHeight++;
			if(bluePrintHeight > heightUpperLimit) bluePrintHeight = heightUpperLimit;
		}else{
			double targetArea = residualArea * scalingFunction(rating/utilizationMin, GROW_BLANK_SCLING_FACTOR);

			double dHeight = targetArea / double(boundingBoxWidth * rating);
			bluePrintHeight = area_t(dHeight);
			if(dHeight - bluePrintHeight > 0) bluePrintHeight++;
			if(bluePrintHeight > heightUpperLimit) bluePrintHeight = heightUpperLimit;
		}

		if(direction == direction2D::UP){
			len_t bluePrintTop = (boundingBoxYH + bluePrintHeight);
			if(bluePrintTop > canvasYH) bluePrintTop = canvasYH;
			if(boundingBoxYH >= bluePrintTop) return false;
			bluePrintRect = Rectangle(boundingBoxXL, boundingBoxYH, boundingBoxXH, bluePrintTop);
		}else{ // direction2D::DOWN
			len_t bluePrintDown = (boundingBoxYL - bluePrintHeight);
			if(bluePrintDown < 0) bluePrintDown = 0;
			if(bluePrintDown >= boundingBoxYL) return false;
			bluePrintRect = Rectangle(boundingBoxXL, bluePrintDown, boundingBoxXH, boundingBoxYL);
		}

    }else{ // direction == direction2D::LEFT or direction::RIGHT
        len_t bluePrintHeight = boundingBoxHeight;
		len_t bluePrintWidth;
		len_t widthUpperLimit = len_t((double(boundingBoxHeight) * aspectRatioMax) - boundingBoxWidth);
		if(widthUpperLimit <= 0) return false;
		assert(widthUpperLimit > 0);

		if(rating >= utilizationMin){
			double dWidth = double(residualArea) / double(boundingBoxHeight * rating);
			bluePrintWidth = len_t(dWidth);

			if((dWidth - bluePrintWidth) > 0) bluePrintWidth++;
			if(bluePrintWidth > widthUpperLimit) bluePrintWidth = widthUpperLimit;
		}else{
			double targetArea = residualArea * scalingFunction(rating/utilizationMin, GROW_BLANK_SCLING_FACTOR);
			double dWidth = targetArea / double(boundingBoxHeight * rating);
			bluePrintWidth = len_t(dWidth);

			if((dWidth - bluePrintWidth) > 0) bluePrintWidth++;
			if(bluePrintWidth > widthUpperLimit) bluePrintWidth = widthUpperLimit;
		}

		if(direction == direction2D::RIGHT){
			len_t bluePrintRight = (boundingBoxXH + bluePrintWidth);
			if(bluePrintRight > canvasXH) bluePrintRight = canvasXH;
			if(boundingBoxXH >= bluePrintRight) return false;
			bluePrintRect = Rectangle(boundingBoxXH, boundingBoxYL, bluePrintRight, boundingBoxYH);
		}else{ // direction2D::LEFT
			len_t bluePrintLeft = (boundingBoxXL - bluePrintWidth);
			if(bluePrintLeft < 0) bluePrintLeft = 0;
			if(bluePrintLeft >= boundingBoxXL) return false;
			bluePrintRect = Rectangle(bluePrintLeft, boundingBoxYL, boundingBoxXL, boundingBoxYH);
		}
    }
	// std::cout << "BluePrintRect = " << bluePrintRect << std::endl;

	using namespace boost::polygon::operators;
	DoughnutPolygonSet bluePrintDps;
	bluePrintDps += bluePrintRect;

	std::vector<Tile *> debris;
	fp->cs->enumerateDirectedArea(bluePrintRect, debris);
	for(Tile *const &dbs : debris){
		bluePrintDps -= dbs->getRectangle();
		// std::cout << "The tile removed is " << *dbs << ", belongs to " << fp->blockTilePayload[dbs]->getName() << ", currentSize = " << bluePrintDps.size() << std::endl;
	}

	// now bluePrintDps contains all the blank spaces in the blue print space that we would like to grow

	// if noting to add, just return;
	int bluePrintSize = bluePrintDps.size();
	if(bluePrintSize == 0){
		// std::cout << "fail due to all filled space";
		return false;
	}
	bool hasMovements = true;
	bool hasDebrisGrow = false;

	DoughnutPolygonSet actualShape;
	bool acceptTable[bluePrintSize];
	for(int i = 0; i < bluePrintSize; ++i){
		acceptTable[i] = false;
	}

	for(Tile *const &t : rect->blockTiles){
		actualShape += t->getRectangle();
	}
	
	while(hasMovements){
		hasMovements = false;
		for(int i = 0; i < bluePrintSize; ++i){
			if(acceptTable[i]) continue;

			DoughnutPolygonSet testShape(actualShape);
			testShape += bluePrintDps[i];
			// 
			if(dps::oneShape(testShape) && dps::noHole(testShape)){ // the shape is valid
				hasDebrisGrow = true;
				hasMovements = true;
				actualShape = testShape;
				acceptTable[i] = true;
				break;	
			}
		}			
	}

	// no part is legal, just return
	if(!hasDebrisGrow){
		// std::cout << "failed due to no legal shape";
		return false;
	}

	// add the accepted peices according to the accept table
	std::vector<DoughnutPolygon> toGrow;
	for(int i = 0; i < bluePrintSize; ++i){
		if(acceptTable[i]) toGrow.push_back(bluePrintDps[i]);
	}

	fp->growRectilinear(toGrow, rect);
	
	// update the Rectilinears that touches the bluePirntRect;
	len_t bluePirntXL = rec::getXL(bluePrintRect);
	len_t bluePirntXH = rec::getXH(bluePrintRect);
	len_t bluePrintYL = rec::getYL(bluePrintRect);
	len_t bluePrintYH = rec::getYH(bluePrintRect);

	len_t updateRectXL = (bluePirntXL == 0)? 0 : (bluePirntXL - 1);
	len_t updateRectXH = (bluePirntXH == canvasXH)? canvasXH : (bluePirntXH + 1);
	len_t updateRectYL = (bluePrintYL == 0)? 0 : (bluePrintYL - 1);
	len_t updateRectYH = (bluePrintYH == canvasYH)? canvasYH : (bluePrintYH + 1);
	
	std::vector<Tile *> updateCands;
	fp->cs->enumerateDirectedArea(Rectangle(updateRectXL, updateRectYL, updateRectXH, updateRectYH), updateCands);
	for(Tile *const &t : updateCands){
		updateUrgency.insert(fp->blockTilePayload[t]);
	}

	return true;
}

bool LegaliseEngine::growWithinBoundingBox(Rectilinear *rt, std::unordered_set<Rectilinear *> &iterationLocked, 
std::unordered_map<Rectilinear *, double> &allRectilinearUrgency, std::unordered_set<Rectilinear *> &updateUrgency, double urgencyMultiplier) const {

	using namespace boost::polygon::operators;
	double utilizationMin = fp->getGlobalUtilizationMin();
	double alterUrgencyMax = allRectilinearUrgency[rt] * urgencyMultiplier;

	Rectangle targetBB = rt->calculateBoundingBox();
	DoughnutPolygonSet petriDishDPS, blankDPS;
	petriDishDPS += targetBB;
	blankDPS += targetBB;

	// find all tiles that crosssect the area, find it and sieve those unfit
	std::vector<Tile *> tilesinPetriDish;	
	fp->cs->enumerateDirectedArea(targetBB, tilesinPetriDish);
	std::unordered_map<Rectilinear *, std::vector<Tile *>> invlovedTiles;
	for(Tile *const &t : tilesinPetriDish){
		Rectilinear *tilesRectilinear = fp->blockTilePayload[t];
		double tilesRectilinearUrgency = allRectilinearUrgency[tilesRectilinear];
		Rectangle tileRectangle = t->getRectangle();
		blankDPS -= tileRectangle;
		if(tilesRectilinear->getType() == rectilinearType::PREPLACED){
			continue;
		}else if(iterationLocked.find(tilesRectilinear) != iterationLocked.end()){
			continue;
		}else if(tilesRectilinearUrgency > alterUrgencyMax){
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

	std::sort(involvedRectilinears.begin(), involvedRectilinears.end(), 
		[&](Rectilinear *A, Rectilinear *B) {return allRectilinearUrgency[A] < allRectilinearUrgency[B];});

	// start filling the candidates to grow	
	std::vector<DoughnutPolygon> growCandidates;
	std::vector<Rectilinear *> growCandidatesRectilinear;
	// start with the blank tiles (they have urgency 0)	
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
		rectDPS &= targetBB;
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
	if(growCandidateSize == 0) return false;
	
	// ready to actually grow those candiate fragments
	DoughnutPolygonSet currentShapeDPS;
	for(Tile *const &t : rt->blockTiles){
		currentShapeDPS += t->getRectangle();
	}

	std::unordered_map<Rectilinear *, DoughnutPolygonSet> rectCurrentShapeDPS;
	for(Rectilinear *const &ivr : involvedRectilinears){
		for(Tile *const &t : ivr->blockTiles){
			rectCurrentShapeDPS[ivr] += t->getRectangle();
		}
	}

	bool keepGrowing = true;		
	bool hasGrow = false;
	bool growCandidateSelect [growCandidateSize];
	for(int i = 0; i < growCandidateSize; ++i){
		growCandidateSelect[i] = false;
	}
	
	while((rt->calculateUtilization() < utilizationMin) && keepGrowing){
		keepGrowing = false;
		for(int i = 0; i < growCandidateSize; ++i){
			if(growCandidateSelect[i]) continue;

			DoughnutPolygon markedDP = growCandidates[i];
			// test if installing this marked part would turn the current shape strange

			DoughnutPolygonSet xSelfDPS(currentShapeDPS);
			xSelfDPS += markedDP;
			if(!(dps::oneShape(xSelfDPS) && dps::noHole(xSelfDPS))){
				// intrducing the markedDP would cause strange shape
				continue;
			}

			// test if pulling off the marked part would harm the victim too bad, skip if markedDP belongs to blank
			Rectilinear *victimRect = growCandidatesRectilinear[i];
			if(victimRect != nullptr){
				// markedDP belongs to other Rectilinear
				DoughnutPolygonSet xVictimDPS(rectCurrentShapeDPS[victimRect]);
				xVictimDPS -= markedDP;

				if(xVictimDPS.empty() || (!dps::oneShape(xVictimDPS) || (!dps::noHole(xVictimDPS)))){
					continue;
				}
			}

			// pass all tests, distribute the tile
			growCandidateSelect[i] = true;
			keepGrowing = true;
			hasGrow = true;
            // BUG FIX: April 20th, -= -> +=
			// currentShapeDPS -= markedDP;
			currentShapeDPS += markedDP;
			DoughnutPolygonSet markedDPS;
			markedDPS += markedDP;
			if(victimRect != nullptr){ // markedDP belongs to other Rectilienar
				rectCurrentShapeDPS[victimRect] -= markedDP;
				// strip the area from victim
				fp->shrinkRectilinear(markedDPS, victimRect);
			}

			// grow the area to our target, rt
			fp->growRectilinear(markedDPS, rt);
		}

	}

	if(!hasGrow) return false;
	// update the Rectilinears that touches the boundingBox;
	len_t BoundingBoxXL = rec::getXL(targetBB);
	len_t BoundingBoxXH = rec::getXH(targetBB);
	len_t BoundingBoxYL = rec::getYL(targetBB);
	len_t BoundingBoxYH = rec::getYH(targetBB);	
	Rectangle canvasSnap = fp->getChipContour();
	len_t canvasXH = rec::getXH(canvasSnap);
	len_t canvasYH = rec::getYH(canvasSnap);	

	len_t updateRectXL = (BoundingBoxXL == 0)? 0 : (BoundingBoxXL - 1);
	len_t updateRectXH = (BoundingBoxXH == canvasXH)? canvasXH : (BoundingBoxXH + 1);
	len_t updateRectYL = (BoundingBoxYL == 0)? 0 : (BoundingBoxYL - 1);
	len_t updateRectYH = (BoundingBoxYH == canvasYH)? canvasYH : (BoundingBoxYH + 1);

	std::vector<Tile *> updateCands;
	fp->cs->enumerateDirectedArea(Rectangle(updateRectXL, updateRectYL, updateRectXH, updateRectYH), updateCands);
	for(Tile *const &t : updateCands){
		updateUrgency.insert(fp->blockTilePayload[t]);
	}

	return true;
}

void LegaliseEngine::evaluateViolentGrowingDirection(Rectilinear *rect, std::vector<direction2D> &growOrder, std::vector<double> &growScore,
std::unordered_map<Rectilinear *, double> &allRectilinearUrgency, std::unordered_set<Rectilinear *> &iterationLocked) const {
    Rectangle canvasProto = fp->getChipContour();
    len_t canvasXH = rec::getXH(canvasProto);
    len_t canvasYH = rec::getYH(canvasProto);
    // [0, 1, 2, 3] -> [UP, RIGHT, DOWN, LEFT]
    double growRating[4];
    Rectangle boundingBox = rect->calculateBoundingBox();
    len_t bbWidth = rec::getWidth(boundingBox);
    len_t bbHeight = rec::getHeight(boundingBox);
    
    len_t BBXL = rec::getXL(boundingBox);
    len_t BBXH = rec::getXH(boundingBox);
    len_t BBYL = rec::getYL(boundingBox);
    len_t BBYH = rec::getYH(boundingBox);

    

    area_t residualArea = rect->getLegalArea() - rect->calculateActualArea();
    double targetArea = residualArea * GROW_VIOLENT_MIN_PCT;
    double aspectRatioMax = fp->getGlobalAspectRatioMax();

    double dVerticalGrowth = targetArea / double(bbWidth);
    len_t verticalGrowth = (len_t) dVerticalGrowth;
    if((dVerticalGrowth - verticalGrowth) > 0)verticalGrowth++;

    double dVerticalGrowthMax = (bbWidth * aspectRatioMax) - bbHeight;
    len_t verticalGrowthMax = (len_t) dVerticalGrowthMax;
    if(verticalGrowth > verticalGrowthMax) verticalGrowth = verticalGrowthMax;

    double dhorizontalGrowth = targetArea / double(bbHeight);
    len_t horizontalGrowth = (len_t) dhorizontalGrowth;
    if((dhorizontalGrowth - horizontalGrowth) > 0) horizontalGrowth++;

    double dhorizontalGrowthMax = (bbHeight * aspectRatioMax) - bbWidth;
    len_t horizontalGrowthMax = (len_t) horizontalGrowthMax;
    if(horizontalGrowth > horizontalGrowthMax) horizontalGrowth = horizontalGrowthMax;


    bool verticalGrowValid = verticalGrowth > 0;
    bool horizontalGrowValid = horizontalGrowth  > 0; 

    bool growUpValid = (BBYH != fp->cs->getCanvasHeight()) && verticalGrowValid; 
    bool growRightValid = (BBXH != fp->cs->getCanvasWidth()) && horizontalGrowValid; 
    bool growDownValid = (BBYL != 0) && verticalGrowValid;
    bool growLeftValid = (BBXL != 0) && horizontalGrowValid;

    if(growUpValid){
        len_t upProbeSky = BBYH + verticalGrowth;
        if(upProbeSky > canvasYH) upProbeSky = canvasYH;
        if(upProbeSky > BBYH){
            Rectangle upProbe =  Rectangle(BBXL, BBYH, BBXH, upProbeSky);
            growRating[0] = evalueViolentGrowingSingleDirection(upProbe, rec::getArea(upProbe), allRectilinearUrgency, iterationLocked);
        }else{
            growRating[0] = -1;
        }
    }else{
        growRating[0] = -1;
    }

    if(growRightValid){
        len_t rightProbeSky = BBXH + horizontalGrowth;
        if(rightProbeSky > canvasXH) rightProbeSky = canvasXH;
        if(rightProbeSky > BBXH){
            Rectangle rightProbe = Rectangle(BBXH, BBYL, BBXH + 1, BBYH);
            growRating[1] = evalueViolentGrowingSingleDirection(rightProbe, rec::getArea(rightProbe), allRectilinearUrgency, iterationLocked);
        }else{
            growRating[1] = -1;
        }
    }else{
        growRating[1] = -1;
    }

    if(growDownValid){
        len_t downProbeGround = BBYL - verticalGrowth;
        if(downProbeGround < 0) downProbeGround = 0;
        if(downProbeGround < BBYL){
            Rectangle downProbe = Rectangle(BBXL, BBYL - 1, BBXH, BBYL);
            growRating[2] = evalueViolentGrowingSingleDirection(downProbe, rec::getArea(downProbe), allRectilinearUrgency, iterationLocked);
        }else{
            growRating[2] = -1;
        }
    }else{
        growRating[2] = -1;
    }

    if(growLeftValid){
        len_t leftProbeGround = BBXL - horizontalGrowth;
        if(leftProbeGround < 0) leftProbeGround = 0;
        if(leftProbeGround < BBXL){
            Rectangle leftProbe = Rectangle(BBXL - 1, BBYL, BBXL, BBYH);
            growRating[3] = evalueViolentGrowingSingleDirection(leftProbe, rec::getArea(leftProbe), allRectilinearUrgency, iterationLocked);
        }else{
            growRating[3] = -1;
        }
    }else{
        growRating[3] = -1;
    }
    // std::cout << "inside evaluation = {" << growRating[0] << ", " << growRating[1] << ", " << growRating[2] << ", " << growRating[3] << "}" << std::endl;

    while (!((growRating[0] == -1) && (growRating[1] == -1) && (growRating[2] == -1) && (growRating[3] == -1))){
        int maxIdx = 0;
        double maxRating = growRating[0];
        for(int i = 1; i < 4; ++i){
            double rating = growRating[i];
            if(rating > maxRating){
                maxIdx = i;
                maxRating = rating;
            }
        }

        if(maxRating != -1){
            growScore.push_back(maxRating);

            if(maxIdx == 0) growOrder.push_back(direction2D::UP);
            else if(maxIdx == 1) growOrder.push_back(direction2D::RIGHT);
            else if(maxIdx == 2) growOrder.push_back(direction2D::DOWN);
            else growOrder.push_back(direction2D::LEFT);

            growRating[maxIdx] = -1;
        }
    }

}

double LegaliseEngine::evalueViolentGrowingSingleDirection(Rectangle &probe, area_t probeArea,
std::unordered_map<Rectilinear *, double> &allRectilinearUrgency, std::unordered_set<Rectilinear *> &iterationLocked) const {
    using namespace boost::polygon::operators;
    DoughnutPolygonSet currentdps;
    currentdps += probe;

    double rating = 0;

    std::vector<Tile *>allTiles;
    fp->cs->enumerateDirectedArea(probe, allTiles);
    for(Tile * const &t : allTiles){
        DoughnutPolygonSet tileInter;
        tileInter += t->getRectangle();
        tileInter &= probe;
        currentdps -= tileInter;

        Rectilinear *tilesRect = fp->blockTilePayload[t];
        if(tilesRect->getType() == rectilinearType::PREPLACED) continue;

        double thisRating = VIOLENT_RATING_BLK_BASE_SCORE;
        double urgency = allRectilinearUrgency[tilesRect];
        bool legalCriticality = (urgency < LEGAL_CRITICALITY_WEIGHT);
        if(legalCriticality) thisRating += VIOLENT_RATING_LEGAL_SCORE;
        urgency = fmod(urgency, LEGAL_CRITICALITY_WEIGHT);

        double criticalityScale = this->criticalitySum - LEGAL_CRITICALITY_WEIGHT;
        urgency = ((criticalityScale - urgency) / criticalityScale) * (1 - (VIOLENT_RATING_BLK_BASE_SCORE + VIOLENT_RATING_LEGAL_SCORE));

        rating += boost::polygon::area(tileInter) * urgency;
    }

    rating += boost::polygon::area(currentdps);

    return rating / double(probeArea);
}

bool LegaliseEngine::growViolent(Rectilinear *rect, direction2D direction, double rating, std::unordered_set<Rectilinear *> &updateUrgency, 
std::unordered_map<Rectilinear *, double> &allRectilinearUrgency, std::unordered_set<Rectilinear *> &iterationLocked) const {
	area_t residualArea = rect->getLegalArea() - rect->calculateActualArea();
    Rectangle boundingBox = rect->calculateBoundingBox();
	len_t boundingBoxXL = rec::getXL(boundingBox);
	len_t boundingBoxXH = rec::getXH(boundingBox);
	len_t boundingBoxYL = rec::getYL(boundingBox);
	len_t boundingBoxYH = rec::getYH(boundingBox);
    len_t boundingBoxWidth = rec::getWidth(boundingBox);
    len_t boundingBoxHeight = rec::getHeight(boundingBox);
    double aspectRatioMax = fp->getGlobalAspectRatioMax();
	double utilizationMin = fp->getGlobalUtilizationMin();

	Rectangle canvasProto = fp->getChipContour();
	len_t canvasXH = rec::getXH(canvasProto);
	len_t canvasYH = rec::getYH(canvasProto);

    // decide how the growing area should look like
    Rectangle bluePrintRect;
    if((direction == direction2D::UP) || (direction == direction2D::DOWN)){
		len_t bluePrintWidth = boundingBoxWidth;
		len_t bluePrintHeight;
		len_t heightUpperLimit = len_t((double(boundingBoxWidth) * aspectRatioMax) - boundingBoxHeight);
		if(heightUpperLimit <= 0){
            std::cout << "retur due to heighupperlimit" << std::endl;
            return false;
        }
		assert(heightUpperLimit > 0);
		
        double targetGrowPCT = scalingFunction(rating/utilizationMin, GROW_VIOLENT_SCLING_FACTOR);
        if(targetGrowPCT < GROW_VIOLENT_MIN_PCT) targetGrowPCT = GROW_VIOLENT_MIN_PCT;
        if(targetGrowPCT > GROW_VIOLENT_MAX_PCT) targetGrowPCT = GROW_VIOLENT_MAX_PCT;
        double targetArea = residualArea * targetGrowPCT; 

        double dHeight = targetArea / double(boundingBoxWidth * rating);
        bluePrintHeight = area_t(dHeight);
        if(dHeight - bluePrintHeight > 0) bluePrintHeight++;
        if(bluePrintHeight > heightUpperLimit) bluePrintHeight = heightUpperLimit;

		if(direction == direction2D::UP){
			len_t bluePrintTop = (boundingBoxYH + bluePrintHeight);
			if(bluePrintTop > canvasYH) bluePrintTop = canvasYH;
			if(boundingBoxYH >= bluePrintTop){
                std::cout << "fail due to 2" << std::endl;
                return false;

            }
			bluePrintRect = Rectangle(boundingBoxXL, boundingBoxYH, boundingBoxXH, bluePrintTop);
		}else{ // direction2D::DOWN
			len_t bluePrintDown = (boundingBoxYL - bluePrintHeight);
			if(bluePrintDown < 0) bluePrintDown = 0;
			if(bluePrintDown >= boundingBoxYL){

                std::cout << "fail due to 3" << std::endl;
                return false;
            }
			bluePrintRect = Rectangle(boundingBoxXL, bluePrintDown, boundingBoxXH, boundingBoxYL);
		}

    }else{
        len_t bluePrintHeight = boundingBoxHeight;
		len_t bluePrintWidth;
		len_t widthUpperLimit = len_t((double(boundingBoxHeight) * aspectRatioMax) - boundingBoxWidth);
		if(widthUpperLimit <= 0){
            std::cout << "fail due to 4" << std::endl;
            return false;
        }
		assert(widthUpperLimit > 0);

        double targetGrowPCT = scalingFunction(rating/utilizationMin, GROW_VIOLENT_SCLING_FACTOR);
        if(targetGrowPCT <= GROW_VIOLENT_MIN_PCT) targetGrowPCT = GROW_VIOLENT_MIN_PCT;
        if(targetGrowPCT > GROW_VIOLENT_MAX_PCT) targetGrowPCT = GROW_VIOLENT_MAX_PCT;
        double targetArea = residualArea * targetGrowPCT; 
        double dWidth = targetArea / double(boundingBoxHeight * rating);
        bluePrintWidth = len_t(dWidth);

        if((dWidth - bluePrintWidth) > 0) bluePrintWidth++;
        if(bluePrintWidth > widthUpperLimit) bluePrintWidth = widthUpperLimit;

		if(direction == direction2D::RIGHT){
			len_t bluePrintRight = (boundingBoxXH + bluePrintWidth);
			if(bluePrintRight > canvasXH) bluePrintRight = canvasXH;
			if(boundingBoxXH >= bluePrintRight){
                std::cout << "fail due to 5" << std::endl;
                return false;
            }
			bluePrintRect = Rectangle(boundingBoxXH, boundingBoxYL, bluePrintRight, boundingBoxYH);
		}else{ // direction2D::LEFT
			len_t bluePrintLeft = (boundingBoxXL - bluePrintWidth);
			if(bluePrintLeft < 0) bluePrintLeft = 0;
			if(bluePrintLeft >= boundingBoxXL){
                std::cout << "fail due to 6" << std::endl;
                return false;
            }
			bluePrintRect = Rectangle(bluePrintLeft, boundingBoxYL, boundingBoxXL, boundingBoxYH);
		}
    }

    // the grow area is decided, now attempt the grow
	using namespace boost::polygon::operators;
    DoughnutPolygonSet petriDishDPS, blankDPS;
	petriDishDPS += bluePrintRect;
	blankDPS += bluePrintRect;

	// find all tiles that crosssect the area, find it and sieve those unfit
	std::vector<Tile *> tilesinPetriDish;	
	fp->cs->enumerateDirectedArea(bluePrintRect, tilesinPetriDish);
	std::unordered_map<Rectilinear *, std::vector<Tile *>> invlovedTiles;
	for(Tile *const &t : tilesinPetriDish){
		Rectilinear *tilesRectilinear = fp->blockTilePayload[t];
		Rectangle tileRectangle = t->getRectangle();
		blankDPS -= tileRectangle;
		if(tilesRectilinear->getType() == rectilinearType::PREPLACED){
			continue;
		}else if(iterationLocked.find(tilesRectilinear) != iterationLocked.end()){
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

    std::sort(involvedRectilinears.begin(), involvedRectilinears.end(), 
		[&](Rectilinear *A, Rectilinear *B) {return allRectilinearUrgency[A] < allRectilinearUrgency[B];});

    // start filling the candidates to grow	
	std::vector<DoughnutPolygon> growCandidates;
	std::vector<Rectilinear *> growCandidatesRectilinear;
	// start with the blank tiles (they have urgency 0)	
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
		rectDPS &= bluePrintRect;
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
	if(growCandidateSize == 0){
        std::cout << "fail due to 11" << std::endl;
        return false;
    }
	
	// ready to actually grow those candiate fragments
	DoughnutPolygonSet currentShapeDPS;
	for(Tile *const &t : rect->blockTiles){
		currentShapeDPS += t->getRectangle();
	}

	std::unordered_map<Rectilinear *, DoughnutPolygonSet> rectCurrentShapeDPS;
	for(Rectilinear *const &ivr : involvedRectilinears){
		for(Tile *const &t : ivr->blockTiles){
			rectCurrentShapeDPS[ivr] += t->getRectangle();
		}
	}

    bool keepGrowing = true;		
	bool hasGrow = false;
	bool growCandidateSelect [growCandidateSize];
	for(int i = 0; i < growCandidateSize; ++i){
		growCandidateSelect[i] = false;
	}
    double violentUtilizationMin = utilizationMin * UTILIZATION_TOLERATNCE_PCT_STEP3;
	
	while((rect->calculateUtilization() > violentUtilizationMin) && keepGrowing){
		keepGrowing = false;
		for(int i = 0; i < growCandidateSize; ++i){
			if(growCandidateSelect[i]) continue;

			DoughnutPolygon markedDP = growCandidates[i];
			// test if installing this marked part would turn the current shape strange

			DoughnutPolygonSet xSelfDPS(currentShapeDPS);
			xSelfDPS += markedDP;
			if(!(dps::oneShape(xSelfDPS) && dps::noHole(xSelfDPS))){
				// intrducing the markedDP would cause strange shape
				continue;
			}

			// test if pulling off the marked part would harm the victim too bad, skip if markedDP belongs to blank
			Rectilinear *victimRect = growCandidatesRectilinear[i];
			if(victimRect != nullptr){
				// markedDP belongs to other Rectilinear
				DoughnutPolygonSet xVictimDPS(rectCurrentShapeDPS[victimRect]);
				xVictimDPS -= markedDP;

				if(xVictimDPS.empty() || (!dps::oneShape(xVictimDPS) || (!dps::noHole(xVictimDPS)))){
					continue;
				}
			}

			// pass all tests, distribute the tile
			growCandidateSelect[i] = true;
			keepGrowing = true;
			hasGrow = true;
			currentShapeDPS -= markedDP;
			DoughnutPolygonSet markedDPS;
			markedDPS += markedDP;
			if(victimRect != nullptr){ // markedDP belongs to other Rectilienar
				rectCurrentShapeDPS[victimRect] -= markedDP;
				// strip the area from victim
				fp->shrinkRectilinear(markedDPS, victimRect);
			}

			// grow the area to our target, rt
			fp->growRectilinear(markedDPS, rect);
		}

	}

	if(!hasGrow){
        std::cout << "fail due to no grow... " << std::endl;
        return false;
    }
	// update the Rectilinears that touches the boundingBox;
	len_t bluePirntXL = rec::getXL(bluePrintRect);
	len_t bluePirntXH = rec::getXH(bluePrintRect);
	len_t bluePrintYL = rec::getYL(bluePrintRect);
	len_t bluePrintYH = rec::getYH(bluePrintRect);

	len_t updateRectXL = (bluePirntXL == 0)? 0 : (bluePirntXL - 1);
	len_t updateRectXH = (bluePirntXH == canvasXH)? canvasXH : (bluePirntXH + 1);
	len_t updateRectYL = (bluePrintYL == 0)? 0 : (bluePrintYL - 1);
	len_t updateRectYH = (bluePrintYH == canvasYH)? canvasYH : (bluePrintYH + 1);
	
	std::vector<Tile *> updateCands;
	fp->cs->enumerateDirectedArea(Rectangle(updateRectXL, updateRectYL, updateRectXH, updateRectYH), updateCands);
	for(Tile *const &t : updateCands){
		updateUrgency.insert(fp->blockTilePayload[t]);
	}

	return true;
}


bool LegaliseEngine::forceFixAspectRatio(Rectilinear *rect) const {
    if(rect->isLegalAspectRatio()) return false;

    Rectangle boundingBox = rect->calculateBoundingBox();
    len_t BBXL = rec::getXL(boundingBox);
    len_t BBXH = rec::getXH(boundingBox);
    len_t BBYL = rec::getYL(boundingBox);
    len_t BBYH = rec::getYH(boundingBox);
    len_t BBWidth = rec::getWidth(boundingBox);
    len_t BBHeight = rec::getHeight(boundingBox);
    double maxAspectRatio = fp->getGlobalAspectRatioMax();

    if(BBWidth > BBHeight){ // Require to Trim off Width Part
        len_t widthMax = len_t(BBHeight * maxAspectRatio);
        len_t trimOff = BBWidth - widthMax;
        if(trimOff <= 0) return false;

        Rectangle leftTrim = Rectangle(BBXL, BBYL, (BBXL + trimOff), BBYH);
        Rectangle rightTrim = Rectangle((BBXH - trimOff), BBYL, BBXH, BBYH);

        area_t leftTrimArea, rightTrimArea;
        bool leftTrimResult = evaluateForceTrim(rect, leftTrim, leftTrimArea);
        bool rightTrimResult = evaluateForceTrim(rect, rightTrim, rightTrimArea);

		if(leftTrimResult && rightTrimResult){
			if(leftTrimArea > rightTrimArea) executeTrim(rect, leftTrim);
			else executeTrim(rect, rightTrim);

			return true;
		}else if(leftTrimResult){
			executeTrim(rect, leftTrim);
			return true;
		}else if(rightTrimResult){
			executeTrim(rect, rightTrim);
			return true;
		}
    }else{
		len_t hightMax = len_t(BBWidth * maxAspectRatio);
		len_t trimOff = BBHeight - hightMax;
		if(trimOff < 0) return false;

		Rectangle downTrim = Rectangle(BBXL, BBYL, BBXH, BBYL + trimOff);
		Rectangle upTrim = Rectangle(BBXL, (BBYH - trimOff), BBXH, BBYH);

		area_t downTrimArea, upTrimArea;
		bool downTrimResult = evaluateForceTrim(rect, downTrim, downTrimArea);
		bool upTrimResult = evaluateForceTrim(rect, upTrim, upTrimArea);

		if(downTrimResult && upTrimResult){
			if(downTrimArea > upTrimArea) executeTrim(rect, downTrim);
			else executeTrim(rect, upTrim);

			return true;
		}else if(downTrimResult){
			executeTrim(rect, downTrim);
			return true;
		}else if(upTrimResult){
			executeTrim(rect, upTrim);
			return true;
		}

    }

	// trimming either side is illegal, give up
	return false;
}

bool LegaliseEngine::evaluateForceTrim(Rectilinear *rect, Rectangle &trimRect, area_t &areaAfterTrim) const {
	using namespace boost::polygon::operators;

	DoughnutPolygonSet trimLook;
	for(Tile *const &t : rect->blockTiles){
		trimLook += t->getRectangle();
	}

	trimLook -= trimRect;

	if(!(dps::oneShape(trimLook) && dps::noHole(trimLook))) return false;

	areaAfterTrim = boost::polygon::area(trimLook);	
	return true;

}

void LegaliseEngine::executeTrim(Rectilinear *rect, Rectangle &trimRect) const {

	using namespace boost::polygon::operators;

	DoughnutPolygonSet trimDPS;
	for(Tile *const &t : rect->blockTiles){
		trimDPS += t->getRectangle();
	}

	trimDPS &= trimRect;
	std::vector<DoughnutPolygon> toShrink;
	int trimDPSSize = trimDPS.size();
	if(trimDPSSize == 0) return;

	for(int i = 0; i < trimDPSSize; ++i){
		toShrink.push_back(trimDPS[i]);
	}

	fp->shrinkRectilinear(toShrink, rect);
	std::cout << "Executed Trim to " << rect->getName() << std::endl;
}





double LegaliseEngine::evalueBlankGrowingSingleDirection(Rectangle &probe, area_t probeArea) const {
    using namespace boost::polygon::operators;
    DoughnutPolygonSet probedps;
    probedps += probe;

    std::vector<Tile *>allTiles;
    fp->cs->enumerateDirectedArea(probe, allTiles);
    for(Tile * const &t : allTiles){
        probedps -= t->getRectangle();
    }

    return double(boost::polygon::area(probedps)) / double(probeArea);
}

double LegaliseEngine::calculateLegalSuburgency(Rectilinear *rect, Tile *candTile) const {
    bool legalAspectRatio = rect->isLegalAspectRatio();
    bool legalUtilization = rect->isLegalUtilization();
    bool legalNoHole = rect->isLegalNoHole();
    bool legalOneShape = rect->isLegalOneShape();

    Rectilinear removedTile(*rect);
    if(candTile->getType() == tileType::OVERLAP){
        removedTile.overlapTiles.erase(candTile);
    }else{
        removedTile.blockTiles.erase(candTile);
    }

    // speical case where entire block is composed of overlap
    if(removedTile.calculateActualArea() == 0) return 4;

    legalAspectRatio = (!removedTile.isLegalAspectRatio()) && legalAspectRatio;
    legalUtilization = (!removedTile.isLegalUtilization()) && legalUtilization;
    legalNoHole = (!removedTile.isLegalNoHole()) && legalNoHole;
    legalOneShape = (!removedTile.isLegalOneShape()) && legalOneShape;

    if(legalNoHole || legalOneShape) return 2;
    else if(legalAspectRatio || legalUtilization) return 1;
    else return 0;
}

double LegaliseEngine::calculateUtilizationSubUrgency(Rectilinear *rect) const {
	double legalMinUtilization = fp->getGlobalUtilizationMin();
	double legalUtilizationRange = 1 - legalMinUtilization;
	double adjustedUtilization = (rect->calculateUtilization() - legalMinUtilization) / legalUtilizationRange; 

    return adjustedUtilization;
}

double LegaliseEngine::calculateAspectRatioSubUrgency(Rectilinear *rect) const {
    // calculate aspectRatio suburgency
    double adjustedAspectRatio = rec::calculateAspectRatio(rect->calculateBoundingBox());
    if(adjustedAspectRatio < 1.0) adjustedAspectRatio = 1 / adjustedAspectRatio;
    adjustedAspectRatio /= fp->getGlobalAspectRatioMax();

    return adjustedAspectRatio;
}

double LegaliseEngine::calculateOverlapRatioSubUrgency(Rectilinear *rect, Tile *candTile) const {

    return double(candTile->getArea()) / double(rect->getLegalArea());
}

double LegaliseEngine::calculateBorderSubUrgency(Rectilinear *rect) const {
    // special case where shape is bizzare
    if(!(rect->isLegalNoHole() && rect->isLegalOneShape())) return 1.0;

    // calculate borderSubUrgency
    len_t totalLength = 0;
    double weightedBorder = 0;
    std::vector<Cord>winding;
    rect->acquireWinding(winding, direction1D::CLOCKWISE);
    int windingCordSize = winding.size();
    for(int i = 0; i < windingCordSize; ++i){
        Cord c1 = winding[i];
        Cord c2 = winding[(i+1) % windingCordSize];
        Line line(c1, c2);
        len_t LineLength = line.calculateLength();
        std::vector<LineTile> positiveSide, negativeSide;
        fp->cs->findLineTile(line, positiveSide, negativeSide);

        if(!positiveSide.empty()){
            Tile *posFirstTile = positiveSide[0].getTile();
            bool outwardSide = true;
            if(posFirstTile->getType() == tileType::BLANK){
                outwardSide = true;
            }else if(posFirstTile->getType() == tileType::BLOCK){
                outwardSide = (fp->blockTilePayload[posFirstTile] == rect)? false : true;
            }else{ // tileType::OVERLAP
                bool foundSide = false;
                for(Rectilinear *const &rt : fp->overlapTilePayload[posFirstTile]){
                    if(rt == rect){
                        outwardSide = false;
                    }
                }
            }
            if(outwardSide){ // outward is at positive side
				for(const LineTile &lt : positiveSide){
					len_t subLineLength = lt.getLine().calculateLength();
					Tile *subLineTile = lt.getTile();
					tileType subLineTileType = subLineTile->getType();

					if(subLineTileType == tileType::BLANK) weightedBorder += (BLANK_EDGE_WEIGHT * subLineLength);
					else if(subLineTileType == tileType::BLOCK){
						Rectilinear *subLineTileCoorespondRect = fp->blockTilePayload[subLineTile];
						if(subLineTileCoorespondRect->getType() == rectilinearType::SOFT){
							weightedBorder +=(BLOCK_EDGE_WEIGHT * subLineLength);
						}else{
							// the block is touching preplaced module
							weightedBorder +=(DEAD_EDGE_WEIGHT * subLineLength);
						}
					} 
					else weightedBorder += (OVERLAP_EDGE_WEIGHT * subLineLength);
				}

            }else{ // outward is at negative side
                if(!negativeSide.empty()){
					for(const LineTile &lt : negativeSide){
						len_t subLineLength = lt.getLine().calculateLength();
						Tile *subLineTile = lt.getTile();
						tileType subLineTileType = subLineTile->getType();

						if(subLineTileType == tileType::BLANK) weightedBorder += (BLANK_EDGE_WEIGHT * subLineLength);
						else if(subLineTileType == tileType::BLOCK){
							Rectilinear *subLineTileCoorespondRect = fp->blockTilePayload[subLineTile];
							if(subLineTileCoorespondRect->getType() == rectilinearType::SOFT){
								weightedBorder +=(BLOCK_EDGE_WEIGHT * subLineLength);
							}else{
								// the block is touching preplaced module
								weightedBorder +=(DEAD_EDGE_WEIGHT * subLineLength);
							}
						} 
						else weightedBorder += (OVERLAP_EDGE_WEIGHT * subLineLength);
					}
                }else{
                    weightedBorder += DEAD_EDGE_WEIGHT * LineLength;
                }
            }

        }else{// positiveSide is deadedge
            weightedBorder += DEAD_EDGE_WEIGHT * LineLength;
        }

        totalLength += LineLength; 
        
    }

	return(weightedBorder / double(totalLength));
}

double LegaliseEngine::calcualteCentreSubUrgency(Rectilinear *rect) const {
	double dCentreX, dCentreY;
	int centreX, centreY;
	rec::calculateCentre(rect->calculateBoundingBox(), dCentreX, dCentreY);
	centreX = int(dCentreX);
	centreY = int(dCentreY);

	Rectangle chipContour = fp->getChipContour();
	len_t chipWidth = rec::getWidth(chipContour);
	len_t chipHeight = rec::getHeight(chipContour);
	
	len_t chipmaxDistance = (chipWidth + chipHeight) / 2;
	len_t rectDistance = (std::abs(centreX - chipWidth/2) + std::abs(centreY - chipHeight/2));

	return 1 - (double(rectDistance) / double(chipmaxDistance));

}

double LegaliseEngine::calculateSizeSubUrgency(Rectilinear *rect) const {
	area_t softRectilinearRequiredAreaRange = maxSoftRectilinearRequiredArea - minSoftRectilinearRequiredArea;
    double sizeSub = 1 - ((double(rect->getLegalArea()) / softRectilinearRequiredAreaRange));
    
	return (sizeSub > 0)? sizeSub : 0;
}

double LegaliseEngine::rateRectilinearLegalCriticality(Rectilinear *rt) const {
    bool area = !(rt->isLegalEnoughArea());
    bool aspectRatio = !(rt->isLegalAspectRatio());
    bool util = !(rt->isLegalUtilization());
    bool noHole = !(rt->isLegalNoHole());
    bool oneShape = !(rt->isLegalOneShape());

    return (area * LEGAL_CRITICALITY_AREA) + (aspectRatio * LEGAL_CRITICALITY_ASPECT_RATIO) + 
            (util * LEGAL_CRITICALITY_UTILIZATION) + (noHole * LEGAL_CRITICALITY_HOLE) + (oneShape * LEGAL_CRITICALITY_ONE_SHAPE);
}

double LegaliseEngine::rateRectilinearAreaCriticality(Rectilinear *rt) const {
    area_t legalArea = rt->getLegalArea();
    area_t residualArea = rt->calculateActualArea() - legalArea;

    if(residualArea > 0) return 0;
    else return double(-residualArea) / double(legalArea);
    
}

