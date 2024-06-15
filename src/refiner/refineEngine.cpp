#include "refineEngine.h"
#include "eVector.h"
#include "cSException.h"
#include "colours.h"

RefineEngine::RefineEngine(Floorplan *floorplan, int maxIter, bool useGradientOrder, len_t initMomentum, double momentumGrowth, bool growGradient, bool shrinkGradient)
	: REFINE_MAX_ITERATION(maxIter), REFINE_USE_GRADIENT_ORDER(useGradientOrder),
	REFINE_INITIAL_MOMENTUM(initMomentum), REFINE_MOMENTUM_GROWTH(momentumGrowth), REFINE_USE_GRADIENT_GROW(growGradient), REFINE_USE_GRADIENT_SHRINK(shrinkGradient) {
    this->fp = floorplan;
    for(Rectilinear *const &rt : fp->softRectilinears){
        rectConnOrder.push_back(rt);
        std::unordered_map<Rectilinear *, std::vector<Connection *>>::iterator it = fp->connectionMap.find(rt);
        if(it == fp->connectionMap.end()){
            this->rectConnWeightSum[rt] = 0;
        }else{
            double weightSum = 0;
            for(Connection *const &cnn : it->second){
                weightSum += cnn->weight;
            }

            this->rectConnWeightSum[rt] = weightSum;
        }
    }
    std::sort(rectConnOrder.begin(), rectConnOrder.end(), [&](Rectilinear *a, Rectilinear *b){return rectConnWeightSum[a] > rectConnWeightSum[b];});
}

Floorplan *RefineEngine::refine(){
    bool hasMovement = true;
	int iterationCounter = 0;
	double initialHPWL = fp->calculateHPWL();

	double hpwlDeltaRecord[3] = {-1, -2, -3};

	double bestHPWL = initialHPWL;
	Floorplan *bestFloorplan = new Floorplan(*fp);

	double iterationHPWLRecord = initialHPWL;
    while(hasMovement){
        hasMovement = false;

		// arrange the order of refining
		std::unordered_map<Rectilinear *, double> rectilinearRating;
		std::vector<Rectilinear *> refineCandidates;
		for(Rectilinear * const &rect : fp->softRectilinears){
			if(rectConnWeightSum.at(rect) == 0) continue;
			refineCandidates.push_back(rect);

			if(REFINE_USE_GRADIENT_ORDER){
				// use gradient magnitude as ordering
				EVector candidateGradient;
				bool hasGradient = fp->calculateRectilinearGradient(rect, candidateGradient);
				if(hasGradient){
					rectilinearRating[rect] = candidateGradient.calculateL1Magnitude();
				}else{
					rectilinearRating[rect] = 0;
				}
			}else{
				// use connection strength as ordering
				rectilinearRating[rect] = rectConnWeightSum[rect];
			}
		}

		std::sort(refineCandidates.begin(), refineCandidates.end(),
			[&](Rectilinear *a, Rectilinear *b){return rectilinearRating[a] > rectilinearRating[b];});
			

		
		for(int eCand = 0; eCand < refineCandidates.size(); ++eCand){
			Rectilinear *improveTarget = refineCandidates[eCand];
            bool enhanceResult = refineRectilinear(improveTarget);

            if(enhanceResult){
				hasMovement = true;
				double currenthpwl = fp->calculateHPWL();
				if(currenthpwl < bestHPWL){
					bestHPWL = currenthpwl;
					delete bestFloorplan;
					bestFloorplan = new Floorplan(*fp);

				}
			}
        }

		double doneIterationHPWL = fp->calculateHPWL();
		double iterationDelta = doneIterationHPWL - iterationHPWLRecord;
		iterationHPWLRecord = doneIterationHPWL;

		// terminate if continuouse 0 improvement for 3 cycles;
		bool badGrow = (iterationDelta > 0);
		bool continuousNoGrow = (iterationDelta == 0) && (hpwlDeltaRecord[0] == 0) && (hpwlDeltaRecord[1] == 0);
		bool oscillationDetected = (iterationDelta == hpwlDeltaRecord[1]) && (hpwlDeltaRecord[0] == hpwlDeltaRecord[2]);
		if(badGrow || continuousNoGrow || oscillationDetected) break;
		

		// terminate if oscilation detected

		hpwlDeltaRecord[2] = hpwlDeltaRecord[1];
		hpwlDeltaRecord[1] = hpwlDeltaRecord[0];
		hpwlDeltaRecord[0] = iterationDelta;

		// std::cout << "Iteration " << iterationCounter + 1 << " Current HPWL = " << doneIterationHPWL;
		// if(hpwlDeltaRecord[0] < 0) std::cout << GREEN << " +" << (hpwlDeltaRecord[0])<< COLORRST << std::endl;
		// else std::cout << " " << RED  << (hpwlDeltaRecord[0]) << COLORRST << std::endl;
		// rectilinearIllegalType rit;
		// std::cout << "Floorplan Legality = " << (this->fp->checkFloorplanLegal(rit) == nullptr) << std::endl;

		if((++iterationCounter) == REFINE_MAX_ITERATION) break;
    }
	return bestFloorplan;

}

bool RefineEngine::refineRectilinear(Rectilinear *rect) const {
	// rectilinearIllegalType rit;
	// bool legcheck0 = (this->fp->checkFloorplanLegal(rit) == nullptr);
	// if(!legcheck0) std::cout << "Legalizing " << rect->getName() << " is illegal in station 0" << std::endl;
	// double hpwl0 = fp->calculateHPWL();
	

	bool hasImprovements = false;
	fillBoundingBox(rect);

	// bool legcheck1 = (this->fp->checkFloorplanLegal(rit) == nullptr);
	// if((!legcheck1) && legcheck0) std::cout << "Legalizing " << rect->getName() << " is illegal in station 1" << std::endl;

	bool growRefineImproves = refineByGrowing(rect);
	hasImprovements |= growRefineImproves;

	// bool legcheck2 = (this->fp->checkFloorplanLegal(rit) == nullptr);
	// if((!legcheck2) && legcheck1) std::cout << "Legalizing " << rect->getName() << " is illegal in station 2" << std::endl;
	
	// double hpwl1 = fp->calculateHPWL();	
	// if(!growRefineImproves){
	// 	std::cout << "Grow: " << BLUE << "x" << COLORRST;
	// }else{
	// 	double netimprove = hpwl0 - hpwl1;
	// 	if(netimprove > 0){
	// 		std::cout << " Grow: " << GREEN << netimprove << COLORRST;
	// 	}else{
	// 		std::cout << " Grow: " << RED << netimprove << COLORRST;
	// 	}
	// }
	
	fillBoundingBox(rect);
		// std::cout << "Floorplan Legality4 = " << (this->fp->checkFloorplanLegal(rit) == nullptr) << std::endl;
	// double hpwl2 = fp->calculateHPWL();
	// double imp2 = hpwl1 - hpwl2;

	// if(imp2 > 0){
	// 	std::cout << " Fill: " << GREEN << imp2 << COLORRST;
	// }else{
	// 	std::cout << " Fill: " << RED << imp2 << COLORRST;
	// }

	bool trimRefineImproves = refineByTrimming(rect);
	hasImprovements |= trimRefineImproves;
		// std::cout << "Floorplan Legality5 = " << (this->fp->checkFloorplanLegal(rit) == nullptr) << std::endl;

	// double hpwl3 = fp->calculateHPWL();

	// if(!trimRefineImproves){
	// 	std::cout << "Shrink: " << BLUE << "x" << COLORRST;
	// }else{
	// 	double netimprove = hpwl2 - hpwl3;
	// 	if(netimprove > 0){
	// 		std::cout << " Shrink: " << GREEN << netimprove << COLORRST;
	// 	}else{
	// 		std::cout << " Shrink: " << RED << netimprove << COLORRST;
	// 	}
	// }
	// std::cout << "End HPWL = " << fp->calculateHPWL() << std::endl;

	// bool afterlegality = (this->fp->checkFloorplanLegal(rit) == nullptr);
	// if(!afterlegality){
	// 	Rectilinear *illegalRect = fp->checkFloorplanLegal(rit);
	// 	std::cout << "Refining " << rect->getName() << "Makes illegal! illegal guy = " << illegalRect->getName() << " " << rit << std::endl;
	// }
	
	return hasImprovements;
}

