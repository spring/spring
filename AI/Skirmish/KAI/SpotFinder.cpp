#include "SpotFinder.h"

//using namespace std;

CSpotFinder::CSpotFinder(AIClasses* ai, int height, int width)
{
	this->ai=ai;
	//MinMetalForSpot = 50;	// from 0-255, the minimum percentage of metal a spot needs to have
							//from the maximum to be saved. Prevents crappier spots in between taken spaces.
							//They are still perfectly valid and will generate metal mind you!

	MapHeight = height; //metal map has 1/2 resolution of normal map
	MapWidth = width;
	TotalCells = MapHeight * MapWidth;
	int cacheHeight = (height + 1) / CACHEFACTOR;
	int cacheWidtht = (width + 1) / CACHEFACTOR;
	int totalCacheCells = cacheHeight * cacheWidtht;
	cachePoints = new CachePoint[totalCacheCells];
	
	for(int i = 0; i < totalCacheCells; i++)
	{
		cachePoints[i].isValid = false;
		cachePoints[i].isMasked = false;
	}
	
	//MexArrayB = new unsigned char [TotalCells];
	//MexArrayC = new unsigned char [TotalCells]; //used for drawing the TGA, not really needed with a couple of changes
	TempAverage = new float [TotalCells];
	xend = new int[height + width];
	//Stopme = false;
	haveTheBestSpotReady = false;
	isValid = false;
	radius = 0;
	L("SpotFinder class");
}

CSpotFinder::~CSpotFinder()
{
	//delete [] MexArrayB;
	//delete [] MexArrayC;
	delete [] TempAverage;
	delete [] cachePoints;
	delete [] xend;
}

/*
Sets a float array as the backing array

TODO: alow this array to be a 2^x size factor bigger than
*/
void CSpotFinder::SetBackingArray(float* map, int height, int width)
{
	assert(height == MapHeight);
	assert(width == MapWidth);
	MexArrayA = map;
	BackingArrayChanged();
}


/*
Returns an updated SumMap.
Remember to use BackingArrayChanged(), SetRadius() and UpdateSumMap()
to get correct data.

*/
float* CSpotFinder::GetSumMap()
{
	if(!isValid)
	{
		MakeSumMap();
		MakeCachePoints();
	}
	else
		UpdateSumMap();
	
	// Super temp:
	
	
	
	return TempAverage;
}



/*
Set the working radius for making the SumMap.
This will trigger a full remake of the SumMap only if the radius changes.
Radius have the same resolution as the map given to SpotFinder.

*/
void CSpotFinder::SetRadius(int radius)
{
	if(this->radius != radius)
	{
		this->radius = radius;
		haveTheBestSpotReady = false;
		isValid = false;
		
		int DoubleRadius = radius * 2;
		int SquareRadius = radius * radius; //used to speed up loops so no recalculation needed
		
		if(MapHeight + MapWidth < DoubleRadius + 1)
		{
			delete [] xend;
			xend = new int[DoubleRadius+1]; 
		}
		
		// TODO: make the edges smoth:
		for (int a=0;a<DoubleRadius+1;a++){ 
			float z=a-radius;
			float floatsqrradius = SquareRadius;
			xend[a]=int(sqrtf(floatsqrradius-z*z));
		}
		
	}
}


