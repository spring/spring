/*#pragma once removed*/
#include "PathFinder.h"


CPathFinder::CPathFinder(AIClasses *ai)
{
	this->ai=ai;
	resmodifier = THREATRES; // 8 = speed, 2 = precision
	PathMapXSize =ai->cb->GetMapWidth() /resmodifier;
	PathMapYSize =ai->cb->GetMapHeight() /resmodifier;
	totalcells = PathMapXSize*PathMapYSize;
	micropather = new MicroPather(ai, totalcells);
	HeightMap = new float[totalcells];
	SlopeMap = new float[totalcells];
	TestMoveArray = new bool[totalcells];
	canMoveIntMaskArray = new unsigned[totalcells];
	NumOfMoveTypes = 0;
}


CPathFinder::~CPathFinder()
{
	delete [] canMoveIntMaskArray;
	delete [] SlopeMap; 
	delete [] HeightMap;
	delete [] TestMoveArray;
	//for(int i = 0; i != MoveArrays.size();i++){
	//	delete [] MoveArrays[i];
	//}
	delete micropather;
}

int microPatherTime;
int patherFunctionSimulationSetup;
void CPathFinder::Init()
{	
	patherTime = ai->math->GetNewTimerGroupNumber("All pather calls (simulated)");
	microPatherTime = ai->math->GetNewTimerGroupNumber("Micropather");
	patherFunctionSimulationSetup = ai->math->GetNewTimerGroupNumber("Pather function-simulation setup time");
	
	const float* springHeightMap = ai->cb->GetHeightMap();
	int mapx = ai->cb->GetMapWidth() -1;
	int hmapx = ai->cb->GetMapWidth() /2;
	int mapy = ai->cb->GetMapHeight() -1;
	int hmapy = ai->cb->GetMapHeight() /2;
	float* newSlopeMap;
	if(hmapx == PathMapXSize)
		newSlopeMap = SlopeMap;
	else
		newSlopeMap = new float[hmapx * hmapy];
	L("Start newSlopeMap"); 
	//assert(false);
	for(int y=2;y<mapy-2;y+=2){ // This must be removed!!!!!!  
		for(int x=2;x<mapx-2;x+=2){
			float3 e1(-SQUARE_SIZE*4,springHeightMap[(y-1)*(mapx+1)+(x-1)]-springHeightMap[(y-1)*(mapx+1)+x+3],0);
			float3 e2( 0,springHeightMap[(y-1)*(mapx+1)+(x-1)]-springHeightMap[(y+3)*(mapx+1)+(x-1)],-SQUARE_SIZE*4);
			
			float3 n=e2.cross(e1);
			n.Normalize();

			e1=float3( SQUARE_SIZE*4,springHeightMap[(y+3)*(mapx+1)+x+3]-springHeightMap[(y+3)*(mapx+1)+x-1],0);
			e2=float3( 0,springHeightMap[(y+3)*(mapx+1)+x+3]-springHeightMap[(y-1)*(mapx+1)+x+3],SQUARE_SIZE*4);
			
			float3 n2=e2.cross(e1);
			n2.Normalize();
			int index = (y/2)*hmapx+(x/2);
			//assert(index < PathMapXSize * PathMapYSize);
			newSlopeMap[index]=1-(n.y+n2.y)*0.5;
		}
	}
	L("End newSlopeMap");
	ai->debug->MakeBWTGA(newSlopeMap,hmapx,hmapy,"SpringSlopeMap");
	if(hmapx > PathMapXSize)
	{
		// Rescale the slope map from spring: (simple test)
		int mapX = hmapx;
		hmapx /= 2;
		hmapy /= 2;
		assert( hmapx / 2 == PathMapXSize); // Temp safty test
		float* tempMap = new float[hmapx * hmapy];
		float* mapDoubleSize = newSlopeMap;
		for(int y = 0; y < hmapy -1; y++){
			for(int x = 0; x < hmapx -1; x++){
				//float avg = (mapDoubleSize[(y*2)*mapX + 2*x] + 
				//			mapDoubleSize[(y*2)*mapX + 2*x+1] +
				//			mapDoubleSize[(y*2+1)*mapX + 2*x] +
				//			mapDoubleSize[(y*2+1)*mapX + 2*x+1]) / 4;
				float maxValue = max( max(mapDoubleSize[(y*2)*mapX + 2*x], mapDoubleSize[(y*2)*mapX + 2*x+1])
									, max(mapDoubleSize[(y*2+1)*mapX + 2*x], mapDoubleSize[(y*2+1)*mapX + 2*x+1]) );
				tempMap[y*hmapx + x] = maxValue;
			}
		}
		delete [] newSlopeMap;
		// Done with a /2 now do one more:  (simple test)
		mapX = hmapx;
		hmapx /= 2;
		hmapy /= 2;
		assert( hmapx == PathMapXSize); // Temp safty test
		mapDoubleSize = tempMap;
		tempMap = SlopeMap;
		for(int y = 0; y < hmapy -1; y++){
			for(int x = 0; x < hmapx -1; x++){
				//float avg = (mapDoubleSize[(y*2)*mapX + 2*x] + 
				//			mapDoubleSize[(y*2)*mapX + 2*x+1] +
				//			mapDoubleSize[(y*2+1)*mapX + 2*x] +
				//			mapDoubleSize[(y*2+1)*mapX + 2*x+1]) / 4;
				float maxValue = max( max(mapDoubleSize[(y*2)*mapX + 2*x], mapDoubleSize[(y*2)*mapX + 2*x+1])
									, max(mapDoubleSize[(y*2+1)*mapX + 2*x], mapDoubleSize[(y*2+1)*mapX + 2*x+1]) );
				tempMap[y*hmapx + x] = maxValue;
			}
		}
		delete [] mapDoubleSize;
	}
	else
	{
		assert(false); // Make shure this never happens.
	}
	
	
	
	AverageHeight = 0;
	for(int x = 0; x < PathMapXSize; x++){
		for(int y = 0; y < PathMapYSize; y++){
			int index = y*PathMapXSize+x;
			HeightMap[index] = *(ai->cb->GetHeightMap() + int(y*resmodifier*resmodifier*PathMapXSize+resmodifier*x));
			if(HeightMap[index] > 0)
				AverageHeight += HeightMap[index];
		}
	}
	AverageHeight /= totalcells;
	
	vector<int> CumulativeSlopeMeterFast;
	
	for(int i = 0; i < totalcells; i++)	{
		float maxslope = 0;
		float tempslope;
		if(i+1 < totalcells && (i + 1) % PathMapXSize){	
			tempslope = abs(HeightMap[i] - HeightMap[i+1]);
				maxslope = max(tempslope,maxslope);
		}
		/*
		if(i - 1 >= 0 && i % PathMapXSize){
			tempslope = abs(HeightMap[i] - HeightMap[i-1]);
			maxslope = max(tempslope,maxslope);
		}
		if(i+PathMapXSize < totalcells){
			tempslope = abs(HeightMap[i] - HeightMap[i+PathMapXSize]);
			maxslope = max(tempslope,maxslope);
		}
		*/
		if(i-PathMapXSize >= 0){
			tempslope = abs(HeightMap[i] - HeightMap[i-PathMapXSize]);
			maxslope = max(tempslope,maxslope);
		}
		
		// Spring is useing this:
		// maxSlope = 1 - cos( unitSlope * 1.5 * PI / 180 )
		
		
		maxslope /= 8; //TODO: hack? - 8 is theoretically the resolution difference between spring map resolution and spring heightmap resolution -firenu
		//SlopeMap[i]= atan(maxslope/resmodifier); 
		unsigned myslope = 0; 
		if(SlopeMap[i] < 0.01){
			//SlopeMap[i] = 0.01;
			myslope = 0.01 * RAD2DEG;
		}
		else
			 myslope = SlopeMap[i] * RAD2DEG; // get the cumulative amount of pixels of each slope
		if(myslope >= CumulativeSlopeMeterFast.size()){
			//CumulativeSlopeMeter.resize(myslope+1,0);
			CumulativeSlopeMeterFast.resize(myslope+1,0);
		}
		CumulativeSlopeMeterFast[myslope]++;
	}
	ai->debug->MakeBWTGA(SlopeMap,PathMapXSize,PathMapYSize,"SlopeMap");
	ai->debug->MakeBWTGA(HeightMap,PathMapXSize,PathMapYSize,"HeightMap");
	for(unsigned i = 0; i < CumulativeSlopeMeterFast.size(); i++){
		for(unsigned s = 0; s < i; s++)
			CumulativeSlopeMeterFast[s] += CumulativeSlopeMeterFast[i];
	}
	//debug for CumulativeSlopeMeter
	//L("CumulativeSlopeMeter");
	CumulativeSlopeMeter.resize(CumulativeSlopeMeterFast.size());
	for(int i = 1; i != CumulativeSlopeMeterFast.size(); i++){
		//L(i << "\t" << double(CumulativeSlopeMeterFast[i]) / totalcells);
		CumulativeSlopeMeter[i] = CumulativeSlopeMeterFast[i];
	}
	
	// The old path system:
	// Get all the different movetypes
	L("Getting Movearrays");
	vector<float> moveslopes;
	vector<int> maxwaterdepths;
	vector<int>	minwaterdepths;
	string sectionstring = "CLASS";
	string errorstring = "-1";
	string Valuestring = "0";
	char k [50];
	//L("Loading sidedata");
	ai->parser->LoadVirtualFile("gamedata\\MOVEINFO.tdf");
	//L("Starting Loop");
	while(Valuestring != errorstring){
		//L("Run Number: " << NumOfMoveTypes);
		sprintf(k,"%i",NumOfMoveTypes);
		ai->parser->GetDef(Valuestring,errorstring,string(sectionstring + k + "\\Name"));
//		L("Movetype: " << Valuestring);
		if(Valuestring != errorstring){
			ai->parser->GetDef(Valuestring,string("10000"),string(sectionstring + k + "\\MaxWaterDepth"));
			maxwaterdepths.push_back(atoi(Valuestring.c_str()));
//			L("Max water depth: " << Valuestring);
			ai->parser->GetDef(Valuestring,string("-10000"),string(sectionstring + k + "\\MinWaterDepth"));
			minwaterdepths.push_back(atoi(Valuestring.c_str()));
//			L("minwaterdepths: " << Valuestring);
			ai->parser->GetDef(Valuestring,string("10000"),string(sectionstring + k + "\\MaxSlope"));
			moveslopes.push_back(atoi(Valuestring.c_str())*DEG2RAD);
//			L("moveslopes: " << Valuestring);
		NumOfMoveTypes++;
		}
	}

	// Clear the canMoveIntMaskArray
	for(int i = 0; i < totalcells; i++)	{
		canMoveIntMaskArray[i] = 0x80000000; // Mark the last bit for air units (temp)
	}
	
	// Add the last, tester movetype
	maxwaterdepths.push_back(20);
	minwaterdepths.push_back(-10000);
	moveslopes.push_back(0.08);
	NumOfMoveTypes++;
	assert(moveslopes.size() == maxwaterdepths.size());
	//MoveArrays.resize(NumOfMoveTypes);
	int m = 0;
	for(m = 0; m < NumOfMoveTypes; m++){
		char k [10];
		itoa (m,k,10);
		//MoveArrays[m] = new bool[totalcells];
		for(int i = 0; i < totalcells; i++)	{
			//MoveArrays[m][i] = false;
			if(SlopeMap[i] > moveslopes[m] || HeightMap[i] <= -maxwaterdepths[m] || HeightMap[i] >= -minwaterdepths[m])
			{
				//MoveArrays[m][i] = false;
				TestMoveArray[i] = false;
				//L("false");

			}
			else{
				//MoveArrays[m][i] = true;
				TestMoveArray[i] = true;
				canMoveIntMaskArray[i] |= 1 << m;
			}
		}	
		itoa (m,k,10);
		ai->debug->MakeBWTGA(TestMoveArray,PathMapXSize,PathMapYSize,string("Movetype bool array number") + string(k));
	}
	

	// The new path system:
	
	/* The indexes for the pather:
	slopeTypes: have index 0 - (slopeTypesSize -1)
	groundUnitWaterLines: have index slopeTypesSize - (slopeTypesSize + groundUnitWaterLinesSize -1)
	waterUnitGroundLines: same pattern
	crushStrengths: same pattern (but unused)
	*/
	int slopeTypesStart = 0;
	int groundUnitWaterLinesStart = slopeTypes.size();
	int waterUnitGroundLinesStart = groundUnitWaterLinesStart + groundUnitWaterLines.size();
	int crushStrengthsStart = waterUnitGroundLinesStart + waterUnitGroundLines.size();
	// Clear out the useless old data:
	unsigned int clearMask = 0xFFFFFFFF;
	for(int i = 0; i < crushStrengthsStart; i++) {
		clearMask ^= 1 << i;
	}
	for(int i = 0; i < totalcells; i++) {
		canMoveIntMaskArray[i] &= clearMask;
	}
	
	m = slopeTypesStart;
	for(list<float>::iterator i = slopeTypes.begin(); i != slopeTypes.end(); i++) {
		char k [10];
		itoa (m,k,10);
		//MoveArrays[m] = new bool[totalcells];
		float moveslope = *i;
		L("num: " << m << ", moveslope: " << moveslope);
		unsigned int bitMask = 1 << m;
		L("bitMask: " << bitMask);
		for(int j = 0; j < totalcells; j++)	{
			//MoveArrays[m][i] = false;
			if(SlopeMap[j] > moveslope)
			{
				//MoveArrays[m][i] = false;
				TestMoveArray[j] = false;
				//L("false");
			}
			else{
				//MoveArrays[m][i] = true;
				TestMoveArray[j] = true;
				//unsigned data = canMoveIntMaskArray[j];
				canMoveIntMaskArray[j] += bitMask;
				//unsigned dataAfter = canMoveIntMaskArray[j];
				//assert(canMoveIntMaskArray[j] & bitMask);
			}
		}	
		itoa (m,k,10);
		ai->debug->MakeBWTGA(TestMoveArray,PathMapXSize,PathMapYSize,string("MoveSlopetype bool array number") + string(k));
		m++;
	}
	
	//m = groundUnitWaterLinesStart;
	for(list<float>::iterator i = groundUnitWaterLines.begin(); i != groundUnitWaterLines.end(); i++) {
		char k [10];
		itoa (m,k,10);
		float depth = *i;
		L("num: " << m << ", groundUnitWaterLine: " << depth);
		unsigned int bitMask = 1 << m;
		L("bitMask: " << bitMask);
		for(int j = 0; j < totalcells; j++)	{
			//MoveArrays[m][i] = false;
			if(HeightMap[j] < -depth)
			{
				//MoveArrays[m][i] = false;
				TestMoveArray[j] = false;
				//L("false");
			}
			else{
				//MoveArrays[m][i] = true;
				TestMoveArray[j] = true;
				//unsigned data = canMoveIntMaskArray[j];
				canMoveIntMaskArray[j] += bitMask; // 
				//unsigned dataAfter = canMoveIntMaskArray[j];
				//L("data: " << (dataAfter - data));
				//assert(canMoveIntMaskArray[j] & bitMask);
			}
		}	
		itoa (m,k,10);
		TestMoveArray[0] = true; //dustefeilfix
		TestMoveArray[1] = false; //dustefeilfix
		ai->debug->MakeBWTGA(TestMoveArray,PathMapXSize,PathMapYSize,string("GroundUnitWaterLine bool array number") + string(k));
		m++;
	}
	
	//m = waterUnitGroundLinesStart;
	for(list<float>::iterator i = waterUnitGroundLines.begin(); i != waterUnitGroundLines.end(); i++) {
		char k [10];
		itoa (m,k,10);
		float depth = *i;
		L("num: " << m << ", waterUnitGroundLine: " << depth);
		unsigned int bitMask = 1 << m;
		L("bitMask: " << bitMask);
		for(int j = 0; j < totalcells; j++)	{
			//MoveArrays[m][i] = false;
			if(HeightMap[j] > -depth)
			{
				//MoveArrays[m][i] = false;
				TestMoveArray[j] = false;
				//L("false");
			}
			else{
				//MoveArrays[m][i] = true;
				TestMoveArray[j] = true;
				//unsigned data = canMoveIntMaskArray[j];
				canMoveIntMaskArray[j] += bitMask;
				//unsigned dataAfter = canMoveIntMaskArray[j];
				//assert(canMoveIntMaskArray[j] & bitMask);
			}
		}	
		itoa (m,k,10);
		TestMoveArray[0] = true; //dustefeilfix
		TestMoveArray[1] = false; //dustefeilfix
		ai->debug->MakeBWTGA(TestMoveArray,PathMapXSize,PathMapYSize,string("WaterUnitGroundLine bool array number") + string(k));
		m++;
	}
	
	for(int i = 0; i < m; i++)	{
		char k [10];
		itoa (i,k,10);
		unsigned int mask = 1 << i;
		//mask |= 1 << 3;
		L("mask: " << mask);
		for(int j = 0; j < totalcells; j++)	{
			bool test = (canMoveIntMaskArray[j] & mask) == mask;
			TestMoveArray[j] = test;
		}
		ai->debug->MakeBWTGA(TestMoveArray,PathMapXSize,PathMapYSize,string("test bool array ") + string(k));
	}
	
	// Make sure that the edges are no-go:
	for(int i = 0; i < PathMapXSize; i++){
		//MoveArrays[m][i] =  false;
		canMoveIntMaskArray[i] = 0;
	}
	for(int i = 0; i < PathMapYSize; i++){
		int k = i*PathMapXSize;
		//MoveArrays[m][k] = false;
		canMoveIntMaskArray[k] = 0;
	}
	for(int i = 0; i < PathMapYSize; i++){
		int k = i*PathMapXSize + PathMapXSize -1;
		//MoveArrays[m][k] = false;
		canMoveIntMaskArray[k] = 0;
	}
	for(int i = 0; i < PathMapXSize; i++){
		int k = PathMapXSize*(PathMapYSize-1) + i;
		//MoveArrays[m][k] = false;
		canMoveIntMaskArray[k] = 0;
	}
	
}
	