bool RefineEngine::fillBoundingBox(Rectilinear *rect) const {
	using namespace boost::polygon::operators;
	
	// This stores the growing shape of rect during the growing process
	DoughnutPolygonSet currentRectDPS;

	Rectangle rectBB = rect->calculateBoundingBox();

	DoughnutPolygonSet petriDishDPS, blankDPS;
	petriDishDPS += rectBB;
	blankDPS += rectBB;

	// find all tiles that crosssect the area, find it and sieve those unfit
	std::vector<Tile *> tilesinPetriDish;	
	std::unordered_map<Rectilinear *, std::vector<Tile *>> invlovedTiles;
	fp->cs->enumerateDirectedArea(rectBB, tilesinPetriDish);
	for(Tile *const &t : tilesinPetriDish){
		Rectilinear *tilesRectilinear = fp->blockTilePayload[t];
		Rectangle tileRectangle = t->getRectangle();
		blankDPS -= tileRectangle;
		if(tilesRectilinear->getType() == rectilinearType::PREPLACED) continue;
		if(tilesRectilinear == rect){
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
	if(growCandidateSize == 0) return false;


	std::unordered_map<Rectilinear *, DoughnutPolygonSet> rectCurrentShapeDPS;
	for(Rectilinear *const &ivr : involvedRectilinears){
		for(Tile *const &t : ivr->blockTiles){
			rectCurrentShapeDPS[ivr] += t->getRectangle();
		}
	}
	bool keepGrowing = true;		
	bool hasGrow = false;
	bool growCandidateSelect [growCandidateSize]; // This records which candidate is selected
	for(int i = 0; i < growCandidateSize; ++i){
		growCandidateSelect[i] = false;
	}

	while(keepGrowing){
		keepGrowing = false;
		for(int i = 0; i < growCandidateSize; ++i){
			if(growCandidateSelect[i]) continue;

			DoughnutPolygon markedDP = growCandidates[i];
			// test if installing this marked part would turn the current shape strange

			DoughnutPolygonSet xSelfDPS(currentRectDPS);
			xSelfDPS += markedDP;
			// April 29, 2024: Add inner Width constraint
			if(!(dps::oneShape(xSelfDPS) && dps::noHole(xSelfDPS) && dps::innerWidthLegal(xSelfDPS))){
				// intrducing the markedDP would cause dpSet to generate strange shape, give up on the tile
				continue;
			}

			// test if pulling off the marked part would harm the victim too bad, skip if markedDP belongs to blank
			Rectilinear *victimRect = growCandidatesRectilinear[i];
			if(victimRect != nullptr){
				// markedDP belongs to other Rectilinear
				DoughnutPolygonSet xVictimDPS(rectCurrentShapeDPS[victimRect]);
				xVictimDPS -= markedDP;

				if(!dps::checkIsLegal(xVictimDPS, victimRect->getLegalArea(), this->fp->getGlobalAspectRatioMin(), 
				this->fp->getGlobalAspectRatioMax(), this->fp->getGlobalUtilizationMin())){
					continue;
				}

				bool actuarization = forecastGrowthBenefits(rect, currentRectDPS, rectCurrentShapeDPS);
				if(!actuarization) continue;
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
				fp->shrinkRectilinear(markedDPS, victimRect);
			}
			fp->growRectilinear(markedDPS, rect);
		}
	}

	return hasGrow;

}

bool RefineEngine::refineByGrowing(Rectilinear *rect) const {
	bool keepMoving = true;
	bool hasMove = false;
	int momentum = REFINE_INITIAL_MOMENTUM;


	while(keepMoving || (momentum != REFINE_INITIAL_MOMENTUM)){
		if(!keepMoving) momentum = REFINE_INITIAL_MOMENTUM;

		keepMoving = false;

		sector rectTowardSector;
		if(!REFINE_USE_GRADIENT_GROW){
			Cord rectOptimalCentre = fp->calculateOptimalCentre(rect);
			double rectBBCentreX, rectBBCentreY;
			Rectangle rectBB = rect->calculateBoundingBox();
			
			rec::calculateCentre(rectBB, rectBBCentreX, rectBBCentreY);
			Cord BBCentre = Cord(len_t(rectBBCentreX), len_t(rectBBCentreY));

			EVector rectToward(BBCentre, rectOptimalCentre);
			angle_t rectTowardAngle = rectToward.calculateDirection();
			rectTowardSector = translateAngleToSector(rectTowardAngle);

		}else{
			EVector gradient;
			if(!fp->calculateRectilinearGradient(rect, gradient)) return false;
			rectTowardSector = translateAngleToSector(gradient.calculateDirection());

		}


		double beforehpwl = fp->calculateHPWL();	
		rectilinearIllegalType rit;
		Rectilinear *ilr = this->fp->checkFloorplanLegal(rit);
		bool stageLegal = (ilr == nullptr);
		// if(!stageLegal){
		// 	std::cout << "Refining " << rect->getName() << "With momentum = " << momentum;
		// 	std::cout << " is illegal, cause = " << ilr->getName() << " " << rit << std::endl;
		// }else{
		// 	std::cout << "Refining " << rect->getName() << "With momentum = " << momentum << "is currently legal" << std::endl;

		// }
		// std::cout << "Selecting toward " << rectTowardSector << std::endl;
		

		switch (rectTowardSector){
			case sector::ONE: {
				// Try East, then North
				bool growEast = growingTowardEast(rect, momentum);
				if(growEast){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool growNorth = growingTowardNorth(rect, momentum);
				if(growNorth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}	
			case sector::TWO: {
				// Try North, then East
				bool growNorth = growingTowardNorth(rect, momentum);
				if(growNorth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool growEast = growingTowardEast(rect, momentum);
				if(growEast){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::THREE: {
				// Try North, then West
				bool growNorth = growingTowardNorth(rect, momentum);
				// std::cout << "Grow North Result = " << growNorth << std::endl;
				// this->fp->debugFloorplanLegal();
				if(growNorth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool growWest = growingTowardWest(rect, momentum);
				// std::cout << "Grow West Result = " << growWest << std::endl;
				// this->fp->debugFloorplanLegal();
				if(growWest){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::FOUR: {
				// Try West, then North
				bool growWest = growingTowardWest(rect, momentum);
				if(growWest){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool growNorth = growingTowardNorth(rect, momentum);
				if(growNorth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}
				break;
			}
			case sector::FIVE: {
				// Try West, then South
				bool growWest = growingTowardWest(rect, momentum);
				if(growWest){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool growSouth = growingTowardSouth(rect, momentum);
				if(growSouth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::SIX: {
				// Try South, then West
				// double entrehpwl = fp->calculateHPWL();
				// std::cout << "Attempt grow South: ";

				bool growSouth = growingTowardSouth(rect, momentum);
				if(growSouth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool growWest = growingTowardWest(rect, momentum);
				if(growWest){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::SEVEN: {
				// Try South, then East
				bool growSouth = growingTowardSouth(rect, momentum);
				if(growSouth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool growEast = growingTowardEast(rect, momentum);
				if(growEast){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::EIGHT: {
				// Try East, then South
				bool growEast = growingTowardEast(rect, momentum);
				if(growEast){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool growSouth = growingTowardSouth(rect, momentum);
				if(growSouth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			default:{
				throw CSException("REFINEENGINE_03");
				break;
			}
		}
	}

	return hasMove;
}

bool RefineEngine::growingTowardNorth(Rectilinear *rect, len_t depth) const {
	using namespace boost::polygon::operators;

	Rectangle chipProto = fp->getChipContour();
	len_t chipXH = rec::getXH(chipProto);
	len_t chipYH = rec::getYH(chipProto);

	double globalAspectRatioMin = fp->getGlobalAspectRatioMin();
	double globalAspectRatioMax = fp->getGlobalAspectRatioMax();
	double globalUtilizationMin = fp->getGlobalUtilizationMin();

	/* STEP 1: Ateempt to grow toward the specified direction */
	// determine the growing area
	Rectangle rectBB = rect->calculateBoundingBox();
	len_t BBXL = rec::getXL(rectBB);
	len_t BBXH = rec::getXH(rectBB);
	len_t BBYL = rec::getYL(rectBB);
	len_t BBYH = rec::getYH(rectBB);
	len_t BBWidth = rec::getWidth(rectBB);
	len_t BBHeight = rec::getHeight(rectBB);
	len_t tryGrowYH = BBYH + depth;
	if(tryGrowYH > chipYH) tryGrowYH = chipYH;

	if(tryGrowYH <= BBYH) return false;

	Rectangle posGrowArea = Rectangle(BBXL, BBYH, BBXH, tryGrowYH);

	// currentRectDPS stores the shape of rect during the trial process
	DoughnutPolygonSet currentRectDPS;
	// this records all surrounding rectiliears that has possibly changed shape by the growing process
	std::unordered_map<Rectilinear *, DoughnutPolygonSet> affectedNeighbors;

	// initizlize currentRetDPS as it's current shape
	for(Tile *const &t : rect->blockTiles){
		currentRectDPS += t->getRectangle();
	}

	// The process of growing up
	bool growNorth = trialGrow(currentRectDPS, rect, posGrowArea, affectedNeighbors);
	if(!growNorth) return false;

	/* STEP 2: Attempt fix the aspect ratio flaw caused by introducing the growth toward specified direction */
	if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){

		/* STEP 2-1 : Fix the aspect Ratio by Trimming off areas from the Negative Shrink Area of rect */
		Rectangle currentBB = dps::calculateBoundingBox(currentRectDPS);
		len_t currentBBWidth = rec::getWidth(currentBB);
		len_t currentBBHeight = rec::getHeight(currentBB);
		
		double dResidualLen = (currentBBWidth * globalAspectRatioMax) - currentBBHeight;
		if(dResidualLen > 0){
			len_t residualLen = len_t(dResidualLen); // round down naturally
			len_t boundingBoxYLMax = rec::getYL(currentBB) + residualLen;
			if(boundingBoxYLMax == rec::getYL(currentBB)) return false;
			assert(boundingBoxYLMax < rec::getYH(currentBB));

			// shed off the marked part at negative shrink area, by an incremental method
			len_t roundShedOffLen = REFINE_INITIAL_MOMENTUM;
			bool shedMoving = true;
			while((rec::getYL(currentBB) < boundingBoxYLMax) && (shedMoving || (roundShedOffLen != REFINE_INITIAL_MOMENTUM))){
				if(!shedMoving) roundShedOffLen = REFINE_INITIAL_MOMENTUM;
				shedMoving = false;

				len_t currentBBXL = rec::getXL(currentBB);
				len_t currentBBYL = rec::getYL(currentBB);
				len_t currentBBXH = rec::getXH(currentBB);
				len_t tryShrinkYH = currentBBYL + roundShedOffLen;
				if(tryShrinkYH > boundingBoxYLMax) tryShrinkYH = boundingBoxYLMax;
				Rectangle shedOffArea = Rectangle(currentBBXL, currentBBYL, currentBBXH, tryShrinkYH);

				DoughnutPolygonSet xShed(currentRectDPS);
				xShed -= shedOffArea;
				if(dps::oneShape(xShed) && dps::noHole(xShed) && dps::innerWidthLegal(xShed) &&(!xShed.empty())){ // the shed would not affect the shape too much, accept change
					currentRectDPS -= shedOffArea;
					roundShedOffLen = len_t(REFINE_INITIAL_MOMENTUM * REFINE_MOMENTUM_GROWTH + 0.5);
					currentBB = dps::calculateBoundingBox(currentRectDPS);
					bool shedMoving = true;
				}
			}
		}

		/* STEP 2-2 : fix aspect ratio by adding area to the sides, pick the plus side (the one that grow would decrease HPWL)*/
		if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){
			Cord optCentre = fp->calculateOptimalCentre(rect);
			double bbcentreX, bbCentreY;
			currentBB = dps::calculateBoundingBox(currentRectDPS);
			len_t currentBBXL = rec::getXL(currentBB);
			len_t currentBBYL = rec::getYL(currentBB);
			len_t currentBBXH = rec::getXH(currentBB);
			len_t currentbbYH = rec::getYH(currentBB);
			currentBBWidth = rec::getWidth(currentBB);
			currentBBHeight = rec::getHeight(currentBB);
			EVector step22Vector = EVector(FCord(bbcentreX, bbCentreY), optCentre);
			angle_t step22Toward = step22Vector.calculateDirection();

			bool eastPlusSide = (step22Toward < (M_PI / 2.0));
			double dSideGrowingTarget = (currentBBHeight * globalAspectRatioMin) - currentBBWidth;
			assert(dSideGrowingTarget > 0);
			len_t sideGrowingTarget = len_t(dSideGrowingTarget + 0.5); // round up by trick
			if(eastPlusSide){ // the vector indicates that the vector is toward NorthEast
				len_t eastGrowXH = currentBBXH + sideGrowingTarget;
				if(eastGrowXH > chipXH) eastGrowXH = chipXH;
				if(eastGrowXH >= currentBBXH){
					Rectangle eastGrowRange = Rectangle(currentBBXH, currentBBYL, eastGrowXH, currentbbYH);
					fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, eastGrowRange);
				}
			}else{
				len_t westGrowXL = currentBBXL - sideGrowingTarget;
				if(westGrowXL < 0) westGrowXL = 0;
				if(westGrowXL <= currentBBXL){
					Rectangle westGrowRange = Rectangle(westGrowXL, currentBBYL, currentBBXL, currentbbYH);
					fixAspectRatioWestSideGrow(currentRectDPS, rect, affectedNeighbors, westGrowRange);
				}
			}

			/* STEP 2-3 : fix aspect ratio by adding area to the sides, now we can only grow on the negative side, this is an uphill move */
			if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){
				currentBB = dps::calculateBoundingBox(currentRectDPS);
				len_t currentBBXL = rec::getXL(currentBB);
				len_t currentBBYL = rec::getYL(currentBB);
				len_t currentBBXH = rec::getXH(currentBB);
				len_t currentbbYH = rec::getYH(currentBB);
				currentBBWidth = rec::getWidth(currentBB);
				currentBBHeight = rec::getHeight(currentBB);

				double dSideGrowingTarget = (currentBBHeight * globalAspectRatioMin) - currentBBWidth;
				assert(dSideGrowingTarget > 0);
				len_t sideGrowingTarget = len_t(dSideGrowingTarget + 0.5); // round up by trick
				if(!eastPlusSide){ // the vector indicates that the vector is toward NorthEast
					len_t eastGrowXH = currentBBXH + sideGrowingTarget;
					if(eastGrowXH > chipXH) eastGrowXH = chipXH;
					if(eastGrowXH >= currentBBXH){
						Rectangle eastGrowRange = Rectangle(currentBBXH, currentBBYL, eastGrowXH, currentbbYH);
						fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, eastGrowRange);
					}
				}else{
					len_t westGrowXL = currentBBXL - sideGrowingTarget;
					if(westGrowXL < 0) westGrowXL = 0;
					if(westGrowXL <= currentBBXL){
						Rectangle westGrowRange = Rectangle(westGrowXL, currentBBYL, currentBBXL, currentbbYH);
						fixAspectRatioWestSideGrow(currentRectDPS, rect, affectedNeighbors, westGrowRange);
					}
				}
			}

		}
	}

	// if STEP 2 cannot fix aspect ratio issues, the growing cannot be executed
	if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)) return false;

	/* STEP3 : Attempt to fix the utilization flaws caused by introducing the growth toward specified direction */
	if(!dps::checkIsLegalUtilization(currentRectDPS, globalUtilizationMin)){
		/* STEP 3-1 Attempt to fix the utilization flaw by growing in the bounding box */
		fixUtilizationGrowBoundingBox(currentRectDPS, rect, affectedNeighbors);	
	}

	// conduct a final legality check for all changes that went through STEP 2 ~ 3
	if(!dps::checkIsLegal(currentRectDPS, rect->getLegalArea(), globalAspectRatioMin, globalAspectRatioMax, globalUtilizationMin)) return false;

	/* STEP 4: make sure all changes acutally improves HPWL and reflect the changes */
	bool actuarization = forecastGrowthBenefits(rect, currentRectDPS, affectedNeighbors);
	if(!actuarization) return false;

	// Actually execute all modifications
	// modify the neighbor affected rectilinears first
	for(std::unordered_map<Rectilinear *, DoughnutPolygonSet>::const_iterator it = affectedNeighbors.begin(); it != affectedNeighbors.end(); ++it){
		Rectilinear *neighbor = it->first;
		DoughnutPolygonSet neighborOrignalDPS;
		for(Tile *const &t : neighbor->blockTiles){
			neighborOrignalDPS += t->getRectangle();
		}

		neighborOrignalDPS -= it->second;
		fp->shrinkRectilinear(neighborOrignalDPS, neighbor);
	}

	// The rect part's grow and shrink
	DoughnutPolygonSet originalMainDPS;
	for(Tile *const &t : rect->blockTiles){
		originalMainDPS += t->getRectangle();
	}

	DoughnutPolygonSet dpsGrowPart(currentRectDPS);
	dpsGrowPart -= originalMainDPS;

	fp->growRectilinear(dpsGrowPart, rect);

	DoughnutPolygonSet dpsShrinkPart;
	for(Tile *const &t : rect->blockTiles){
		dpsShrinkPart += t->getRectangle();
	}

	dpsShrinkPart -= currentRectDPS;
	fp->shrinkRectilinear(dpsShrinkPart, rect);

	return true;
}

bool RefineEngine::growingTowardSouth(Rectilinear *rect, len_t depth) const {
	using namespace boost::polygon::operators;

	Rectangle chipProto = fp->getChipContour();
	len_t chipXH = rec::getXH(chipProto);
	len_t chipYH = rec::getYH(chipProto);

	double globalAspectRatioMin = fp->getGlobalAspectRatioMin();
	double globalAspectRatioMax = fp->getGlobalAspectRatioMax();
	double globalUtilizationMin = fp->getGlobalUtilizationMin();

	/* STEP 1: Ateempt to grow toward the specified direction */
	// determine the growing area
	Rectangle rectBB = rect->calculateBoundingBox();
	len_t BBXL = rec::getXL(rectBB);
	len_t BBXH = rec::getXH(rectBB);
	len_t BBYL = rec::getYL(rectBB);
	len_t BBYH = rec::getYH(rectBB);
	len_t BBWidth = rec::getWidth(rectBB);
	len_t BBHeight = rec::getHeight(rectBB);
	len_t tryGrowYL = BBYL - depth;
	if(tryGrowYL < 0) tryGrowYL = 0;

	if(tryGrowYL >= BBYL) return false;

	Rectangle posGrowArea = Rectangle(BBXL, tryGrowYL, BBXH, BBYL);

	// currentRectDPS stores the shape of rect during the trial process
	DoughnutPolygonSet currentRectDPS;
	// this records all surrounding rectiliears that has possibly changed shape by the growing process
	std::unordered_map<Rectilinear *, DoughnutPolygonSet> affectedNeighbors;

	// initizlize currentRetDPS as it's current shape
	for(Tile *const &t : rect->blockTiles){
		currentRectDPS += t->getRectangle();
	}

	// The process of growing up
	bool growSouth = trialGrow(currentRectDPS, rect, posGrowArea, affectedNeighbors);
	if(!growSouth) return false;

	/* STEP 2: Attempt fix the aspect ratio flaw caused by introducing the growth toward specified direction */
	if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){

		/* STEP 2-1 : Fix the aspect Ratio by Trimming off areas from the Negative Shrink Area of rect */
		Rectangle currentBB = dps::calculateBoundingBox(currentRectDPS);
		len_t currentBBWidth = rec::getWidth(currentBB);
		len_t currentBBHeight = rec::getHeight(currentBB);
		
		double dResidualLen = (currentBBWidth * globalAspectRatioMax) - currentBBHeight;
		if(dResidualLen > 0){
			len_t residualLen = len_t(dResidualLen); // round down naturally
			len_t boundingBoxYHMin = rec::getYH(currentBB) - residualLen;
			if(boundingBoxYHMin == rec::getYL(currentBB)) return false;
			assert(boundingBoxYHMin > rec::getYL(currentBB));
			// shed off the marked part at negative shrink area, by an incremental method
			len_t roundShedOffLen = REFINE_INITIAL_MOMENTUM;
			bool shedMoving = true;
			while((rec::getYH(currentBB) > boundingBoxYHMin) && (shedMoving || (roundShedOffLen != REFINE_INITIAL_MOMENTUM))){
				if(!shedMoving) roundShedOffLen = REFINE_INITIAL_MOMENTUM;
				shedMoving = false;

				len_t currentBBXL = rec::getXL(currentBB);
				len_t currentBBXH = rec::getXH(currentBB);
				len_t currentBBYH = rec::getYH(currentBB);
				len_t tryShrinkYL = currentBBYH - roundShedOffLen;
				if(tryShrinkYL < boundingBoxYHMin) tryShrinkYL = boundingBoxYHMin;
				Rectangle shedOffArea = Rectangle(currentBBXL, tryShrinkYL, currentBBXH, currentBBYH);

				DoughnutPolygonSet xShed(currentRectDPS);
				xShed -= shedOffArea;
				if(dps::oneShape(xShed) && dps::noHole(xShed) && dps::innerWidthLegal(xShed) &&(!xShed.empty())){ // the shed would not affect the shape too much, accept change
					currentRectDPS -= shedOffArea;
					roundShedOffLen = len_t(REFINE_INITIAL_MOMENTUM * REFINE_MOMENTUM_GROWTH + 0.5);
					currentBB = dps::calculateBoundingBox(currentRectDPS);
					bool shedMoving = true;
				}
			}
		}

		/* STEP 2-2 : fix aspect ratio by adding area to the sides, pick the plus side (the one that grow would decrease HPWL)*/
		if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){
			Cord optCentre = fp->calculateOptimalCentre(rect);
			double bbcentreX, bbCentreY;
			currentBB = dps::calculateBoundingBox(currentRectDPS);
			len_t currentBBXL = rec::getXL(currentBB);
			len_t currentBBYL = rec::getYL(currentBB);
			len_t currentBBXH = rec::getXH(currentBB);
			len_t currentBBYH = rec::getYH(currentBB);
			currentBBWidth = rec::getWidth(currentBB);
			currentBBHeight = rec::getHeight(currentBB);
			EVector step22Vector = EVector(FCord(bbcentreX, bbCentreY), optCentre);
			angle_t step22Toward = step22Vector.calculateDirection();

			bool eastPlusSide = (step22Toward > (-(M_PI / 2.0)));
			double dSideGrowingTarget = (currentBBHeight * globalAspectRatioMin) - currentBBWidth;
			assert(dSideGrowingTarget >= 0);
			len_t sideGrowingTarget = len_t(dSideGrowingTarget + 0.5); // round up by trick
			if(eastPlusSide){ // Growing East would improve HPWL compared to grow west
				len_t eastGrowXH = currentBBXH + sideGrowingTarget;
				if(eastGrowXH > chipXH) eastGrowXH = chipXH;
				if(eastGrowXH >= currentBBXH){
					Rectangle eastGrowRange = Rectangle(currentBBXH, currentBBYL, eastGrowXH, currentBBYH);
					fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, eastGrowRange);
				}
			}else{ // Growing West would improve HPwL compared to grow East
				len_t westGrowXL = currentBBXL - sideGrowingTarget;
				if(westGrowXL < 0) westGrowXL = 0;
				if(westGrowXL <= currentBBXL){
					Rectangle westGrowRange = Rectangle(westGrowXL, currentBBYL, currentBBXL, currentBBYH);
					fixAspectRatioWestSideGrow(currentRectDPS, rect, affectedNeighbors, westGrowRange);
				}
			}

			/* STEP 2-3 : fix aspect ratio by adding area to the sides, now we can only grow on the negative side, this is an uphill move */
			if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){
				currentBB = dps::calculateBoundingBox(currentRectDPS);
				len_t currentBBXL = rec::getXL(currentBB);
				len_t currentBBYL = rec::getYL(currentBB);
				len_t currentBBXH = rec::getXH(currentBB);
				len_t currentBBYH = rec::getYH(currentBB);
				currentBBWidth = rec::getWidth(currentBB);
				currentBBHeight = rec::getHeight(currentBB);

				double dSideGrowingTarget = (currentBBHeight * globalAspectRatioMin) - currentBBWidth;
				assert(dSideGrowingTarget >= 0);
				len_t sideGrowingTarget = len_t(dSideGrowingTarget + 0.5); // round up by trick
				if(!eastPlusSide){ // Grown West last time, try East
					len_t eastGrowXH = currentBBXH + sideGrowingTarget;
					if(eastGrowXH > chipXH) eastGrowXH = chipXH;
					if(eastGrowXH >= currentBBXH){
						Rectangle eastGrowRange = Rectangle(currentBBXH, currentBBYL, eastGrowXH, currentBBYH);
						fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, eastGrowRange);
					}
				}else{ // Grown East last time, try West
					len_t westGrowXL = currentBBXL - sideGrowingTarget;
					if(westGrowXL < 0) westGrowXL = 0;
					if(westGrowXL <= currentBBXL){
						Rectangle westGrowRange = Rectangle(westGrowXL, currentBBYL, currentBBXL, currentBBYH);
						fixAspectRatioWestSideGrow(currentRectDPS, rect, affectedNeighbors, westGrowRange);
					}
				}
			}

		}
	}

	// if STEP 2 cannot fix aspect ratio issues, the growing cannot be executed
	if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)) return false;


	/* STEP3 : Attempt to fix the utilization flaws caused by introducing the growth toward specified direction */
	if(!dps::checkIsLegalUtilization(currentRectDPS, globalUtilizationMin)){
		/* STEP 3-1 Attempt to fix the utilization flaw by growing in the bounding box */
		fixUtilizationGrowBoundingBox(currentRectDPS, rect, affectedNeighbors);	
	}

	// conduct a final legality check for all changes that went through STEP 2 ~ 3
	if(!dps::checkIsLegal(currentRectDPS, rect->getLegalArea(), globalAspectRatioMin, globalAspectRatioMax, globalUtilizationMin)) return false;


	/* STEP 4: make sure all changes acutally improves HPWL and reflect the changes */
	bool actuarization = forecastGrowthBenefits(rect, currentRectDPS, affectedNeighbors);
	if(!actuarization) return false;

	// Actually execute all modifications
	// modify the neighbor affected rectilinears first
	for(std::unordered_map<Rectilinear *, DoughnutPolygonSet>::const_iterator it = affectedNeighbors.begin(); it != affectedNeighbors.end(); ++it){
		Rectilinear *neighbor = it->first;
		DoughnutPolygonSet neighborOrignalDPS;
		for(Tile *const &t : neighbor->blockTiles){
			neighborOrignalDPS += t->getRectangle();
		}

		neighborOrignalDPS -= it->second;
		fp->shrinkRectilinear(neighborOrignalDPS, neighbor);
	}

	// The rect part's grow and shrink
	DoughnutPolygonSet originalMainDPS;
	for(Tile *const &t : rect->blockTiles){
		originalMainDPS += t->getRectangle();
	}

	DoughnutPolygonSet dpsGrowPart(currentRectDPS);
	dpsGrowPart -= originalMainDPS;

	fp->growRectilinear(dpsGrowPart, rect);

	DoughnutPolygonSet dpsShrinkPart;
	for(Tile *const &t : rect->blockTiles){
		dpsShrinkPart += t->getRectangle();
	}

	dpsShrinkPart -= currentRectDPS;
	fp->shrinkRectilinear(dpsShrinkPart, rect);

	return true;

}

bool RefineEngine::growingTowardEast(Rectilinear *rect, len_t depth) const {
	using namespace boost::polygon::operators;

	Rectangle chipProto = fp->getChipContour();
	len_t chipXH = rec::getXH(chipProto);
	len_t chipYH = rec::getYH(chipProto);

	double globalAspectRatioMin = fp->getGlobalAspectRatioMin();
	double globalAspectRatioMax = fp->getGlobalAspectRatioMax();
	double globalUtilizationMin = fp->getGlobalUtilizationMin();

	/* STEP 1: Ateempt to grow toward the specified direction */
	// determine the growing area
	Rectangle rectBB = rect->calculateBoundingBox();
	len_t BBXH = rec::getXH(rectBB);
	len_t BBYL = rec::getYL(rectBB);
	len_t BBYH = rec::getYH(rectBB);
	len_t BBWidth = rec::getWidth(rectBB);
	len_t BBHeight = rec::getHeight(rectBB);
	len_t tryGrowXH = BBXH + depth;
	if(tryGrowXH > chipXH) tryGrowXH = chipXH;

	if(tryGrowXH <= BBXH) return false;

	Rectangle posGrowArea = Rectangle(BBXH, BBYL, tryGrowXH, BBYH);

	// currentRectDPS stores the shape of rect during the trial process
	DoughnutPolygonSet currentRectDPS;
	// this records all surrounding rectiliears that has possibly changed shape by the growing process
	std::unordered_map<Rectilinear *, DoughnutPolygonSet> affectedNeighbors;

	// initizlize currentRetDPS as it's current shape
	for(Tile *const &t : rect->blockTiles){
		currentRectDPS += t->getRectangle();
	}

	// The process of growing up
	bool growEast = trialGrow(currentRectDPS, rect, posGrowArea, affectedNeighbors);
	if(!growEast) return false;

	/* STEP 2: Attempt fix the aspect ratio flaw caused by introducing the growth toward specified direction */
	if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){

		/* STEP 2-1 : Fix the aspect Ratio by Trimming off areas from the Negative Shrink Area of rect */
		Rectangle currentBB = dps::calculateBoundingBox(currentRectDPS);
		len_t currentBBWidth = rec::getWidth(currentBB);
		len_t currentBBHeight = rec::getHeight(currentBB);
		
		double dResidualLen = (currentBBHeight * globalAspectRatioMax) - currentBBWidth;
		if(dResidualLen > 0){
			len_t residualLen = len_t(dResidualLen); // round down naturally
			len_t boundingBoxXLMax = rec::getXL(currentBB) + residualLen;
			if(boundingBoxXLMax == rec::getXL(currentBB)) return false;
			assert(boundingBoxXLMax < rec::getXH(currentBB));
			// shed off the marked part at negative shrink area, by an incremental method
			len_t roundShedOffLen = REFINE_INITIAL_MOMENTUM;
			bool shedMoving = true;
			while((rec::getXL(currentBB) < boundingBoxXLMax) && (shedMoving || (roundShedOffLen != REFINE_INITIAL_MOMENTUM))){
				if(!shedMoving) roundShedOffLen = REFINE_INITIAL_MOMENTUM;
				shedMoving = false;

				len_t currentBBXL = rec::getXL(currentBB);
				len_t currentBBYL = rec::getYL(currentBB);
				len_t currentBBYH = rec::getYH(currentBB);
				len_t tryShrinkXH = currentBBXL + roundShedOffLen;
				if(tryShrinkXH > boundingBoxXLMax) tryShrinkXH = boundingBoxXLMax;
				Rectangle shedOffArea = Rectangle(currentBBXL, currentBBYL, tryShrinkXH, currentBBYH);

				DoughnutPolygonSet xShed(currentRectDPS);
				xShed -= shedOffArea;
				if(dps::oneShape(xShed) && dps::noHole(xShed) && dps::innerWidthLegal(xShed) && (!xShed.empty())){ // the shed would not affect the shape too much, accept change
					currentRectDPS -= shedOffArea;
					roundShedOffLen = len_t(REFINE_INITIAL_MOMENTUM * REFINE_MOMENTUM_GROWTH + 0.5);
					currentBB = dps::calculateBoundingBox(currentRectDPS);
					bool shedMoving = true;
				}
			}
		}

		/* STEP 2-2 : fix aspect ratio by adding area to the sides, pick the plus side (the one that grow would decrease HPWL)*/
		if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){
			Cord optCentre = fp->calculateOptimalCentre(rect);
			double bbcentreX, bbCentreY;
			currentBB = dps::calculateBoundingBox(currentRectDPS);
			len_t currentBBXL = rec::getXL(currentBB);
			len_t currentBBYL = rec::getYL(currentBB);
			len_t currentBBXH = rec::getXH(currentBB);
			len_t currentBBYH = rec::getYH(currentBB);
			currentBBWidth = rec::getWidth(currentBB);
			currentBBHeight = rec::getHeight(currentBB);
			EVector step22Vector = EVector(FCord(bbcentreX, bbCentreY), optCentre);
			angle_t step22Toward = step22Vector.calculateDirection();

			bool northPlusSide = (step22Toward >= 0);
			double dSideGrowingTarget = (currentBBWidth * globalAspectRatioMin) - currentBBHeight;
			assert(dSideGrowingTarget >= 0);
			len_t sideGrowingTarget = len_t(dSideGrowingTarget + 0.5); // round up by trick
			if(northPlusSide){ // Growing North would improve HPWL compared to grow South
				len_t northGrowYH = currentBBYH + sideGrowingTarget;
				if(northGrowYH > chipYH) northGrowYH = chipYH;
				if(northGrowYH >= currentBBYH){
					Rectangle northGrowRange = Rectangle(currentBBXL, currentBBYH, currentBBXH, northGrowYH);
					fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, northGrowRange);
				}
			}else{ // Growing South would improve HPwL compared to grow North
				len_t southGrowYL = currentBBYL - sideGrowingTarget;
				if(southGrowYL < 0) southGrowYL = 0;
				if(southGrowYL < currentBBYL){
					Rectangle southGrowRange = Rectangle(currentBBXL, southGrowYL, currentBBXH, currentBBYL);
					fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, southGrowRange);
				}
			}

			/* STEP 2-3 : fix aspect ratio by adding area to the sides, now we can only grow on the negative side, this is an uphill move */
			if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){
				currentBB = dps::calculateBoundingBox(currentRectDPS);
				len_t currentBBXL = rec::getXL(currentBB);
				len_t currentBBYL = rec::getYL(currentBB);
				len_t currentBBXH = rec::getXH(currentBB);
				len_t currentBBYH = rec::getYH(currentBB);
				currentBBWidth = rec::getWidth(currentBB);
				currentBBHeight = rec::getHeight(currentBB);

				double dSideGrowingTarget = (currentBBWidth * globalAspectRatioMin) - currentBBHeight;
				assert(dSideGrowingTarget >= 0);
				len_t sideGrowingTarget = len_t(dSideGrowingTarget + 0.5); // round up by trick
				if(!northPlusSide){ // Growing North would improve HPWL compared to grow South
					len_t northGrowYH = currentBBYH + sideGrowingTarget;
					if(northGrowYH > chipYH) northGrowYH = chipYH;
					if(northGrowYH >= currentBBYH){
						Rectangle northGrowRange = Rectangle(currentBBXL, currentBBYH, currentBBXH, northGrowYH);
						fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, northGrowRange);
					}
				}else{ // Growing South would improve HPwL compared to grow North
					len_t southGrowYL = currentBBYL - sideGrowingTarget;
					if(southGrowYL < 0) southGrowYL = 0;
					if(southGrowYL < currentBBYL){
						Rectangle southGrowRange = Rectangle(currentBBXL, southGrowYL, currentBBXH, currentBBYL);
						fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, southGrowRange);
					}
				}
			}

		}
	}

	// if STEP 2 cannot fix aspect ratio issues, the growing cannot be executed
	if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)) return false;

	/* STEP3 : Attempt to fix the utilization flaws caused by introducing the growth toward specified direction */
	if(!dps::checkIsLegalUtilization(currentRectDPS, globalUtilizationMin)){
		/* STEP 3-1 Attempt to fix the utilization flaw by growing in the bounding box */
		fixUtilizationGrowBoundingBox(currentRectDPS, rect, affectedNeighbors);	
	}

	// conduct a final legality check for all changes that went through STEP 2 ~ 3
	if(!dps::checkIsLegal(currentRectDPS, rect->getLegalArea(), globalAspectRatioMin, globalAspectRatioMax, globalUtilizationMin)) return false;


	/* STEP 4: make sure all changes acutally improves HPWL and reflect the changes */
	bool actuarization = forecastGrowthBenefits(rect, currentRectDPS, affectedNeighbors);
	if(!actuarization) return false;

	// Actually execute all modifications
	// modify the neighbor affected rectilinears first
	for(std::unordered_map<Rectilinear *, DoughnutPolygonSet>::const_iterator it = affectedNeighbors.begin(); it != affectedNeighbors.end(); ++it){
		Rectilinear *neighbor = it->first;
		DoughnutPolygonSet neighborOrignalDPS;
		for(Tile *const &t : neighbor->blockTiles){
			neighborOrignalDPS += t->getRectangle();
		}

		neighborOrignalDPS -= it->second;
		if(neighborOrignalDPS.empty()) continue;
		fp->shrinkRectilinear(neighborOrignalDPS, neighbor);
	}

	// The rect part's grow and shrink
	DoughnutPolygonSet originalMainDPS;
	for(Tile *const &t : rect->blockTiles){
		originalMainDPS += t->getRectangle();
	}

	DoughnutPolygonSet dpsGrowPart(currentRectDPS);
	dpsGrowPart -= originalMainDPS;

	fp->growRectilinear(dpsGrowPart, rect);

	DoughnutPolygonSet dpsShrinkPart;
	for(Tile *const &t : rect->blockTiles){
		dpsShrinkPart += t->getRectangle();
	}

	dpsShrinkPart -= currentRectDPS;
	fp->shrinkRectilinear(dpsShrinkPart, rect);

	return true;

}