/*
This is a temp/test only
*/
void CSpotFinder::MakeCachePoints() {
	for (int y = 0; y < MapHeight / CACHEFACTOR; y++){
		for (int x = 0; x < MapWidth / CACHEFACTOR; x++){
			int cacheIndex = y * MapWidth/CACHEFACTOR + x;
			cachePoints[cacheIndex].maxValueInBox = -FLT_MAX;
			cachePoints[cacheIndex].isValid = true;
			cachePoints[cacheIndex].x = x * CACHEFACTOR;
			cachePoints[cacheIndex].y = y * CACHEFACTOR;
		}
	}
	
	for (int y = 0; y < MapHeight; y++){
		// Find the cache line:
		int cacheY = y / CACHEFACTOR;
		
		for (int x = 0; x < MapWidth; x++){
			int cacheX = x / CACHEFACTOR;
			float sum = TempAverage[y * MapWidth + x];
			
			int cacheIndex = cacheY * MapWidth/CACHEFACTOR + cacheX;
			//L("cacheIndex: " << cacheIndex << ", maxValueInBox: " << cachePoints[cacheIndex].maxValueInBox << ", sum: " << sum << ", x: " << x << ", y: " << y);
			
			if(cachePoints[cacheIndex].maxValueInBox < sum)
			{
				// Found a better spot for this cache element
				cachePoints[cacheIndex].maxValueInBox = sum;
				cachePoints[cacheIndex].x = x;
				cachePoints[cacheIndex].y = y;
				//L("cacheIndex: " << cacheIndex << ", sum: " << sum << ", x: " << x << ", y: " << y);
			}
		}
	}
}

CachePoint * CSpotFinder::GetBestCachePoint(int x, int y)
{
	int cacheY = y;// / CACHEFACTOR;
	int cacheX = x;// / CACHEFACTOR;
	int cacheIndex = cacheY * MapWidth/CACHEFACTOR + cacheX;
	//L("GetBestCachePoint: cacheIndex: " << cacheIndex);
	if(!cachePoints[cacheIndex].isValid)
	{
		// temp:
		MakeCachePoints();
	}
	
	return &cachePoints[cacheIndex];
}



/*
Internal function:
Makes the array with the sumes of all points inside the radius of that point.

*/
float* CSpotFinder::MakeSumMap()
{
	L("Starting MakeSumMap");
	isValid = true;
	int XtractorRadius = radius;

	//int SquareRadius = XtractorRadius * XtractorRadius; //used to speed up loops so no recalculation needed
	//int DoubleSquareRadius = DoubleRadius * DoubleRadius; // same as above
	
	// Time stuff:
	//ai->math->TimerStart();
	
	double MaxValue = 0;
	// Now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y < MapHeight; y++){
		for (int x = 0; x < MapWidth; x++){
			//double TotalMetal = 0;
			float TotalMetal = 0;
			if(x == 0 && y == 0) // First Spot needs full calculation
			for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
				if (sy >= 0 && sy < MapHeight){
					for (int sx=x-xend[a];sx<=x+xend[a];sx++){ 
						if (sx >= 0 && sx < MapWidth){
							TotalMetal += MexArrayA[sy * MapWidth + sx]; //get the metal from all pixels around the extractor radius  
						}
					}
				}
			}
			// Quick calc test:		
			if(x > 0)
			{
				TotalMetal = TempAverage[y * MapWidth + x -1];
				for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
					if (sy >= 0 && sy < MapHeight){
						int addX = x+xend[a];
						int remX = x-xend[a] -1;
						if(addX < MapWidth)
							TotalMetal += MexArrayA[sy * MapWidth + addX];
						if(remX >= 0)
							TotalMetal -= MexArrayA[sy * MapWidth + remX];
					}
				}
			} else if(y > 0){
				// x == 0 here
				TotalMetal = TempAverage[(y-1) * MapWidth];
				// Remove the top half:
				int a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++){
					if (sx < MapWidth){
						int remY = y-xend[a] -1;
						if(remY >= 0)
							TotalMetal -= MexArrayA[remY * MapWidth + sx];
					}
				}
				// Add the bottom half:
				a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++){
					if (sx < MapWidth){
						int addY = y+xend[a];
						if(addY < MapHeight)
							TotalMetal += MexArrayA[addY * MapWidth + sx];
					}
				}

				TotalMetal = TotalMetal; // Comment out for debug
			}
			TempAverage[y * MapWidth + x] = TotalMetal; //set that spots metal making ability (divide by cells to values are small)
			if (MaxValue < TotalMetal)
			{
				MaxValue = TotalMetal;  //find the spot with the highest metal to set as the map's max
				// This is also the best spot on the map, remember it ?
				haveTheBestSpotReady = true;
				bestSpotX = x;
				bestSpotY = y;
			}
		}
	}
	L("MakeSumMap done");
	// Done
	return TempAverage;
}


