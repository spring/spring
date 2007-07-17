#include "DefenseMatrix.h"
#include "CoverageHandler.h"

CR_BIND(CDefenseMatrix ,(NULL))

CR_REG_METADATA(CDefenseMatrix,(
				CR_MEMBER(ChokeMapsByMovetype),
				CR_MEMBER(ChokePointArray),
				CR_MEMBER(BuildMaskArray),
				CR_MEMBER(ai),
				CR_RESERVED(128),
				CR_POSTLOAD(PostLoad)
				));

CDefenseMatrix::CDefenseMatrix(AIClasses* ai) {
	this -> ai = ai;
}
CDefenseMatrix::~CDefenseMatrix() {
//	for (unsigned int i = 0; i != ChokeMapsByMovetype.size();i++){
//		delete [] ChokeMapsByMovetype[i];
//	}
//	delete [] ChokePointArray;
//	delete [] BuildMaskArray;
}

int getDefensePosTime;
void CDefenseMatrix::PostLoad()
{
	getDefensePosTime = ai->math->GetNewTimerGroupNumber("GetDefensePos()");
	
//	ai->pather->CreateDefenseMatrix();	
	spotFinder = new CSpotFinder(ai, ai->pather->PathMapYSize, ai->pather->PathMapXSize);
	spotFinder->SetBackingArray(&ChokePointArray.front(), ai->pather->PathMapYSize, ai->pather->PathMapXSize);
}

void CDefenseMatrix::Init() {
//	ChokePointArray = new float[ai->pather->totalcells];
//	BuildMaskArray = new int[ai->pather->totalcells]; // This is a temp only, it will be used to mask bad spots that workers cant build at
//	for(int i = 0; i < ai->pather->totalcells; i++)
//		BuildMaskArray[i] = 0;
	ChokePointArray.resize(ai->pather->totalcells);
	BuildMaskArray.resize(ai->pather->totalcells);
	
	getDefensePosTime = ai->math->GetNewTimerGroupNumber("GetDefensePos()");
	
	ai->pather->CreateDefenseMatrix();	
	spotFinder = new CSpotFinder(ai, ai->pather->PathMapYSize, ai->pather->PathMapXSize);
	spotFinder->SetBackingArray(&ChokePointArray.front(), ai->pather->PathMapYSize, ai->pather->PathMapXSize);
}

void CDefenseMatrix::MaskBadBuildSpot(float3 pos)
{
	int f3multiplier = 8*THREATRES;
	int x = (int) (pos.x / f3multiplier);
	int y = (int) (pos.z / f3multiplier);
	BuildMaskArray[y * ai->pather->PathMapXSize + x]++;
}

void CDefenseMatrix::UnmaskBadBuildSpot(float3 pos)
{
	int f3multiplier = 8*THREATRES;
	int x = (int) (pos.x / f3multiplier);
	int y = (int) (pos.z / f3multiplier);
	if (BuildMaskArray[y * ai->pather->PathMapXSize + x]>0) BuildMaskArray[y * ai->pather->PathMapXSize + x]--;
}