bool RefineEngine::growingTowardWest(Rectilinear *rect, len_t depth) const {
	using namespace boost::polygon::operators;

	Rectangle chipProto = fp->getChipContour();
	len_t chipXH = rec::getXH(chipProto);
	len_t chipYH = rec::getYH(chipProto);

	double globalAspectRatioMin = fp->getGlobalAspectRatioMin();
	double globalAspectRatioMax = fp->getGlobalAspectRatioMax();
	double globalUtilizationMin = fp->getGlobalUtilizationMin();

	/* STEP 1: Ateempt to grow toward the specified direction */
	// determine the growing area
	Rectangle rectBB = rect->calculateBoundingBox();
	len_t BBXL = rec::getXL(rectBB);
	len_t BBYL = rec::getYL(rectBB);
	len_t BBYH = rec::getYH(rectBB);
	len_t BBWidth = rec::getWidth(rectBB);
	len_t BBHeight = rec::getHeight(rectBB);
	len_t tryGrowXL = BBXL - depth;
	if(tryGrowXL < 0) tryGrowXL = 0;
	if(tryGrowXL >= BBXL){
		return false;
	}

	Rectangle posGrowArea = Rectangle(tryGrowXL, BBYL, BBXL, BBYH);

	// currentRectDPS stores the shape of rect during the trial process
	DoughnutPolygonSet currentRectDPS;
	// this records all surrounding rectiliears that has possibly changed shape by the growing process
	std::unordered_map<Rectilinear *, DoughnutPolygonSet> affectedNeighbors;

	// initizlize currentRetDPS as it's current shape
	for(Tile *const &t : rect->blockTiles){
		currentRectDPS += t->getRectangle();
	}

	// The process of growing up
	bool growWest = trialGrow(currentRectDPS, rect, posGrowArea, affectedNeighbors);
	// std::cout << "Trial grow west result: " << growWest << " dd " << std::endl;
	// fp->debugFloorplanLegal();
	// std::cout << "Check if current state legal: " << std::endl;
	// std::cout << "currentRectDPS: ";
	// std::cout << dps::checkIsLegal(currentRectDPS, rect->getLegalArea(), fp->getGlobalAspectRatioMin(), fp->getGlobalAspectRatioMax(), fp->getGlobalUtilizationMin()) << std::endl;
	// for(std::unordered_map<Rectilinear *, DoughnutPolygonSet>::iterator it = affectedNeighbors.begin(); it != affectedNeighbors.end(); ++it){
		// std::cout << it->first->getName() << ": ";
		// std::cout << dps::checkIsLegal(currentRectDPS, it->first->getLegalArea(), fp->getGlobalAspectRatioMin(), fp->getGlobalAspectRatioMax(), fp->getGlobalUtilizationMin()) << std::endl;
	// }
	// std::cout << "End check current state" << std::endl;
	if(!growWest) return false;

	/* STEP 2: Attempt fix the aspect ratio flaw caused by introducing the growth toward specified direction */
	if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){

		/* STEP 2-1 : Fix the aspect Ratio by Trimming off areas from the Negative Shrink Area of rect */
		Rectangle currentBB = dps::calculateBoundingBox(currentRectDPS);
		len_t currentBBWidth = rec::getWidth(currentBB);
		len_t currentBBHeight = rec::getHeight(currentBB);
		
		double dResidualLen = (currentBBHeight * globalAspectRatioMax) - currentBBWidth;
		if(dResidualLen > 0){
			len_t residualLen = len_t(dResidualLen); // round down naturally
			len_t boundingBoxXHMin = rec::getXH(currentBB) - residualLen;
			if(boundingBoxXHMin == rec::getXH(currentBB)) return false;
			// Bug Fix , ">" -> ">="
			assert(boundingBoxXHMin > rec::getXL(currentBB));
			// shed off the marked part at negative shrink area, by an incremental method
			len_t roundShedOffLen = REFINE_INITIAL_MOMENTUM;
			bool shedMoving = true;
			while((rec::getXH(currentBB) > boundingBoxXHMin) && (shedMoving || (roundShedOffLen != REFINE_INITIAL_MOMENTUM))){
				if(!shedMoving) roundShedOffLen = REFINE_INITIAL_MOMENTUM;
				shedMoving = false;

				len_t currentBBXH = rec::getXH(currentBB);
				len_t currentBBYL = rec::getYL(currentBB);
				len_t currentBBYH = rec::getYH(currentBB);
				len_t tryShrinkXL = currentBBXH - roundShedOffLen;
				if(tryShrinkXL < boundingBoxXHMin) tryShrinkXL = boundingBoxXHMin;
				Rectangle shedOffArea = Rectangle(tryShrinkXL, currentBBYL, currentBBXH, currentBBYH);

				DoughnutPolygonSet xShed(currentRectDPS);
				xShed -= shedOffArea;
				if(dps::oneShape(xShed) && dps::noHole(xShed) && dps::innerWidthLegal(xShed) && (!xShed.empty())){ // the shed would not affect the shape too much, accept change
					currentRectDPS -= shedOffArea;
					roundShedOffLen = len_t(REFINE_INITIAL_MOMENTUM * REFINE_MOMENTUM_GROWTH + 0.5);
					currentBB = dps::calculateBoundingBox(currentRectDPS);
					bool shedMoving = true;
				}
			}
		}

		/* STEP 2-2 : fix aspect ratio by adding area to the sides, pick the plus side (the one that grow would decrease HPWL)*/
		if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){
			Cord optCentre = fp->calculateOptimalCentre(rect);
			double bbcentreX, bbCentreY;
			currentBB = dps::calculateBoundingBox(currentRectDPS);
			len_t currentBBXL = rec::getXL(currentBB);
			len_t currentBBYL = rec::getYL(currentBB);
			len_t currentBBXH = rec::getXH(currentBB);
			len_t currentBBYH = rec::getYH(currentBB);
			currentBBWidth = rec::getWidth(currentBB);
			currentBBHeight = rec::getHeight(currentBB);
			EVector step22Vector = EVector(FCord(bbcentreX, bbCentreY), optCentre);
			angle_t step22Toward = step22Vector.calculateDirection();

			bool northPlusSide = (step22Toward >= 0);
			double dSideGrowingTarget = (currentBBWidth * globalAspectRatioMin) - currentBBHeight;
			assert(dSideGrowingTarget >= 0);
			len_t sideGrowingTarget = len_t(dSideGrowingTarget + 0.5); // round up by trick
			if(northPlusSide){ // Growing North would improve HPWL compared to grow South
				len_t northGrowYH = currentBBYH + sideGrowingTarget;
				if(northGrowYH > chipYH) northGrowYH = chipYH;
				if(northGrowYH >= currentBBYH){
					Rectangle northGrowRange = Rectangle(currentBBXL, currentBBYH, currentBBXH, northGrowYH);
					fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, northGrowRange);
				}
			}else{ // Growing South would improve HPwL compared to grow North
				len_t southGrowYL = currentBBYL - sideGrowingTarget;
				if(southGrowYL < 0) southGrowYL = 0;
				if(southGrowYL < currentBBYL){
					Rectangle southGrowRange = Rectangle(currentBBXL, southGrowYL, currentBBXH, currentBBYL);
					fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, southGrowRange);
				}
			}

			/* STEP 2-3 : fix aspect ratio by adding area to the sides, now we can only grow on the negative side, this is an uphill move */
			if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)){
				currentBB = dps::calculateBoundingBox(currentRectDPS);
				len_t currentBBXL = rec::getXL(currentBB);
				len_t currentBBYL = rec::getYL(currentBB);
				len_t currentBBXH = rec::getXH(currentBB);
				len_t currentBBYH = rec::getYH(currentBB);
				currentBBWidth = rec::getWidth(currentBB);
				currentBBHeight = rec::getHeight(currentBB);

				double dSideGrowingTarget = (currentBBWidth * globalAspectRatioMin) - currentBBHeight;
				assert(dSideGrowingTarget >= 0);
				len_t sideGrowingTarget = len_t(dSideGrowingTarget + 0.5); // round up by trick
				if(!northPlusSide){ // Growing North would improve HPWL compared to grow South
					len_t northGrowYH = currentBBYH + sideGrowingTarget;
					if(northGrowYH > chipYH) northGrowYH = chipYH;
					if(northGrowYH >= currentBBYH){
						Rectangle northGrowRange = Rectangle(currentBBXL, currentBBYH, currentBBXH, northGrowYH);
						fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, northGrowRange);
					}
				}else{ // Growing South would improve HPwL compared to grow North
					len_t southGrowYL = currentBBYL - sideGrowingTarget;
					if(southGrowYL < 0) southGrowYL = 0;
					if(southGrowYL < currentBBYL){
						Rectangle southGrowRange = Rectangle(currentBBXL, southGrowYL, currentBBXH, currentBBYL);
						fixAspectRatioEastSideGrow(currentRectDPS, rect, affectedNeighbors, southGrowRange);
					}
				}
			}

		}
	}

	// if STEP 2 cannot fix aspect ratio issues, the growing cannot be executed
	if(!dps::checkIsLegalAspectRatio(currentRectDPS, globalAspectRatioMin, globalAspectRatioMax)) return false;

	/* STEP3 : Attempt to fix the utilization flaws caused by introducing the growth toward specified direction */
	if(!dps::checkIsLegalUtilization(currentRectDPS, globalUtilizationMin)){
		/* STEP 3-1 Attempt to fix the utilization flaw by growing in the bounding box */
		fixUtilizationGrowBoundingBox(currentRectDPS, rect, affectedNeighbors);	
	}

	// conduct a final legality check for all changes that went through STEP 2 ~ 3
	if(!dps::checkIsLegal(currentRectDPS, rect->getLegalArea(), globalAspectRatioMin, globalAspectRatioMax, globalUtilizationMin)) return false;

	/* STEP 4: make sure all changes acutally improves HPWL and reflect the changes */
	bool actuarization = forecastGrowthBenefits(rect, currentRectDPS, affectedNeighbors);
	if(!actuarization) return false;

	// Actually execute all modifications
	// modify the neighbor affected rectilinears first
	for(std::unordered_map<Rectilinear *, DoughnutPolygonSet>::const_iterator it = affectedNeighbors.begin(); it != affectedNeighbors.end(); ++it){
		Rectilinear *neighbor = it->first;
		DoughnutPolygonSet neighborOrignalDPS;
		for(Tile *const &t : neighbor->blockTiles){
			neighborOrignalDPS += t->getRectangle();
		}

		neighborOrignalDPS -= it->second;
		fp->shrinkRectilinear(neighborOrignalDPS, neighbor);
	}

	// The rect part's grow and shrink
	DoughnutPolygonSet originalMainDPS;
	for(Tile *const &t : rect->blockTiles){
		originalMainDPS += t->getRectangle();
	}

	DoughnutPolygonSet dpsGrowPart(currentRectDPS);
	dpsGrowPart -= originalMainDPS;

	fp->growRectilinear(dpsGrowPart, rect);

	DoughnutPolygonSet dpsShrinkPart;
	for(Tile *const &t : rect->blockTiles){
		dpsShrinkPart += t->getRectangle();
	}

	dpsShrinkPart -= currentRectDPS;
	fp->shrinkRectilinear(dpsShrinkPart, rect);

	return true;

}

