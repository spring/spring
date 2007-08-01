#include "SpotFinder.h"

//using namespace std;

CSpotFinder::CSpotFinder(AIClasses* ai, int height, int width) {
	this->ai = ai;
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

	for (int i = 0; i < totalCacheCells; i++) {
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
	//L("SpotFinder class");
}

CSpotFinder::~CSpotFinder() {
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
void CSpotFinder::MakeCachePoints()
{
//	float currentBest = FLT_MIN;
//	int currentBestX = 0;
//	int currentBestY = 0;
	
	for (int y = 0; y < MapHeight / CACHEFACTOR; y++){
		for (int x = 0; x < MapWidth / CACHEFACTOR; x++){
			int cacheIndex = y * MapWidth/CACHEFACTOR + x;
			cachePoints[cacheIndex].maxValueInBox = FLT_MIN;
			cachePoints[cacheIndex].isValid = true;
		}
	}
	
	for (int y = 0; y < MapHeight; y++){
		// Find the cache line:
		int cacheY = y / CACHEFACTOR;
		
		for (int x = 0; x < MapWidth; x++){
			int cacheX = x / CACHEFACTOR;
			float sum = TempAverage[y * MapWidth + x];
			
			int cacheIndex = cacheY * MapWidth/CACHEFACTOR + cacheX;
			////L("cacheIndex: " << cacheIndex << ", maxValueInBox: " << cachePoints[cacheIndex].maxValueInBox << ", sum: " << sum << ", x: " << x << ", y: " << y);
			
			if(cachePoints[cacheIndex].maxValueInBox < sum)
			{
				// Found a better spot for this cache element
				cachePoints[cacheIndex].maxValueInBox = sum;
				cachePoints[cacheIndex].x = x;
				cachePoints[cacheIndex].y = y;
				////L("cacheIndex: " << cacheIndex << ", sum: " << sum << ", x: " << x << ", y: " << y);
			}
		}
	}
}

CachePoint * CSpotFinder::GetBestCachePoint(int x, int y)
{
	int cacheY = y;// / CACHEFACTOR;
	int cacheX = x;// / CACHEFACTOR;
	int cacheIndex = cacheY * MapWidth/CACHEFACTOR + cacheX;
	////L("GetBestCachePoint: cacheIndex: " << cacheIndex);
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
	//L("Starting MakeSumMap");
	isValid = true;
	int XtractorRadius = radius;
//	int DoubleRadius = XtractorRadius * 2;
	//int SquareRadius = XtractorRadius * XtractorRadius; //used to speed up loops so no recalculation needed
	//int DoubleSquareRadius = DoubleRadius * DoubleRadius; // same as above
	
	// Time stuff:
	ai->math->TimerStart();
	
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
				////L("cacheIndex: " << cacheIndex << ", sum: " << sum << ", x: " << x << ", y: " << y);
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
 * updates the given spot, based on the changed values on and around that point
 * radius has the same resolution as the map given to SpotFinder
 * does not affect the current radius of SumMap
 * update is not delayed (so do not call more than needed), unless SumMap is already invalid
*/
void CSpotFinder::UpdateSumMap(int coordx, int coordy, int clearRadius) {
	if (!isValid)
		return;

	// TODO: this is not needed all the time
	haveTheBestSpotReady = false;

	int XtractorRadius = radius;
	int clearRealRadius = clearRadius + radius +1;

	// redo the whole averaging process around the picked
	// spot so other spots can be found around it
	// TODO: fix it so that it does not work as a box update

	for (int y = coordy - clearRealRadius; y <= coordy + clearRealRadius; y++) {
		if (y >=0 && y < MapHeight) {
			for (int x = coordx - clearRealRadius; x <= coordx + clearRealRadius; x++) {
				if (x >=0 && x < MapWidth) {
					float TotalMetal = 0;

					if (x == 0 && y == 0) {
						for (int sy = y - XtractorRadius, a = 0; sy <= y + XtractorRadius; sy++, a++) {
							if (sy >= 0 && sy < MapHeight) {
								for (int sx = x - xend[a]; sx <= x + xend[a]; sx++) {
									if (sx >= 0 && sx < MapWidth) {
 										//get the metal from all pixels around the extractor radius
										TotalMetal += MexArrayA[sy * MapWidth + sx];
									}
								}
							}
						}
					}

					if (x > 0) {
						TotalMetal = TempAverage[y * MapWidth + x - 1];

						for (int sy = y - XtractorRadius, a = 0; sy <= y + XtractorRadius; sy++, a++) {
							if (sy >= 0 && sy < MapHeight) {
								int addX = x + xend[a];
								int remX = x - xend[a] - 1;

								if (addX < MapWidth)
									TotalMetal += MexArrayA[sy * MapWidth + addX];
								if (remX >= 0)
									TotalMetal -= MexArrayA[sy * MapWidth + remX];
							}
						}
					}
					else if (y > 0) {
						// x == 0 here
						TotalMetal = TempAverage[(y - 1) * MapWidth];
						// remove the top half
						int a = XtractorRadius;

						for (int sx = 0; sx <= XtractorRadius; sx++, a++) {
							if (sx < MapWidth) {
								int remY = y - xend[a] - 1;

								if (remY >= 0)
									TotalMetal -= MexArrayA[remY * MapWidth + sx];
							}
						}
						// add the bottom half
						a = XtractorRadius;
						for (int sx = 0; sx <= XtractorRadius; sx++, a++) {
							if (sx < MapWidth) {
								int addY = y + xend[a];
								if (addY < MapHeight)
									TotalMetal += MexArrayA[addY * MapWidth + sx];
							}
						}
					}
					TempAverage[y * MapWidth + x] = TotalMetal;
 					//set that spot's metal amount
					//MexArrayB[y * MapWidth + x] = TotalMetal * 255 / MaxMetal;
				}
			}
		}
	}
}



/*
 * Call when the backing array have changed, and this
 * change has not beed notified with UpdateSumMap().

 * This will trigger a full remake of the SumMap, when
 * it is requested again. (calling this will not update
 * SumMap)
 */
void CSpotFinder::BackingArrayChanged() {
	haveTheBestSpotReady = false;
	isValid = false;
}
