#include <fstream>

#include "cornerStitching.h"
#include "cSException.h"

bool CornerStitching::checkPointInCanvas(const Cord &point) const{
	return rec::isContained(mCanvasSizeBlankTile->getRectangle(), point);
}

bool CornerStitching::checkRectangleInCanvas(const Rectangle &rect) const{
	return rec::isContained(mCanvasSizeBlankTile->getRectangle(), rect);
}

void CornerStitching::collectAllTiles(std::unordered_set<Tile *> &allTiles) const{
	// If there are no tiles, just return the blank tile
	if(mAllNonBlankTilesMap.empty()){
		allTiles.insert(this->mCanvasSizeBlankTile);
		return;
	}

	// call herlper function, Use DFS to travese the entire graph, this funciton uses a random tile as seed.
	collectAllTilesDFS(mAllNonBlankTilesMap.begin()->second, allTiles);
}

void CornerStitching::collectAllTilesDFS(Tile *currentSearch, std::unordered_set<Tile *> &allTiles) const{
	
	allTiles.insert(currentSearch);
	
	if(currentSearch->rt != nullptr){
		if(allTiles.find(currentSearch->rt) == allTiles.end()){
			collectAllTilesDFS(currentSearch->rt, allTiles);
		}
	}
	if(currentSearch->tr != nullptr){
		if(allTiles.find(currentSearch->tr) == allTiles.end()){
			collectAllTilesDFS(currentSearch->tr, allTiles);
		}
	}
	if(currentSearch->bl != nullptr){
		if(allTiles.find(currentSearch->bl) == allTiles.end()){
			collectAllTilesDFS(currentSearch->bl, allTiles);
		}
	}
	if(currentSearch->lb != nullptr){
		if(allTiles.find(currentSearch->lb) == allTiles.end()){
			collectAllTilesDFS(currentSearch->lb, allTiles);
		}
	}
}

void CornerStitching::enumerateDirectedAreaRProcedure(Rectangle box, std::vector <Tile *> &allTiles, Tile *targetTile) const{

	// R1) Enumerate the tile
	tileType targetTileType = targetTile->getType();
	if((targetTileType == tileType::BLOCK) ||(targetTileType == tileType::OVERLAP)){
		allTiles.push_back(targetTile);
	}

	// R2) If the right edge of the tile is outside (or touch) the seearch area, return
	if(targetTile->getXHigh() >= rec::getXH(box)) return;

	// R3) Use neighbor-finding algo to locate all the tiles that touch the right side of the current tile and also intersect the search area
	std::vector<Tile *> rightNeighbors;
	findRightNeighbors(targetTile, rightNeighbors);

	Rectangle targetTileRect = targetTile->getRectangle();

	for(Tile *const &t : rightNeighbors){
		// make sure the tile is in the AOI
		if(!rec::hasIntersect(t->getRectangle(), box, false)) continue;

		// R4) If bottom left corner of the neighbor touches the current tile
		bool R4 = (t->getYLow() >= targetTile->getYLow());

		// R5) If the bottom edge ofthe search area cuts both the target tile and its neighbor
		len_t bottomEdge = rec::getYL(box);
		bool cutTargetTile = (targetTile->getYLow() <= bottomEdge) && (targetTile->getYHigh() > bottomEdge);
		bool cutNeighbor = (t->getYLow() <= bottomEdge) && (t->getYHigh() > bottomEdge);
		bool R5 = cutTargetTile && cutNeighbor;

		if(R4 || R5) enumerateDirectedAreaRProcedure(box, allTiles, t);
	}

}

void CornerStitching::findLineTileHorizontalPositive(Tile *initTile, Line line, std::vector<LineTile> &positiveSide){

	if(initTile == nullptr){
		throw CSException("CORNERSTITCHING_20");
	}

	Tile *index = initTile;
	len_t lineY = line.getLow().y();
	len_t lineXLow = line.getLow().x();
	len_t lineXHigh = line.getHigh().x();

	len_t indexXLow = index->getXLow();
	len_t indexXHigh = index->getXHigh();
	len_t indexYLow = index->getYLow();
	len_t indexYHigh = index->getYHigh();

	bool yDownInRange = (lineY >= indexYLow);
	bool yUpInRange = (lineY < indexYHigh);
	bool yCordInTile = (yDownInRange && yUpInRange);
	
	bool lineToTileLeft = (lineXHigh <= indexXLow);
	bool lineToTileRight = (lineXLow >= indexXHigh);

	// find a tile that the line tangents with it's lower edge or crosses it
	while(!(yCordInTile && (!lineToTileLeft) && (!lineToTileRight))){
		if(!yCordInTile){
			//adjust vertical range
			if(lineY > indexYLow){
				assert(index->rt != nullptr);
				index = index->rt;

			}else{
				assert(index->lb != nullptr);
				index = index->lb;
			}
		}else{
			// Vertical range correct! adjust horizontal range
			if(lineToTileLeft){
				assert(index->bl != nullptr);
				index = index->bl;
			}else{
				assert(index->tr != nullptr);
				index = index->tr;
			}
		}

		indexXLow = index->getXLow();
		indexXHigh = index->getXHigh();
		indexYLow = index->getYLow();
		indexYHigh = index->getYHigh();

		yDownInRange = (lineY >= indexYLow);
		yUpInRange = (lineY < indexYHigh);
		yCordInTile = (yDownInRange && yUpInRange);
		
		lineToTileLeft = (lineXHigh <= indexXLow);
		lineToTileRight = (lineXLow >= indexXHigh);
	}

	Cord inTileCordLeft = (lineXLow > indexXLow)? Cord(lineXLow, lineY) : Cord(indexXLow, lineY);
	Cord inTileCordRight = (lineXHigh < indexXHigh)? Cord(lineXHigh, lineY) : Cord(indexXHigh, lineY);

	if(indexXLow > lineXLow) findLineTileHorizontalPositive(index, Line(line.getLow(), inTileCordLeft), positiveSide);
	positiveSide.push_back(LineTile(Line(inTileCordLeft, inTileCordRight), index));
	if(indexXHigh < lineXHigh) findLineTileHorizontalPositive(index, Line(inTileCordRight, line.getHigh()), positiveSide);
}

void CornerStitching::findLineTileHorizontalNegative(Tile *initTile, Line line, std::vector<LineTile> &negativeSide){

	if(initTile == nullptr){
		throw CSException("CORNERSTITCHING_21");
	}

	Tile *index = initTile;
	len_t lineY = line.getLow().y();
	len_t lineXLow = line.getLow().x();
	len_t lineXHigh = line.getHigh().x();

	len_t indexXLow = index->getXLow();
	len_t indexXHigh = index->getXHigh();
	len_t indexYLow = index->getYLow();
	len_t indexYHigh = index->getYHigh();

	bool yDownInRange = (lineY > indexYLow);
	bool yUpInRange = (lineY <= indexYHigh);
	bool yCordInTile = (yDownInRange && yUpInRange);
	
	bool lineToTileLeft = (lineXHigh <= indexXLow);
	bool lineToTileRight = (lineXLow >= indexXHigh);

	// find a tile that the line tangents with it's lower edge or crosses it
	while(!(yCordInTile && (!lineToTileLeft) && (!lineToTileRight))){
		if(!yCordInTile){
			//adjust vertical range
			if(lineY > indexYLow){
				assert(index->rt != nullptr);
				index = index->rt;

			}else{
				assert(index->lb != nullptr);
				index = index->lb;
			}
		}else{
			// Vertical range correct! adjust horizontal range
			if(lineToTileLeft){
				assert(index->bl != nullptr);
				index = index->bl;
			}else{
				assert(index->tr != nullptr);
				index = index->tr;
			}
		}

		indexXLow = index->getXLow();
		indexXHigh = index->getXHigh();
		indexYLow = index->getYLow();
		indexYHigh = index->getYHigh();

		yDownInRange = (lineY > indexYLow);
		yUpInRange = (lineY <= indexYHigh);
		yCordInTile = (yDownInRange && yUpInRange);
		
		lineToTileLeft = (lineXHigh <= indexXLow);
		lineToTileRight = (lineXLow >= indexXHigh);
	}

	Cord inTileCordLeft = (lineXLow > indexXLow)? Cord(lineXLow, lineY) : Cord(indexXLow, lineY);
	Cord inTileCordRight = (lineXHigh < indexXHigh)? Cord(lineXHigh, lineY) : Cord(indexXHigh, lineY);

	if(indexXLow > lineXLow) findLineTileHorizontalNegative(index, Line(line.getLow(), inTileCordLeft), negativeSide);
	negativeSide.push_back(LineTile(Line(inTileCordLeft, inTileCordRight), index));
	if(indexXHigh < lineXHigh) findLineTileHorizontalNegative(index, Line(inTileCordRight, line.getHigh()), negativeSide);
}

void CornerStitching::findLineTileVerticalPositive(Tile *initTile, Line line, std::vector<LineTile> &positiveSide){

	// if(initTile == nullptr){
	// 	throw CSException("CORNERSTITCHING_22");
	// }
	
	Tile *index = initTile;
	len_t lineX = line.getLow().x();
	len_t lineYLow = line.getLow().y();
	len_t lineYHigh = line.getHigh().y();

	len_t indexXLow = index->getXLow();
	len_t indexXHigh = index->getXHigh();
	len_t indexYLow = index->getYLow();
	len_t indexYHigh = index->getYHigh();

	bool xDownInRange = (lineX >= indexXLow);
	bool xUpInRange = (lineX < indexXHigh);
	bool xCordInTile = (xDownInRange && xUpInRange);
	
	bool lineToTileDown = (lineYHigh <= indexYLow);
	bool lineToTileUp = (lineYLow >= indexYHigh);

	while(!(xCordInTile && (!lineToTileDown) && (!lineToTileUp))){
		if(!xCordInTile){
			//adjust horizontal range
			if(lineX > indexXLow){
				assert(index->tr != nullptr);
				index = index->tr;
			}else{
				assert(index->bl != nullptr);
				index = index->bl;
			}
		}else{
			// horizontal range correct! adjust vertical range
			if(lineToTileDown){
				assert(index->lb != nullptr);
				index = index->lb;
			}else{
				assert(index->rt != nullptr);
				index = index->rt;
			}
		}

		indexXLow = index->getXLow();
		indexXHigh = index->getXHigh();
		indexYLow = index->getYLow();
		indexYHigh = index->getYHigh();

		xDownInRange = (lineX >= indexXLow);
		xUpInRange = (lineX < indexXHigh);
		xCordInTile = (xDownInRange && xUpInRange);
		
		lineToTileDown = (lineYHigh <= indexYLow);
		lineToTileUp = (lineYLow >= indexYHigh);
	}

	Cord inTileCordDown = (lineYLow > indexYLow)? Cord(lineX, lineYLow) : Cord(lineX, indexYLow);
	Cord inTileCordUp = (lineYHigh < indexYHigh)? Cord(lineX, lineYHigh) : Cord(lineX, indexYHigh);

	if(indexYLow > lineYLow) findLineTileVerticalPositive(index, Line(line.getLow(), inTileCordDown), positiveSide);
	positiveSide.push_back(LineTile(Line(inTileCordDown, inTileCordUp), index));
	if(indexYHigh < lineYHigh) findLineTileVerticalPositive(index, Line(inTileCordUp, line.getHigh()), positiveSide);
}