bool RefineEngine::fixAspectRatioEastSideGrow(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect,
std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors, Rectangle &eastGrow) const {

	len_t growAreaXH = rec::getXH(eastGrow);
	len_t roundGrowLen = REFINE_INITIAL_MOMENTUM;
	bool hasGrown = false;
	bool growMoving = true;
	Rectangle boundingBox = dps::calculateBoundingBox(currentRectDPS);
	while((rec::getXH(boundingBox) < growAreaXH) && (growMoving || (roundGrowLen != REFINE_INITIAL_MOMENTUM))){
		growMoving = false;
		if(!growMoving) roundGrowLen = REFINE_INITIAL_MOMENTUM;

		len_t boundingBoxXH = rec::getXH(boundingBox);
		len_t boundingBoxYL = rec::getYL(boundingBox);
		len_t boundingBoxYH = rec::getYH(boundingBox);

		len_t roundTargetXH = boundingBoxXH + roundGrowLen;
		if(roundTargetXH > growAreaXH) roundTargetXH = growAreaXH;

		Rectangle growingBluePrint = Rectangle(boundingBoxXH, boundingBoxYL, roundTargetXH, boundingBoxYH);
		bool growResult = trialGrow(currentRectDPS, rect, growingBluePrint, affectedNeighbors);
		if(growResult){
			hasGrown = true;
			growMoving = true;
			boundingBox = dps::calculateBoundingBox(currentRectDPS);
			roundGrowLen = len_t (roundGrowLen * REFINE_MOMENTUM_GROWTH + 0.5);
		}
	}

	return hasGrown;
}

