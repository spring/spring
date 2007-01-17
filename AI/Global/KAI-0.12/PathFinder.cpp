#include "PathFinder.h"

CPathFinder::CPathFinder(AIClasses* ai) {
	this -> ai = ai;
	// 8 = speed, 2 = precision
	resmodifier = THREATRES;
	PathMapXSize = int(ai -> cb -> GetMapWidth() / resmodifier);
	PathMapYSize = int(ai -> cb -> GetMapHeight() / resmodifier);
	totalcells = PathMapXSize*PathMapYSize;
	micropather = new MicroPather(this, ai, totalcells);
	HeightMap = new float[totalcells];
	SlopeMap = new float[totalcells];
	TestMoveArray = new bool[totalcells];
	NumOfMoveTypes = 0;
}

CPathFinder::~CPathFinder() {
	delete SlopeMap;
	delete HeightMap;
	delete TestMoveArray;

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
			HeightMap[index] = *(ai -> cb -> GetHeightMap() + int(y * resmodifier * resmodifier * PathMapXSize + resmodifier * x));

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
				maxslope = max(tempslope, maxslope);
		}

		if (i - 1 >= 0 && i % PathMapXSize) {
			tempslope = fabs(HeightMap[i] - HeightMap[i - 1]);
			maxslope = max(tempslope, maxslope);
		}

		if (i + PathMapXSize < totalcells) {
			tempslope = fabs(HeightMap[i] - HeightMap[i + PathMapXSize]);
			maxslope = max(tempslope, maxslope);
		}

		if (i - PathMapXSize >= 0) {
			tempslope = fabs(HeightMap[i] - HeightMap[i - PathMapXSize]);
			maxslope = max(tempslope, maxslope);
		}

		SlopeMap[i] = maxslope * 6 / resmodifier;
		if (SlopeMap[i] < 1)
			SlopeMap[i] = 1;
	}

	// get all the different movetypes
	// L("Getting Movearrays");
	vector<int> moveslopes;
	vector<int> maxwaterdepths;
	vector<int>	minwaterdepths;
	string sectionstring = "CLASS";
	string errorstring = "-1";
	string Valuestring = "0";
	char k[50];

	// L("Loading sidedata");
	ai -> parser -> LoadVirtualFile("gamedata\\MOVEINFO.tdf");

	// L("Starting Loop");
	while(Valuestring != errorstring) {
		// L("Run Number: " << NumOfMoveTypes);
		sprintf(k, "%i", NumOfMoveTypes);
		ai -> parser -> GetDef(Valuestring, errorstring, string(sectionstring + k + "\\Name"));
		// L("Movetype: " << Valuestring);

		if (Valuestring != errorstring) {
			ai -> parser -> GetDef(Valuestring, string("10000"), string(sectionstring + k + "\\MaxWaterDepth"));
			maxwaterdepths.push_back(atoi(Valuestring.c_str()));
			// L("Max water depth: " << Valuestring);
			ai -> parser -> GetDef(Valuestring, string("-10000"), string(sectionstring + k + "\\MinWaterDepth"));
			minwaterdepths.push_back(atoi(Valuestring.c_str()));
			// L("minwaterdepths: " << Valuestring);
			ai -> parser -> GetDef(Valuestring, string("10000"), string(sectionstring + k + "\\MaxSlope"));
			moveslopes.push_back(atoi(Valuestring.c_str()));
			// L("moveslopes: " << Valuestring);
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
		char k[10];
		itoa (m, k, 10);
		MoveArrays[m] = new bool[totalcells];

		for (int i = 0; i < totalcells; i++) {
			MoveArrays[m][i] = false;

			if (SlopeMap[i] > moveslopes[m] || HeightMap[i] <= -maxwaterdepths[m] || HeightMap[i] >= -minwaterdepths[m]) {
				MoveArrays[m][i] = false;
				TestMoveArray[i] = true;
				// L("false");
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

		itoa (m, k, 10);
	}
}


void CPathFinder::CreateDefenseMatrix() {
	// L("Starting pathing");
	ai -> math -> TimerStart();
	int enemycomms[16];
	float3 enemyposes[16];
	ai -> dm -> ChokeMapsByMovetype.resize(NumOfMoveTypes);

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
		int numberofenemyplayers = ai -> cheat -> GetEnemyUnits(enemycomms);

		for (int i = 0; i < numberofenemyplayers; i++) {
			// L("Enemy comm: " << enemycomms[i]);
			enemyposes[i] = ai -> cheat -> GetUnitPos(enemycomms[i]);
		}

		float3 mypos = ai -> cb -> GetUnitPos(ai -> uh -> AllUnitsByCat[CAT_BUILDER] -> front());
		int reruns = 35;
		ai -> dm -> ChokeMapsByMovetype[m] = new float[totalcells];
		char k[10];
		itoa(m, k, 10);

		micropather -> SetMapData(MoveArrays[m], ai -> dm -> ChokeMapsByMovetype[m], PathMapXSize, PathMapYSize);
		double pathCostSum = 0;

		for (int i = 0; i < totalcells; i++) {
			ai -> dm -> ChokeMapsByMovetype[m][i] = 1;
		}

		int runcounter = 0;

		// HACK
		if (numberofenemyplayers > 0 && m == PATHTOUSE) {
			for (int r = 0; r < reruns; r++) {
				// L("reruns: " << r);
				for (int startpos = 0; startpos < numberofenemyplayers; startpos++) {
					if (micropather -> Solve(Pos2Node(enemyposes[startpos]), Pos2Node(mypos), &path, &totalcost) == MicroPather::SOLVED) {
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
												ai -> dm -> ChokeMapsByMovetype[m][actualy * PathMapXSize + actualx] += costmask[(myy + Range) * maskwidth + (myx+Range)];
											}
										}
									}
								}
							}
						}
						runcounter++;
					}
					// L("Enemy Pos " << startpos << " Cost: " << totalcost);
					pathCostSum += totalcost;
					// L("Time Taken: " << clock() - timetaken);
				}
			}

			// L("pathCostSum:  " << pathCostSum);
			// L("paths calculated, resmodifier: " << resmodifier );
		}
	}

	delete[] costmask;
	char c[512];
	sprintf(c, "Time Taken to create chokepoints: %f", ai -> math -> TimerSecs());
	ai -> cb -> SendTextMsg(c, 0);
}