void CPathFinder::CreateDefenseMatrix(){
	//L("Starting pathing");
	ai->math->TimerStart();
	const int* enemycomms = ai->sh->GetEnemiesList();
	float3 enemyposes[16];
	ai->dm->ChokeMapsByMovetype.resize(NumOfMoveTypes + 1);
	ai->debug->MakeBWTGA(SlopeMap,PathMapXSize,PathMapYSize,string("SlopeMap"));
	//ai->debug->MakeBWTGA(TestMoveArray,PathMapXSize,PathMapYSize,string("Plane Move Array"));

	int Range = sqrt(float(PathMapXSize*PathMapYSize))/ THREATRES / 3;
	L("Range: " << Range);
	int squarerange = Range*Range;
	int maskwidth = (2*Range+1);
	float* costmask = new float[maskwidth*maskwidth];
	for (int x = 0; x < maskwidth; x++){
		for (int y = 0; y < maskwidth; y++){
			int distance = (x - Range)*(x - Range) + (y - Range)*(y - Range);
			if(distance <= squarerange){
				costmask[y * maskwidth + x] = (distance - squarerange)*(distance - squarerange)/squarerange * 2;
				//L("costmask[y * maskwidth + x]: " << costmask[y * maskwidth + x]);
			}
			else{
				costmask[y * maskwidth + x] = 0;
			}
		}
	}
	int kitty = ai->dm->ChokeMapsByMovetype.size();
	ai->dm->ChokeMapsByMovetype[NumOfMoveTypes] = new float [totalcells];
	for(int i = 0; i < totalcells; i++)	{
		ai->dm->ChokeMapsByMovetype[NumOfMoveTypes][i] = 1;
	}
	//ai->debug->MakeBWTGA(costmask,maskwidth,maskwidth,string("CostMask"));
	for(int m = 0; m < NumOfMoveTypes;m++){	
		int numberofenemyplayers = ai->sh->GetNumberOfEnemies();
		for(int i = 0; i < numberofenemyplayers; i++){
			L("Enemy comm: " << enemycomms[i]);
			enemyposes[i] = ai->cheat->GetUnitPos(enemycomms[i]);
		}
		// There are mods without a starting builder....
		int startUnit = -1;
		if(ai->uh->AllUnitsByCat[CAT_BUILDER]->size())
			startUnit = ai->uh->AllUnitsByCat[CAT_BUILDER]->front();
		else
		{
			//
			for(unsigned i = 0; i < ai->uh->AllUnitsByCat.size(); i++)
			{
				if(ai->uh->AllUnitsByCat[i]->size())
				{
					startUnit = ai->uh->AllUnitsByCat[i]->front();
					break;	
				}
			}
		}
		float3 mypos = ai->cb->GetUnitPos(startUnit);
		int reruns = 30; // 35 TEMP
		ai->dm->ChokeMapsByMovetype[m] = new float [totalcells];
		char k [10];
		itoa (m,k,10);
		
		//micropather->SetMapData(MoveArrays[m], ai->dm->ChokeMapsByMovetype[m], PathMapXSize, PathMapYSize);
		//micropather->SetMapData(canMoveIntMaskArray, ai->dm->ChokeMapsByMovetype[m], PathMapXSize, PathMapYSize, 1 << m);
		// Hack:
		// Take the commanders unit slope type, and the most strict water type:
		const UnitDef* comDef = ai->cb->GetUnitDef(startUnit);
		unsigned int pathToUse = ai->ut->unittypearray[comDef->id].moveSlopeType;
		L("pathToUse: " << pathToUse);
		pathToUse |= 1 << slopeTypes.size();
		L("pathToUse: " << pathToUse);
		micropather->SetMapData(canMoveIntMaskArray, ai->dm->ChokeMapsByMovetype[m], PathMapXSize, PathMapYSize, pathToUse);
		double pathCostSum = 0;
		for(int i = 0; i < totalcells; i++)	{
			ai->dm->ChokeMapsByMovetype[m][i] = 1;
		}

		int runcounter = 0;


		if(numberofenemyplayers > 0 && m == (NumOfMoveTypes -1)){ // HACK
			for(int r = 0; r < reruns; r++){
				//L("reruns: " << r);
				for(int startpos = 0; startpos < numberofenemyplayers; startpos++){
					if(micropather->Solve(Pos2Node_U(&enemyposes[startpos]), Pos2Node_U(&mypos), path, totalcost) == MicroPather::SOLVED){
						for(int i = CHOKEPOINTMAP_IGNORE_CELLS; i < int(path.size()- CHOKEPOINTMAP_IGNORE_CELLS); i++){
							if(i%2){
								int x,y;
								Node2XY_U(path[i],&x,&y);
								/*float3 pos1 = Node2Pos(path[i-1]);
								float3 pos2 =  Node2Pos(path[i]);
								pos1.y = 100 + ai->cb->GetElevation(pos1.x ,pos1.z);
								pos2.y = 100 + ai->cb->GetElevation(pos2.x ,pos2.z);
								L("Line: " << pos1.x << "," << pos1.z << ", pos2: " << pos2.x << "," << pos2.z);
								ai->cb->CreateLineFigure(pos1,pos2,10,1,100000000,457);*/
								int x_start = x - Range;
								if(x_start < 0)
									x_start = 0;
								int x_end = x + Range;
								if(x_end >= PathMapXSize)
									x_end = PathMapXSize -1;
								int y_start = y - Range;
								if(y_start < 0)
									y_start = 0;
								int y_end = y + Range;
								if(y_end >= PathMapYSize)
									y_end = PathMapYSize -1;
								int costmaskIndex = (y_start - (y - Range)) * maskwidth + (x_start - (x - Range));
								int chokeMapsIndex = y_start * PathMapXSize + x_start;
								for (int myy = y_start; myy <= y_end; myy++){
									for (int myx = x_start; myx <= x_end; myx++){
										ai->dm->ChokeMapsByMovetype[m][chokeMapsIndex] += costmask[costmaskIndex];
										costmaskIndex++;
										chokeMapsIndex++;
									}
									chokeMapsIndex = myy * PathMapXSize + x_start;
									costmaskIndex = (myy - (y - Range)) * maskwidth + (x_start - (x - Range));
									//}
								}
								/*
								for (int myx = -Range; myx <= Range; myx++){
									int actualx = x + myx;
									if (actualx >= 0 && actualx < PathMapXSize){
										for (int myy = -Range; myy <= Range; myy++){
											int actualy = y + myy;
											if (actualy >= 0 && actualy < PathMapYSize){
												ai->dm->ChokeMapsByMovetype[m][actualy * PathMapXSize + actualx] += costmask[(myy+Range) * maskwidth + (myx+Range)];
											}
										}
									}
								}*/
							}
							//SlopeMap[y*PathMapXSize+x] += 30;
						}
						runcounter++;
					}
					//L("Enemy Pos " << startpos << " Cost: " << totalcost);
					pathCostSum += totalcost;
					//L("Time Taken: " << clock() - timetaken);
				}			
			}

		//L("pathCostSum:  " << pathCostSum);	
		
		//L("paths calculated, resmodifier: " << resmodifier );

		

		char k [10];
		itoa (m,k,10);
		if( m == (NumOfMoveTypes -1))
			ai->debug->MakeBWTGA(ai->dm->ChokeMapsByMovetype[m],PathMapXSize,PathMapYSize,string("ChokePoint Array for Movetype ") + string(k));
		/*float maxvalue = 0.1;
		for(int i = 0; i < totalcells; i++)	{
			if(chokemap[i] > maxvalue)
				maxvalue = chokemap[i];
		}
		for(int i = 0; i < totalcells; i++)	{
			chokemap[i] /= maxvalue;
		}*/

		//Radius debug
		/*int radius = 2;
		void * startNode = XY2Node(xvector[0],zvector[0]);
		void * senterNode = XY2Node(xvector[1],zvector[1]);
		float3 senterPos = Node2Pos(senterNode);
		float3 distPos = Node2Pos(XY2Node(radius,radius));
		for(int i = 0; i < 20; i++)
		{
			float dx1 = sin(i * 3.1415 / 10) * distPos.x;
			float dy1 = cos(i * 3.1415 / 10) * distPos.z;
			float dx2 = sin((i + 1) * 3.1415 / 10) * distPos.x;
			float dy2 = cos((i + 1) * 3.1415 / 10) * distPos.z;
			float3 pos1 = senterPos + float3(dx1,0,dy1);
			pos1.y = 100 + ai->cb->GetElevation(pos1.x, pos1.z);
			float3 pos2 = senterPos + float3(dx2,0,dy2);
			pos2.y = 100 + ai->cb->GetElevation(pos2.x, pos2.z);
			//L("Line: " << pos1.x << "," << pos1.z << ", pos2: " << pos2.x << "," << pos2.z);
			ai->cb->CreateLineFigure(pos1,pos2,20,0,100000000,456);
		}
		
		micropather->FindBestPathToPointOnRadius(startNode, senterNode, &path, &totalcost, radius);
		L("totalcost: " << totalcost << ", path.size(): " << path.size());
		for(int i = 1; i < path.size(); i++)
		{
			float3 pos1 = Node2Pos(path[i-1]);
			float3 pos2 =  Node2Pos(path[i]);
			pos1.y = 100 + ai->cb->GetElevation(pos1.x ,pos1.z);
			pos2.y = 100 + ai->cb->GetElevation(pos2.x ,pos2.z);
			//L("Line: " << pos1.x << "," << pos1.z << ", pos2: " << pos2.x << "," << pos2.z);
			ai->cb->CreateLineFigure(pos1,pos2,10,1,100000000,457);
		}*/

		}
	}
	delete [] costmask;
	char c[500];
	sprintf(c,"Time Taken to create chokepoints: %f",ai->math->TimerSecs());
	ai->cb->SendTextMsg(c,0);
	L(c);

}