float3 CDefenseMatrix::GetDefensePos(const UnitDef* def, float3 builderpos, float heightK, CCoverageHandler *ch)
{
	ai->math->StartTimer(getDefensePosTime);
	ai->ut->UpdateChokePointArray();  // TODO: UpdateChokePointArray() needs fixing.
	int f3multiplier = 8*THREATRES;
	int Range = int(ai->ut->GetMaxRange(def) / f3multiplier);

	//super hack by firenu (shouldnt happen anyway):
	if (Range <= 0) {
		L("GetDefensePos: Range was zero for a defense building: " << def->humanName);
		Range = 1;
	}


	int bestspotx = 0;
	int bestspoty = 0;
	L("GetDefensePos: Range: " << Range);
	float averagemapsize = sqrt(float(ai->pather->PathMapXSize*ai->pather->PathMapYSize)) * f3multiplier;
	/*
	ai->math->TimerStart();
	spotFinder->SetRadius(Range);
	float* sumMap = spotFinder->GetSumMap();
	for(int x = 0; x < ai->pather->PathMapXSize; x++){
		for(int y = 0; y < ai->pather->PathMapYSize; y++){
			
			float myscore = 0;
			for (int myx = x - Range; myx <= x + Range; myx++){
				if (myx >= 0 && myx < ai->pather->PathMapXSize){
					for (int myy = y - Range; myy <= y + Range; myy++){
						int distance = (x - myx)*(x - myx) + (y - myy)*(y - myy);// - 0.5;
						if (myy >= 0 && myy < ai->pather->PathMapYSize && (distance) <= squarerange){							
							myscore += Chokepointmap[myy * ai->pather->PathMapXSize + myx];
						}
					}
				}
			}
			
			// Test how large the dif is between sumMap and myscore:
			
			float fastSumMap = sumMap[y * ai->pather->PathMapXSize + x];
			float delta = fastSumMap - myscore;
			float errorSize = abs(delta /myscore);
			if(delta != 0)
				L("fastSumMap: " << fastSumMap << ", myscore: " << myscore << ", errorSize: " << errorSize << ", delta: " << delta);
			
			
			float3 spotpos = float3(x*f3multiplier,0,y*f3multiplier);		
			myscore = myscore/(builderpos.distance2D(spotpos) + averagemapsize / 4) * (ai->pather->HeightMap[y * ai->pather->PathMapXSize + x]+200) / (ai->tm->ThreatAtThisPoint(spotpos)+0.01);
			//L("Checking defense spot. Distance: " << builderpos.distance2D(spotpos) << " Height: " << ai->pather->HeightMap[y * ai->pather->PathMapXSize + x] << " Threat " << ai->tm->ThreatAtThisPoint(spotpos)<< " Score: " << myscore);
			if(myscore> bestscore && ai->pather->VehicleArray[y * ai->pather->PathMapXSize + x]){
				bestscore = myscore;
				bestspotx = x;
				bestspoty = y;
			}
		}
	}
	L("old:" << ai->math->TimerSecs());
	*/
	float bestscore_fast = 0;
	int bestspotx_fast = 0;
	int bestspoty_fast = 0;
	ai->math->TimerStart();
	
	spotFinder->SetRadius(Range);
	float* sumMap = spotFinder->GetSumMap();
	// Hack to find a good start: 
	{
		int x = (int) (builderpos.x / f3multiplier);
		int y = (int) (builderpos.z / f3multiplier);
		if (x<0) x=0;
		if (y<0) y=0;
		if (x>ai->pather->PathMapXSize-1) x=ai->pather->PathMapXSize-1;
		if (y>ai->pather->PathMapYSize-1) y=ai->pather->PathMapYSize-1;
		float fastSumMap = sumMap[y * ai->pather->PathMapXSize + x];
		float3 spotpos = float3(x*f3multiplier,0,y*f3multiplier);
		float myscore = fastSumMap/(builderpos.distance2D(spotpos) + averagemapsize / 8) * ((ai->pather->HeightMap[y * ai->pather->PathMapXSize + x]*heightK + 200) / (ai->pather->AverageHeight + 10)) / (ai->tm->ThreatAtThisPoint(spotpos)+0.01);
		bestscore_fast = myscore;
		bestspotx_fast = x;
		bestspoty_fast = y;
		//L("Starting with bestscore_fast: " << bestscore_fast);
	}
	
	
	
	//L("ai->pather->PathMapXSize: " << ai->pather->PathMapXSize);
	//L("ai->pather->PathMapYSize: " << ai->pather->PathMapYSize);
	//L("ai->pather->PathMapXSize / CACHEFACTOR: " << ai->pather->PathMapXSize / CACHEFACTOR);
	//L("ai->pather->PathMapYSize / CACHEFACTOR: " << ai->pather->PathMapYSize / CACHEFACTOR);
	int skipCount = 0;
	int testCount = 0;
	for(int x = 0; x < ai->pather->PathMapXSize / CACHEFACTOR; x++){
		for(int y = 0; y < ai->pather->PathMapYSize / CACHEFACTOR; y++){
			//L("x: " << x << ", y: " << y);
			
			CachePoint *cachePoint = spotFinder->GetBestCachePoint(x, y);
			float bestScoreInThisBox = cachePoint->maxValueInBox;
			
			// Guess that this point is as good as posible:
			
			// Make the best posible build spot (the one nearest to the builder):
			float bestX = builderpos.x / f3multiplier;
			float bestY = builderpos.z / f3multiplier;
			if(bestX > x * CACHEFACTOR)
			{
				if(bestX > (x * CACHEFACTOR + CACHEFACTOR))
				{
					bestX = x * CACHEFACTOR + CACHEFACTOR;
				}
			} else {
				bestX = x * CACHEFACTOR;
			}
			
			if(bestY > y * CACHEFACTOR)
			{
				if(bestY > (y * CACHEFACTOR + CACHEFACTOR))
				{
					bestY = y * CACHEFACTOR + CACHEFACTOR;
				}
			} else {
				bestY = y * CACHEFACTOR;
			}
			
			//L("bestX: " << bestX << ", bestY: " << bestY);
			
			float3 bestPosibleSpotpos = float3(bestX*f3multiplier,0,bestY*f3multiplier);
			float bestThreatAtThisPoint = 0.01 + ai->tm->GetAverageThreat() -1; // This must be guessed atm, set it to the best posible (slow)
			//L("bestThreatAtThisPoint: " << bestThreatAtThisPoint);
			float bestDistance = builderpos.distance2D(bestPosibleSpotpos);
			//L("bestDistance: " << bestDistance);
			//L("cachePoint->maxValueInBox: " << cachePoint->maxValueInBox << ", cachePoint->x: " << cachePoint->x << ", cachePoint->y: " << cachePoint->y);
			float bestHeight = ai->pather->HeightMap[cachePoint->y * ai->pather->PathMapXSize + cachePoint->x]*heightK + 200; // This must be guessed atm
			//L("bestHeight: " << bestHeight);
			float bestPosibleMyScore = bestScoreInThisBox / (bestDistance + averagemapsize / 8) * (bestHeight + 200) / bestThreatAtThisPoint;
			//L("bestPosibleMyScore: " << bestPosibleMyScore);
			// Have a best posible score for all points inside the size of the cache box.
			// If this is better than the current known best, test if any points inside the box is better:
			//L("bestscore_fast: " << bestscore_fast);
			if(bestPosibleMyScore > bestscore_fast)
			{
				testCount++;
				// Must test all the points inside this box:
				for(int sx = x * CACHEFACTOR; sx < ai->pather->PathMapXSize && sx < (x * CACHEFACTOR + CACHEFACTOR); sx++){
					for(int sy = y * CACHEFACTOR; sy < ai->pather->PathMapYSize && sy < (y * CACHEFACTOR + CACHEFACTOR); sy++){
						if (ai->uh->Distance2DToNearestFactory(sx*f3multiplier,sy*f3multiplier)>DEFCBS_RADIUS) continue;
						if (ch && ch->GetCoverage(float3(sx*f3multiplier,0,sy*f3multiplier))!=0) continue;
						float fastSumMap = sumMap[sy * ai->pather->PathMapXSize + sx];
						float3 spotpos = float3(sx*f3multiplier,0,sy*f3multiplier);
						float myscore = fastSumMap/(builderpos.distance2D(spotpos) + averagemapsize / 4) *
						(ai->pather->HeightMap[sy * ai->pather->PathMapXSize + sx]*heightK+200) / (ai->tm->ThreatAtThisPoint(spotpos)+0.01);
						//L("Checking defense spot. fastSumMap: " << fastSumMap << ", Distance: " << builderpos.distance2D(spotpos) << " Height: " << ai->pather->HeightMap[sy * ai->pather->PathMapXSize + sx] << " Threat " << ai->tm->ThreatAtThisPoint(spotpos)<< " Score: " << myscore);
						if(myscore > bestscore_fast && BuildMaskArray[sy * ai->pather->PathMapXSize + sx] == 0 && ai->cb->CanBuildAt(def,spotpos)){ // COULD BE REALLY SLOW!
							bestscore_fast = myscore;
							bestspotx_fast = sx;
							bestspoty_fast = sy;
							//L("new bestscore_fast: " << myscore << "sx: " << sx << ", sy: " << sy);
						}
					}
				}
			}
			else
			{
				//L("Skiping box");
				skipCount++;
			}	
			

		}
	}
	L("spotFinder new time:" << ai->math->TimerSecs());
	L("skipCount: " << skipCount << ", testCount: " << testCount);
	/*
	if(bestspotx_fast != bestspotx || bestspoty_fast != bestspoty || bestscore_fast != bestscore)
	{
		L("ERROR: bestscore_fast: " << bestscore_fast << ", bestscore: "  << bestscore_fast);
		L("bestspotx_fast: " << bestspotx_fast << ", bestspotx: "  << bestspotx << ", bestspoty_fast: " << bestspoty_fast << ", bestspoty: "  << bestspoty );
	}*/
	bestspotx = bestspotx_fast;
	bestspoty = bestspoty_fast;
	
	
	/*
	ai->math->TimerStart();
	//spotFinder->BackingArrayChanged();
	spotFinder->SetRadius(Range);
	float* sumMap = spotFinder->GetSumMap();
	//L("GetSumMap:" << ai->math->TimerSecs());
	ai->math->TimerStart();
	bestscore = 0;
	bestspotx = 0;
	bestspoty = 0;
	for(int x = 0; x < ai->pather->PathMapXSize; x++){
		for(int y = 0; y < ai->pather->PathMapYSize; y++){
			float myscore = sumMap[y * ai->pather->PathMapXSize + x];
			float3 spotpos = float3(x*f3multiplier,0,y*f3multiplier);		
			myscore = myscore/(builderpos.distance2D(spotpos) + averagemapsize / 4) * (ai->pather->HeightMap[y * ai->pather->PathMapXSize + x]+200) / (ai->tm->ThreatAtThisPoint(spotpos)+0.01);
			//L("Checking defense spot. Distance: " << builderpos.distance2D(spotpos) << " Height: " << ai->pather->HeightMap[y * ai->pather->PathMapXSize + x] << " Threat " << ai->tm->ThreatAtThisPoint(spotpos)<< " Score: " << myscore);
			if(myscore> bestscore && ai->pather->VehicleArray[y * ai->pather->PathMapXSize + x]){
				bestscore = myscore;
				bestspotx = x;
				bestspoty = y;
			}
		}
	}
	
	L("spotFinder new time:" << ai->math->TimerSecs());
	*/
	ai->math->StopTimer(getDefensePosTime);
	return float3(bestspotx*f3multiplier,0,bestspoty*f3multiplier);
}

