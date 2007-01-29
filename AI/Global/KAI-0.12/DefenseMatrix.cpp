#include "DefenseMatrix.h"

CDefenseMatrix::CDefenseMatrix(AIClasses *ai) {
	this -> ai = ai;
}
CDefenseMatrix::~CDefenseMatrix() {
	for (unsigned int i = 0; i != ChokeMapsByMovetype.size();i++) { 
		delete [] ChokeMapsByMovetype[i];
	}

	delete[] ChokePointArray;
	delete[] BuildMaskArray;
}

void CDefenseMatrix::Init() {
//	std::cout << "CDefenseMatrix::Init()" << std::endl;

	ChokePointArray = new float[ai -> pather -> totalcells];
	// temp only, will be used to mask bad spots that workers can't build at
	BuildMaskArray = new int[ai -> pather -> totalcells];

	for (int i = 0; i < ai -> pather -> totalcells; i++)
		BuildMaskArray[i] = 0;
	
	ai -> pather -> CreateDefenseMatrix();

	// KLOOTNOTE: this is where we could start crashing with EE
	spotFinder = new CSpotFinder(ai, ai -> pather -> PathMapYSize, ai -> pather -> PathMapXSize);
	spotFinder -> SetBackingArray(ChokePointArray, ai -> pather -> PathMapYSize, ai -> pather -> PathMapXSize);
}

void CDefenseMatrix::MaskBadBuildSpot(float3 pos) {
	int f3multiplier = 8 * THREATRES;
	int x = (int) (pos.x / f3multiplier);
	int y = (int) (pos.z / f3multiplier);

	BuildMaskArray[y * ai -> pather -> PathMapXSize + x] = 1;
}

float3 CDefenseMatrix::GetDefensePos(const UnitDef* def, float3 builderpos) {
	ai -> ut -> UpdateChokePointArray();
	int f3multiplier = 8 * THREATRES;
	int Range = int(ai -> ut -> GetMaxRange(def) / f3multiplier);
	int bestspotx = 0;
	int bestspoty = 0;
	// L("GetDefensePos: Range: " << Range);
	float averagemapsize = sqrt(float(ai -> pather -> PathMapXSize*ai -> pather -> PathMapYSize)) * f3multiplier;
	float bestscore_fast = 0;
	int bestspotx_fast = 0;
	int bestspoty_fast = 0;
	ai -> math -> TimerStart();

	spotFinder -> SetRadius(Range);
	float* sumMap = spotFinder -> GetSumMap();

	// hack to find a good start
	{
		int x = (int) (builderpos.x / f3multiplier);
		int y = (int) (builderpos.z / f3multiplier);
		float fastSumMap = sumMap[y * ai -> pather -> PathMapXSize + x];
		float3 spotpos = float3(x*f3multiplier,0,y*f3multiplier);
		float myscore = fastSumMap/(builderpos.distance2D(spotpos) + averagemapsize / 8) * ((ai -> pather -> HeightMap[y * ai -> pather -> PathMapXSize + x] + 200) / (ai -> pather -> AverageHeight + 10)) / (ai -> tm -> ThreatAtThisPoint(spotpos)+0.01);
		bestscore_fast = myscore;
		bestspotx_fast = x;
		bestspoty_fast = y;
		// L("Starting with bestscore_fast: " << bestscore_fast);
	}



	// L("ai -> pather -> PathMapXSize: " << ai -> pather -> PathMapXSize);
	// L("ai -> pather -> PathMapYSize: " << ai -> pather -> PathMapYSize);
	// L("ai -> pather -> PathMapXSize / CACHEFACTOR: " << ai -> pather -> PathMapXSize / CACHEFACTOR);
	// L("ai -> pather -> PathMapYSize / CACHEFACTOR: " << ai -> pather -> PathMapYSize / CACHEFACTOR);

	int skipCount = 0;
	int testCount = 0;
	for (int x = 0; x < ai -> pather -> PathMapXSize / CACHEFACTOR; x++) {
		for (int y = 0; y < ai -> pather -> PathMapYSize / CACHEFACTOR; y++) {
			// L("x: " << x << ", y: " << y);

			CachePoint* cachePoint = spotFinder -> GetBestCachePoint(x, y);
			float bestScoreInThisBox = cachePoint -> maxValueInBox;

			// guess that this point is as good as posible

			// make best posible build spot (nearest to builder)
			float bestX = builderpos.x / f3multiplier;
			float bestY = builderpos.z / f3multiplier;

			if (bestX > x * CACHEFACTOR) {
				if (bestX > (x * CACHEFACTOR + CACHEFACTOR)) {
					bestX = x * CACHEFACTOR + CACHEFACTOR;
				}
			}
			else {
				bestX = x * CACHEFACTOR;
			}

			if (bestY > y * CACHEFACTOR) {
				if (bestY > (y * CACHEFACTOR + CACHEFACTOR)) {
					bestY = y * CACHEFACTOR + CACHEFACTOR;
				}
			}
			else {
				bestY = y * CACHEFACTOR;
			}

			// L("bestX: " << bestX << ", bestY: " << bestY);

			float3 bestPosibleSpotpos = float3(bestX*f3multiplier,0,bestY*f3multiplier);
 			// this must be guessed, set it to the best posible (slow)
			float bestThreatAtThisPoint = 0.01 + ai -> tm -> GetAverageThreat() - 1;
			// L("bestThreatAtThisPoint: " << bestThreatAtThisPoint);
			float bestDistance = builderpos.distance2D(bestPosibleSpotpos);
			// L("bestDistance: " << bestDistance);
			// L("cachePoint -> maxValueInBox: " << cachePoint -> maxValueInBox << ", cachePoint -> x: " << cachePoint -> x << ", cachePoint -> y: " << cachePoint -> y);
			float bestHeight = ai -> pather -> HeightMap[cachePoint -> y * ai -> pather -> PathMapXSize + cachePoint -> x] + 200;
			// L("bestHeight: " << bestHeight);
			float bestPosibleMyScore = bestScoreInThisBox / (bestDistance + averagemapsize / 4) * (bestHeight + 200) / bestThreatAtThisPoint;
			// L("bestPosibleMyScore: " << bestPosibleMyScore);
			// have a best posible score for all points inside the size of the cache box
			// if this is better than the current known best, test if any point inside the box is better
			// L("bestscore_fast: " << bestscore_fast);

			if (bestPosibleMyScore > bestscore_fast) {
				testCount++;
				// must test all the points inside this box
				for (int sx = x * CACHEFACTOR; sx < ai -> pather -> PathMapXSize && sx < (x * CACHEFACTOR + CACHEFACTOR); sx++) {
					for (int sy = y * CACHEFACTOR; sy < ai -> pather -> PathMapYSize && sy < (y * CACHEFACTOR + CACHEFACTOR); sy++) {
						float fastSumMap = sumMap[sy * ai -> pather -> PathMapXSize + sx];
						float3 spotpos = float3(sx * f3multiplier, 0, sy * f3multiplier);
						float myscore = fastSumMap / (builderpos.distance2D(spotpos) + averagemapsize / 4) * (ai -> pather -> HeightMap[sy * ai -> pather -> PathMapXSize + sx]+200) / (ai -> tm -> ThreatAtThisPoint(spotpos) + 0.01);
						// L("Checking defense spot. fastSumMap: " << fastSumMap << ", Distance: " << builderpos.distance2D(spotpos) << " Height: " << ai -> pather -> HeightMap[sy * ai -> pather -> PathMapXSize + sx] << " Threat " << ai -> tm -> ThreatAtThisPoint(spotpos)<< " Score: " << myscore);
						// THIS COULD BE REALLY SLOW!
						if (myscore > bestscore_fast && BuildMaskArray[sy * ai -> pather -> PathMapXSize + sx] == 0 && ai -> cb -> CanBuildAt(def, spotpos)) {
							bestscore_fast = myscore;
							bestspotx_fast = sx;
							bestspoty_fast = sy;
							// L("new bestscore_fast: " << myscore << "sx: " << sx << ", sy: " << sy);
						}
					}
				}
			}
			else {
				// L("Skiping box");
				skipCount++;
			}
		}
	}

	// L("spotFinder new time:" << ai -> math -> TimerSecs());
	// L("skipCount: " << skipCount << ", testCount: " << testCount);
	bestspotx = bestspotx_fast;
	bestspoty = bestspoty_fast;

	return float3(bestspotx * f3multiplier, 0, bestspoty * f3multiplier);
}