bool RefineEngine::fixAspectRatioWestSideGrow(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, 
std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors, Rectangle &westGrow) const {

	len_t growAreaXL = rec::getXL(westGrow);
	len_t roundGrowLen = REFINE_INITIAL_MOMENTUM;
	bool hasGrown = false;
	bool growMoving = true;
	Rectangle boundingBox = dps::calculateBoundingBox(currentRectDPS);
	while((rec::getXL(boundingBox) > growAreaXL) && (growMoving || (roundGrowLen != REFINE_INITIAL_MOMENTUM))){
		growMoving = false;
		if(!growMoving) roundGrowLen = REFINE_INITIAL_MOMENTUM;

		len_t boundingBoxXL = rec::getXL(boundingBox);
		len_t boundingBoxYL = rec::getYL(boundingBox);
		len_t boundingBoxYH = rec::getYH(boundingBox);

		len_t roundTargetXL = boundingBoxXL - roundGrowLen;
		if(roundTargetXL < growAreaXL) roundTargetXL = growAreaXL;

		Rectangle growingBluePrint = Rectangle(roundTargetXL, boundingBoxYL, boundingBoxXL, boundingBoxYH);
		bool growResult = trialGrow(currentRectDPS, rect, growingBluePrint, affectedNeighbors);
		if(growResult){
			hasGrown = true;
			growMoving = true;
			boundingBox = dps::calculateBoundingBox(currentRectDPS);
			roundGrowLen = len_t (roundGrowLen * REFINE_MOMENTUM_GROWTH + 0.5);
		}
	}

	return hasGrown;
}