void CornerStitching::findLineTileVerticalNegative(Tile *initTile, Line line, std::vector<LineTile> &negativeSide){

	// if(initTile == nullptr){
	// 	throw CSException("CORNERSTITCHING_23");
	// }

	Tile *index = initTile;
	len_t lineX = line.getLow().x();
	len_t lineYLow = line.getLow().y();
	len_t lineYHigh = line.getHigh().y();

	len_t indexXLow = index->getXLow();
	len_t indexXHigh = index->getXHigh();
	len_t indexYLow = index->getYLow();
	len_t indexYHigh = index->getYHigh();

	bool xDownInRange = (lineX > indexXLow);
	bool xUpInRange = (lineX <= indexXHigh);
	bool xCordInTile = (xDownInRange && xUpInRange);
	
	bool lineToTileDown = (lineYHigh <= indexYLow);
	bool lineToTileUp = (lineYLow >= indexYHigh);

	while(!(xCordInTile && (!lineToTileDown) && (!lineToTileUp))){
		if(!xCordInTile){
			//adjust horizontal range
			if(lineX > indexXLow){
				assert(index->tr != nullptr);
				index = index->tr;
			}else{
				assert(index->bl != nullptr);
				index = index->bl;
			}
		}else{
			// horizontal range correct! adjust vertical range
			if(lineToTileDown){
				assert(index->lb != nullptr);
				index = index->lb;
			}else{
				assert(index->rt != nullptr);
				index = index->rt;
			}
		}

		indexXLow = index->getXLow();
		indexXHigh = index->getXHigh();
		indexYLow = index->getYLow();
		indexYHigh = index->getYHigh();

		xDownInRange = (lineX > indexXLow);
		xUpInRange = (lineX <= indexXHigh);
		xCordInTile = (xDownInRange && xUpInRange);
		
		lineToTileDown = (lineYHigh <= indexYLow);
		lineToTileUp = (lineYLow >= indexYHigh);
	}

	Cord inTileCordDown = (lineYLow > indexYLow)? Cord(lineX, lineYLow) : Cord(lineX, indexYLow);
	Cord inTileCordUp = (lineYHigh < indexYHigh)? Cord(lineX, lineYHigh) : Cord(lineX, indexYHigh);


	if(indexYLow > lineYLow) findLineTileVerticalNegative(index, Line(line.getLow(), inTileCordDown), negativeSide);
	negativeSide.push_back(LineTile(Line(inTileCordDown, inTileCordUp), index));
	if(indexYHigh < lineYHigh) findLineTileVerticalNegative(index, Line(inTileCordUp, line.getHigh()), negativeSide);
}

CornerStitching::CornerStitching()
	: mCanvasWidth(1), mCanvasHeight(1) {
		mCanvasSizeBlankTile = new Tile(tileType::BLANK, Cord(0, 0), 1, 1);
}

CornerStitching::CornerStitching(len_t chipWidth, len_t chipHeight)
	: mCanvasWidth(chipWidth), mCanvasHeight(chipHeight) {
		// if((chipWidth <= 0) || (chipHeight <= 0)){
		// 	throw CSException("CORNERSTITCHING_04");
		// }
		mCanvasSizeBlankTile = new Tile(tileType::BLANK, Cord(0, 0), chipWidth, chipHeight);
}

CornerStitching::CornerStitching(const CornerStitching &other){
	this->mCanvasWidth = other.mCanvasWidth;
	this->mCanvasHeight = other.mCanvasHeight;
	this->mCanvasSizeBlankTile = new Tile(*(other.mCanvasSizeBlankTile));
	
	if(other.mAllNonBlankTilesMap.size() == 0) return;

	std::unordered_set <Tile *> oldAllTiles;
	other.collectAllTiles(oldAllTiles);

	std::unordered_map <Tile *, Tile *> oldNewPairs;
	for(Tile *const &oldTile : oldAllTiles){
		oldNewPairs[oldTile] = new Tile(*oldTile);
	}

	// maintain the pointers of the new Tiles, while simultanuously maintain mAllNonBlankTilesMap
	for(std::unordered_map <Tile *, Tile *>::iterator it = oldNewPairs.begin(); it != oldNewPairs.end(); ++it){

		Tile *father = it->first;
		Tile *son = it->second;

		// insert to mAllNonBlankTilesMap if tile is not BLANK
		if(son->getType() != tileType::BLANK){
			this->mAllNonBlankTilesMap[son->getLowerLeft()] = son;
		}

		// maintain the links using the map data-structure
		if(father->rt == nullptr) son->rt = nullptr;
		else son->rt = oldNewPairs[father->rt];
		
		if(father->tr == nullptr) son->tr = nullptr;
		else son->tr = oldNewPairs[father->tr];

		if(father->bl == nullptr) son->bl = nullptr;
		else son->bl = oldNewPairs[father->bl];

		if(father->lb == nullptr) son->lb = nullptr;
		else son->lb = oldNewPairs[father->lb];
	}

}

CornerStitching::~CornerStitching(){
	delete(mCanvasSizeBlankTile);

	// If there is no BLOCK or OVERLAP tiles, remove the only blank tile and return
	if(mAllNonBlankTilesMap.size() == 0) return;
	
	std::unordered_set<Tile *> allOldTiles;
	collectAllTiles(allOldTiles);
	for(Tile *const &oldTiles : allOldTiles){
		delete(oldTiles);
	}

}

bool CornerStitching::operator == (const CornerStitching &other) const{

	// compare the canvas info
	if((mCanvasWidth != other.mCanvasWidth) || (mCanvasHeight != other.mCanvasHeight)) return false;
	// Collect all tiles in ours and others
	std::unordered_set<Tile *> oursAllTiles;
	this->collectAllTiles(oursAllTiles);

	std::unordered_set<Tile *> theirsAllTiles;
	other.collectAllTiles(theirsAllTiles);

	if(oursAllTiles.size() != theirsAllTiles.size()) return false;

	// Categorize collected tiles using their tileType
	std::vector<Tile *>ourBlockArr;
	std::vector<Tile *>ourBlankArr;
	std::vector<Tile *>ourOverlapArr;
	for(Tile *const &t : oursAllTiles){
		tileType tType = t->getType();
		if(tType == tileType::BLOCK) ourBlockArr.push_back(t);
		else if(tType == tileType::BLANK) ourBlankArr.push_back(t);
		else if(tType == tileType::OVERLAP) ourOverlapArr.push_back(t);
	}

	std::unordered_map <Cord, Tile *> theirBlockMap;
	std::unordered_map <Cord, Tile *> theirBlankMap;
	std::unordered_map <Cord, Tile *> theirOverlapMap;
	for(Tile *const &t : theirsAllTiles){
		tileType tType = t->getType();
		if(tType == tileType::BLOCK) theirBlockMap[t->getLowerLeft()] = t;
		else if(tType == tileType::BLANK) theirBlankMap[t->getLowerLeft()] = t;
		else if(tType == tileType::OVERLAP) theirOverlapMap[t->getLowerLeft()] = t;
	}

	if(ourBlockArr.size() != theirBlockMap.size()) return false;
	if(ourBlankArr.size() != theirBlankMap.size()) return false;
	if(ourOverlapArr.size() != theirOverlapMap.size()) return false;

	// Create a hash to map each our tiles to theris
	std::unordered_map <Tile *, Tile *> tileMap;

	// Map tileType::BLOCK tiles from ours to theirs
	for(Tile *const &t : ourBlockArr){
		std::unordered_map <Cord, Tile *>::iterator it = theirBlockMap.find(t->getLowerLeft());
		// if not found than there is no matching tile in ours and thiers
		if(it == theirBlockMap.end()) return false;

		// check if two rectangles are equal
		if(t->getRectangle() != (it->second->getRectangle())) return false;

		// add to link
		tileMap[t] = it->second;

		theirBlockMap.erase(it);
	}
	//there exist excessive elements, indicating that at least two ours tile map to the same thiers tile
	if(!theirBlockMap.empty()) return false;


	// Map tileType::BLANK tiles from ours to theirs
	for(Tile *const &t : ourBlankArr){
		std::unordered_map <Cord, Tile *>::iterator it = theirBlankMap.find(t->getLowerLeft());
		// if not found than there is no matching tile in ours and thiers
		if(it == theirBlankMap.end()) return false;

		// check if two rectangles are equal
		if(t->getRectangle() != (it->second->getRectangle())) return false;

		// add to link
		tileMap[t] = it->second;

		theirBlankMap.erase(it);
	}
	if(!theirBlankMap.empty()) return false;

	// Map tileType::OVERLAP tiles from ours to theirs
	for(Tile *const &t : ourOverlapArr){
		std::unordered_map <Cord, Tile *>::iterator it = theirOverlapMap.find(t->getLowerLeft());
		// if not found than there is no matching tile in ours and thiers
		if(it == theirOverlapMap.end()) return false;

		// check if two rectangles are equal
		if(t->getRectangle() != (it->second->getRectangle())) return false;

		// add to link
		tileMap[t] = it->second;

		theirOverlapMap.erase(it);
	}
	if(!theirOverlapMap.empty()) return false;

	// Check the links are consistent
	for(Tile *const &t : oursAllTiles){
	   Tile *theirTile = tileMap[t]; 

	   // check rt links is correct
	   if(t->rt == nullptr){
			if(theirTile->rt != nullptr) return false;
	   }else{
			if(tileMap[t->rt] != theirTile->rt) return false;
	   }

	   // check if tr link is correct
	   if(t->tr == nullptr){
			if(theirTile->tr != nullptr) return false;
	   }else{
			if(tileMap[t->tr] != theirTile->tr) return false;
	   }

	   // check if bl link is correct
	   if(t->bl == nullptr){
			if(theirTile->bl != nullptr) return false;
	   }else{
			if(tileMap[t->bl] != theirTile->bl) return false;
	   }

		// check if lb link is correct
		if(t->lb == nullptr){
			if(theirTile->lb != nullptr) return false;
		}else{
			if(tileMap[t->lb] != theirTile->lb) return false;
		}

	}
	
	// all checkings done, two cornerStitching is equal
	return true;

	
}

len_t CornerStitching::getCanvasWidth() const{
	return this->mCanvasWidth;
}

len_t CornerStitching::getCanvasHeight() const{
	return this->mCanvasHeight;
}

Tile *CornerStitching::findPoint(const Cord &key) const{

	// throw exception if point finding (key) out of canvas range
	// if(!checkPointInCanvas(key)){
	// 	throw CSException("CORNERSTITCHING_01");
	// }

	// Find a seed to start, if empty just return the blank tile.
	Tile *index;
	if(mAllNonBlankTilesMap.empty()){
		return mCanvasSizeBlankTile;
	}else{
		index = mAllNonBlankTilesMap.begin()->second; 
	} 

	while(!(rec::isContained(index->getRectangle(), key))){

		Rectangle indexRec = index->getRectangle();
		bool yDownInRange = (key.y() >= rec::getYL(indexRec));
		bool yUpInRange = (key.y() < rec::getYH(indexRec));
		bool YCordInTile = (yDownInRange && yUpInRange);

		if(!YCordInTile){
			// Adjust vertical range
			if(key.y() >= index->getYLow()){
				assert(index->rt != nullptr);
				index = index->rt;
			}else{
				assert(index->lb != nullptr);
				index = index->lb;
			}
		}else{
			// Vertical range correct! adjust horizontal range
			if(key.x() >= index->getXLow()){
				assert(index->tr != nullptr);
				index = index->tr;
			}else{
				assert(index->bl != nullptr);
				index = index->bl;
			}
		}
	}
	
	return index;
}   