/*
Invalidates the given spot and all around it.
Radius have the same resolution as the map given to SpotFinder.
Dont affect the current radius of SumMap.
The update is delayed until needed, so that the cost of calling this function is low.

*/
void CSpotFinder::InvalidateSumMap(int coordx, int coordy, int clearRadius)
{
	if(!isValid)
		return;
	
	int clearRealRadius = clearRadius + radius +1;
	
	int cacheY_Start = (coordy - clearRealRadius) / CACHEFACTOR;
	if(cacheY_Start < 0)
		cacheY_Start = 0;
	int cacheX_Start = (coordx - clearRealRadius) / CACHEFACTOR;
	if(cacheX_Start < 0)
		cacheX_Start = 0;
	
	int cacheY_End = (coordy + clearRealRadius) / CACHEFACTOR +1;
	if(cacheY_End >= MapHeight /CACHEFACTOR )
		cacheY_End = MapHeight /CACHEFACTOR -1;
	
	int cacheX_End = (coordx + clearRealRadius) / CACHEFACTOR +1;
	if(cacheX_End >= MapWidth /CACHEFACTOR )
		cacheX_End = MapWidth /CACHEFACTOR -1;	
		
	int CacheMapWidth = MapWidth/CACHEFACTOR;
	// TODO: make this use the real radius, and not a box
	for (int y = cacheY_Start; y <= cacheY_End; y++)
	{
		for (int x = cacheX_Start; x <= cacheX_End; x++)
		{
			cachePoints[y * CacheMapWidth + x].isValid = false;
		}
	}
}


/*
Internal function for updating/creating both the cachePoint and SumMap at
all invalid cachePoints.

*/
void CSpotFinder::UpdateSumMap()
{
	int cacheMapHeight = MapHeight / CACHEFACTOR;
	int cacheMapWidth = MapWidth / CACHEFACTOR;
	for (int y = 0; y < cacheMapHeight; y++){
		for (int x = 0; x < cacheMapWidth; x++){
			int cacheIndex = y * cacheMapWidth + x;
			if(!cachePoints[cacheIndex].isValid)
				UpdateSumMapArea(x, y);
		}
	}
}