void CPathFinder::PrintData(string s) {
	s = s;
	// L(s);
}

void CPathFinder::ClearPath() {
	path.resize(0);
}

unsigned CPathFinder::Checksum() {
	return micropather -> Checksum();
}


void* CPathFinder::XY2Node(int x, int y) {
	return (void*) (y * PathMapXSize + x);
}

void CPathFinder::Node2XY(void* node, int* x, int* y) {
	long index = (long)node;
	*y = index / PathMapXSize;
	*x = index - (*y * PathMapXSize);
}

float3 CPathFinder::Node2Pos(void* node) {
	float3 pos;
	int multiplier = int(8 * resmodifier);
	long index = (long) node;
	pos.z = (index / PathMapXSize) * multiplier;
	pos.x = (index - ((index / PathMapXSize) * PathMapXSize)) * multiplier;

	return pos;
}

void* CPathFinder::Pos2Node(float3 pos) {
	// L("pos.x = " << pos.x << " pos.z= " << pos.z << " node: " << (pos.z / (8 * THREATRES)) * PathMapXSize +  (pos.x / (8 * THREATRES)));
	return ((void*) (int(pos.z / 8 / THREATRES) * PathMapXSize + int((pos.x / 8 / THREATRES))));
}

/*
 * radius is in full res.
 * returns the path cost.
 */
float CPathFinder::MakePath(vector<float3>* posPath, float3* startPos, float3* endPos, int radius) {
	// L("Makepath radius Started");
	ai -> math -> TimerStart();
	float totalcost;
	ClearPath();
	int sx, sy, ex, ey;
	ai -> math -> F3MapBound(startPos);
	ai -> math -> F3MapBound(endPos);
	ex = int(endPos -> x / (8 * resmodifier));
	ey = int(endPos -> z / (8 * resmodifier));
	sy = int(startPos -> z / (8 * resmodifier));
	sx = int(startPos -> x / (8 * resmodifier));
	radius /= int(8 * resmodifier);

	// L("StartPos : " << startPos -> x << "," << startPos -> z << " End Pos: " << endPos -> x << "," << endPos -> z);
	if (micropather -> FindBestPathToPointOnRadius(XY2Node(sx, sy), XY2Node(ex, ey), &path, &totalcost, radius) == MicroPather::SOLVED) {
		// L("attack solution solved! Path size = " << path.size());
		posPath -> reserve(path.size());

		for (unsigned i = 0; i < path.size(); i++) {
			// L("adding path point");
			float3 mypos = Node2Pos(path[i]);
			mypos.y = ai -> cb -> GetElevation(mypos.x, mypos.z);
			posPath -> push_back(mypos);
		}
	}
	else {
		// L("MakePath: path failed");
		// ai -> cb -> SendTextMsg("ATTACK FAILED!", 0);
	}

	return totalcost;
}


