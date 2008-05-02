#include "MetalMap.h"
#include "time.h"

using std::string;

CMetalMap::CMetalMap(IAICallback* callback)
{
	this->callback=callback;
	MinMetalForSpot = 30;	// from 0-255, the minimum percentage of metal a spot needs to have
							//from the maximum to be saved. Prevents crappier spots in between taken spaces.
							//They are still perfectly valid and will generate metal mind you!
	MaxSpots = 15000; //If more spots than that are found the map is considered a metalmap, tweak this as needed
	MetalMapHeight =callback->GetMapHeight() / 2; //metal map has 1/2 resolution of normal map
	MetalMapWidth =callback->GetMapWidth() / 2;
	TotalCells = MetalMapHeight * MetalMapWidth;
	XtractorRadius = int(callback->GetExtractorRadius()/16.0);
	DoubleRadius = XtractorRadius * 2;
	SquareRadius = XtractorRadius * XtractorRadius; //used to speed up loops so no recalculation needed
	DoubleSquareRadius = DoubleRadius * DoubleRadius; // same as above 
	MexArrayA = new unsigned char [TotalCells];	
	MexArrayB = new unsigned char [TotalCells];
	MexArrayC = new unsigned char [TotalCells]; //used for drawing the TGA, not really needed with a couple of changes
	TempAverage = new int [TotalCells];
	TotalMetal = MaxMetal = NumSpotsFound = 0; //clear variables just in case!
	Stopme = false;
	//L("Metal class logging works!");
}

CMetalMap::~CMetalMap()
{
	delete [] MexArrayA;
	delete [] MexArrayB;
	delete [] MexArrayC;
	delete [] TempAverage;
}

void CMetalMap::Init()
{
	callback->SendTextMsg("KAI Metal Class by Krogothe",0); // Leave this line if you want to use this class in your AI
	if(!LoadMetalMap()) //if theres no available load file, create one and save it
	{
		GetMetalPoints();
		SaveMetalMap();
	}
	char k[200];
	sprintf(k,"Metal Spots Found %i",NumSpotsFound);
	callback->SendTextMsg(k,0);
}

