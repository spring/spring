#include "IncCREG.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

CR_BIND(CDefenseMatrix, (NULL));
CR_REG_METADATA(CDefenseMatrix, (
	CR_MEMBER(ChokeMapsByMovetype),
	CR_MEMBER(ChokePointArray),
	CR_MEMBER(BuildMaskArray),

	// CR_MEMBER(spotFinder),
	CR_MEMBER(ai),
// These two only matter during one frame, at AI-Init mid-game -> do not care
//	CR_MEMBER(defAddQueue),
//	CR_MEMBER(defRemoveQueue),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

CDefenseMatrix::CDefenseMatrix(AIClasses* ai)
	: spotFinder(NULL)
	, ai(ai)
{
}



void CDefenseMatrix::PostLoad() {
	spotFinder = new CSpotFinder(ai, ai->pather->PathMapYSize, ai->pather->PathMapXSize);
	spotFinder->SetBackingArray(&ChokePointArray.front(), ai->pather->PathMapYSize, ai->pather->PathMapXSize);
}


void CDefenseMatrix::Init() {
	ChokePointArray.resize(ai->pather->totalcells);
	// used to mask bad spots that workers can't build at
	BuildMaskArray.resize(ai->pather->totalcells, 0);

	ai->pather->CreateDefenseMatrix();

	spotFinder = new CSpotFinder(ai, ai->pather->PathMapYSize, ai->pather->PathMapXSize);
	spotFinder->SetBackingArray(&ChokePointArray.front(), ai->pather->PathMapYSize, ai->pather->PathMapXSize);

	std::vector<DefPos>::const_iterator def;
	for (def = defAddQueue.begin(); def != defAddQueue.end(); ++def) {
		AddDefense(def->pos, def->def);
	}
	for (def = defRemoveQueue.begin(); def != defRemoveQueue.end(); ++def) {
		RemoveDefense(def->pos, def->def);
	}
	defAddQueue.clear();
	defRemoveQueue.clear();
}

void CDefenseMatrix::MaskBadBuildSpot(float3 pos) {
	if (MAPPOS_IN_BOUNDS(pos)) {
		const int f3multiplier = 8 * THREATRES;
		const int x = (int) (pos.x / f3multiplier);
		const int y = (int) (pos.z / f3multiplier);

		BuildMaskArray[y * ai->pather->PathMapXSize + x] = 1;
	}
}

float3 CDefenseMatrix::GetDefensePos(const UnitDef* def, float3 builderpos) {
	ai->ut->UpdateChokePointArray();

	int f3multiplier = 8 * THREATRES;
	int Range = int(ai->ut->GetMaxRange(def) / f3multiplier);
	int bestspotx = 0;
	int bestspoty = 0;
	float averagemapsize = sqrt(float(ai->pather->PathMapXSize * ai->pather->PathMapYSize)) * f3multiplier;
	float bestscore_fast = 0.0f;
	int bestspotx_fast = 0;
	int bestspoty_fast = 0;
	ai->math->TimerStart();

	spotFinder->SetRadius(Range);
	float* sumMap = spotFinder->GetSumMap();

	// hack to find a good start
	{
		int x = (int) (builderpos.x / f3multiplier);
		int y = (int) (builderpos.z / f3multiplier);
		float fastSumMap = sumMap[y * ai->pather->PathMapXSize + x];
		float3 spotpos = float3(x * f3multiplier, 0, y * f3multiplier);
		float myscore = fastSumMap / (builderpos.distance2D(spotpos) + averagemapsize / 8) * ((ai->pather->HeightMap[y * ai->pather->PathMapXSize + x] + 200) / (ai->pather->AverageHeight + 10)) / (ai->tm->ThreatAtThisPoint(spotpos) + 0.01);
		bestscore_fast = myscore;
		bestspotx_fast = x;
		bestspoty_fast = y;
	}

	int skipCount = 0;
	int testCount = 0;

	for (int x = 0; x < ai->pather->PathMapXSize / CACHEFACTOR; x++) {
		for (int y = 0; y < ai->pather->PathMapYSize / CACHEFACTOR; y++) {
			// KLOOTNOTE: SOMETIMES RETURNS UNINITIALIZED CRAP?
			// (gdb) print cachePoint->y   $2 = 219024104
			// (gdb) print cachePoint->x   $3 = -1215908928
			CachePoint* cachePoint = spotFinder->GetBestCachePoint(x, y);

			if (!cachePoint) {
				return ZeroVector;
			}

			float bestScoreInThisBox = cachePoint->maxValueInBox;

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

			float3 bestPosibleSpotpos = float3(bestX * f3multiplier, 0, bestY * f3multiplier);
			// this must be guessed, set it to the best possible (slow)
			float bestThreatAtThisPoint = 0.01 + ai->tm->GetAverageThreat() - 1;
			float bestDistance = builderpos.distance2D(bestPosibleSpotpos);
			float bestHeight = ai->pather->HeightMap[cachePoint->y * ai->pather->PathMapXSize + cachePoint->x] + 200;
			float bestPosibleMyScore = bestScoreInThisBox / (bestDistance + averagemapsize / 4) * (bestHeight + 200) / bestThreatAtThisPoint;
			// have a best posible score for all points inside the size of the cache box
			// if this is better than the current known best, test if any point inside the box is better

			if (bestPosibleMyScore > bestscore_fast) {
				testCount++;
				// must test all the points inside this box
				for (int sx = x * CACHEFACTOR; sx < ai->pather->PathMapXSize && sx < (x * CACHEFACTOR + CACHEFACTOR); sx++) {
					for (int sy = y * CACHEFACTOR; sy < ai->pather->PathMapYSize && sy < (y * CACHEFACTOR + CACHEFACTOR); sy++) {
						float fastSumMap = sumMap[sy * ai->pather->PathMapXSize + sx];
						float3 spotpos = float3(sx * f3multiplier, 0, sy * f3multiplier);
						float myscore = fastSumMap / (builderpos.distance2D(spotpos) + averagemapsize / 4) * (ai->pather->HeightMap[sy * ai->pather->PathMapXSize + sx]+200) / (ai->tm->ThreatAtThisPoint(spotpos) + 0.01);
						// THIS COULD BE REALLY SLOW!
						if (myscore > bestscore_fast && BuildMaskArray[sy * ai->pather->PathMapXSize + sx] == 0 && ai->cb->CanBuildAt(def, spotpos)) {
							bestscore_fast = myscore;
							bestspotx_fast = sx;
							bestspoty_fast = sy;
						}
					}
				}
			}
			else {
				// skip box
				skipCount++;
			}
		}
	}

	bestspotx = bestspotx_fast;
	bestspoty = bestspoty_fast;

	return float3(bestspotx * f3multiplier, 0, bestspoty * f3multiplier);
}



void CDefenseMatrix::AddDefense(float3 pos, const UnitDef* def) {

	if (!IsInitialized()) {
		DefPos defPos = {pos, def};
		defAddQueue.push_back(defPos);
		return;
	}

	int f3multiplier = 8 * THREATRES;
	int Range = int(ai->ut->GetMaxRange(def) / f3multiplier);
	int squarerange = Range * Range;
	int x, y;
	ai->math->F32XY(pos, &x, &y, 8);

	// TODO: test if this works
	for (int myx = x - Range; myx <= x + Range; myx++) {
		if (myx >= 0 && myx < ai->pather->PathMapXSize) {
			for (int myy = y - Range; myy <= y + Range; myy++) {
				int distance = int((x - myx) * (x - myx) + (y - myy) * (y - myy) - 0.5);
				if (myy >= 0 && myy < ai->pather->PathMapYSize && (distance) <= squarerange) {
					for (int i = 0; i < ai->pather->NumOfMoveTypes;i++) {
						ChokeMapsByMovetype[i][myy * ai->pather->PathMapXSize + myx] /= 2;
					}
				}
			}
		}
	}

	spotFinder->InvalidateSumMap(x, y, Range + 1);
	// ai->debug->MakeBWTGA(Chokepointmap, ai->tm->GetThreatMapWidth(), ai->tm->GetThreatMapHeight(), "DebugPathMatrix", 1);
}


void CDefenseMatrix::RemoveDefense(float3 pos, const UnitDef* def) {

	if (!IsInitialized()) {
		DefPos defPos = {pos, def};
		defRemoveQueue.push_back(defPos);
		return;
	}

	int f3multiplier = 8 * THREATRES;
	int Range = int(ai->ut->GetMaxRange(def) / f3multiplier);
	int squarerange = Range * Range;
	int x, y;
	ai->math->F32XY(pos, &x, &y, 8);

	// TODO: test if this works
	for (int myx = x - Range; myx <= x + Range; myx++) {
		if (myx >= 0 && myx < ai->pather->PathMapXSize) {
			for (int myy = y - Range; myy <= y + Range; myy++) {
				int distance = int((x - myx) * (x - myx) + (y - myy) * (y - myy) - 0.5);

				if (myy >= 0 && myy < ai->pather->PathMapYSize && (distance) <= squarerange) {
					for (int i = 0; i < ai->pather->NumOfMoveTypes;i++) {
						ChokeMapsByMovetype[i][myy * ai->pather->PathMapXSize + myx] *= 2;
					}
				}
			}
		}
	}

	spotFinder->InvalidateSumMap(x, y, Range);
}