void CornerStitching::findLineTile(Line &line, std::vector<LineTile> &positiveSide, std::vector<LineTile> &negativeSide){
	
	// throw exception if the line is out of canvas range
	// fix bug 2024/04/12, allow the line to attatch to left border
	
	// if(!(checkPointInCanvas(line.getLow()) && checkPointInCanvas(line.getHigh()))){
	Cord lineLow = line.getLow();
	Cord lineHigh = line.getHigh();
	len_t lineLowX = lineLow.x();
	len_t lineLowY = lineLow.y();
	len_t lineHighX = lineHigh.x();
	len_t lineHighY = lineHigh.y();
	bool lineLowLegal = (lineLowX >= 0) && (lineLowX <= mCanvasWidth) && (lineLowY >= 0) && (lineLowY <= mCanvasHeight);
	bool lineHighLegal = (lineHighX >= 0) && (lineHighX <= mCanvasWidth) && (lineHighY >= 0) && (lineHighY <= mCanvasHeight);
	// if(!(lineLowLegal && lineHighLegal)){
	// 	throw CSException("CORNERSTITCHING_19");
	// }

	// Find a seed to start, if empty just return the blank tile.
	Tile *index;
	if(mAllNonBlankTilesMap.empty()){
		// empty, return the blank tile
		positiveSide.push_back(LineTile(line, mCanvasSizeBlankTile));
		negativeSide.push_back(LineTile(line, mCanvasSizeBlankTile));

		return;
	}else{
		index = mAllNonBlankTilesMap.begin()->second; 
	} 

	if(line.getOrient() == orientation2D::HORIZONTAL){
		len_t lineY = line.getLow().y();
		len_t lineXLow = line.getLow().x();
		len_t lineXHigh = line.getHigh().x();

		len_t indexXLow = index->getXLow();
		len_t indexXHigh = index->getXHigh();
		len_t indexYLow = index->getYLow();
		len_t indexYHigh = index->getYHigh();

		bool yDownInRange = (lineY >= indexYLow);
		bool yUpInRange = (lineY <= indexYHigh);
		bool yCordInTile = (yDownInRange && yUpInRange);
		
		bool lineToTileLeft = (lineXHigh <= indexXLow);
		bool lineToTileRight = (lineXLow >= indexXHigh);

		// find a tile that tangents or is crossed by the line to start with
		while(!(yCordInTile && (!lineToTileLeft) && (!lineToTileRight))){

			if(!yCordInTile){
				//adjust vertical range
				if(lineY > indexYLow){
					assert(index->rt != nullptr);
					index = index->rt;

				}else{
					assert(index->lb != nullptr);
					index = index->lb;
				}
			}else{
				// Vertical range correct! adjust horizontal range
				if(lineToTileLeft){
					assert(index->bl != nullptr);
					index = index->bl;
				}else{
					assert(index->tr != nullptr);
					index = index->tr;
				}
			}

			indexXLow = index->getXLow();
			indexXHigh = index->getXHigh();
			indexYLow = index->getYLow();
			indexYHigh = index->getYHigh();

			yDownInRange = (lineY >= indexYLow);
			yUpInRange = (lineY <= indexYHigh);
			yCordInTile = (yDownInRange && yUpInRange);
			
			lineToTileLeft = (lineXHigh <= indexXLow);
			lineToTileRight = (lineXLow >= indexXHigh);
		}

		Cord inTileCordLeft = (lineXLow > indexXLow)? Cord(lineXLow, lineY) : Cord(indexXLow, lineY);
		Cord inTileCordRight = (lineXHigh < indexXHigh)? Cord(lineXHigh, lineY) : Cord(indexXHigh, lineY);

		if(lineY == indexYLow){ // index tile belongs to the positive side
			// collect the positive part (up)
			if(indexXLow > lineXLow) findLineTileHorizontalPositive(index, Line(line.getLow(), inTileCordLeft), positiveSide);
			positiveSide.push_back(LineTile(Line(inTileCordLeft, inTileCordRight), index));
			if(indexXHigh < lineXHigh) findLineTileHorizontalPositive(index, Line(inTileCordRight, line.getHigh()), positiveSide);


			// collect the negative part (down)
			if(lineY != mCanvasSizeBlankTile->getYLow()){
				findLineTileHorizontalNegative(index, line, negativeSide);
			}
			
		}else if(lineY == indexYHigh){ // index tile belongs to the negative side
			// collect the positive part
			if(lineY != mCanvasSizeBlankTile->getYHigh()){
				findLineTileHorizontalPositive(index, line, positiveSide);
			}

			// collect the negative part
			if(indexXLow > lineXLow) findLineTileHorizontalNegative(index, Line(line.getLow(), inTileCordLeft), negativeSide);
			negativeSide.push_back(LineTile(Line(inTileCordLeft, inTileCordRight), index));
			if(indexXHigh < lineXHigh) findLineTileHorizontalNegative(index, Line(inTileCordRight, line.getHigh()), negativeSide);
			
		}else{ // the line pierces through the tile
			if(indexXLow > lineXLow){
				findLineTileHorizontalPositive(index, Line(line.getLow(), inTileCordLeft), positiveSide);
				findLineTileHorizontalNegative(index, Line(line.getLow(), inTileCordLeft), negativeSide);
			}
			positiveSide.push_back(LineTile(Line(inTileCordLeft, inTileCordRight), index));
			negativeSide.push_back(LineTile(Line(inTileCordLeft, inTileCordRight), index));
			if(indexXHigh < lineXHigh){
				findLineTileHorizontalPositive(index, Line(inTileCordRight, line.getHigh()), positiveSide);
				findLineTileHorizontalNegative(index, Line(inTileCordRight, line.getHigh()), negativeSide);
			}
		}

	}else{ // orientation2D::VERTICAL
		len_t lineX = line.getLow().x();
		len_t lineYLow = line.getLow().y();
		len_t lineYHigh = line.getHigh().y();

		len_t indexXLow = index->getXLow();
		len_t indexXHigh = index->getXHigh();
		len_t indexYLow = index->getYLow();
		len_t indexYHigh = index->getYHigh();

		bool xDownInRange = (lineX >= indexXLow);
		bool xUpInRange = (lineX <= indexXHigh);
		bool xCordInTile = (xDownInRange && xUpInRange);
		
		bool lineToTileDown = (lineYHigh <= indexYLow);
		bool lineToTileUp = (lineYLow >= indexYHigh);

		while(!(xCordInTile && (!lineToTileDown) && (!lineToTileUp))){
			if(!xCordInTile){
				//adjust horizontal range
				if(lineX > indexXLow){
					assert(index->tr != nullptr);
					index = index->tr;
				}else{
					assert(index->bl != nullptr);
					index = index->bl;
				}
			}else{
				// horizontal range correct! adjust vertical range
				if(lineToTileDown){
					assert(index->lb != nullptr);
					index = index->lb;
				}else{
					assert(index->rt != nullptr);
					index = index->rt;
				}
			}

			indexXLow = index->getXLow();
			indexXHigh = index->getXHigh();
			indexYLow = index->getYLow();
			indexYHigh = index->getYHigh();

			xDownInRange = (lineX >= indexXLow);
			xUpInRange = (lineX <= indexXHigh);
			xCordInTile = (xDownInRange && xUpInRange);
			
			lineToTileDown = (lineYHigh <= indexYLow);
			lineToTileUp = (lineYLow >= indexYHigh);
		}

		Cord inTileCordDown = (lineYLow > indexYLow)? Cord(lineX, lineYLow) : Cord(lineX, indexYLow);
		Cord inTileCordUp = (lineYHigh < indexYHigh)? Cord(lineX, lineYHigh) : Cord(lineX, indexYHigh);

		if(lineX == indexXLow){ // index tile belongs to the positve side (tile is to the line's right)
			if(indexYLow > lineYLow) findLineTileVerticalPositive(index, Line(line.getLow(), inTileCordDown), positiveSide);
			positiveSide.push_back(LineTile(Line(inTileCordDown, inTileCordUp), index));
			if(indexYHigh < lineYHigh) findLineTileVerticalPositive(index, Line(inTileCordUp, line.getHigh()), positiveSide);

			// collect the negative side
			if(lineX != mCanvasSizeBlankTile->getXLow()){
				findLineTileVerticalNegative(index, line, negativeSide);
			}

		}else if(lineX == indexXHigh){ // index tile belongs to the positve side (tile is to the line's left)
		 	// collect the positive side
			if(lineX != mCanvasSizeBlankTile->getXHigh()){
				findLineTileVerticalPositive(index, line, positiveSide);
			}

			if(indexYLow > lineYLow) findLineTileVerticalNegative(index, Line(line.getLow(), inTileCordDown), negativeSide);
			negativeSide.push_back(LineTile(Line(inTileCordDown, inTileCordUp), index));
			if(indexYHigh < lineYHigh) findLineTileVerticalNegative(index, Line(inTileCordUp, line.getHigh()), negativeSide);

		}else{ // the line pierces throught the tile

			if(indexYLow > lineYLow){
				findLineTileVerticalPositive(index, Line(line.getLow(), inTileCordDown), positiveSide);
				findLineTileVerticalNegative(index, Line(line.getLow(), inTileCordDown), negativeSide);
			}
			positiveSide.push_back(LineTile(Line(inTileCordDown, inTileCordUp), index));
			negativeSide.push_back(LineTile(Line(inTileCordDown, inTileCordUp), index));
			if(indexYHigh < lineYHigh){
				findLineTileVerticalPositive(index, Line(inTileCordUp, line.getHigh()), positiveSide);
				findLineTileVerticalNegative(index, Line(inTileCordUp, line.getHigh()), negativeSide);
			}

		}
	}
}

void CornerStitching::findTopNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const{
	if(centre == nullptr) return;
	if(centre->rt == nullptr) return;

	Tile *n = centre->rt;
	while(n->getXLow() > centre->getXLow()){
	
		neighbors.push_back(n);
		n = n->bl;
	}
	neighbors.push_back(n);
}

void CornerStitching::findDownNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const{
	if(centre == nullptr) return;
	if(centre->lb == nullptr) return;

	Tile *n = centre->lb;
	while(n->getXHigh() < centre->getXHigh()){
		neighbors.push_back(n);
		n = n->tr;
	}
	neighbors.push_back(n);
}

void CornerStitching::findLeftNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const{
	if(centre == nullptr) return;
	if(centre->bl == nullptr) return;

	Tile *n = centre->bl;
	while(n->getYHigh() < centre->getYHigh()){
		neighbors.push_back(n);
		n = n->rt;
	}
	neighbors.push_back(n);
}

void CornerStitching::findRightNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const{
	if(centre == nullptr) return;
	if(centre->tr == nullptr) return;

	Tile *n = centre->tr;
	// the last neighbor is the first tile encountered whose lower y cord <= lower y cord of starting tile
	while(n->getYLow() > centre->getYLow()){
		neighbors.push_back(n);
		n = n->lb;
	}
	neighbors.push_back(n);
}

void CornerStitching::findAllNeighbors(Tile *centre, std::vector<Tile *> &neighbors) const{
	if(centre == nullptr) return;

	findTopNeighbors(centre, neighbors);
	findLeftNeighbors(centre, neighbors);
	findDownNeighbors(centre, neighbors);
	findRightNeighbors(centre, neighbors);
}