void CMetalMap::GetMetalPoints()
{		
	// Time stuff:
	int timetaken = clock();

	int* xend = new int[DoubleRadius+1]; 
	for (int a=0;a<DoubleRadius+1;a++){ 
		float z=a-XtractorRadius;
		float floatsqrradius = SquareRadius;
		xend[a]=int(sqrt(floatsqrradius-z*z));
	}
	//Load up the metal Values in each pixel
	const unsigned char *metalMapArray = callback->GetMetalMap();
	double TotalMetalDouble  = 0;
	for (int i = 0; i < TotalCells; i++){
		TotalMetalDouble +=  MexArrayA[i] = metalMapArray[i];		// Count the total metal so you can work out an average of the whole map
	}
	AverageMetal = TotalMetalDouble / TotalCells;  //do the average
	
	// Quick test for no metal map:
	if(TotalMetalDouble < 0.9){
		// The map dont have any metal, just stop.
		NumSpotsFound = 0;
		delete[] xend;
		timetaken = time (NULL) - timetaken;
		char c[200];
		sprintf(c,"Time taken to generate spots: %i seconds.",timetaken);
		//L("Time taken to generate spots: " << timetaken << " seconds.");
		return;
	}
	// Now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y < MetalMapHeight; y++){
		for (int x = 0; x < MetalMapWidth; x++){
			TotalMetal = 0;			
			if(x == 0 && y == 0) // First Spot needs full calculation
			for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
				if (sy >= 0 && sy < MetalMapHeight){
					for (int sx=x-xend[a];sx<=x+xend[a];sx++){ 
						if (sx >= 0 && sx < MetalMapWidth){
							TotalMetal += MexArrayA[sy * MetalMapWidth + sx]; //get the metal from all pixels around the extractor radius  
						}
					} 
				}
			}
			// Quick calc test:		
			if(x > 0)
			{
				TotalMetal = TempAverage[y * MetalMapWidth + x -1];
				for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
					if (sy >= 0 && sy < MetalMapHeight){
						int addX = x+xend[a];
						int remX = x-xend[a] -1;
						if(addX < MetalMapWidth)
							TotalMetal += MexArrayA[sy * MetalMapWidth + addX];
						if(remX >= 0)
							TotalMetal -= MexArrayA[sy * MetalMapWidth + remX];
					}
				}
			} else if(y > 0){
				// x == 0 here
				TotalMetal = TempAverage[(y-1) * MetalMapWidth];
				// Remove the top half:
				int a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++){
					if (sx < MetalMapWidth){
						int remY = y-xend[a] -1;
						if(remY >= 0)
							TotalMetal -= MexArrayA[remY * MetalMapWidth + sx];
					}
				}
				// Add the bottom half:
				a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++){
					if (sx < MetalMapWidth){
						int addY = y+xend[a];
						if(addY < MetalMapHeight)
							TotalMetal += MexArrayA[addY * MetalMapWidth + sx];
					}
				}

				TotalMetal = TotalMetal; // Comment out for debug
			}
			TempAverage[y * MetalMapWidth + x] = TotalMetal; //set that spots metal making ability (divide by cells to values are small)
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
	if(numberOfValues > 256) // Make shure that the list wont be too big
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
					speedTempMetal_x = spotIndex%MetalMapWidth;
					speedTempMetal_y = spotIndex/MetalMapWidth;
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
			BufferSpot.y=TempMetal *callback->GetMaxMetal() * MaxMetal / 255; //Gets the actual amount of metal an extractor can make
			VectoredSpots.push_back(BufferSpot);
			MexArrayC[coordy * MetalMapWidth + coordx] = TempMetal; //plot TGA array (not necessary) for debug
			NumSpotsFound += 1;
			
			// Small speedup of "wipes the metal around the spot so its not counted twice":
			for (int sy=coordy-XtractorRadius,a=0;sy<=coordy+XtractorRadius;sy++,a++){
				if (sy >= 0 && sy < MetalMapHeight){
					int clearXStart = coordx-xend[a];
					int clearXEnd = coordx+xend[a];
					if(clearXStart < 0)
						clearXStart = 0;
					if(clearXEnd >= MetalMapWidth)
						clearXEnd = MetalMapWidth -1;
					for(int xClear = clearXStart; xClear <= clearXEnd; xClear++){
						MexArrayA[sy * MetalMapWidth + xClear] = 0; //wipes the metal around the spot so its not counted twice
						MexArrayB[sy * MetalMapWidth + xClear] = 0;
						TempAverage[sy * MetalMapWidth + xClear] = 0;						
					}
				}
			}

			// Redo the whole averaging process around the picked spot so other spots can be found around it
			for (int y = coordy - DoubleRadius; y <= coordy + DoubleRadius; y++)
			{
				if(y >=0 && y < MetalMapHeight)
				{
					for (int x = coordx - DoubleRadius; x <= coordx + DoubleRadius; x++)
					{						
						if(x >=0 && x < MetalMapWidth)
						{
							TotalMetal = 0;
							if(x == 0 && y == 0) // Comment out for debug
								for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
									if (sy >= 0 && sy < MetalMapHeight){
										for (int sx=x-xend[a];sx<=x+xend[a];sx++){ 
											if (sx >= 0 && sx < MetalMapWidth){
												TotalMetal += MexArrayA[sy * MetalMapWidth + sx]; //get the metal from all pixels around the extractor radius  
											}
										} 
									}
								}

							// Quick calc test:
							if(x > 0){
								TotalMetal = TempAverage[y * MetalMapWidth + x -1];
								for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++){
									if (sy >= 0 && sy < MetalMapHeight){
										int addX = x+xend[a];
										int remX = x-xend[a] -1;
										if(addX < MetalMapWidth)
											TotalMetal += MexArrayA[sy * MetalMapWidth + addX];
										if(remX >= 0)
											TotalMetal -= MexArrayA[sy * MetalMapWidth + remX];
									}
								}
							} 
							else if(y > 0){
								// x == 0 here
								TotalMetal = TempAverage[(y-1) * MetalMapWidth];
								// Remove the top half:
								int a = XtractorRadius;
								for (int sx=0;sx<=XtractorRadius;sx++,a++){
									if (sx < MetalMapWidth){
										int remY = y-xend[a] -1;
										if(remY >= 0)
											TotalMetal -= MexArrayA[remY * MetalMapWidth + sx];
									}
								}
								// Add the bottom half:
								a = XtractorRadius;
								for (int sx=0;sx<=XtractorRadius;sx++,a++){
									if (sx < MetalMapWidth){
										int addY = y+xend[a];
										if(addY < MetalMapHeight)
											TotalMetal += MexArrayA[addY * MetalMapWidth + sx];
									}
								}
							}
							TempAverage[y * MetalMapWidth + x] = TotalMetal;
							MexArrayB[y * MetalMapWidth + x] = TotalMetal * 255 / MaxMetal; //set that spots metal amount 
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
		callback->SendTextMsg("Metal Map Found",0);
		NumSpotsFound = 0; //no point in saving spots if the map is a metalmap
	}
	timetaken = clock() - timetaken;
	char c[200];
	sprintf(c,"Time taken to generate spots: %ims.",timetaken);
	//L("Time taken to generate spots: " << timetaken << "ms.");
	//callback->SendTextMsg(c,0);
	
}

void CMetalMap::SaveMetalMap()
{
	string filename = string("AI/RAI/Metal/") + string(callback->GetMapName());
	filename.resize(filename.size()-3);
	filename += string("Metal");
	FILE *save_file = fopen(filename.c_str(), "wb");
	fwrite(&NumSpotsFound, sizeof(int), 1, save_file);
	//L("Spots found: " << NumSpotsFound << " AverageMetal: " << AverageMetal);
	fwrite(&AverageMetal, sizeof(float), 1, save_file);
	for(int i = 0; i < NumSpotsFound; i++){
		////L("Loaded i: " << i << ", x; " << VectoredSpots[i].x << ", y; " << VectoredSpots[i].z << ", value: " << VectoredSpots[i].y );
		fwrite(&VectoredSpots[i], sizeof(float3), 1, save_file);
	}
	fclose(save_file);
//	callback->SendTextMsg("Metal Spots created and saved!",0);
}

bool CMetalMap::LoadMetalMap()
{
	string filename = string("AI/RAI/Metal/") + string(callback->GetMapName());
	filename.resize(filename.size()-3);
	filename += string("Metal");
	FILE *load_file;
	// load Spots if file exists 
	if(load_file = fopen(filename.c_str(), "rb")){
		fread(&NumSpotsFound, sizeof(int), 1, load_file);
		VectoredSpots.resize(NumSpotsFound);
		fread(&AverageMetal, sizeof(float), 1, load_file);
		for(int i = 0; i < NumSpotsFound; i++){
			fread(&VectoredSpots[i], sizeof(float3), 1, load_file);
			////L("Loaded i: " << i << ", x; " << VectoredSpots[i].x << ", y; " << VectoredSpots[i].z << ", value: " << VectoredSpots[i].y );
		}
		fclose(load_file);
//		callback->SendTextMsg("Metal Spots loaded from file",0);
		return true;
	}
		return false;
}