float CPathFinder::FindBestPath(vector<float3>* posPath, float3* startPos, float myMaxRange, vector<float3>* possibleTargets) {
	// L("FindBestPath Started");
	// L("startPos: x: " << startPos -> x << ", z: " << startPos -> z);
	ai -> math -> TimerStart();
	float totalcost;
	ClearPath();

	// make a list with the points that will count as end nodes
	static vector<void*> endNodes;
	int radius = int(myMaxRange / (8 * resmodifier));
	int offsetSize = 0;

	endNodes.resize(0);
	endNodes.reserve(possibleTargets -> size() * radius * 10);
	// L("possibleTargets -> size(): " << possibleTargets -> size());

	pair<int, int>* offsets;

	{
		// L("radius: " << radius);
		int DoubleRadius = radius * 2;
		// used to speed up loops so no recalculation needed
		int SquareRadius = radius * radius;
		int* xend = new int[DoubleRadius + 1];

		for (int a = 0; a < DoubleRadius + 1; a++) {
			float z = a - radius;
			float floatsqrradius = SquareRadius;
			xend[a] = int(sqrt(floatsqrradius - z * z));
		}

		offsets = new pair<int, int>[DoubleRadius * 5];
		int index = 0;

		offsets[index].first = 0;
		// L("offsets[index].first: " << offsets[index].first);
		offsets[index].second = 0;
		// L("offsets[index].second: " << offsets[index].second);
		index++;

		for (int a = 1; a < radius + 1; a++) {
			// L("a: " << a);
			int endPos = xend[a];
			int startPos = xend[a - 1];
			// L("endPos: " << endPos);

			while (startPos <= endPos) {
				// L("startPos: " << startPos);
				offsets[index].first = startPos;
				// L("offsets[index].first: " << offsets[index].first);
				offsets[index].second = a;
				// L("offsets[index].second: " << offsets[index].second);
				startPos++;
				index++;
			}

			startPos--;
		}

		int index2 = index;
		for (int a = 0; a < index2 - 2; a++) {
			offsets[index].first = offsets[a].first;
			// L("offsets[index].first: " << offsets[index].first);
			offsets[index].second = DoubleRadius - ( offsets[a].second);
			// L("offsets[index].second: " << offsets[index].second);
			index++;
		}

		index2 = index;

		for (int a = 0; a < index2; a++) {
			offsets[index].first =  -( offsets[a].first);
			// L("offsets[index].first: " << offsets[index].first);
			offsets[index].second = offsets[a].second;
			// L("offsets[index].second: " << offsets[index].second);
			index++;
		}

		for (int a = 0; a < index; a++) {
			offsets[a].first = offsets[a].first;
			// L("offsets[index].first: " << offsets[a].first);
			offsets[a].second = offsets[a].second - radius;
			// L("offsets[index].second: " << offsets[a].second);
		}

		offsetSize = index;
		delete[] xend;
	}

	for (unsigned i = 0; i < possibleTargets -> size(); i++) {
		float3 f = (*possibleTargets)[i];
		int x, y;
		// L("Added: x: " << f.x << ", z: " << f.z);
		// TODO: make the circle here
		
		ai -> math -> F3MapBound(&f);
		void * node = Pos2Node(f);
		Node2XY(node, &x, &y);

		for (int j = 0; j < offsetSize; j++) {
			int sx = x + offsets[j].first;
			int sy = y + offsets[j].second
;
			if (sx >= 0 && sx < PathMapXSize && sy >= 0 && sy < PathMapYSize)
				endNodes.push_back(XY2Node(sx, sy));
		}

		// L("node: " << ((int) node) << ", x: " << x << ", y: " << y);
	}

	// L("endNodes.size(): " << endNodes.size());

	ai -> math -> F3MapBound(startPos);
	delete[] offsets;
	
	if (micropather -> FindBestPathToAnyGivenPoint(Pos2Node(*startPos), endNodes, &path, &totalcost) == MicroPather::SOLVED) {
		// L("attack solution solved! Path size = " << path.size());
        posPath -> reserve(path.size());

		for (unsigned i = 0; i < path.size(); i++) {
			// L("adding path point");
			int x, y;
			Node2XY(path[i], &x, &y);
			// L("node: " << ((int) path[i]) << ". x: " << x << ", y: " << y);
			float3 mypos = Node2Pos(path[i]);
			// L("mypos: x: " << mypos.x << ", z: " << mypos.z);
			mypos.y = ai -> cb -> GetElevation(mypos.x, mypos.z);
			posPath -> push_back(mypos);
		}
	}

//	else
//		L("FindBestPath: path failed!");

	return totalcost;
}


//alias hack to above function for one target (added for convenience in other parts of the code)
float CPathFinder::FindBestPathToRadius(vector<float3>* posPath, float3* startPos, float radiusAroundTarget, float3* target) {
	vector<float3> foo;
	foo.push_back(*target);

	return (this -> FindBestPath(posPath, startPos, radiusAroundTarget, &foo));
}