void CDefenseMatrix::AddDefense(float3 pos, const UnitDef* def) {
	int f3multiplier = 8*THREATRES;
	int Range = int(ai -> ut -> GetMaxRange(def) / f3multiplier);
	int squarerange = Range * Range;
	int x,y;
	ai -> math -> F32XY(pos,&x,&y,8);
	// TODO: test if this works:
	
	
	for (int myx = x - Range; myx <= x + Range; myx++){
		if (myx >= 0 && myx < ai -> pather -> PathMapXSize){
			for (int myy = y - Range; myy <= y + Range; myy++){
				int distance = int((x - myx)*(x - myx) + (y - myy)*(y - myy) - 0.5);
				if (myy >= 0 && myy < ai -> pather -> PathMapYSize && (distance) <= squarerange){
					for(int i = 0; i < ai -> pather -> NumOfMoveTypes;i++){
						ChokeMapsByMovetype[i][myy * ai -> pather -> PathMapXSize + myx] /= 2;
					}
				}
			}
		}
	}
	spotFinder -> InvalidateSumMap(x, y, Range +1);
	
	
	////L("Adding a defense: " << def -> humanName << " of range: " << Range << "at " << x << "," << y);
	//ai -> debug -> MakeBWTGA(Chokepointmap,ai -> tm -> ThreatMapWidth,ai -> tm -> ThreatMapHeight,"DebugPathMatrix",1);
}


void CDefenseMatrix::RemoveDefense(float3 pos, const UnitDef* def) {
	int f3multiplier = 8*THREATRES;
	int Range = int(ai -> ut -> GetMaxRange(def) / f3multiplier);
	int squarerange = Range * Range;
	int x,y;
	ai -> math -> F32XY(pos,&x,&y,8);
	// TODO: test if this works:
	
	for (int myx = x - Range; myx <= x + Range; myx++){
		if (myx >= 0 && myx < ai -> pather -> PathMapXSize){
			for (int myy = y - Range; myy <= y + Range; myy++){
				int distance = int((x - myx)*(x - myx) + (y - myy)*(y - myy) - 0.5);
				if (myy >= 0 && myy < ai -> pather -> PathMapYSize && (distance) <= squarerange){
					for(int i = 0; i < ai -> pather -> NumOfMoveTypes;i++){
						ChokeMapsByMovetype[i][myy * ai -> pather -> PathMapXSize + myx] *= 2;
					}
				}
			}
		}
	}
	spotFinder -> InvalidateSumMap(x, y, Range);
}