void CPathFinder::ClearPath()
{
	path.resize( 0 );
}

unsigned CPathFinder::Checksum()
{
	return micropather->Checksum();
}

unsigned CPathFinder::XY2Node_U(int x, int y)
{
	return (unsigned) (y*PathMapXSize+x);
}

void CPathFinder::Node2XY_U(unsigned node, int* x, int* y)
{
	*y = node / PathMapXSize;
	*x = node - *y * PathMapXSize;
}

float3 CPathFinder::Node2Pos_U(unsigned node)
{
	float3 pos;
	int multiplier = 8 * resmodifier;
	pos.z = (node / PathMapXSize) * multiplier; /////////////////////OPTiMIZE
	pos.x = (node - ((node / PathMapXSize) * PathMapXSize)) * multiplier;
	return pos;

}

unsigned CPathFinder::Pos2Node_U(float3* pos)
{
	//L("pos.x = " << pos.x << " pos.z= " << pos.z << " node: " << (pos.z/(8*THREATRES)) * PathMapXSize +  (pos.x / (8*THREATRES)));
	return (unsigned)(int(pos->z/8/THREATRES) * PathMapXSize + int((pos->x / 8/THREATRES)));
}



float CPathFinder::FindBestPath(vector<float3> *posPath, float3 *startPos, float myMaxRange, vector<float3> *possibleTargets, float cutoff)
{
	//L("FindBestPath Started");
	//L("startPos: x: " << startPos->x << ", z: " << startPos->z);
	assert(myMaxRange >= 0);
	ai->math->StartTimer(patherTime);
	float totalcost;
	//ClearPath();
	// Make a list with the points that will count as end nodes.
	static vector<unsigned> endNodes;
	int radius = myMaxRange / (8 *resmodifier);
	int offsetSize = 0;
	endNodes.resize(0);
	endNodes.reserve(possibleTargets->size() * (radius) * 10);
	//L("possibleTargets->size(): " << possibleTargets->size());
	pair<int, int> * offsets;
	if(radius > 0)
	{
		
		//L("radius: " << radius);
		int DoubleRadius = radius * 2;
		int SquareRadius = radius * radius; //used to speed up loops so no recalculation needed
		int * xend = new int[DoubleRadius+1]; 
		//L("1");
		for (int a=0;a<DoubleRadius+1;a++){ 
			float z=a-radius;
			float floatsqrradius = SquareRadius;
			xend[a]=sqrt(floatsqrradius-z*z);
		}
		//L("2");
		offsets = new pair<int, int>[DoubleRadius*5];
		int index = 0;
		int startPos = 0;
		//L("3");
		offsets[index].first = 0;
		//L("offsets[index].first: " << offsets[index].first);
		offsets[index].second = 0;
		//L("offsets[index].second: " << offsets[index].second);
		index++;
		
		for (int a=1;a<radius+1;a++){ 
			//L("a: " << a);
			int endPos = xend[a];
			int startPos = xend[a-1];
			//L("endPos: " << endPos);
			while(startPos <= endPos)
			{
				//L("startPos: " << startPos);
				offsets[index].first = startPos;
				//L("offsets[index].first: " << offsets[index].first);
				offsets[index].second = a;
				//L("offsets[index].second: " << offsets[index].second);
				startPos++;
				index++;
			}
			startPos--;
		}
		int index2 = index;
		for (int a=0;a<index2-2;a++)
		{
			offsets[index].first = offsets[a].first;
			//L("offsets[index].first: " << offsets[index].first);
			offsets[index].second = DoubleRadius - ( offsets[a].second);
			//L("offsets[index].second: " << offsets[index].second);
			index++;
		}
		index2 = index;
		//L("3.7");
		for (int a=0;a<index2;a++)
		{
			offsets[index].first =  - ( offsets[a].first);
			//L("offsets[index].first: " << offsets[index].first);
			offsets[index].second = offsets[a].second;
			//L("offsets[index].second: " << offsets[index].second);
			index++;
		}
		for (int a=0;a<index;a++)
		{
			offsets[a].first = offsets[a].first;// + radius;
			//L("offsets[index].first: " << offsets[a].first);
			offsets[a].second = offsets[a].second - radius;
			//L("offsets[index].second: " << offsets[a].second);

		}
		offsetSize = index;
		delete [] xend;
	}
	else
	{
		// Radius is 0 (slow fix)
		endNodes.reserve(possibleTargets->size());
		offsets = new pair<int, int>[1];
		offsets[0].first = 0;
		offsets[0].second = 0;
		offsetSize = 1;
	}

	for(unsigned i = 0; i < possibleTargets->size(); i++)
	{
		float3 f = (*possibleTargets)[i];
		int x, y;
		//L("Added: x: " << f.x << ", z: " << f.z);
		// TODO: Make the circle here:
		
		ai->math->F3MapBound(&f);
		unsigned node = Pos2Node_U(&f);
		Node2XY_U(node, &x, &y);
		for(int j = 0; j < offsetSize; j++)
		{
			int sx = x + offsets[j].first; 
			int sy = y + offsets[j].second;
			if(sx >= 0 && sx < PathMapXSize && sy >= 0 && sy < PathMapYSize)
				endNodes.push_back(XY2Node_U(sx, sy));
		}
		//L("node: " << ((int) node) << ", x: " << x << ", y: " << y);
		//endNodes.push_back(node);
	}
	ai->math->F3MapBound(startPos);
	//L("endNodes.size(): " << endNodes.size());
	delete [] offsets;
	
	ai->math->StartTimer(microPatherTime);
	if(micropather->FindBestPathToAnyGivenPoint( Pos2Node_U(startPos), endNodes, path, totalcost, cutoff) == MicroPather::SOLVED)
	{
		ai->math->StopTimer(microPatherTime);
		// Solved
		//L("attack solution solved! Path size = " << path.size());
        posPath->reserve(path.size());
        posPath->clear();
        //posPath->resize(path.size());
		for(unsigned i = 0; i < path.size(); i++) {
			//L("adding path point");
			int x, y;
			Node2XY_U(path[i], &x, &y);
			//L("node: " << ((int) path[i]) << ". x: " << x << ", y: " << y);
			float3 mypos = Node2Pos_U(path[i]);
			//L("mypos: x: " << mypos.x << ", z: " << mypos.z);
			mypos.y = ai->cb->GetElevation(mypos.x, mypos.z);
			posPath->push_back(mypos);
		}
		//char c[500];
		//sprintf(c,"Time Taken: %f Total Cost %i",ai->math->TimerSecs(), int(totalcost));
		//L(c);
	}
	else
	{
		ai->math->StopTimer(microPatherTime);
		L("FindBestPath: path failed!");
	}
		
	ai->math->StopTimer(patherTime);
	return totalcost;
}