/*
Internal function for updating/creating both the cachePoint and SumMap at
the given cache point.

Note: Currently needs that the SumMap is correct in the one to the left / up.
(not needed if that point is (0,0))
*/
void CSpotFinder::UpdateSumMapArea(int cacheX, int cacheY)
{
	// Since this point isnt valid, we must use the one to the left / up.
	// That point IS valid, or (0,0).
	
	int Y_Start = cacheY * CACHEFACTOR;
	int X_Start = cacheX * CACHEFACTOR;
	if(X_Start == 0)
		Y_Start -= 1;
	else
		X_Start -= 1;
	if(Y_Start < 0)
		Y_Start = 0;
	
	int Y_End = cacheY * CACHEFACTOR + CACHEFACTOR;
	if(Y_End >= MapHeight )
		Y_End = MapHeight -1;
	
	int X_End = cacheX * CACHEFACTOR + CACHEFACTOR;
	if(X_End >= MapWidth )
		X_End = MapWidth -1;	
		
	//int CacheMapWidth = MapWidth/CACHEFACTOR;
	// cachePoints[y * CacheMapWidth + x].isValid = false;
	float currentBest = FLT_MIN;
	int currentBestX = 0;
	int currentBestY = 0;
	int XtractorRadius = radius;
	for (int y = Y_Start; y <= Y_End; y++)
	{
		for (int x = X_Start; x <= X_End; x++)
		{
			float TotalMetal = 0;
			if(x == 0 && y == 0) // Comment out for debug
				for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
					if (sy >= 0 && sy < MapHeight){
						for (int sx=x-xend[a];sx<=x+xend[a];sx++){ 
							if (sx >= 0 && sx < MapWidth){
								TotalMetal += MexArrayA[sy * MapWidth + sx]; //get the metal from all pixels around the extractor radius  
							}
						} 
					}
				}

			// Quick calc test:
			if(x > 0){
				TotalMetal = TempAverage[y * MapWidth + x -1];
				for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
					if (sy >= 0 && sy < MapHeight){
						int addX = x+xend[a];
						int remX = x-xend[a] -1;
						if(addX < MapWidth)
							TotalMetal += MexArrayA[sy * MapWidth + addX];
						if(remX >= 0)
							TotalMetal -= MexArrayA[sy * MapWidth + remX];
					}
				}
			} 
			else if(y > 0){
				// x == 0 here
				TotalMetal = TempAverage[(y-1) * MapWidth];
				// Remove the top half:
				int a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++){
					if (sx < MapWidth){
						int remY = y-xend[a] -1;
						if(remY >= 0)
							TotalMetal -= MexArrayA[remY * MapWidth + sx];
					}
				}
				// Add the bottom half:
				a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++){
					if (sx < MapWidth){
						int addY = y+xend[a];
						if(addY < MapHeight)
							TotalMetal += MexArrayA[addY * MapWidth + sx];
					}
				}
			}
			TempAverage[y * MapWidth + x] = TotalMetal;
			if(currentBest < TotalMetal)
			{
				// Found a better spot for this cache element
				currentBest = TotalMetal;
				currentBestX = x;
				currentBestY = y;
				//L("cacheIndex: " << cacheIndex << ", sum: " << sum << ", x: " << x << ", y: " << y);
			}
			// end
		}
	}
	int cacheIndex = cacheY * MapWidth/CACHEFACTOR + cacheX;
	cachePoints[cacheIndex].maxValueInBox = currentBest;
	cachePoints[cacheIndex].x = currentBestX;
	cachePoints[cacheIndex].y = currentBestY;
	cachePoints[cacheIndex].isValid = true;
}


/*
Updates the given spot, based on that the values on, and around, that point have changed.
Radius have the same resolution as the map given to SpotFinder.
Dont affect the current radius of SumMap.
This update is not delayed (so dont call more than needed), unless the SumMap is already unvalid.

*/
void CSpotFinder::UpdateSumMap(int coordx, int coordy, int clearRadius)
{
	if(!isValid)
		return;
	haveTheBestSpotReady = false; // TODO: this is not needed all the time
	
	int XtractorRadius = radius;
	//int SquareRadius = XtractorRadius * XtractorRadius; //used to speed up loops so no recalculation needed
	//int DoubleSquareRadius = DoubleRadius * DoubleRadius; // same as above
	
	// Time stuff:
	
	int clearRealRadius = clearRadius + radius +1;
	// Redo the whole averaging process around the picked spot so other spots can be found around it
	// TODO: fix it so that it dont work as a box update
	for (int y = coordy - clearRealRadius; y <= coordy + clearRealRadius; y++)
	{
		if(y >=0 && y < MapHeight)
		{
			for (int x = coordx - clearRealRadius; x <= coordx + clearRealRadius; x++)
			{						
				if(x >=0 && x < MapWidth)
				{
					//double TotalMetal = 0;
					float TotalMetal = 0;
					if(x == 0 && y == 0) // Comment out for debug
						for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
							if (sy >= 0 && sy < MapHeight){
								for (int sx=x-xend[a];sx<=x+xend[a];sx++){ 
									if (sx >= 0 && sx < MapWidth){
										TotalMetal += MexArrayA[sy * MapWidth + sx]; //get the metal from all pixels around the extractor radius  
									}
								} 
							}
						}

					// Quick calc test:
					if(x > 0){
						TotalMetal = TempAverage[y * MapWidth + x -1];
						for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
							if (sy >= 0 && sy < MapHeight){
								int addX = x+xend[a];
								int remX = x-xend[a] -1;
								if(addX < MapWidth)
									TotalMetal += MexArrayA[sy * MapWidth + addX];
								if(remX >= 0)
									TotalMetal -= MexArrayA[sy * MapWidth + remX];
							}
						}
					} 
					else if(y > 0){
						// x == 0 here
						TotalMetal = TempAverage[(y-1) * MapWidth];
						// Remove the top half:
						int a = XtractorRadius;
						for (int sx=0;sx<=XtractorRadius;sx++,a++){
							if (sx < MapWidth){
								int remY = y-xend[a] -1;
								if(remY >= 0)
									TotalMetal -= MexArrayA[remY * MapWidth + sx];
							}
						}
						// Add the bottom half:
						a = XtractorRadius;
						for (int sx=0;sx<=XtractorRadius;sx++,a++){
							if (sx < MapWidth){
								int addY = y+xend[a];
								if(addY < MapHeight)
									TotalMetal += MexArrayA[addY * MapWidth + sx];
							}
						}
					}
					TempAverage[y * MapWidth + x] = TotalMetal;
					//MexArrayB[y * MapWidth + x] = TotalMetal * 255 / MaxMetal; //set that spots metal amount 
					// end
				}
			}
		}
	}


}