void CDefenseMatrix::AddDefense(float3 pos, const UnitDef* def)
{
	
	int f3multiplier = 8*THREATRES;
	int Range = int(ai->ut->GetMaxRange(def) / f3multiplier);
	int squarerange = Range * Range;
	int x,y;
	ai->math->F32XY(pos,&x,&y,8);
	// TODO: test if this works:
	
	
	for (int myx = x - Range; myx <= x + Range; myx++){
		if (myx >= 0 && myx < ai->pather->PathMapXSize){
			for (int myy = y - Range; myy <= y + Range; myy++){
				int distance = int((x - myx)*(x - myx) + (y - myy)*(y - myy) - 0.5);
				if (myy >= 0 && myy < ai->pather->PathMapYSize && (distance) <= squarerange){
					for (unsigned int i = 0; i != ChokeMapsByMovetype.size();i++) {
						ChokeMapsByMovetype[i][myy * ai->pather->PathMapXSize + myx] /= 2;
					}
				}
			}
		}
	}
	spotFinder->InvalidateSumMap(x, y, Range +1);
	
	
	//L("Adding a defense: " << def->humanName << " of range: " << Range << "at " << x << "," << y);
	//ai->debug->MakeBWTGA(Chokepointmap,ai->tm->ThreatMapWidth,ai->tm->ThreatMapHeight,"DebugPathMatrix",1);
}

void CDefenseMatrix::RemoveDefense(float3 pos, const UnitDef* def)
{
	int f3multiplier = 8*THREATRES;
	int Range = int(ai->ut->GetMaxRange(def) / f3multiplier);
	int squarerange = Range * Range;
	int x,y;
	ai->math->F32XY(pos,&x,&y,8);
	// TODO: test if this works:
	
	for (int myx = x - Range; myx <= x + Range; myx++){
		if (myx >= 0 && myx < ai->pather->PathMapXSize){
			for (int myy = y - Range; myy <= y + Range; myy++){
				int distance = int((x - myx)*(x - myx) + (y - myy)*(y - myy) - 0.5);
				if (myy >= 0 && myy < ai->pather->PathMapYSize && (distance) <= squarerange){
					for (unsigned int i = 0; i != ChokeMapsByMovetype.size();i++) {
						ChokeMapsByMovetype[i][myy * ai->pather->PathMapXSize + myx] *= 2;
					}
				}
			}
		}
	}
	spotFinder->InvalidateSumMap(x, y, Range);
}