bool RefineEngine::fixAspectRatioNorthSideGrow(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, 
std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors, Rectangle &northGrow) const {

	len_t growAreaYH = rec::getYH(northGrow);
	len_t roundGrowLen = REFINE_INITIAL_MOMENTUM;
	bool hasGrown = false;
	bool growMoving = true;
	Rectangle boundingBox = dps::calculateBoundingBox(currentRectDPS);
	while((rec::getYH(boundingBox) < growAreaYH) && (growMoving || (roundGrowLen != REFINE_INITIAL_MOMENTUM))){
		growMoving = false;
		if(!growMoving) roundGrowLen = REFINE_INITIAL_MOMENTUM;

		len_t boundingBoxXL = rec::getXL(boundingBox);
		len_t boundingBoxXH = rec::getXH(boundingBox);
		len_t boundingBoxYH = rec::getYH(boundingBox);

		len_t roundTargetYH = boundingBoxYH + roundGrowLen;
		if(roundTargetYH > growAreaYH) roundTargetYH = growAreaYH;

		Rectangle growingBluePrint = Rectangle(boundingBoxXL, boundingBoxYH, boundingBoxXH, roundTargetYH);
		bool growResult = trialGrow(currentRectDPS, rect, growingBluePrint, affectedNeighbors);
		if(growResult){
			hasGrown = true;
			growMoving = true;
			boundingBox = dps::calculateBoundingBox(currentRectDPS);
			roundGrowLen = len_t (roundGrowLen * REFINE_MOMENTUM_GROWTH + 0.5);
		}
	}

	return hasGrown;
}