//alias hack to above function for one target. (added for convenience in other parts of the code
float CPathFinder::FindBestPathToRadius(vector<float3> *posPath, float3 *startPos, float radiusAroundTarget, float3 *target, float cutoff) {
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> foo;
	foo.push_back(*target);
	ai->math->StopTimer(patherFunctionSimulationSetup);
	return this->FindBestPath(posPath, startPos, radiusAroundTarget, &foo, cutoff);
}


/*
	Edited the function names to have consistent parameters and support the things they are used for directly.
	Please modify to make them more efficient if desired.
	-firenu
*/


float CPathFinder::PathToPos(vector<float3>* pathToPos, float3 startPos, float3 target)
{
	//TODO: placeholder
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> hack;
	hack.push_back(target);
	ai->math->StopTimer(patherFunctionSimulationSetup);
	return this->FindBestPath(pathToPos, &startPos, 0, &hack);
}
float CPathFinder::PathToPosRadius(vector<float3>* pathToPos, float3 startPos, float3 target, float radius)
{
	//TODO: placeholder
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> hack;
	hack.push_back(target);
	ai->math->StopTimer(patherFunctionSimulationSetup);
	return this->FindBestPath(pathToPos, &startPos, radius, &hack);
}
float CPathFinder::PathToSet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets)
{
	//TODO: placeholder
	return this->FindBestPath(pathToPos, &startPos, 0, possibleTargets);
}
float CPathFinder::PathToSet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float threatCutoff)
{
	//TODO: placeholder // OBS might not be used? ask firenu :D
	return this->FindBestPath(pathToPos, &startPos, 0, possibleTargets, threatCutoff);
}
float CPathFinder::PathToSetRadius(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float radius)
{
	//TODO: placeholder
	return this->FindBestPath(pathToPos, &startPos, radius, possibleTargets);
}
float CPathFinder::PathToSetRadius(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float radius, float threatCutoff)
{
	//TODO: placeholder
	return this->FindBestPath(pathToPos, &startPos, radius, possibleTargets, threatCutoff);
}

