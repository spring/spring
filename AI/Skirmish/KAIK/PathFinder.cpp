#include <cassert>

#include "IncEngine.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

CPathFinder::CPathFinder(AIClasses* aic) {
	ai = aic;

	// 8 = speed, 2 = precision
	resmodifier = THREATRES;

	PathMapXSize   = int(ai->cb->GetMapWidth() / resmodifier);
	PathMapYSize   = int(ai->cb->GetMapHeight() / resmodifier);
	totalcells     = PathMapXSize * PathMapYSize;
	micropather    = new MicroPather(this, ai, totalcells);
	TestMoveArray  = new bool[totalcells];
	NumOfMoveTypes = 0;

	HeightMap.resize(totalcells, 0.0f);
	SlopeMap.resize(totalcells, 0.0f);
}

CPathFinder::~CPathFinder() {
	delete[] TestMoveArray;

	for (unsigned int i = 0; i != MoveArrays.size(); i++) {
		delete[] MoveArrays[i];
	}

	delete micropather;
}


void CPathFinder::Init() {
	AverageHeight = 0;

	for (int x = 0; x < PathMapXSize; x++) {
		for (int y = 0; y < PathMapYSize; y++) {
			int index = y * PathMapXSize + x;
			HeightMap[index] = *(ai->cb->GetHeightMap() + int(y * resmodifier * resmodifier * PathMapXSize + resmodifier * x));

			if (HeightMap[index] > 0)
				AverageHeight += HeightMap[index];
		}
	}

	AverageHeight /= totalcells;

	for (int i = 0; i < totalcells; i++) {
		float maxslope = 0;
		float tempslope;

		if (i + 1 < totalcells && (i + 1) % PathMapXSize) {
			tempslope = fabs(HeightMap[i] - HeightMap[i + 1]);
				maxslope = std::max(tempslope, maxslope);
		}

		if (i - 1 >= 0 && i % PathMapXSize) {
			tempslope = fabs(HeightMap[i] - HeightMap[i - 1]);
			maxslope = std::max(tempslope, maxslope);
		}

		if (i + PathMapXSize < totalcells) {
			tempslope = fabs(HeightMap[i] - HeightMap[i + PathMapXSize]);
			maxslope = std::max(tempslope, maxslope);
		}

		if (i - PathMapXSize >= 0) {
			tempslope = fabs(HeightMap[i] - HeightMap[i - PathMapXSize]);
			maxslope = std::max(tempslope, maxslope);
		}

		SlopeMap[i] = maxslope * 6 / resmodifier;
		if (SlopeMap[i] < 1)
			SlopeMap[i] = 1;
	}

	// get all the different movetypes
	std::vector<int> moveslopes;
	std::vector<int> maxwaterdepths;
	std::vector<int> minwaterdepths;

	NumOfMoveTypes = ai->ut->moveDefs.size();

	std::map<int, MoveData*>::const_iterator it;
	for (it = ai->ut->moveDefs.begin(); it != ai->ut->moveDefs.end(); it++) {
		const MoveData* md = it->second;

		if (md->moveType == MoveData::Ship_Move) {
			minwaterdepths.push_back(md->depth);
			maxwaterdepths.push_back(10000);
		} else {
			minwaterdepths.push_back(-10000);
			maxwaterdepths.push_back(md->depth);
		}

		moveslopes.push_back(md->maxSlope);
	}

	// add the last, tester movetype
	minwaterdepths.push_back(-10000);
	maxwaterdepths.push_back(20);
	moveslopes.push_back(25);
	NumOfMoveTypes++;
	assert(moveslopes.size() == maxwaterdepths.size());
	MoveArrays.resize(NumOfMoveTypes);

	for (int m = 0; m < NumOfMoveTypes; m++) {
		MoveArrays[m] = new bool[totalcells];

		for (int i = 0; i < totalcells; i++) {
			MoveArrays[m][i] = false;

			if (SlopeMap[i] > moveslopes[m] || HeightMap[i] <= -maxwaterdepths[m] || HeightMap[i] >= -minwaterdepths[m]) {
				MoveArrays[m][i] = false;
				TestMoveArray[i] = true;
			}
			else {
				MoveArrays[m][i] = true;
				TestMoveArray[i] = true;
			}
		}

		// make sure that the edges are no-go
		for (int i = 0; i < PathMapXSize; i++) {
			MoveArrays[m][i] = false;
		}

		for (int i = 0; i < PathMapYSize; i++) {
			int k = i * PathMapXSize;
			MoveArrays[m][k] = false;
		}

		for (int i = 0; i < PathMapYSize; i++) {
			int k = i * PathMapXSize + PathMapXSize - 1;
			MoveArrays[m][k] = false;
		}

		for (int i = 0; i < PathMapXSize; i++) {
			int k = PathMapXSize * (PathMapYSize - 1) + i;
			MoveArrays[m][k] = false;
		}
	}
}