bool RefineEngine::fixAspectRatioSouthSideGrow(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, 
std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors, Rectangle &southGrow) const{

	len_t growAreaYL = rec::getYL(southGrow);
	len_t roundGrowLen = REFINE_INITIAL_MOMENTUM;
	bool hasGrown = false;
	bool growMoving = true;
	Rectangle boundingBox = dps::calculateBoundingBox(currentRectDPS);
	while((rec::getYL(boundingBox) > growAreaYL) && (growMoving || (roundGrowLen != REFINE_INITIAL_MOMENTUM))){
		growMoving = false;
		if(!growMoving) roundGrowLen = REFINE_INITIAL_MOMENTUM;

		len_t boundingBoxXL = rec::getXL(boundingBox);
		len_t boundingBoxXH = rec::getXH(boundingBox);
		len_t boundingBoxYL = rec::getYL(boundingBox);

		len_t roundTargetYL = boundingBoxYL - roundGrowLen;
		if(roundTargetYL < growAreaYL) roundTargetYL = growAreaYL;

		Rectangle growingBluePrint = Rectangle(boundingBoxXL, roundTargetYL, boundingBoxXH, boundingBoxYL);
		bool growResult = trialGrow(currentRectDPS, rect, growingBluePrint, affectedNeighbors);
		if(growResult){
			hasGrown = true;
			growMoving = true;
			boundingBox = dps::calculateBoundingBox(currentRectDPS);
			roundGrowLen = len_t (roundGrowLen * REFINE_MOMENTUM_GROWTH + 0.5);
		}
	}

	return hasGrown;
}

bool RefineEngine::fixUtilizationGrowBoundingBox(DoughnutPolygonSet &currentRectDPS, Rectilinear *rect, 
std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors) const{
	using namespace boost::polygon::operators;

	Rectangle rectBB = rect->calculateBoundingBox();
	DoughnutPolygonSet petriDishDPS, blankDPS;
	petriDishDPS += rectBB;
	blankDPS += rectBB;

	// find all tiles that crosssect the area, find it and sieve those unfit
	std::vector<Tile *> tilesinPetriDish;	
	std::unordered_map<Rectilinear *, std::vector<Tile *>> invlovedTiles;
	fp->cs->enumerateDirectedArea(rectBB, tilesinPetriDish);
	for(Tile *const &t : tilesinPetriDish){
		Rectilinear *tilesRectilinear = fp->blockTilePayload[t];
		Rectangle tileRectangle = t->getRectangle();
		blankDPS -= tileRectangle;
		if(tilesRectilinear->getType() == rectilinearType::PREPLACED) continue;
		if(tilesRectilinear == rect) continue;

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
	if(growCandidateSize == 0) return false;

	// if there are candidates Rectilinears that are invloved and not yet registerd in affectedNeighbor, register them
	for(Rectilinear *const &ivr : involvedRectilinears){
		if(affectedNeighbors.find(ivr) == affectedNeighbors.end()){
			// this rectilinear is not yet present in affected Neighbor history, add
			for(Tile *const &t : ivr->blockTiles){
				affectedNeighbors[ivr] += t->getRectangle();
			}
		}
	}

	bool keepGrowing = true;		
	bool hasGrow = false;
	bool growCandidateSelect [growCandidateSize]; // This records which candidate is selected
	for(int i = 0; i < growCandidateSize; ++i){
		growCandidateSelect[i] = false;
	}

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
				DoughnutPolygonSet xVictimDPS(affectedNeighbors[victimRect]);
				xVictimDPS -= markedDP;

				// if removing the piece makes the victim rectilinear illegal, quit	
				if(!dps::checkIsLegal(xVictimDPS, victimRect->getLegalArea(), fp->getGlobalAspectRatioMin(), 
				fp->getGlobalAspectRatioMax(), fp->getGlobalUtilizationMin())){
					continue;
				}
				// If removing the piece acutally harms the HPWL (the other piece increases HPWL by moving)
				if(rectConnWeightSum.at(victimRect) > rectConnWeightSum.at(rect)){
					Cord optimalCentre = fp->calculateOptimalCentre(victimRect);

					Rectangle beforeBB = dps::calculateBoundingBox(affectedNeighbors[victimRect]);
					double beforeBBX, beforeBBY;
					rec::calculateCentre(beforeBB, beforeBBX, beforeBBY);
					double beforeScore = std::abs(beforeBBX - optimalCentre.x()) + std::abs(beforeBBY - optimalCentre.y());

					Rectangle afterBB = dps::calculateBoundingBox(xVictimDPS);
					double afterBBX, afterBBY;
					rec::calculateCentre(afterBB, afterBBX, afterBBY);
					double afterScore = std::abs(afterBBX - optimalCentre.x()) + std::abs(afterBBY - optimalCentre.y());

					if(beforeScore < afterScore) continue;
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
				affectedNeighbors[victimRect] -= markedDP;
			}
		}
	}

	return hasGrow;


}


bool RefineEngine::forecastGrowthBenefits(Rectilinear *rect, DoughnutPolygonSet &currentRectDPS,
std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbors) const {

	// for testing 
	// return true;
	double originalHPWL = fp->calculateHPWL();	
	double afterHPWL = 0;

	std::unordered_map<Rectilinear *, FCord> rectCentreCache;

	for(Connection *const &c : fp->allConnections){

		Rectilinear *firstRect = c->vertices[0];
		double boundingBoxCentreX, boundingBoxCentreY;
		if(rectCentreCache.find(firstRect) == rectCentreCache.end()){
			// Cache miss, calculate the centre and cache it
			Rectangle firstRectBB;
			if(firstRect == rect){
				firstRectBB = dps::calculateBoundingBox(currentRectDPS);
			}else if(affectedNeighbors.find(firstRect) != affectedNeighbors.end()){
				firstRectBB = dps::calculateBoundingBox(affectedNeighbors[firstRect]);
			}else{
				firstRectBB = firstRect->calculateBoundingBox();
			}
			rec::calculateCentre(firstRectBB, boundingBoxCentreX, boundingBoxCentreY);
			rectCentreCache[firstRect] = FCord(boundingBoxCentreX, boundingBoxCentreY);
		}else{
			FCord cachedCentre = rectCentreCache[firstRect];
			boundingBoxCentreX = cachedCentre.x();
			boundingBoxCentreY = cachedCentre.y();
		}

		double minX, maxX, minY, maxY;
		minX = maxX = boundingBoxCentreX;
		minY = maxY = boundingBoxCentreY;

		for(int i = 1; i < c->vertices.size(); ++i){
			Rectilinear *otherRects = c->vertices[i];
			if(rectCentreCache.find(otherRects) == rectCentreCache.end()){
				// Cache miss, calculate the centre and cache it
				Rectangle otherRectBB;
				if(otherRects == rect){
					otherRectBB = dps::calculateBoundingBox(currentRectDPS);
				}else if(affectedNeighbors.find(otherRects) != affectedNeighbors.end()){
					otherRectBB = dps::calculateBoundingBox(affectedNeighbors[otherRects]);
				}else{
					otherRectBB = otherRects->calculateBoundingBox();
				}
				rec::calculateCentre(otherRectBB, boundingBoxCentreX, boundingBoxCentreY);
				rectCentreCache[otherRects] = FCord(boundingBoxCentreX, boundingBoxCentreY);
			}else{
				FCord cachedCentre = rectCentreCache[otherRects];
				boundingBoxCentreX = cachedCentre.x();
				boundingBoxCentreY = cachedCentre.y();
			}

			if(boundingBoxCentreX < minX) minX = boundingBoxCentreX;
			if(boundingBoxCentreX > maxX) maxX = boundingBoxCentreX;
			if(boundingBoxCentreY < minY) minY = boundingBoxCentreY;
			if(boundingBoxCentreY > maxY) maxY = boundingBoxCentreY;
		}

		afterHPWL += (c->weight * ((maxX - minX)+(maxY - minY)));
	}

	return (originalHPWL > afterHPWL);
}