bool CornerStitching::searchArea(Rectangle box) const{

	// if(!checkRectangleInCanvas(box)){
	// 	throw CSException("CORNERSTITCHING_02");
	// }

	// Use point-finding algo to locate the tile containin the upperleft corner of AOI
	Tile *currentFind = findPoint(Cord(rec::getXL(box), rec::getYH(box) - 1));

	while(currentFind->getYHigh() > rec::getYL(box)){
		// check if the tile is solid
		if(currentFind->getType() != tileType::BLANK){
			// This is an edge of a solid tile
			return true;
		}else if(currentFind->getXHigh() < rec::getXH(box)){
			// See if the right edge within AOI, right must be a tile
			return true;
		}

		// Move down to the next tile touching the left edge of AOI
		if(currentFind->getYLow() < 1) break;

		currentFind = findPoint(Cord(rec::getXL(box), currentFind->getYLow() - 1));
	}

	return false;
}

bool CornerStitching::searchArea(Rectangle box, Tile *someTile) const{

	// if(!checkRectangleInCanvas(box)){
	// 	throw CSException("CORNERSTITCHING_02");
	// }

	// Use point-finding algo to locate the tile containin the upperleft corner of AOI
	Tile *currentFind = findPoint(Cord(rec::getXL(box), rec::getYH(box) - 1));

	while(currentFind->getYHigh() > rec::getYL(box)){
		// check if the tile is solid
		if(currentFind->getType() != tileType::BLANK){
			// This is an edge of a solid tile
			someTile = currentFind;
			return true;
		}else if(currentFind->getXHigh() < rec::getXH(box)){
			// See if the right edge within AOI, right must be a tile
			someTile = currentFind;
			return true;
		}

		// Move down to the next tile touching the left edge of AOI
		if(currentFind->getYLow() < 1) break;

		currentFind = findPoint(Cord(rec::getXL(box), currentFind->getYLow() - 1));
	}

	return false;
}

void CornerStitching::enumerateDirectedArea(Rectangle box, std::vector <Tile *> &allTiles) const{
	// modified by ryan
	// don't throw error
	// modify box to fit inside canvas
	/*
	bool intersects = gtl::intersects(box, mCanvasSizeBlankTile->getRectangle(), false);
	if (!intersects){
		return;
	}

	// modify box
	gtl::intersect(box, mCanvasSizeBlankTile->getRectangle(), false); 
	*/

	// modified by Hank, sorry pal, safeties first

	// if(!checkRectangleInCanvas(box)){
	// 	throw CSException("CORNERSTITCHING_03");
	// }

	// Use point-finding algo to locate the tile containin the upperleft corner of AOI
	Tile *leftTouchTile = findPoint(Cord(rec::getXL(box), rec::getYH(box) - 1));

	while(leftTouchTile->getYHigh() > rec::getYL(box)){
		enumerateDirectedAreaRProcedure(box, allTiles, leftTouchTile);
		if(leftTouchTile->getYLow() < 1) break;
		// step to the next tile along the left edge
		leftTouchTile = findPoint(Cord(rec::getXL(box), leftTouchTile->getYLow() - 1 ));
	}
}

Tile *CornerStitching::insertTile(const Tile &tile){
	// check if the input prototype is within the canvas
	// if(!checkRectangleInCanvas(tile.getRectangle())){
	// 	throw CSException("CORNERSTITCHING_06");
	// }

	// // check if the tile inserting position already exist anotehr tile
	// if(searchArea(tile.getRectangle())){
	// 	throw CSException("CORNERSTITCHING_07");
	// }
	// // insert tile type shall not be BLANK
	// if(tile.getType() == tileType::BLANK){
	// 	throw CSException("CORNERSTITCHING_12");
	// }

	// Special case when inserting the first tile in the system
	if(mAllNonBlankTilesMap.empty()){
		Tile *tdown, *tup, *tleft, *tright;

		bool hasDownTile = (tile.getYLow() != mCanvasSizeBlankTile->getYLow());
		bool hasUpTile = (tile.getYHigh() != mCanvasSizeBlankTile->getYHigh());
		bool hasLeftTile = (tile.getXLow() != mCanvasSizeBlankTile->getXLow());
		bool hasRightTile = (tile.getXHigh() != mCanvasSizeBlankTile->getXHigh());

		Tile *newTile = new Tile(tile);

		if(hasDownTile){
			tdown = new Tile(tileType::BLANK, Cord(0, 0), this->mCanvasWidth, newTile->getYLow());
			newTile->lb = tdown;
		}

		if(hasUpTile){
			tup = new Tile(tileType::BLANK, Cord(0, newTile->getYHigh()), 
								this->mCanvasWidth, (this->mCanvasHeight - newTile->getYHigh()));
			newTile->rt = tup;
		}

		if(hasLeftTile){
			tleft = new Tile(tileType::BLANK, Cord(0, newTile->getYLow()),
								newTile->getXLow(), newTile->getHeight());
			newTile->bl = tleft;
			tleft->tr = newTile;

			
			if(hasDownTile) tleft->lb = tdown;
			if(hasUpTile) tleft->rt = tup;
			
			if(hasUpTile) tup->lb = tleft;
		}else{
			if(hasUpTile) tup->lb = newTile;
		}

		if(hasRightTile){
			tright = new Tile(tileType::BLANK, newTile->getLowerRight(), 
								(mCanvasWidth - newTile->getXHigh()), newTile->getHeight());
			newTile->tr = tright;
			tright->bl = newTile;

			if(hasDownTile) tright->lb = tdown;
			if(hasUpTile) tright->rt = tup;

			if(hasDownTile) tdown->rt = tright;
		}else{
			if(hasDownTile) tdown->rt = newTile;
		}

		// push index into the map and exit
		mAllNonBlankTilesMap[newTile->getLowerLeft()] = newTile;
		return newTile;
	}


	/*  STEP 1)
		Find the space tile containing the top edge of the area to be occupied by the new tile
		(due to the strip property, a single space tile must contain the entire edge).
		Then split the top space tile along a horizontal line into a piece entirely above the new tile 
		and a piece overlapping the new tile. Update corne stitches in the tiles adjoining the new tile
	*/

	bool tileTouchesSky = (tile.getYHigh() == mCanvasSizeBlankTile->getYHigh());
	bool cleanTopCut = true;
	Tile *origTop;
	if(!tileTouchesSky){
		origTop = findPoint(tile.getUpperLeft());
		cleanTopCut = (origTop->getYLow() == tile.getYHigh());
	}

	if((!tileTouchesSky) && (!cleanTopCut)){
		cutTileHorizontally(origTop, tile.getYHigh() - origTop->getYLow());
	}

	/*  STEP 2)
		Find the space tile containing the bottom edge of the area to be occupied by the new tile, 
		split it in similar fation in STEP 1, and update stitches around it.
	*/

	bool tileTouchesGround = (tile.getYLow() == mCanvasSizeBlankTile->getYLow());
	bool cleanBottomCut = true;
	Tile *origBottom;
	if(!tileTouchesGround){
		Cord targetTileLL = tile.getLowerLeft();
		origBottom = findPoint(Cord(targetTileLL.x(), (targetTileLL.y() - 1)));
		cleanBottomCut = (origBottom->getYHigh() == tile.getYLow());
	}

	if((!tileTouchesGround) && (!cleanBottomCut)){
		cutTileHorizontally(origBottom, tile.getYLow() - origBottom->getYLow());
	}

	/*  STEP 3)
		Work down along the left side ofthe area of the new tile. Each tile along this edge must be a space tile that
		spans the entire width of the new solid tile. Split the tile into a piece entirely to the left of the new tile, 
		one entirely within the new tile, and one to the right. The splitting may make it possible to merge the left and 
		right remainders vertically with the tiles just above them: merge whenever possible. Finally, merge the centre
		space tile with the solid tile that is forming. Each split or merge requires stitches to be updated and adjoining tiles.
	*/

	Tile *splitTile = findPoint(Cord(tile.getXLow(), (tile.getYHigh() - 1)));
	len_t splitTileYLow = splitTile->getYLow();
	Tile *oldSplitTile;

	// Merge assisting indexes
	len_t leftMergeWidth = 0, rightMergeWidth = 0;
	bool topMostMerge = true;

	while(true){

		// calculate the borders of left blank tile and right blank tile
		len_t blankLeftBorder = splitTile->getXLow();
		len_t tileLeftBorder = tile.getXLow();
		len_t tileRightBorder = tile.getXHigh();
		len_t blankRightBorder = splitTile->getXHigh();

		// The middle tile that's completely within the new tile.
		Tile *newMid = new Tile(tileType::BLANK, Cord(tileLeftBorder, splitTile->getYLow()), tile.getWidth(), splitTile->getHeight());

		// initialize bl, tr pointer in case the left and right tile do not exist
		newMid->bl = splitTile->bl;
		newMid->tr = splitTile->tr;

		std::vector<Tile *> topNeighbors;
		findTopNeighbors(splitTile, topNeighbors);
		std::vector<Tile *>bottomNeighbors;
		findDownNeighbors(splitTile, bottomNeighbors);
		std::vector<Tile *>leftNeighbors;
		findLeftNeighbors(splitTile, leftNeighbors);
		std::vector<Tile *>rightNeighbors;
		findRightNeighbors(splitTile, rightNeighbors);

		// split the left piece if necessary, maintain pointers all around
		bool leftSplitNecessary = (blankLeftBorder != tileLeftBorder);

		if(leftSplitNecessary){
			Tile *newLeft = new Tile(tileType::BLANK, splitTile->getLowerLeft(), (tileLeftBorder - blankLeftBorder) ,splitTile->getHeight());

			newLeft->tr = newMid;
			newLeft->bl = splitTile->bl;

			newMid->bl = newLeft;

			// fix pointers about the top neighbors of new created left tile
			bool rtModified = false;
			for(int i = 0; i < topNeighbors.size(); ++i){
				if(topNeighbors[i]->getXLow() < tileLeftBorder){
					if(!rtModified){
						rtModified = true;
						newLeft->rt = topNeighbors[i];
					}
					if(topNeighbors[i]->getXLow() >= blankLeftBorder){
						topNeighbors[i]->lb = newLeft;
					}else{
						break;
					}
				}
			}
			
			// fix pointers about the bottom neighbors of new created left tile
			bool lbModified = false;
			for(int i = 0; i < bottomNeighbors.size(); ++i){
				if(bottomNeighbors[i]->getXLow() < tileLeftBorder){
					if(!lbModified){
						lbModified = true;
						newLeft->lb = bottomNeighbors[i];
					}
					if(bottomNeighbors[i]->getXHigh() <= tileLeftBorder){
						bottomNeighbors[i]->rt = newLeft;
					}
				}else{
					break;
				}
			}

			// change tr pointers of left neighbors to the new created left tile
			for(int i = 0; i < leftNeighbors.size(); ++i){
				if(leftNeighbors[i]->tr == splitTile){
					leftNeighbors[i]->tr = newLeft;
				}
			}

		}else{
			// change the tr pointers of the left neighbors to newMid
			for(int i = 0; i < leftNeighbors.size(); ++i){
				if(leftNeighbors[i]->tr == splitTile){
					leftNeighbors[i]->tr = newMid;
				}
			}
		}


		// split the right piece if necessary, maintain pointers all around
		bool rightSplitNecessary = (tileRightBorder != blankRightBorder);
		if(rightSplitNecessary){
			Tile *newRight = new Tile(tileType::BLANK, newMid->getLowerRight(), (blankRightBorder - tileRightBorder), newMid->getHeight());

			newRight->tr = splitTile->tr;
			newRight->bl = newMid;

			newMid->tr = newRight;

			// fix pointers about the top neighbors of new created right tile
			bool rtModified = false;
			for(int i = 0; i < topNeighbors.size(); ++i){
				if(topNeighbors[i]->getXHigh() > tileRightBorder){
					if(!rtModified){
						rtModified = true;
						newRight->rt = topNeighbors[i];
					}
					if(topNeighbors[i]->getXLow() >= tileRightBorder){
						topNeighbors[i]->lb = newRight;
					}
				}else{
					break;
				}
			}

			// fix pointers about the bottom neighbors of new created right tile
			bool lbModified = false;
			for(int i = 0 ; i < bottomNeighbors.size(); ++i){
				if(bottomNeighbors[i]->getXHigh() > tileRightBorder){
					if(!lbModified){
						lbModified = true;
						newRight->lb = bottomNeighbors[i];
					}
					if(bottomNeighbors[i]->getXHigh() <= blankRightBorder){
						bottomNeighbors[i]->rt = newRight;
					}else{
						break;
					}
				}
			}

			// also change bl pointers of right neighbors to the newly created left tile
			for(int i = 0; i < rightNeighbors.size(); ++i){
				if(rightNeighbors[i]->bl == splitTile){
					rightNeighbors[i]->bl = newRight;
				}
			}
			
		}else{
			// change bl pointers of right neighbors back to newMid
			for(int i = 0; i < rightNeighbors.size(); ++i){
				if(rightNeighbors[i]->bl == splitTile){
					rightNeighbors[i]->bl = newMid;
				}
			}
		}

		// link rt & lb pointers for newMid and modify the surrounding neighbor pointers 
		bool rtModified = false;
		for(int i = 0; i < topNeighbors.size(); ++i){
			if(topNeighbors[i]->getXLow() < tileRightBorder){
				if(!rtModified){
					rtModified = true;
					newMid->rt = topNeighbors[i];
				}
				if(topNeighbors[i]->getXLow() >= tileLeftBorder){
					topNeighbors[i]->lb = newMid;
				}else{
					break;
				}
			}
		}

		bool lbModified = false;
		for(int i = 0; i < bottomNeighbors.size(); ++i){
			if(bottomNeighbors[i]->getXHigh() > tileLeftBorder){
				if(!lbModified){
					lbModified = true;
					newMid->lb = bottomNeighbors[i];
				}
				if(bottomNeighbors[i]->getXHigh() <= tileRightBorder){
					bottomNeighbors[i]->rt = newMid;
				}else{
					break;
				}
			}
		}

		// Start Mergin Process, the tiles are split accordingly. We may merge all blank tiles possible, midTiles would be merged lastly 

		// Merge the left tile if necessary
		bool initTopLeftMerge = false;
		if(topMostMerge){
			Tile *initTopLeftUp, *initTopLeftDown;
			if(leftSplitNecessary){
				initTopLeftDown = newMid->bl;
				if(initTopLeftDown->rt != nullptr){
					initTopLeftUp= initTopLeftDown->rt;
					bool sameWidth = (initTopLeftUp->getWidth() == initTopLeftDown->getWidth());
					bool xAligned = (initTopLeftUp->getXLow() == initTopLeftDown->getXLow());
					if((initTopLeftUp->getType() == tileType::BLANK) &&  sameWidth && xAligned){
						initTopLeftMerge = true;
					}
				}
			}
		}

		bool leftNeedsMerge = ((leftMergeWidth > 0) && (leftMergeWidth == (tileLeftBorder - blankLeftBorder))) || initTopLeftMerge;
		if(leftNeedsMerge){
			Tile *mergeUp = newMid->bl->rt;
			Tile *mergeDown = newMid->bl;
			mergeTilesVertically(mergeUp, mergeDown);

		}
		// update merge width for below merging blocks
		leftMergeWidth = tileLeftBorder - blankLeftBorder;

		// Merge the right tiles if necessary
		bool initTopRightMerge = false;
		if(topMostMerge){
			Tile *initTopRightUp, *initTopRightDown;
			if(rightSplitNecessary){
				initTopRightDown = newMid->tr;
				if(initTopRightDown->rt != nullptr){
					initTopRightUp = initTopRightDown->rt;
					bool sameWidth = (initTopRightUp->getWidth() == initTopRightDown->getWidth());
					bool xAligned = (initTopRightUp->getXLow() == initTopRightDown->getXLow());
					if((initTopRightUp->getType() == tileType::BLANK) && (sameWidth) && (xAligned)){
						initTopRightMerge = true;
					}
				}
			}
		}
		
		bool rightNeedsMerge = ((rightMergeWidth > 0) && (rightMergeWidth == (blankRightBorder - tileRightBorder))) || initTopRightMerge;        
		if(rightNeedsMerge){
			Tile *mergeUp = newMid->tr->rt;
			Tile *mergeDown = newMid->tr;
			mergeTilesVertically(mergeUp, mergeDown);
		}
		// update right merge width for latter blocks
		rightMergeWidth = blankRightBorder - tileRightBorder;

		// Merge the middle tiles, it MUST be merga without the first time

		if(!topMostMerge){
			Tile *mergeUp = newMid->rt;
			Tile *mergeDown = newMid;
			mergeTilesVertically(mergeUp, mergeDown);

			// relink the newMid to the merged tile 
			newMid = mergeUp;
		}

		oldSplitTile = splitTile;

		// if the merging process hits the last merge, process some potential mergings tiles below the working tiles
		if(splitTileYLow == tile.getYLow()){

			// detect & merge left bottom and the tiles below
			bool lastDownLeftMerge = false;
			Tile *lastBotLeftUp, *lastBotLeftDown;
			if(leftSplitNecessary){
				lastBotLeftUp = newMid->bl;
				if(lastBotLeftUp->lb != nullptr){
					lastBotLeftDown = lastBotLeftUp->lb;
					bool sameWidth = (lastBotLeftUp->getWidth() == lastBotLeftDown->getWidth());
					bool xAligned = (lastBotLeftUp->getXLow() == lastBotLeftDown->getXLow());
					if(lastBotLeftDown->getType() == tileType::BLANK && sameWidth && xAligned){
						lastDownLeftMerge = true;
					}
				}
			}
			if(lastDownLeftMerge){
				mergeTilesVertically(lastBotLeftUp, lastBotLeftDown);
			}

			// detect & merge right bottom and the tiles below
			bool lastDownRightmerge = false;
			Tile *lastBotRightUp, *lastBotRightDown;
			if(rightSplitNecessary){
				// 08/06/2024 bug fix: lastBotRightUp finds incorrect tile, lastBotRightUp = newMid->tr does not point to the correct tile 
				lastBotRightUp = findPoint(newMid->getLowerRight());
				if(lastBotRightUp->lb != nullptr){
					lastBotRightDown = lastBotRightUp->lb;
					bool sameWidth = (lastBotRightUp->getWidth() == lastBotRightDown->getWidth());
					bool xAligned = (lastBotRightUp->getXLow() == lastBotRightDown->getXLow());
					if(lastBotRightDown->getType() == tileType::BLANK && sameWidth && xAligned){
						lastDownRightmerge = true;
					}

				}
			}
			if(lastDownRightmerge){
				mergeTilesVertically(lastBotRightUp, lastBotRightDown);
			}

			// last step is to substitute newMid to the input tile
			delete(oldSplitTile);
			newMid->setType(tile.getType());
			mAllNonBlankTilesMap[newMid->getLowerLeft()] = newMid;
			return newMid;
		}

		// the merging process has not yet hit the bottom tile, working downwards to next splitTile
		delete(oldSplitTile);
		topMostMerge = false;
		splitTile = findPoint(Cord(tile.getXLow(), splitTileYLow - 1));
		splitTileYLow = splitTile->getYLow();
		
   }

}

