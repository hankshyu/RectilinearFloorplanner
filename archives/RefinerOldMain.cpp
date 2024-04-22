#include <iostream>
#include <random>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iomanip>

#include "boost/polygon/polygon.hpp"
#include "colours.h"
#include "units.h"
#include "cord.h"
#include "line.h"
#include "rectangle.h"
#include "tile.h"
#include "cornerStitching.h"
#include "cSException.h"
#include "rectilinear.h"
#include "doughnutPolygon.h"
#include "doughnutPolygonSet.h"
#include "globalResult.h"
#include "floorplan.h"

#include "eVector.h"
#include "lineTile.h"
#include "legalResult.h"

#include "legaliseEngine.h"
#include "refineEngine.h"

int main(int argc, char const *argv[]) {

	try{
		using namespace boost::polygon::operators;

		EVector v1(Cord(0, 0), Cord(-1 ,0));
		std::cout << v1.calculateDirection() << std::endl;

		std::cout << CYAN << "Rectilinear Floorplan Tool" << COLORRST << std::endl << std::endl;
		using namespace boost::polygon::operators;

		GlobalResult gr;
		gr.readGlobalResult("inputs/iccadNew/case04-output.txt");
		
		// assert(argc == 3);
		// std::string inputDir = std::string(argv[1]);
		// std::string inputCase = std::string(argv[2]);

		// std::cout << argv[1] << std::endl;
		// gr.readGlobalResult(std::string(argv[1]));
		// gr.readGlobalResult(inputLoc);


		Floorplan fp(gr, 0.5, 2, 0.8);
		LegaliseEngine le(&fp);

		rectilinearIllegalType rtIllegalType;
		fp.visualiseFloorplan("outputs/case01-phase0-fp.txt");
		if(fp.checkFloorplanLegal(rtIllegalType) == nullptr){
			std::cout << "[GlobalResultPorting] Legal Solution Acquired !!! Final HPWL = " << fp.calculateHPWL() << std::endl;
			goto LEGAL_FOUND;
		}else{
			std::cout << "[GlobalResultPorting] Floorplan Illegal, current HPWL = " << fp.calculateHPWL() << std::endl << std::endl;
		}


		fp.removePrimitiveOvelaps(true);
		fp.visualiseFloorplan("outputs/case01-phase1-fp.txt");

		if(fp.checkFloorplanLegal(rtIllegalType) == nullptr){
			std::cout << "[PrimitiveOverlapRemoval] Legal Solution Acquired !!! Final HPWL = " << fp.calculateHPWL() << std::endl;
			goto LEGAL_FOUND;	
		}else{
			std::cout << "[PrimitiveOverlapRemoval] Floorplan Illegal, current HPWL = " << fp.calculateHPWL() << ", illegal cause = " << rtIllegalType << std::endl << std::endl;
		}


		le.distributeOverlap();
		fp.visualiseFloorplan("outputs/case01-phase2-fp.txt");

		if(fp.checkFloorplanLegal(rtIllegalType) == nullptr){
			std::cout << "[OverlapDistribution] Legal Solution Acquired !!! Final HPWL = " << fp.calculateHPWL() << std::endl;
			goto LEGAL_FOUND;
		}else{
			std::cout << "[OverlapDistribution] Not yet legal, current HPWL = " << fp.calculateHPWL() << ", illegal cause = " << rtIllegalType << std::endl;
		}
		

		le.huntFixAtypicalShapes();
		le.softRectilinearIllegalReport();
		le.legalise();

		fp.visualiseFloorplan("outputs/case01-phase3-fp.txt");
		// fp.cs->visualiseCornerStitching("outputs/case01-cs.txt");

		if(fp.checkFloorplanLegal(rtIllegalType) == nullptr){
			std::cout << "[Legalise] Legal Solution Acquired !!! Final HPWL = " << fp.calculateHPWL() << std::endl;
			goto LEGAL_FOUND;
		}else{
			std::cout << "[Legalise] Not yet legal, current HPWL = " << fp.calculateHPWL() << ", illegal cause = " << rtIllegalType << std::endl;
			le.softRectilinearIllegalReport();
			goto LEGAL_NOT_FOUND;
		}


	LEGAL_FOUND:{

		std::cout << YELLOW << "Tool Exit Successfully from Leal Found Section" << COLORRST << std::endl;
		double beforeRefineHPWL = fp.calculateHPWL();
		RefineEngine refiner(&fp);
		refiner.refine();
		fp.visualiseFloorplan("outputs/case01-phase4-fp.txt");
		double finalHPWL = fp.calculateHPWL();
		if(finalHPWL < beforeRefineHPWL){
		std::cout << "[RefineEngine] Refinement reduces HPWL from " << beforeRefineHPWL << " -> " << finalHPWL << " ";
		std::cout << GREEN << " -" << (beforeRefineHPWL - finalHPWL) << "(" << (beforeRefineHPWL - finalHPWL)*100.0 /beforeRefineHPWL << "%)" << COLORRST << std::endl;
		}else{
			std::cout << MAGENTA <<"[RefineEngine] Refinement does not improve HPWL" << COLORRST << std::endl;
		}

		// final Legal checking
		for(Rectilinear *rt : fp.softRectilinears){
			rectilinearIllegalType rit;
			assert(rt->isLegal(rit));
		}
		std::cout << YELLOW << "PASS FINAL LEGAL CHECK >< " << std::endl;
		return 0;
	}

	LEGAL_NOT_FOUND:{
		std::cout << MAGENTA << "Tool Exit Successfully from Not Legal Section" << COLORRST << std::endl;
		return 4;
	}
	}catch(CSException &c){
		std::cout << "Exception caught -> ";
		std::cout << c.what() << std::endl;
		abort();
	}catch(...){
		std::cout << "Excpetion not caught but aborted!" << std::endl;
		abort();
	}

}