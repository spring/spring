#include <cassert>

#include "IncExternAI.h"
#include "IncGlobalAI.h"

CPathFinder::CPathFinder(AIClasses* aic) {
	ai = aic;

	// 8 = speed, 2 = precision
	resmodifier = THREATRES;

	PathMapXSize   = int(ai->cb->GetMapWidth() / resmodifier);
	PathMapYSize   = int(ai->cb->GetMapHeight() / resmodifier);
	totalcells     = PathMapXSize*PathMapYSize;
	micropather    = new MicroPather(this, ai, totalcells);
	HeightMap      = new float[totalcells];
	SlopeMap       = new float[totalcells];
	TestMoveArray  = new bool[totalcells];
	NumOfMoveTypes = 0;
}

CPathFinder::~CPathFinder() {
	delete[] SlopeMap;
	delete[] HeightMap;
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
	std::stringstream moveTypeSStr;
	std::string moveTypeStr;
	std::string sectionstring = "CLASS";
	std::string errorstring = "-1";
	std::string Valuestring = "0";

	// FIXME: can be a .lua script now
	if (!ai->parser->LoadVirtualFile("gamedata\\MOVEINFO.tdf")) {
		L(ai, "[CPathFinder::Init()]");
		L(ai, "\tmod move-data not in TDF format, aborting AI initialization");
		assert(false);
	}

	while (Valuestring != errorstring) {
		moveTypeStr.clear();
		moveTypeSStr.str("");
		moveTypeSStr << NumOfMoveTypes;
		moveTypeSStr >> moveTypeStr;

		ai->parser->GetDef(Valuestring, errorstring, std::string(sectionstring + moveTypeStr + "\\Name"));

		if (Valuestring != errorstring) {
			ai->parser->GetDef(Valuestring, std::string("10000"), std::string(sectionstring + moveTypeStr + "\\MaxWaterDepth"));
			maxwaterdepths.push_back(atoi(Valuestring.c_str()));

			ai->parser->GetDef(Valuestring, std::string("-10000"), std::string(sectionstring + moveTypeStr + "\\MinWaterDepth"));
			minwaterdepths.push_back(atoi(Valuestring.c_str()));

			ai->parser->GetDef(Valuestring, std::string("10000"), std::string(sectionstring + moveTypeStr + "\\MaxSlope"));
			moveslopes.push_back(atoi(Valuestring.c_str()));

			NumOfMoveTypes++;
		}
	}

	// add the last, tester movetype
	maxwaterdepths.push_back(20);
	minwaterdepths.push_back(-10000);
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
	int enemycomms[16];
	float3 enemyposes[16];
	ai->dm->ChokeMapsByMovetype.resize(NumOfMoveTypes);

	int Range = int(sqrtf(float(PathMapXSize * PathMapYSize)) / THREATRES / 3);
	int squarerange = Range * Range;
	int maskwidth = (2 * Range + 1);
	float* costmask = new float[maskwidth * maskwidth];

	for (int x = 0; x < maskwidth; x++) {
		for (int y = 0; y < maskwidth; y++) {
			int distance = (x - Range) * (x - Range) + (y - Range) * (y - Range);

			if (distance <= squarerange) {
				costmask[y * maskwidth + x] = (distance - squarerange) * (distance - squarerange) / squarerange * 2;
			}
			else {
				costmask[y * maskwidth + x] = 0;
			}
		}
	}

	for (int m = 0; m < 	NumOfMoveTypes;m++) {
		int numberofenemyplayers = ai->cheat->GetEnemyUnits(enemycomms);

		for (int i = 0; i < numberofenemyplayers; i++) {
			enemyposes[i] = ai->cheat->GetUnitPos(enemycomms[i]);
		}

		float3 mypos = ai->cb->GetUnitPos(ai->uh->AllUnitsByCat[CAT_BUILDER].front());
		ai->dm->ChokeMapsByMovetype[m].resize(totalcells);
		int reruns = 35;

		micropather->SetMapData(MoveArrays[m], &ai->dm->ChokeMapsByMovetype[m][0], PathMapXSize, PathMapYSize);
		double pathCostSum = 0.0;

		for (int i = 0; i < totalcells; i++) {
			ai->dm->ChokeMapsByMovetype[m][i] = 1;
		}

		int runcounter = 0;

		// HACK
		if (numberofenemyplayers > 0 && m == PATHTOUSE) {
			for (int r = 0; r < reruns; r++) {
				for (int startpos = 0; startpos < numberofenemyplayers; startpos++) {
					if (micropather->Solve(Pos2Node(enemyposes[startpos]), Pos2Node(mypos), &path, &totalcost) == MicroPather::SOLVED) {
						for (int i = 12; i < int(path.size() - 12); i++) {
							if (i % 2) {
								int x, y;
								Node2XY(path[i], &x, &y);

								for (int myx = -Range; myx <= Range; myx++) {
									int actualx = x + myx;

									if (actualx >= 0 && actualx < PathMapXSize) {
										for (int myy = -Range; myy <= Range; myy++) {
											int actualy = y + myy;

											if (actualy >= 0 && actualy < PathMapYSize){
												ai->dm->ChokeMapsByMovetype[m][actualy * PathMapXSize + actualx] += costmask[(myy + Range) * maskwidth + (myx+Range)];
											}
										}
									}
								}
							}
						}

						runcounter++;
					}

					pathCostSum += totalcost;
				}
			}
		}
	}

	delete[] costmask;
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
	const int radius = int(myMaxRange / (8 * resmodifier));
	int offsetSize = 0;

	std::vector<std::pair<int, int> > offsets;
	std::vector<int> xend;

	std::vector<void*> endNodes;
	endNodes.reserve(possibleTargets.size() * radius * 10);

	{
		const int DoubleRadius = radius * 2;
		const int SquareRadius = radius * radius;

		xend.resize(DoubleRadius + 1);
		offsets.resize(DoubleRadius * 5);

		for (int a = 0; a < DoubleRadius + 1; a++) {
			const float z = a - radius;
			const float floatsqrradius = SquareRadius;
			xend[a] = int(sqrt(floatsqrradius - z * z));
		}

		offsets[0].first = 0;
		offsets[0].second = 0;

		int index = 1;
		int index2 = 1;

		for (int a = 1; a < radius + 1; a++) {
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

		for (int a = 0; a < index2 - 2; a++) {
			assert(index < offsets.size());
			assert(a < offsets.size());
			offsets[index].first = offsets[a].first;
			offsets[index].second = DoubleRadius - (offsets[a].second);
			index++;
		}

		index2 = index;

		for (int a = 0; a < index2; a++) {
			assert(index < offsets.size());
			assert(a < offsets.size());
			offsets[index].first = -(offsets[a].first);
			offsets[index].second = offsets[a].second;
			index++;
		}

		for (int a = 0; a < index; a++) {
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