bool CPathFinder::PathExists(float3 startPos, float3 target)
{
	//TODO: placeholder
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> pathToPos;
	vector<float3> hack;
	hack.push_back(target);
	ai->math->StopTimer(patherFunctionSimulationSetup);
	float cost = this->FindBestPath(&pathToPos, &startPos, 0, &hack);
	return cost > 0 || pathToPos.size() > 2;
}
bool CPathFinder::PathExistsToAny(float3 startPos, vector<float3> targets)
{
	//TODO: placeholder
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> pathToPos;
	ai->math->StopTimer(patherFunctionSimulationSetup);
//	vector<float3> hack;
//	hack.push_back(target);
	float cost = this->FindBestPath(&pathToPos, &startPos, 0, &targets);
	return cost > 0 || pathToPos.size() > 2;
}
float CPathFinder::ManeuverToPos(float3* destination, float3 startPos, float3 target)
{
	//TODO: placeholder (used for finding enemy location rather than for maneuvering, but w/e)
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> pathToPos;
	ai->math->StopTimer(patherFunctionSimulationSetup);
	float cost = this->FindBestPathToRadius(&pathToPos, &startPos, 0, &target);
	if(pathToPos.size() > 0) {
		*destination = pathToPos.back();
		return cost;
	} else {
		return 0;
	}
}
float CPathFinder::ManeuverToPosRadius(float3* destination, float3 startPos, float3 target, float radius)
{
	//TODO: placeholder
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> pathToPos;
	ai->math->StopTimer(patherFunctionSimulationSetup);
	float cost = this->FindBestPathToRadius(&pathToPos, &startPos, radius, &target);
	if(pathToPos.size() > 0) {
		*destination = pathToPos.back();
		return cost;
	} else {
		return 0;
	}
}
float CPathFinder::ManeuverToPosRadiusAndCanFire(float3* destination, float3 startPos, float3 target, float radius)
{
	//TODO: placeholder. not used yet, no arty definition
	return ManeuverToPosRadius(destination, startPos, target, radius);
}

//fix these last:
float CPathFinder::PathToPrioritySet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets)
{
	//TODO: placeholder
	//try to path to nr 1 in possibleTargets. if impossible or exceeds threat at a given point, try the next in the list.
	//obs, its supposed to try ->front() first
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> hack;
	hack.push_back(possibleTargets->back());
	ai->math->StopTimer(patherFunctionSimulationSetup);
	
	//int FindBestPathToPrioritySet( unsigned startNode, vector<unsigned>& endNodes, vector< unsigned >& path, unsigned& priorityIndexFound, float& cost);
	return this->FindBestPath(pathToPos, &startPos, 0, &hack);
}
float CPathFinder::PathToPrioritySet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float threatCutoff)
{
	//TODO: placeholder
	//try to path to nr 1 in possibleTargets. if impossible or exceeds threat at a given point, try the next in the list.
	ai->math->StartTimer(patherFunctionSimulationSetup);
	vector<float3> hack;
	hack.push_back(possibleTargets->back());
	ai->math->StopTimer(patherFunctionSimulationSetup);
	return this->FindBestPath(pathToPos, &startPos, 0, &hack, threatCutoff);
}