void CPathFinder::CreateDefenseMatrix() {
	int enemyStartUnitIDs[255] = {-1};
	float3 enemyStartPositions[255] = {ZeroVector};

	const int range = std::max(1.0f, sqrtf(float(PathMapXSize * PathMapYSize)) / THREATRES / 3);
	const int rangeSq = range * range;
	const int maskWidth = (2 * range + 1);
	std::vector<float> costMask(maskWidth * maskWidth);

	for (int x = 0; x < maskWidth; x++) {
		for (int y = 0; y < maskWidth; y++) {
			const int index = y * maskWidth + x;
			const int distSq = (x - range) * (x - range) + (y - range) * (y - range);

			if (distSq <= rangeSq) {
				costMask[index] = ((distSq - rangeSq) * (distSq - rangeSq)) / (rangeSq * 2);
			} else {
				costMask[index] = 0.0f;
			}
		}
	}

	ai->dm->ChokeMapsByMovetype.resize(NumOfMoveTypes);

	for (int m = 0; m < NumOfMoveTypes;m++) {
		const int numEnemies = ai->ccb->GetEnemyUnits(enemyStartUnitIDs);

		for (int i = 0; i < numEnemies; i++) {
			enemyStartPositions[i] = ai->ccb->GetUnitPos(enemyStartUnitIDs[i]);
		}

		const float3& myPos = ai->cb->GetUnitPos(ai->uh->AllUnitsByCat[CAT_BUILDER].front());
		const int reruns = 35;

		ai->dm->ChokeMapsByMovetype[m].resize(totalcells);
		micropather->SetMapData(MoveArrays[m], &ai->dm->ChokeMapsByMovetype[m][0], PathMapXSize, PathMapYSize);

		for (int i = 0; i < totalcells; i++) {
			ai->dm->ChokeMapsByMovetype[m][i] = 1;
		}

		// HACK:
		//    for each enemy start-unit, find a path to its position <N> times
		//    for each found path, deposit cost-crumbs at every second waypoint
		//    regions where many paths overlap indicate choke-points
		if (numEnemies > 0 && m == PATHTOUSE) {
			for (int r = 0; r < reruns; r++) {
				for (int startPosIdx = 0; startPosIdx < numEnemies; startPosIdx++) {
					void* startPos = Pos2Node(enemyStartPositions[startPosIdx]);
					void* goalPos = Pos2Node(myPos);

					if (micropather->Solve(startPos, goalPos, &path, &totalcost) != MicroPather::SOLVED) {
						continue;
					}

					for (int i = 12; i < int(path.size() - 12); i++) {
						if ((i % 2) == 0) { continue; }

						int x, y;

						Node2XY(path[i], &x, &y);

						for (int myx = -range; myx <= range; myx++) {
							const int actualx = x + myx;

							if (actualx < 0 || actualx >= PathMapXSize) {
								continue;
							}

							for (int myy = -range; myy <= range; myy++) {
								const int actualy = y + myy;
								const int cmIndex = actualy * PathMapXSize + actualx;

								if (actualy < 0 || actualy >= PathMapYSize) {
									continue;
								}

								ai->dm->ChokeMapsByMovetype[m][cmIndex] += costMask[(myy + range) * maskWidth + (myx + range)];
							}

						}
					}
				}
			}
		}
	}
}

void CPathFinder::PrintData(std::string) {
}

unsigned CPathFinder::Checksum() {
	return micropather->Checksum();
}


void* CPathFinder::XY2Node(int x, int y) {
	return (void*) (y * PathMapXSize + x);
}

void CPathFinder::Node2XY(void* node, int* x, int* y) {
	size_t index = (size_t)node;
	*y = index / PathMapXSize;
	*x = index - (*y * PathMapXSize);
}

float3 CPathFinder::Node2Pos(void* node) {
	float3 pos;
	int multiplier = int(8 * resmodifier);
	size_t index = (size_t)node;
	pos.z = (index / PathMapXSize) * multiplier;
	pos.x = (index - ((index / PathMapXSize) * PathMapXSize)) * multiplier;

	return pos;
}

void* CPathFinder::Pos2Node(float3 pos) {
	return ((void*) (int(pos.z / 8 / THREATRES) * PathMapXSize + int((pos.x / 8 / THREATRES))));
}

/*
 * radius is in full res.
 * returns the path cost.
 */
float CPathFinder::MakePath(F3Vec& posPath, float3& startPos, float3& endPos, int radius) {
	ai->math->TimerStart();
	path.clear();

	ai->math->F3MapBound(startPos);
	ai->math->F3MapBound(endPos);

	float totalcost = 0.0f;

	int ex = int(endPos.x / (8 * resmodifier));
	int ey = int(endPos.z / (8 * resmodifier));
	int sy = int(startPos.z / (8 * resmodifier));
	int sx = int(startPos.x / (8 * resmodifier));

	radius /= int(8 * resmodifier);

	if (micropather->FindBestPathToPointOnRadius(XY2Node(sx, sy), XY2Node(ex, ey), &path, &totalcost, radius) == MicroPather::SOLVED) {
		posPath.reserve(path.size());

		for (unsigned i = 0; i < path.size(); i++) {
			float3 mypos = Node2Pos(path[i]);
			mypos.y = ai->cb->GetElevation(mypos.x, mypos.z);
			posPath.push_back(mypos);
		}
	}

	return totalcost;
}