bool RefineEngine::refineByTrimming(Rectilinear *rect) const {

	area_t residualArea = rect->calculateResidualArea();
	if(residualArea <= 0) return false;

	bool keepMoving = true;
	bool hasMove = false;
	int momentum = REFINE_INITIAL_MOMENTUM;
	while(keepMoving || (momentum != REFINE_INITIAL_MOMENTUM)){
		if(!keepMoving) momentum = REFINE_INITIAL_MOMENTUM;
		
		keepMoving = false;

		sector rectTowardSector;

		if(!REFINE_USE_GRADIENT_SHRINK){
			Cord rectOptimalCentre = fp->calculateOptimalCentre(rect);
			double rectBBCentreX, rectBBCentreY;
			Rectangle rectBB = rect->calculateBoundingBox();
			
			rec::calculateCentre(rectBB, rectBBCentreX, rectBBCentreY);
			FCord rectBBCentre (rectBBCentreX, rectBBCentreY);

			EVector rectToward(rectBBCentre, rectOptimalCentre);
			angle_t rectTowardAngle = rectToward.calculateDirection();
			// change the angle from toward to behind, for trimming
			rectTowardAngle = flipAngle(rectTowardAngle);
			rectTowardSector = translateAngleToSector(rectTowardAngle);

		}else{
			EVector gradient;
			if(!fp->calculateRectilinearGradient(rect, gradient)) return false;
			rectTowardSector = translateAngleToSector(flipAngle(gradient.calculateDirection()));

		}

		switch (rectTowardSector){
			case sector::ONE: {
				// Try East, then North
				bool trimEast = executeRefineTrimming(rect, direction2D::EAST, momentum);
				if(trimEast){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool trimNorth = executeRefineTrimming(rect, direction2D::NORTH, momentum);
				if(trimNorth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}	
			case sector::TWO: {
				// Try North, then East
				bool trimNorth = executeRefineTrimming(rect, direction2D::NORTH, momentum);
				if(trimNorth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool trimEast = executeRefineTrimming(rect, direction2D::EAST, momentum);
				if(trimEast){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::THREE: {
				// Try North, then West
				bool trimNorth = executeRefineTrimming(rect, direction2D::NORTH, momentum);
				if(trimNorth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool trimWest = executeRefineTrimming(rect, direction2D::WEST, momentum);
				if(trimWest){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::FOUR: {
				// Try West, then North
				bool trimWest = executeRefineTrimming(rect, direction2D::WEST, momentum);
				if(trimWest){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool trimNorth = executeRefineTrimming(rect, direction2D::NORTH, momentum);
				if(trimNorth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::FIVE: {
				// Try West, then South
				bool trimWest = executeRefineTrimming(rect, direction2D::WEST, momentum);
				if(trimWest){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool trimSouth = executeRefineTrimming(rect, direction2D::SOUTH, momentum);
				if(trimSouth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::SIX: {
				// Try South, then West
				bool trimSouth = executeRefineTrimming(rect, direction2D::SOUTH, momentum);
				if(trimSouth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool trimWest = executeRefineTrimming(rect, direction2D::WEST, momentum);
				if(trimWest){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::SEVEN: {
				// Try South, then East
				bool trimSouth = executeRefineTrimming(rect, direction2D::SOUTH, momentum);
				if(trimSouth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool trimEast = executeRefineTrimming(rect, direction2D::EAST, momentum);
				if(trimEast){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			case sector::EIGHT: {
				// Try East, then South
				bool trimEast = executeRefineTrimming(rect, direction2D::EAST, momentum);
				if(trimEast){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				bool trimSouth = executeRefineTrimming(rect, direction2D::SOUTH, momentum);
				if(trimSouth){
					keepMoving = true;
					hasMove = true;
					momentum = len_t(momentum * REFINE_MOMENTUM_GROWTH + 0.5);
					continue;
				}

				break;
			}
			default:
				throw CSException("REFINEENGINE_02");
				break;
		}
	}

	return hasMove;
}

bool RefineEngine::executeRefineTrimming(Rectilinear *rect, direction2D direction, len_t detph) const {
	using namespace boost::polygon::operators;

	if((direction != direction2D::NORTH) && (direction != direction2D::SOUTH) && (direction != direction2D::EAST) && (direction != direction2D::WEST)){
		throw CSException("REFINEENGINE_01");
	}
	
	Rectangle currentBB = rect->calculateBoundingBox();
	len_t currentBBXL = rec::getXL(currentBB);
	len_t currentBBXH = rec::getXH(currentBB);
	len_t currentBBYH = rec::getYH(currentBB);
	len_t currentBBYL = rec::getYL(currentBB);
	len_t currentBBWidth = rec::getWidth(currentBB);
	len_t currentbBHeight = rec::getHeight(currentBB);

	// determine the rectangle to trim off
	Rectangle trimRectangle;
	switch (direction)
	{
		case direction2D::NORTH:{
			len_t targetTrimYL = (currentBBYH - detph);
			if(targetTrimYL <= currentBBYL) return false;
			trimRectangle = Rectangle(currentBBXL, targetTrimYL, currentBBXH, currentBBYH);
			break;
		}
		case direction2D::SOUTH:{
			len_t targetTrimYH = (currentBBYL + detph);
			if(targetTrimYH >= currentBBYH) return false;
			trimRectangle = Rectangle(currentBBXL, currentBBYL, currentBBXH, targetTrimYH);
			break;
		}
		case direction2D::EAST:{
			len_t targetTrimXL = (currentBBXH - detph);
			if(targetTrimXL <= currentBBXL) return false;
			trimRectangle = Rectangle(targetTrimXL, currentBBYL, currentBBXH, currentBBYH);
			break;
		}
		case direction2D::WEST:{
			len_t targetTrimXH = (currentBBXL + detph);
			if(targetTrimXH >= currentBBXH) return false;
			trimRectangle = Rectangle(currentBBXL, currentBBYL, targetTrimXH, currentBBYH);
			break;
		}
		default:
			throw CSException("REFINEENGINE_01");
			break;
	}

	DoughnutPolygonSet currentRectDPS;
	for(Tile *const &t : rect->blockTiles){
		currentRectDPS += t->getRectangle();	
	}

	DoughnutPolygonSet trimPart(currentRectDPS);
	trimPart &= trimRectangle;
	if(trimPart.empty()) return false;

	currentRectDPS -= trimRectangle;

	bool trimIsLegal = dps::checkIsLegal(currentRectDPS, rect->getLegalArea(), fp->getGlobalAspectRatioMin(), fp->getGlobalAspectRatioMax(), fp->getGlobalUtilizationMin());
	// GREEDY
	std::unordered_map<Rectilinear *, DoughnutPolygonSet> affectedNeighbors;
	bool actuarization = forecastGrowthBenefits(rect, currentRectDPS, affectedNeighbors);
	if(!actuarization) return false;
	
	if(trimIsLegal){
		fp->shrinkRectilinear(trimPart, rect);
		Rectangle newBB = rect->calculateBoundingBox();
		double bbx, bby;
		rec::calculateCentre(newBB, bbx, bby);

		return true;
	}else{
		return false;
	}
}

bool RefineEngine::trialGrow(DoughnutPolygonSet &dpSet, Rectilinear *dpSetRect ,Rectangle &growArea, std::unordered_map<Rectilinear *, DoughnutPolygonSet> &affectedNeighbor) const {
	using namespace boost::polygon::operators;

	DoughnutPolygonSet petriDishDPS, blankDPS;
	petriDishDPS += growArea;
	blankDPS += growArea;

	// find all tiles that crosssect the area, find it and sieve those unfit
	std::vector<Tile *> tilesinPetriDish;	
	std::unordered_map<Rectilinear *, std::vector<Tile *>> invlovedTiles;
	fp->cs->enumerateDirectedArea(growArea, tilesinPetriDish);
	for(Tile *const &t : tilesinPetriDish){
		Rectilinear *tilesRectilinear = fp->blockTilePayload[t];
		Rectangle tileRectangle = t->getRectangle();
		blankDPS -= tileRectangle;
		if(tilesRectilinear->getType() == rectilinearType::PREPLACED) continue;

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
		[&](Rectilinear *A, Rectilinear *B) {return this->rectConnWeightSum.at(A) < rectConnWeightSum.at(B);});

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
		rectDPS &= growArea;
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
	if(growCandidateSize == 0) return false;

	// actually grow these condidate fragments
	// register those invloved Rectilinears that is not registered in affectedNeighbors
	for(Rectilinear *const &ivr : involvedRectilinears){
		if(affectedNeighbor.find(ivr) == affectedNeighbor.end()){
			// this rectilinear is not yet present in affected Neighbor history, add
			for(Tile *const &t : ivr->blockTiles){
				affectedNeighbor[ivr] += t->getRectangle();
			}
		}
	}

	bool keepGrowing = true;		
	bool hasGrow = false;
	bool growCandidateSelect [growCandidateSize]; // This records which candidate is selected
	for(int i = 0; i < growCandidateSize; ++i){
		growCandidateSelect[i] = false;
	}

	while(keepGrowing){
		keepGrowing = false;
		for(int i = 0; i < growCandidateSize; ++i){
			if(growCandidateSelect[i]) continue;

			DoughnutPolygon markedDP = growCandidates[i];
			// test if installing this marked part would turn the current shape strange

			DoughnutPolygonSet xSelfDPS(dpSet);
			xSelfDPS += markedDP;
			if(!(dps::oneShape(xSelfDPS) && dps::noHole(xSelfDPS) && dps::innerWidthLegal(xSelfDPS))){
				// intrducing the markedDP would cause dpSet to generate strange shape, give up on the tile
				continue;
			}

			// test if pulling off the marked part would harm the victim too bad, skip if markedDP belongs to blank
			Rectilinear *victimRect = growCandidatesRectilinear[i];
			if(victimRect != nullptr){

				DoughnutPolygonSet xVictimDPS(affectedNeighbor[victimRect]);
				xVictimDPS -= markedDP;

				if(!dps::checkIsLegal(xVictimDPS, victimRect->getLegalArea(), fp->getGlobalAspectRatioMin(), 
					fp->getGlobalAspectRatioMax(), fp->getGlobalUtilizationMin())){
					continue;
				}
				if(rectConnWeightSum.at(victimRect) > rectConnWeightSum.at(dpSetRect)){
					Cord optimalCentre = fp->calculateOptimalCentre(victimRect);

					Rectangle beforeBB = dps::calculateBoundingBox(affectedNeighbor[victimRect]);
					double beforeBBX, beforeBBY;
					rec::calculateCentre(beforeBB, beforeBBX, beforeBBY);
					double beforeScore = std::abs(beforeBBX - optimalCentre.x()) + std::abs(beforeBBY - optimalCentre.y());

					Rectangle afterBB = dps::calculateBoundingBox(xVictimDPS);
					double afterBBX, afterBBY;
					rec::calculateCentre(afterBB, afterBBX, afterBBY);
					double afterScore = std::abs(afterBBX - optimalCentre.x()) + std::abs(afterBBY - optimalCentre.y());

					if(beforeScore < afterScore) continue;
				}
			}

			// pass all tests, distribute the tile
			growCandidateSelect[i] = true;
			keepGrowing = true;
			hasGrow = true;
			dpSet += markedDP;
			DoughnutPolygonSet markedDPS;
			markedDPS += markedDP;
			if(victimRect != nullptr){ // markedDP belongs to other Rectilienar
				affectedNeighbor[victimRect] -= markedDP;
			}
		}
	}

	return hasGrow;
}

angle_t RefineEngine::calculateBestAngle(Rectilinear *rect) const {

    double centreX, centreY;
    rec::calculateCentre(rect->calculateBoundingBox(), centreX, centreY);
    Cord bbCentre = Cord(len_t(centreX), len_t(centreY));

    Cord optimalCentre = fp->calculateOptimalCentre(rect);

    EVector directionVector(bbCentre, optimalCentre);
    return directionVector.calculateDirection();

}