void CornerStitching::removeTile(Tile *tile){
	// look up if the tile exist in the cornerStitching system
	assert(tile != nullptr);
	Cord tileLL = tile->getLowerLeft();
	std::unordered_map<Cord, Tile*>::iterator deadTileIt = mAllNonBlankTilesMap.find(tileLL);
	// there is no such index
	// if(deadTileIt == mAllNonBlankTilesMap.end()){
	// 	throw (CSException("CORNERSTITCHING_17"));
	// }
	// // the index does not point to the tile to delete
	// if(mAllNonBlankTilesMap[tileLL] != tile){
	// 	throw (CSException("CORNERSTITCHING_18"));
	// }	

	// special case when there is only one noeBlank tile left in the cornerStitching system
	if(mAllNonBlankTilesMap.size() == 1){
		if(tile->rt != nullptr) delete(tile->rt);
		if(tile->tr != nullptr) delete(tile->tr);
		if(tile->bl != nullptr) delete(tile->bl);
		if(tile->lb != nullptr) delete(tile->lb);

		mAllNonBlankTilesMap.clear();
		delete(tile);
		return;
	}

	// store the geometric properties for later use
	Rectangle origDeadTile = tile->getRectangle();
	
	/*  STEP 1)
		Change the type of the dead tile to tileType::BLANK
		and remove the tile from mAllNonBlankTilesMap
	*/
	tile->setType(tileType::BLANK);
	mAllNonBlankTilesMap.erase(deadTileIt);

	/*  STEP 2)
		Use the neighbor-finding algorithm to search from top to bottom through all the tiles
		that adjoin the right edge of the dead tile
	*/
	std::vector<Tile *>rightNeighbors;
	findRightNeighbors(tile, rightNeighbors);

	std::vector<Tile *>leftNeighbors;
	findLeftNeighbors(tile, leftNeighbors);

	// this is for backup when leftNeighbors are empty, tiles are in descent in YLow
	std::vector<Tile *>deadTileRemains;

	/*  STEP 3)
		For each space tile fond in STEP 2), split either the neighbor or the dead tile, or both, so that the 
		two tiles have the same ertical span, then merge the tiles together horizontally. Carry on until the bottom
		edge of the orignal dead tile is reached
	*/
	for(int i = 0; i < rightNeighbors.size(); ++i){
		Tile *rNeighbor = rightNeighbors[i];
		if(i == 0){
			if((rNeighbor->getYHigh() > tile->getYHigh()) && (rNeighbor->getType() == tileType::BLANK)){
				Tile *newRNeighbor = cutTileHorizontally(rNeighbor, tile->getYHigh() - rNeighbor->getYLow());
				rNeighbor = newRNeighbor;
			}
		}

		if(i == (rightNeighbors.size() - 1)){
			if((rNeighbor->getYLow() < tile->getYLow()) && (rNeighbor->getType() == tileType::BLANK)){
				cutTileHorizontally(rNeighbor, tile->getYLow() - rNeighbor->getYLow());
			}
		}

		// now all the tiles are YLow >= tile.YLow && YHigh <= tile.YHigh

		Tile *mergeLeft, *mergeRight;
		if(rNeighbor->getYLow() > tile->getYLow()){
			// necessary to split
			mergeRight = rNeighbor;
			Tile *afterCutTile = cutTileHorizontally(tile, rNeighbor->getYLow() - tile->getYLow());
			mergeLeft = tile;
			tile = afterCutTile;
		}else{
			// the two tiles are already aligned, happens at the last(lower right corner) neighbor tile
			mergeLeft = tile;
			mergeRight = rNeighbor;
		}

		if(mergeRight->getType() == tileType::BLANK){
			mergeTilesHorizontally(mergeLeft, mergeRight);
		}
		deadTileRemains.push_back(mergeLeft);
	}

	/*  STEP 4)
		Scan upwards along the left edge of the original dead tile to find all the space tiles that are left 
		neighbors of the original dead tile (already stored the result at leftNeighbors)

		STEP 5)
		For each space tile found in STEP 4), merge the space tile with the adjoining remains of the original dead tile.
		Do this by repeating STEP 2) ~ STEP 3), treating the current space tile like the dead tile in STEP 2) ~ STEP 3)

		STEP 6)
		It is also necessary to do vertical merging in STEP 5). After each horizontal merge in STEP 5), check to see if 
		the result tile can be merged with the tiles just above and below it, and merge if possible.
	*/

	// int debugFileIdx = 1;

	bool foundFirstRightDeadTile = false;
	for(int leftTileIdx = 0; leftTileIdx < leftNeighbors.size(); ++leftTileIdx){
		Tile *lNeighbor = leftNeighbors[leftTileIdx];
		std::vector<Tile *>rOrigDeadTiles;
		findRightNeighbors(lNeighbor, rOrigDeadTiles);

		len_t origDeadTileYH = rec::getYH(origDeadTile);
		for(int rTileIdx = (rOrigDeadTiles.size() - 1); rTileIdx >= 0; --rTileIdx){
			Tile *rDeadTile = rOrigDeadTiles[rTileIdx];
			if(rDeadTile->getYLow() >= origDeadTileYH) break;

			if(!foundFirstRightDeadTile){
				if(rDeadTile->getYLow() < rec::getYL(origDeadTile)) continue;
				else foundFirstRightDeadTile = true;
				// check if necessary to split the lNeighbor
				len_t lNeighborYLow = lNeighbor->getYLow();
				len_t rDeadTileYLow = rDeadTile->getYLow();
				if(lNeighborYLow < rDeadTileYLow){
					if(lNeighbor->getType() == tileType::BLANK){
						cutTileHorizontally(lNeighbor, rDeadTileYLow - lNeighborYLow);
					}
				}
			}

			// now all rDeadTile shall have YLow aligned with rDeadTile
			len_t lNeighborYHigh = lNeighbor->getYHigh();
			len_t rDeadTileYHigh = rDeadTile->getYHigh();
			if(lNeighborYHigh > rDeadTileYHigh){
				// a potentail cut for lNeighbor
				if(lNeighbor->getType() == tileType::BLANK){
					Tile *newDownlNeighborTile = cutTileHorizontally(lNeighbor, (rDeadTileYHigh - lNeighbor->getYLow()));
					if(rDeadTile->getType() == tileType::BLANK){
						rDeadTile = mergeTilesHorizontally(newDownlNeighborTile, rDeadTile);
					}
				}
			}else if(lNeighborYHigh < rDeadTileYHigh){
				// cut rDeadTile if possible if it's tileType::BLANK
				// next round don't fetch another rDeadTile, use upper cut
				if(rDeadTile->getType() == tileType::BLANK){
					rDeadTile = cutTileHorizontally(rDeadTile, (lNeighborYHigh - rDeadTile->getYLow()));
					// offset the increment of rtileIdx
					if(lNeighbor->getType() == tileType::BLANK){
						rDeadTile = mergeTilesHorizontally(lNeighbor, rDeadTile);
					}
				}
			}else{ // lNeighborYHigh == rDeadTileYHigh
				if((lNeighbor->getType() == tileType::BLANK) && (rDeadTile->getType() == tileType::BLANK)){
					rDeadTile = mergeTilesHorizontally(lNeighbor, rDeadTile);
				}
			}

			// check if merging shall take place, always check if merging with the bottom tile is possible,
			// if the tile hits the ceiling, also attempt to merge with the top tile
			if(rDeadTile->lb != nullptr){
				Tile *downMergeCand = rDeadTile->lb;
				if(downMergeCand != nullptr){
					if(downMergeCand->getType() == tileType::BLANK){
						bool xAligned = (downMergeCand->getXLow() == rDeadTile->getXLow());
						bool sameWidth = (downMergeCand->getWidth() == rDeadTile->getWidth());
						if(xAligned && sameWidth){
							mergeTilesVertically(rDeadTile, downMergeCand);
						}
					}
				}
			}	

			// this is tile that hits the ceiling,  attempt to merge with the tile above 
			if(rDeadTile->getYHigh() >= origDeadTileYH){
				Tile *UpMergeCand = rDeadTile->rt;
				if(UpMergeCand != nullptr){
					if(UpMergeCand->getType() == tileType::BLANK){
						bool xAligned = (UpMergeCand->getXLow() == rDeadTile->getXLow());
						bool sameWidth = (UpMergeCand->getWidth() == rDeadTile->getWidth());
						if(xAligned & sameWidth){
							rDeadTile = mergeTilesVertically(UpMergeCand, rDeadTile);
						}
					}
				}
			}
			// std::string outFileName = "./outputs/case09/case09-output-debug-" + std::to_string(debugFileIdx) + ".txt";
			// debugFileIdx++;
			// visualiseTileDistribution(outFileName);

			if(rDeadTile->getYHigh() >= origDeadTileYH) break;
		}

	}

	// Case if lNeighbor is empty (The removed tile is aligned with the left border of the chip contour)
	if(leftNeighbors.empty()){
		// if deadTileRemains is empty, both left and right of the tile touches the chip contour
		// push in the tile to check if merging is possible
		if(deadTileRemains.empty()) deadTileRemains.push_back(tile);
		for(int i = deadTileRemains.size() - 1; i >= 0; --i){
			// attempt to merge the tile with the lower tile
			Tile *deadTileDebris = deadTileRemains[i];
			Tile *deadTileDebrirsLB = deadTileDebris->lb;
			if(deadTileDebrirsLB != nullptr){
				if(deadTileDebrirsLB->getType() == tileType::BLANK){
					bool xAligned = (deadTileDebrirsLB->getXLow() == deadTileDebris->getXLow());
					bool sameWidth = (deadTileDebrirsLB->getWidth() == deadTileDebris->getWidth());
					if(xAligned && sameWidth) mergeTilesVertically(deadTileDebris, deadTileDebrirsLB);
				}
			}

			if(i == 0){
				// the tile that hits the roof, attemp to merge with the upper tile too
				Tile *deadTileDebrisRT = deadTileDebris->rt;
				if(deadTileDebrisRT != nullptr){
					if(deadTileDebrisRT->getType() == tileType::BLANK){
						bool xAligned = (deadTileDebrisRT->getXLow() == deadTileDebris->getXLow());
						bool sameWidth = (deadTileDebrisRT->getWidth() == deadTileDebris->getWidth());
						if(xAligned && sameWidth) mergeTilesVertically(deadTileDebrisRT, deadTileDebris);
					}
				}
			}

		}
	}

}