float CPathFinder::FindBestPath(F3Vec& posPath, float3& startPos, float myMaxRange, F3Vec& possibleTargets) {
	ai->math->TimerStart();
	path.clear();

	// make a list with the points that will count as end nodes
	float totalcost = 0.0f;
	const unsigned int radius = int(myMaxRange / (8 * resmodifier));
	int offsetSize = 0;

	std::vector<std::pair<int, int> > offsets;
	std::vector<int> xend;

	std::vector<void*> endNodes;
	endNodes.reserve(possibleTargets.size() * radius * 10);

	{
		const unsigned int DoubleRadius = radius * 2;
		const unsigned int SquareRadius = radius * radius;

		xend.resize(DoubleRadius + 1);
		offsets.resize(DoubleRadius * 5);

		for (size_t a = 0; a < DoubleRadius + 1; a++) {
			const float z = (int) (a - radius);
			const float floatsqrradius = SquareRadius;
			xend[a] = int(sqrt(floatsqrradius - z * z));
		}

		offsets[0].first = 0;
		offsets[0].second = 0;

		size_t index = 1;
		size_t index2 = 1;

		for (size_t a = 1; a < radius + 1; a++) {
			int endPosIdx = xend[a];
			int startPosIdx = xend[a - 1];

			while (startPosIdx <= endPosIdx) {
				assert(index < offsets.size());
				offsets[index].first = startPosIdx;
				offsets[index].second = a;
				startPosIdx++;
				index++;
			}

			startPosIdx--;
		}

		index2 = index;

		for (size_t a = 0; a < index2 - 2; a++) {
			assert(index < offsets.size());
			assert(a < offsets.size());
			offsets[index].first = offsets[a].first;
			offsets[index].second = DoubleRadius - (offsets[a].second);
			index++;
		}

		index2 = index;

		for (size_t a = 0; a < index2; a++) {
			assert(index < offsets.size());
			assert(a < offsets.size());
			offsets[index].first = -(offsets[a].first);
			offsets[index].second = offsets[a].second;
			index++;
		}

		for (size_t a = 0; a < index; a++) {
			assert(a < offsets.size());
			offsets[a].first = offsets[a].first; // ??
			offsets[a].second = offsets[a].second - radius;
		}

		offsetSize = index;
	}

	for (unsigned i = 0; i < possibleTargets.size(); i++) {
		float3& f = possibleTargets[i];
		int x, y;
		// TODO: make the circle here

		ai->math->F3MapBound(f);
		Node2XY(Pos2Node(f), &x, &y);

		for (int j = 0; j < offsetSize; j++) {
			const int sx = x + offsets[j].first;
			const int sy = y + offsets[j].second;

			if (sx >= 0 && sx < PathMapXSize && sy >= 0 && sy < PathMapYSize) {
				endNodes.push_back(XY2Node(sx, sy));
			}
		}
	}

	ai->math->F3MapBound(startPos);

	if (micropather->FindBestPathToAnyGivenPoint(Pos2Node(startPos), endNodes, &path, &totalcost) == MicroPather::SOLVED) {
        posPath.reserve(path.size());

		for (unsigned i = 0; i < path.size(); i++) {
			int x, y;

			Node2XY(path[i], &x, &y);
			float3 mypos = Node2Pos(path[i]);
			mypos.y = ai->cb->GetElevation(mypos.x, mypos.z);
			posPath.push_back(mypos);
		}
	}

	return totalcost;
}

float CPathFinder::FindBestPathToRadius(std::vector<float3>& posPath, float3& startPos, float radiusAroundTarget, const float3& target) {
	std::vector<float3> posTargets;
	posTargets.push_back(target);
	float dist = FindBestPath(posPath, startPos, radiusAroundTarget, posTargets);
	return dist;
}



bool CPathFinder::IsPositionReachable(const MoveData* md, const float3& pos) const {
	if (md == 0) {
		// aircraft or building
		return true;
	}
	if (!MAPPOS_IN_BOUNDS(pos)) {
		return false;
	}

	const float* hgtMap = ai->cb->GetHeightMap();
	const float* slpMap  = ai->cb->GetSlopeMap();

	const int WH = ai->cb->GetMapWidth();
	const int WS = WH >> 1;
	const int xh = (pos.x / SQUARE_SIZE);
	const int zh = (pos.z / SQUARE_SIZE);
	const int xs = xh >> 1;
	const int zs = zh >> 1;

	bool heightOK = false;
	bool slopeOK  = false;

	switch (md->moveType) {
		case MoveData::Ship_Move: {
			heightOK = (hgtMap[zh * WH + xh] < -md->depth);
			slopeOK  = true;
		} break;
		case MoveData::Ground_Move: {
			heightOK = (hgtMap[zh * WH + xh] > -md->depth);
			slopeOK  = (slpMap[zs * WS + xs] <  md->maxSlope);
		} break;
		case MoveData::Hover_Move: {
			heightOK = true;
			slopeOK  = (slpMap[zs * WS + xs] < md->maxSlope);
		} break;
	}

	return (heightOK && slopeOK);
}