/*
void CSpotFinder::GetMetalPoints()
{		
	// Time stuff:
	ai->math->TimerStart();

	int* xend = new int[DoubleRadius+1]; 
	for (int a=0;a<DoubleRadius+1;a++){ 
		float z=a-XtractorRadius;
		float floatsqrradius = SquareRadius;
		xend[a]=sqrt(floatsqrradius-z*z);
	}
	
	// Now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y < MapHeight; y++){
		for (int x = 0; x < MapWidth; x++){
			TotalMetal = 0;			
			if(x == 0 && y == 0) // First Spot needs full calculation
			for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
				if (sy >= 0 && sy < MapHeight){
					for (int sx=x-xend[a];sx<=x+xend[a];sx++){ 
						if (sx >= 0 && sx < MapWidth){
							TotalMetal += MexArrayA[sy * MapWidth + sx]; //get the metal from all pixels around the extractor radius  
						}
					} 
				}
			}
			// Quick calc test:		
			if(x > 0)
			{
				TotalMetal = TempAverage[y * MapWidth + x -1];
				for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
					if (sy >= 0 && sy < MapHeight){
						int addX = x+xend[a];
						int remX = x-xend[a] -1;
						if(addX < MapWidth)
							TotalMetal += MexArrayA[sy * MapWidth + addX];
						if(remX >= 0)
							TotalMetal -= MexArrayA[sy * MapWidth + remX];
					}
				}
			} else if(y > 0){
				// x == 0 here
				TotalMetal = TempAverage[(y-1) * MapWidth];
				// Remove the top half:
				int a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++){
					if (sx < MapWidth){
						int remY = y-xend[a] -1;
						if(remY >= 0)
							TotalMetal -= MexArrayA[remY * MapWidth + sx];
					}
				}
				// Add the bottom half:
				a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++){
					if (sx < MapWidth){
						int addY = y+xend[a];
						if(addY < MapHeight)
							TotalMetal += MexArrayA[addY * MapWidth + sx];
					}
				}

				TotalMetal = TotalMetal; // Comment out for debug
			}
			TempAverage[y * MapWidth + x] = TotalMetal; //set that spots metal making ability (divide by cells to values are small)
			if (MaxMetal < TotalMetal)
				MaxMetal = TotalMetal;  //find the spot with the highest metal to set as the map's max
		}
	}
	// Make a list for the distribution of values
	int * valueDist = new int[256];
	for (int i = 0; i < 256; i++){ // Clear the array (useless?)
		valueDist[i] = 0;
	}

	for (int i = 0; i < TotalCells; i++){ // this will get the total metal a mex placed at each spot would make
		MexArrayB[i] = TempAverage[i] * 255 / MaxMetal;  //scale the metal so any map will have values 0-255, no matter how much metal it has
		MexArrayC[i] = 0; // clear out the array since its never been used.
		int value = MexArrayB[i];
		valueDist[value]++;
	}
	
	// Find the current best value
	int bestValue = 0;
	int numberOfValues = 0;
	int usedSpots = 0;
	for (int i = 255; i >= 0; i--){
		if(valueDist[i] != 0){
			bestValue = i;
			numberOfValues = valueDist[i];
			break;
		}
	}

	// Make a list of the indexes of the best spots
	if(numberOfValues > 256) // Make sure that the list wont be too big
		numberOfValues = 256;
	int *bestSpotList = new int[numberOfValues];	
	for (int i = 0; i < TotalCells; i++){			
		if (MexArrayB[i] == bestValue){
			// Add the index of this spot to the list.
			bestSpotList[usedSpots] = i;
			usedSpots++;
			if(usedSpots == numberOfValues){
				// The list is filled, stop the loop.
				usedSpots = 0;
				break;
			}
		}
	}

	int printDebug1 = 100;
	for (int a = 0; a < MaxSpots; a++){	
		if(!Stopme){
			TempMetal = 0; //reset tempmetal so it can find new spots
			// Take the first spot
			int speedTempMetal_x;
			int speedTempMetal_y;
			int speedTempMetal = 0;
			bool found = false;
			while(!found){				
				if(usedSpots == numberOfValues){
					// The list is empty now, refill it:
					// Make a list of all the best spots:
					for (int i = 0; i < 256; i++){ // Clear the array
						valueDist[i] = 0;
					}
					// Find the metal distribution
					for (int i = 0; i < TotalCells; i++){
						int value = MexArrayB[i];
						valueDist[value]++;
					}
					// Find the current best value
					bestValue = 0;
					numberOfValues = 0;
					usedSpots = 0;
					for (int i = 255; i >= 0; i--){
						if(valueDist[i] != 0){
							bestValue = i;
							numberOfValues = valueDist[i];
							break;
						}
					}
					// Make a list of the indexes of the best spots
					if(numberOfValues > 256) // Make sure that the list wont be too big
						numberOfValues = 256;
					delete[] bestSpotList;
					bestSpotList = new int[numberOfValues];
					
					for (int i = 0; i < TotalCells; i++){			
						if (MexArrayB[i] == bestValue){
							// Add the index of this spot to the list.
							bestSpotList[usedSpots] = i;
							usedSpots++;
							if(usedSpots == numberOfValues){
								// The list is filled, stop the loop.
								usedSpots = 0;
								break;
							}
						}
					}
				}
				// The list is not empty now.
				int spotIndex = bestSpotList[usedSpots];
				if(MexArrayB[spotIndex] == bestValue){
					// The spot is still valid, so use it
					speedTempMetal_x = spotIndex%MapWidth;
					speedTempMetal_y = spotIndex/MapWidth;
					speedTempMetal = bestValue;
					found = true;
				}
				// Update the bestSpotList index
				usedSpots++;
			}			
			coordx = speedTempMetal_x;
			coordy = speedTempMetal_y;
			TempMetal = speedTempMetal;
		}
		if (TempMetal < MinMetalForSpot)
			Stopme = 1; // if the spots get too crappy it will stop running the loops to speed it all up

		if (!Stopme){

			BufferSpot.x=coordx * 16 + 8; // format metal coords to game-coords
			BufferSpot.z=coordy * 16 + 8;
			BufferSpot.y=TempMetal *ai->cb->GetMaxMetal() * MaxMetal / 255; //Gets the actual amount of metal an extractor can make
			VectoredSpots.push_back(BufferSpot);
			MexArrayC[coordy * MapWidth + coordx] = TempMetal; //plot TGA array (not necessary) for debug
			NumSpotsFound += 1;
			
			// Small speedup of "wipes the metal around the spot so its not counted twice":
			for (int sy=coordy-XtractorRadius,a=0;sy<=coordy+XtractorRadius;sy++,a++){
				if (sy >= 0 && sy < MapHeight){
					int clearXStart = coordx-xend[a];
					int clearXEnd = coordx+xend[a];
					if(clearXStart < 0)
						clearXStart = 0;
					if(clearXEnd >= MapWidth)
						clearXEnd = MapWidth -1;
					for(int xClear = clearXStart; xClear <= clearXEnd; xClear++){
						MexArrayA[sy * MapWidth + xClear] = 0; //wipes the metal around the spot so its not counted twice
						MexArrayB[sy * MapWidth + xClear] = 0;
						TempAverage[sy * MapWidth + xClear] = 0;						
					}
				}
			}

			// Redo the whole averaging process around the picked spot so other spots can be found around it
			for (int y = coordy - DoubleRadius; y <= coordy + DoubleRadius; y++)
			{
				if(y >=0 && y < MapHeight)
				{
					for (int x = coordx - DoubleRadius; x <= coordx + DoubleRadius; x++)
					{						
						if(x >=0 && x < MapWidth)
						{
							TotalMetal = 0;
							if(x == 0 && y == 0) // Comment out for debug
								for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
									if (sy >= 0 && sy < MapHeight){
										for (int sx=x-xend[a];sx<=x+xend[a];sx++){ 
											if (sx >= 0 && sx < MapWidth){
												TotalMetal += MexArrayA[sy * MapWidth + sx]; //get the metal from all pixels around the extractor radius  
											}
										} 
									}
								}

							// Quick calc test:
							if(x > 0){
								TotalMetal = TempAverage[y * MapWidth + x -1];
								for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
									if (sy >= 0 && sy < MapHeight){
										int addX = x+xend[a];
										int remX = x-xend[a] -1;
										if(addX < MapWidth)
											TotalMetal += MexArrayA[sy * MapWidth + addX];
										if(remX >= 0)
											TotalMetal -= MexArrayA[sy * MapWidth + remX];
									}
								}
							} 
							else if(y > 0){
								// x == 0 here
								TotalMetal = TempAverage[(y-1) * MapWidth];
								// Remove the top half:
								int a = XtractorRadius;
								for (int sx=0;sx<=XtractorRadius;sx++,a++){
									if (sx < MapWidth){
										int remY = y-xend[a] -1;
										if(remY >= 0)
											TotalMetal -= MexArrayA[remY * MapWidth + sx];
									}
								}
								// Add the bottom half:
								a = XtractorRadius;
								for (int sx=0;sx<=XtractorRadius;sx++,a++){
									if (sx < MapWidth){
										int addY = y+xend[a];
										if(addY < MapHeight)
											TotalMetal += MexArrayA[addY * MapWidth + sx];
									}
								}
							}
							TempAverage[y * MapWidth + x] = TotalMetal;
							MexArrayB[y * MapWidth + x] = TotalMetal * 255 / MaxMetal; //set that spots metal amount 
							// end
						}
					}
				}
			}

	


		}
	}
	// Kill the lists:
	delete[] bestSpotList;
	delete[] valueDist;
	delete[] xend;
	if (NumSpotsFound > MaxSpots * 0.95){ // 0.95 used for for reliability, fucking with is bad juju
		ai->cb->SendTextMsg("Metal Map Found",0);
		NumSpotsFound = 0; //no point in saving spots if the map is a metalmap
	}
	L("Time taken to generate spots: " << ai->math->TimerSecs() << " seconds.");
	//ai->cb->SendTextMsg(c,0);
	
}
*/

// Test

/*
Call when the backing array have changed, and this change has not beed notified
with UpdateSumMap().

This will trigger a full remake of the SumMap, when it is requested again.
(calling this will not update SumMap)
*/
void CSpotFinder::BackingArrayChanged()
{
	haveTheBestSpotReady = false;
	isValid = false;
}