Tile *CornerStitching::cutTileHorizontally(Tile *origTop, len_t newDownHeight){

	// check if the cut is valid on the Y axis
	// if(origTop->getHeight() <= newDownHeight){
	// 	throw CSException("CORNERSTITCHING_08");
	// }

	Tile *newDown = new Tile(origTop->getType(), origTop->getLowerLeft(),origTop->getWidth(), newDownHeight);

	newDown->rt = origTop;
	newDown->lb = origTop->lb;
	newDown->bl = origTop->bl;

	// maniputlate surronding tiles of origTop and newDown

	// change lower-neighbors' rt pointer to newDown
	std::vector <Tile *> origDownNeighbors;
	findDownNeighbors(origTop, origDownNeighbors);
	for(Tile *const &t : origDownNeighbors){
		if(t->rt == origTop) t->rt = newDown;
	}

	// 1. find the correct tr for newDown
	// 2. change right neighbors to point their bl to the correct tile (whether to switch to newDown or keep origTop)
	std::vector <Tile *> origRightNeighbors;
	findRightNeighbors(origTop, origRightNeighbors);
	
	bool rightModified = false;
	for(int i = 0; i < origRightNeighbors.size(); ++i){
		Tile *rNeighbor = origRightNeighbors[i];
		len_t rNeighborYLow = rNeighbor->getYLow();
		if(rNeighborYLow < newDown->getYHigh()){
			if(!rightModified){
				rightModified = true;
				newDown->tr = rNeighbor;
			}
			// 08/06/2023 bug fix: change "tile" -> "newDown", add break statement to terminate unecessary serarch early
			if(rNeighborYLow >= newDown->getYLow()){
				rNeighbor->bl = newDown;
			}else{
				break;
			}
		}
	}

	// 1. find the new correct bl for origTop
	// 2. change left neighbors to point their tr to the correct tile (whether to switch to newDown or keep origTop)
	std::vector <Tile *> origLeftNeighbors;
	findLeftNeighbors(origTop, origLeftNeighbors);

	for(int i = 0; i < origLeftNeighbors.size(); ++i){
		Tile *lNeighbor = origLeftNeighbors[i];
		if(lNeighbor->getYHigh() > newDown->getYHigh()){
			// 02/29/2024: remove unecessary flag variable
			origTop->bl = lNeighbor;
			break;
		}else{
			lNeighbor->tr = newDown;
		}
	}

	origTop->setLowerLeft(newDown->getUpperLeft());
	origTop->setHeight(origTop->getHeight() - newDownHeight);
	origTop->lb = newDown;

	return newDown;
}

Tile *CornerStitching::cutTileVertically(Tile *origRight, len_t newLeftWidth){

	// check if the cut is valid on the X axis
	// if(origRight->getWidth() <= newLeftWidth){
	// 	throw CSException("CORNERSTITCHING_09");
	// }

	Tile *newLeft = new Tile(origRight->getType(), origRight->getLowerLeft(), newLeftWidth, origRight->getHeight());

	newLeft->tr = origRight;
	newLeft->lb = origRight->lb;
	newLeft->bl = origRight->bl;

	// manipulate the surrounding tiles of origRight and newLeft

	// change left-neighbors' tr pointer to newLeft
	std::vector<Tile *> origLeftNeighbors;
	findLeftNeighbors(origRight, origLeftNeighbors);
	for(Tile *&t : origLeftNeighbors){
		if(t->tr == origRight) t->tr = newLeft;
	}

	// 1. find the correct rt for newRight
	// 2. change top neighbors to point their lb to the correct tile (whether to switch to newLeft or keep origRight)
	std::vector<Tile *> origTopNeighbors;
	findTopNeighbors(origRight, origTopNeighbors);
	
	bool topModified = false;
	for(int i = 0; i < origTopNeighbors.size(); ++i){
		Tile *tNeighbor = origTopNeighbors[i];
		len_t tNeighborXLow = tNeighbor->getXLow();
		if(tNeighborXLow < newLeft->getXHigh()){
			if(!topModified){
				topModified = true;
				newLeft->rt = tNeighbor;
			}
			if(tNeighborXLow >= newLeft->getXLow()){
				tNeighbor->lb = newLeft;
			}else{
				break;
			}
		}
	}

	// 1. find the new correct lb for origRight
	// 2. change down neighbors to point their rt to the correct tile (whether to switch to newLeft or keep origRight)
	std::vector<Tile *> origDownNeighbors;
	findDownNeighbors(origRight, origDownNeighbors);

	for(int i = 0; i < origDownNeighbors.size(); ++i){
		Tile *dNeighbor = origDownNeighbors[i];
		if(dNeighbor->getXHigh() > newLeft->getXHigh()){
			origRight->lb = dNeighbor;	
			break;
		}else{
			dNeighbor->rt = newLeft;
		}
	}

	origRight->setLowerLeft(newLeft->getLowerRight());
	origRight->setWidth(origRight->getWidth() - newLeftWidth);
	origRight->bl = newLeft;

	return newLeft;
	
}

Tile *CornerStitching::mergeTilesVertically(Tile *mergeUp, Tile *mergeDown){

	// test if merge up is actually above mergeDown
	// if(mergeUp->getYLow() <= mergeDown->getYLow()){
	// 	throw CSException("CORNERSTITCHING_10");
	// }

	// check if two input tiles (mergeUp and mergeDown) are actually mergable
	bool sameWidth = (mergeUp->getWidth() == mergeDown->getWidth());
	bool xAligned = (mergeUp->getXLow() == mergeDown->getXLow());
	bool tilesAttatched = (mergeDown->getYHigh() == mergeUp->getYLow());
	// if(!(sameWidth && xAligned && tilesAttatched)){
	// 	throw CSException("CORNERSTITCHING_11");
	// }

	std::vector<Tile *> mergedownLeftNeighbors;
	findLeftNeighbors(mergeDown, mergedownLeftNeighbors);
	for(int i = 0; i < mergedownLeftNeighbors.size(); ++i){
		if(mergedownLeftNeighbors[i]->tr == mergeDown){
			mergedownLeftNeighbors[i]->tr = mergeUp;
		}
	}

	std::vector<Tile *> mergedownDownNeighbors;
	findDownNeighbors(mergeDown, mergedownDownNeighbors);
	for(int i = 0; i < mergedownDownNeighbors.size(); ++i){
		if(mergedownDownNeighbors[i]->rt == mergeDown){
			mergedownDownNeighbors[i]->rt = mergeUp;
		}
	}
	std::vector<Tile *> mergedownRightNeighbors;
	findRightNeighbors(mergeDown, mergedownRightNeighbors);
	for(int i = 0; i < mergedownRightNeighbors.size(); ++i){
		if(mergedownRightNeighbors[i]->bl == mergeDown){
			mergedownRightNeighbors[i]->bl = mergeUp;
		}
	}
	
	mergeUp->bl = mergeDown->bl;
	mergeUp->lb = mergeDown->lb;
	
	mergeUp->setLowerLeft(mergeDown->getLowerLeft());
	mergeUp->setHeight(mergeUp->getHeight() + mergeDown->getHeight());
	
	delete(mergeDown);
	return mergeUp;
}

Tile *CornerStitching::mergeTilesHorizontally(Tile *mergeLeft, Tile *mergeRight){
	
	// Test if merge left is actually on the left on mergeRight
	// if(mergeLeft->getXLow() >= mergeRight->getXLow()){
	// 	throw CSException("CORNERSTITCHING_13");
	// }

	// check if two input tiles (mergeLeft and mergeRight) are actually mergable
	bool sameHeight = (mergeLeft->getHeight() == mergeRight->getHeight());
	bool yAligned = (mergeLeft->getYLow() == mergeRight->getYLow());
	bool tilesAttatched = (mergeLeft->getXHigh() == mergeRight->getXLow());
	// if(!(sameHeight && yAligned && tilesAttatched)){
	// 	throw CSException("CORNERSTITCHING_14");
	// }

	std::vector<Tile *> mergeRightTopNeighbors;
	findTopNeighbors(mergeRight, mergeRightTopNeighbors);
	for(int i = 0; i < mergeRightTopNeighbors.size(); ++i){
		if(mergeRightTopNeighbors[i]->lb == mergeRight){
			mergeRightTopNeighbors[i]->lb = mergeLeft;
		}
	}

	std::vector<Tile *> mergeRightRightNeighbors;
	findRightNeighbors(mergeRight, mergeRightRightNeighbors);
	for(int i = 0; i < mergeRightRightNeighbors.size(); ++i){
		if(mergeRightRightNeighbors[i]->bl == mergeRight){
			mergeRightRightNeighbors[i]->bl = mergeLeft;
		}
	}

	std::vector<Tile *> mergeRightDownNeighbors;
	findDownNeighbors(mergeRight, mergeRightDownNeighbors);
	for(int i = 0; i < mergeRightDownNeighbors.size(); ++i){
		if(mergeRightDownNeighbors[i]->rt == mergeRight){
			mergeRightDownNeighbors[i]->rt = mergeLeft;
		}
	}

	mergeLeft->rt = mergeRight->rt;
	mergeLeft->tr = mergeRight->tr;

	mergeLeft->setWidth(mergeLeft->getWidth() + mergeRight->getWidth());

	delete(mergeRight);
	return mergeLeft;

}

void CornerStitching::visualiseCornerStitching(const std::string &outputFileName) const{
	std::ofstream ofs;
	ofs.open(outputFileName, std::fstream::out);
	// if(!ofs.is_open()) throw(CSException("CORNERSTITCHING_05"));

	std::unordered_set<Tile *> allTiles;
	collectAllTiles(allTiles);

	// write out the total tile numbers
	ofs << allTiles.size() << std::endl;
	// write the chip contour 
	ofs << mCanvasWidth << " " << mCanvasHeight << std::endl;
	// Then start to write info for each file
	for(Tile *const &tile : allTiles){
		unsigned long long tileHash;
		ofs << *tile << std::endl;

		Tile *rtTile = tile->rt;
		ofs << "rt: ";
		if(rtTile == nullptr){
			ofs << "nullptr" << std::endl; 
		}else{
			ofs << *rtTile << std::endl;
		}

		Tile *trTile = tile->tr;
		ofs << "tr: ";
		if(trTile == nullptr){
			ofs << "nullptr" << std::endl; 
		}else{
			ofs << *trTile << std::endl;
		}

		Tile *blTile = tile->bl;
		ofs << "bl: ";
		if(blTile == nullptr){
			ofs << "nullptr" << std::endl; 
		}else{
			ofs << *blTile << std::endl;
		}

		Tile *lbTile = tile->lb;
		ofs << "lb: ";
		if(lbTile == nullptr){
			ofs << "nullptr" << std::endl; 
		}else{
			ofs << *lbTile << std::endl;
		}
	}
	ofs.close();
}

bool CornerStitching::debugBlankMerged(Tile *&tile1, Tile *&tile2) const{
	std::unordered_set<Tile *> allTilesSet;
	collectAllTiles(allTilesSet);

	std::vector<Tile *> allTilesArr (allTilesSet.begin(), allTilesSet.end());
	
	for(int i = 0; i < allTilesArr.size(); ++i){
		for(int j = (i+1); j < allTilesArr.size(); ++j){
			tile1 = allTilesArr[i];
			tile2 = allTilesArr[j];
			assert(tile1 != nullptr);
			assert(tile2 != nullptr);
			if((tile1->getType() != tileType::BLANK) || (tile2->getType() != tileType::BLANK)) continue;

			if((tile1->getLowerLeft() == tile2->getUpperLeft()) && (tile1->getLowerRight() == tile2->getUpperRight())){
				return false;
			}else if((tile1->getUpperLeft() == tile2->getLowerLeft()) && (tile1->getUpperRight() == tile2->getLowerRight())){
				return false;
			}else if((tile1->getLowerLeft() == tile2->getLowerRight()) && (tile1->getUpperLeft() == tile2->getUpperRight())){
				return false;
			}else if((tile1->getLowerRight() == tile2->getLowerLeft()) && (tile1->getUpperRight() == tile2->getUpperLeft())){
				return false;
			}
		}
	}

	return true;
}

bool CornerStitching::debugPointerAttatched(Tile *&tile1, Tile *&tile2) const{
	std::unordered_set<Tile *> allTilesSet;
	collectAllTiles(allTilesSet);

	std::vector<Tile *> allTilesArr (allTilesSet.begin(), allTilesSet.end());
	for(int i = 0; i < allTilesArr.size(); ++i){
		Tile *tile = allTilesArr[i];
		Tile *tileRT = tile->rt;
		Tile *tileTR = tile->tr;
		Tile *tileBL = tile->bl;
		Tile *tileLB = tile->lb;

		if(tileRT != nullptr){
			if(tileRT->getYLow() != tile->getYHigh()){
				tile1 = tile;
				tile2 = tileRT;
				return false;
			}
		}

		if(tileTR != nullptr){
			if(tileTR->getXLow() != tile->getXHigh()){
				tile1 = tile;
				tile2 = tileTR;
				return false;
			}
		}

		if(tileBL != nullptr){
			if(tileBL->getXHigh() != tile->getXLow()){
				tile1 = tile;
				tile2 = tileBL;
				return false;
			}
		}

		if(tileLB != nullptr){
			if(tileLB->getYHigh() != tile->getYLow()){
				tile1 = tile;
				tile2 = tileLB;
				return false;
			}
		}

	}
	return true;

}

bool CornerStitching::conductSelfTest() const{
	Tile *tile1, *tile2;
	bool blankMergeTest = debugBlankMerged(tile1, tile2);
	if(!blankMergeTest){
		std::cout << "Fail blank merge Test " << *tile1 << " " << *tile2 << std::endl;
	}

	bool pointerAttatched = debugPointerAttatched(tile1, tile2);
	if(!pointerAttatched){
		std::cout << "Fail pointer attatch Test " << *tile1 << " " << *tile2 << std::endl;
	}
	return (blankMergeTest && pointerAttatched);
}

Tile* CornerStitching::generalSplitTile(Tile* originalTile, Rectangle newRect, std::vector<Tile*>& newNeighbors){    
    const Rectangle originalRect = originalTile->getRectangle();
	Rectangle centerRect = originalRect;
    if (!gtl::contains(originalRect, newRect)){
        std::cerr << "new Tile not contained in original Tile\n";
        return NULL;
    }

    // find neighbors
    std::vector<Tile*> topNeighbors, bottomNeighbors, leftNeighbors, rightNeighbors;
    findTopNeighbors(originalTile, topNeighbors);
    findRightNeighbors(originalTile, rightNeighbors);        
    findDownNeighbors(originalTile, bottomNeighbors);
    findLeftNeighbors(originalTile, leftNeighbors);

    // split top if necessary
    if (rec::getYH(newRect) < rec::getYH(originalRect)){
        // create new tile to represent the top
		Rectangle topRect = centerRect;
		gtl::yl(topRect, rec::getYH(newRect));

		Tile* newTopTile = new Tile(originalTile->getType(), topRect);

        // find new tr, bl pointers
        Tile* newTrPtr = NULL, *newBlPtr = NULL;
        Cord newTrNeighborPoint(newTopTile->getXHigh(), newTopTile->getYLow() - 1);
        for (Tile* rightNeighbor: rightNeighbors){
            if (rightNeighbor->checkCordInTile(newTrNeighborPoint)){
                newTrPtr = rightNeighbor;
                break;
            }
        }
        Cord newBlNeighborPoint(newTopTile->getXLow() - 1, newTopTile->getYLow());
        for (Tile* leftNeighbor: leftNeighbors){
            if (leftNeighbor->checkCordInTile(newBlNeighborPoint)){
                newBlPtr = leftNeighbor;
                break;
            }
        }

        // update pointers of old and new tile
        // new
        newTopTile->tr = originalTile->tr;
        newTopTile->rt = originalTile->rt;
        newTopTile->bl = newBlPtr;
        newTopTile->lb = originalTile;
        // old
        originalTile->rt = newTopTile;
        originalTile->tr = newTrPtr;

        // adjust the ptrs of left, top, right neighbors
        // left
        for (Tile* leftNeighbor: leftNeighbors){
            int neighborYh = leftNeighbor->getYHigh();
            if (newTopTile->getYLow() < neighborYh && neighborYh <= newTopTile->getYHigh()){
                leftNeighbor->tr = newTopTile;
            }
        }
        // top
        for (Tile* topNeighbor: topNeighbors){
            int neighborXl = topNeighbor->getXLow();
            if (newTopTile->getXLow() <= neighborXl && neighborXl < newTopTile->getXHigh()){
                topNeighbor->lb = newTopTile;
            }
        }
        // right
        for (Tile* rightNeighbor: rightNeighbors){
            int neighborYl = rightNeighbor->getYLow();
            if (newTopTile->getYLow() <= neighborYl && neighborYl < newTopTile->getYHigh()){
                rightNeighbor->bl = newTopTile;
            }
        }

        // adjust x, y, width, height of old tile
        int newHeight = originalTile->getHeight() - newTopTile->getHeight();
        originalTile->setHeight(newHeight);

        // add newTile to neighbors
		newNeighbors.push_back(newTopTile);

        // top neighbor of originalTile is now newTopTile
        topNeighbors.clear();
        topNeighbors.push_back(newTopTile);

		// update shape of centerRect
		gtl::yh(centerRect, rec::getYL(topRect));

		// update mAllNonBlankTilesMap
		if (originalTile->getType() != tileType::BLANK){
			Cord topLL = newTopTile->getLowerLeft();
			Cord bottomLL = originalTile->getLowerLeft();
			this->mAllNonBlankTilesMap[topLL] = newTopTile;
			this->mAllNonBlankTilesMap[bottomLL] = originalTile;
		}
    }

    // split bottom if necessary
    if (rec::getYL(newRect) > rec::getYL(originalRect)){
        // create new tile to represent the bottom
		Rectangle bottomRect = centerRect;
		gtl::yh(bottomRect, rec::getYL(newRect));

		Tile* newBottomTile = new Tile(originalTile->getType(), bottomRect);

        // find new tr, bl pointers
        Tile* newTrPtr = NULL, *newBlPtr = NULL;
        Cord newTrNeighborPoint(newBottomTile->getXHigh(), newBottomTile->getYHigh() - 1);
        for (Tile* rightNeighbor: rightNeighbors){
            if (rightNeighbor->checkCordInTile(newTrNeighborPoint)){
                newTrPtr = rightNeighbor;
                break;
            }
        }
        Cord newBlNeighborPoint(newBottomTile->getXLow() - 1, newBottomTile->getYHigh());
        for (Tile* leftNeighbor: leftNeighbors){
            if (leftNeighbor->checkCordInTile(newBlNeighborPoint)){
                newBlPtr = leftNeighbor;
                break;
            }
        }

        // update pointers of old and new tile
        // new
        newBottomTile->tr = newTrPtr;
        newBottomTile->rt = originalTile;
        newBottomTile->bl = originalTile->bl;
        newBottomTile->lb = originalTile->lb;
        // old
        originalTile->lb = newBottomTile;
        originalTile->bl = newBlPtr;

        // adjust the ptrs of left, bottom, right neighbors
        // left
        for (Tile* leftNeighbor: leftNeighbors){
            int neighborYh = leftNeighbor->getYHigh();
            if (newBottomTile->getYLow() < neighborYh && neighborYh <= newBottomTile->getYHigh()){
                leftNeighbor->tr = newBottomTile;
            }
        }
        // bottom
        for (Tile* bottomNeighbor: bottomNeighbors){
            int neighborXh = bottomNeighbor->getXHigh();
            if (newBottomTile->getXLow() < neighborXh && neighborXh <= newBottomTile->getXHigh()){
                bottomNeighbor->rt = newBottomTile;
            }
        }
        // right
        for (Tile* rightNeighbor: rightNeighbors){
            int neighborYl = rightNeighbor->getYLow();
            if (newBottomTile->getYLow() <= neighborYl && neighborYl < newBottomTile->getYHigh()){
                rightNeighbor->bl = newBottomTile;
            }
        }

        // adjust x, y, width, height of old tile
        int newHeight = originalTile->getHeight() - newBottomTile->getHeight();
        originalTile->setHeight(newHeight);
        originalTile->setLowerLeft(newBottomTile->getUpperLeft());

		// add tile to new neighbors
		newNeighbors.push_back(newBottomTile);

        // bottom neighbor of originalTile is now newBottomTile
        bottomNeighbors.clear();
        bottomNeighbors.push_back(newBottomTile);

		// update shape of center rect
		gtl::yl(centerRect, rec::getYH(bottomRect));
		
		// update mAllNonBlankTilesMap
		if (originalTile->getType() != tileType::BLANK){
			Cord bottomLL = newBottomTile->getLowerLeft();
			Cord topLL = originalTile->getLowerLeft();
			this->mAllNonBlankTilesMap[bottomLL] = newBottomTile;
			this->mAllNonBlankTilesMap[topLL] = originalTile;
		}
    }

    // split right if necessary
    if (rec::getXH(newRect) < rec::getXH(originalRect)){
        // create new tile to represent the right
		Rectangle rightRect = centerRect;
		gtl::xl(rightRect, rec::getXH(newRect));

        Tile* newRightTile = new Tile(originalTile->getType(), rightRect);

        // find new rt, lb pointers
        Tile* newRtPtr = NULL, *newLbPtr = NULL;
        Cord newRtNeighborPoint(newRightTile->getXLow() - 1, newRightTile->getYHigh());
        for (Tile* topNeighbor: topNeighbors){
            if (topNeighbor->checkCordInTile(newRtNeighborPoint)){
                newRtPtr = topNeighbor;
                break;
            }
        }
        Cord newLbNeighborPoint(newRightTile->getXLow(), newRightTile->getYLow() - 1);
        for (Tile* bottomNeighbor: bottomNeighbors){
            if (bottomNeighbor->checkCordInTile(newLbNeighborPoint)){
                newLbPtr = bottomNeighbor;
                break;
            }
        }

        // modify pointers of old and new tile
        // new
        newRightTile->tr = originalTile->tr;
        newRightTile->rt = originalTile->rt;
        newRightTile->bl = originalTile;
        newRightTile->lb = newLbPtr;
        // old
        originalTile->rt = newRtPtr;
        originalTile->tr = newRightTile;

        // adjust the ptrs of top, right, bottom neighbors
        // top
        for (Tile* topNeighbor: topNeighbors){
            int neighborXl = topNeighbor->getXLow();
            if (newRightTile->getXLow() <= neighborXl && neighborXl < newRightTile->getXHigh()){
                topNeighbor->lb = newRightTile;
            }
        }
        // right
        for (Tile* rightNeighbor: rightNeighbors){
            int neighborYl = rightNeighbor->getYLow();
            if (newRightTile->getYLow() <= neighborYl && neighborYl < newRightTile->getYHigh()){
                rightNeighbor->bl = newRightTile;
            }
        }
        // bottom
        for (Tile* bottomNeighbor: bottomNeighbors){
            int neighborXh = bottomNeighbor->getXHigh();
            if (newRightTile->getXLow() < neighborXh && neighborXh <= newRightTile->getXHigh()){
                bottomNeighbor->rt = newRightTile;
            }
        }

        // adjust x,y,width,height of old tile;
        int newWidth = originalTile->getWidth() - newRightTile->getWidth();
        originalTile->setWidth(newWidth);

        // add newTile to neighbors
		newNeighbors.push_back(newRightTile);

		// update shape of centerRect
		gtl::xh(centerRect, rec::getXL(rightRect));
		
		// update mAllNonBlankTilesMap
		if (originalTile->getType() != tileType::BLANK){
			Cord rightLL = newRightTile->getLowerLeft();
			Cord leftLL = originalTile->getLowerLeft();
			this->mAllNonBlankTilesMap[rightLL] = newRightTile;
			this->mAllNonBlankTilesMap[leftLL] = originalTile;
		}
    }

    // split left if necessary
    if (gtl::xl(originalRect) < gtl::xl(newRect)){
        // create new tile to represent the left
		Rectangle newLeft = centerRect;
		gtl::xh(newLeft, rec::getXL(newRect));

		Tile* newLeftTile = new Tile(originalTile->getType(), newLeft);

        // find new rt, lb pointers
        Tile* newRtPtr = NULL, *newLbPtr = NULL;
        Cord newRtNeighborPoint(newLeftTile->getXHigh() - 1, newLeftTile->getYHigh());
        for (Tile* topNeighbor: topNeighbors){
            if (topNeighbor->checkCordInTile(newRtNeighborPoint)){
                newRtPtr = topNeighbor;
                break;
            }
        }
        Cord newLbNeighborPoint(newLeftTile->getXHigh(), newLeftTile->getYLow() - 1);
        for (Tile* bottomNeighbor: bottomNeighbors){
            if (bottomNeighbor->checkCordInTile(newLbNeighborPoint)){
                newLbPtr = bottomNeighbor;
                break;
            }
        }

        // modify pointers of old and new tile
        // new
        newLeftTile->tr = originalTile;
        newLeftTile->rt = newRtPtr;
        newLeftTile->bl = originalTile->bl;
        newLeftTile->lb = originalTile->lb;
        // old
        originalTile->bl = newLeftTile;
        originalTile->lb = newLbPtr;

        // adjust the ptrs of bottom, left, top neighbors
        // bottom
        for (Tile* bottomNeighbor: bottomNeighbors){
            int neighborXh = bottomNeighbor->getXHigh();
            if (newLeftTile->getXLow() < neighborXh && neighborXh <= newLeftTile->getXHigh()){
                bottomNeighbor->rt = newLeftTile;
            }
        }
        // left
        for (Tile* leftNeighbor: leftNeighbors){
            int neighborYh = leftNeighbor->getYHigh();
            if (newLeftTile->getYLow() < neighborYh && neighborYh <= newLeftTile->getYHigh()){
                leftNeighbor->tr = newLeftTile;
            }
        }
        // top
        for (Tile* topNeighbor: topNeighbors){
            int neighborXl = topNeighbor->getXLow();
            if (newLeftTile->getXLow() <= neighborXl && neighborXl < newLeftTile->getXHigh()){
                topNeighbor->lb = newLeftTile;
            }
        }

        // adjust x,y,width,height of old tile;
        int newWidth = originalTile->getWidth() - newLeftTile->getWidth();
        originalTile->setWidth(newWidth);
        originalTile->setLowerLeft(newLeftTile->getLowerRight());

        // add newTile to neighbors
		newNeighbors.push_back(newLeftTile);

		// update shape of center rect
		gtl::xl(centerRect, rec::getXH(newLeft));
		
		// update mAllNonBlankTilesMap
		if (originalTile->getType() != tileType::BLANK){
			Cord leftLL = newLeftTile->getLowerLeft();
			Cord rightLL = originalTile->getLowerLeft();
			this->mAllNonBlankTilesMap[leftLL] = newLeftTile;
			this->mAllNonBlankTilesMap[rightLL] = originalTile;
		}
    }

	assert(gtl::equivalence(centerRect, originalTile->getRectangle()));
    return originalTile;
}
