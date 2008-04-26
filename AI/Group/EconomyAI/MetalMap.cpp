#include "ExternalAI/IAICallback.h"
#include "MetalMap.h"

CMetalMap::CMetalMap(IAICallback* aicb, bool verbose)
{
	this->aicb		= aicb;
	this->verbose	= verbose;
	MinMetalForSpot = 30;	// from 0-255, the minimum percentage of metal a spot needs to have
							//from the maximum to be saved. Prevents crappier spots in between taken spaces.
							//They are still perfectly valid and will generate metal mind you!

	MaxSpots = (int) (20.37183 * (float) aicb->GetMapHeight() * (float) aicb->GetMapWidth() / (aicb->GetExtractorRadius() * aicb->GetExtractorRadius()));
	//   max num of spots	= total area / area of one extractor
	//						= total area / (PI * radius^2)
	// The dimensions of GetMapHeight and GetExtractorRadius are different, hence the extra factor 64 ( = 20.3 * PI)
	if(MaxSpots > 5000)
		MaxSpots = 5000;
	
	MetalMapHeight		= aicb->GetMapHeight() / 2; //metal map has 1/2 resolution of normal map
	MetalMapWidth		= aicb->GetMapWidth() / 2;
	TotalCells			= MetalMapHeight * MetalMapWidth;

	XtractorRadius		= (int) aicb->GetExtractorRadius()/ 16;
	DoubleRadius		= XtractorRadius * 2;
	SquareRadius		= XtractorRadius * XtractorRadius; //used to speed up loops so no recalculation needed
	DoubleSquareRadius	= DoubleRadius * DoubleRadius; // same as above 

	MexArrayA			= new unsigned char [TotalCells];	
	MexArrayB			= new unsigned char [TotalCells];

	TempAverage			= new int [TotalCells];
	Stopme 				= false;
	IsMetalMap			= false;
	AverageMetalPerSpot = 0; //clear variables just in case!
	TotalMetal = MaxMetal = NumSpotsFound = 0;
}

CMetalMap::~CMetalMap()
{
	delete [] MexArrayA;
	delete [] MexArrayB;
	delete [] TempAverage;
}

void CMetalMap::Init()
{
	if(!LoadMetalMap()) //if theres no available load file, create one and save it
	{
		GetMetalPoints();
		SaveMetalMap();
	}
	if(verbose)
	{
		char output[50];
		sprintf(output, "Metal spots found: %i",NumSpotsFound );
		aicb->SendTextMsg(output,0);
		sprintf(output, "Max metal spots: %i",MaxSpots );
		aicb->SendTextMsg(output,0);
		sprintf(output, "Average metal per spot: %f",AverageMetalPerSpot);
		aicb->SendTextMsg(output,0);
	}
}

void CMetalMap::GetMetalPoints()
{
	int* xend = new int[DoubleRadius+1]; 
	for (int a=0;a<DoubleRadius+1;a++)
	{ 
		float z=(float)a-XtractorRadius;
		float floatsqrradius = (float)SquareRadius;
		xend[a]=(int)sqrt(floatsqrradius-z*z);
	}
	//Load up the metal Values in each pixel
	const unsigned char *metalMapArray = aicb->GetMetalMap();
	double TotalMetalDouble  = 0;
	for (int i = 0; i < TotalCells; i++)
	{
		TotalMetalDouble +=  MexArrayA[i] = metalMapArray[i];		// Count the total metal so you can work out an average of the whole map
	}
	AverageMetal = (float)TotalMetalDouble / TotalCells;  //do the average
	
	// Quick test for no metal map:
	if(TotalMetalDouble < 0.9)
	{
		// The map dont have any metal, just stop.
		NumSpotsFound = 0;
		delete[] xend;
		return;
	}
	// Now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y < MetalMapHeight; y++)
	{
		for (int x = 0; x < MetalMapWidth; x++)
		{
			TotalMetal = 0;			
			if(x == 0 && y == 0) // First Spot needs full calculation
			for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++)
			{
				if (sy >= 0 && sy < MetalMapHeight)
				{
					for (int sx=x-xend[a];sx<=x+xend[a];sx++)
					{ 
						if (sx >= 0 && sx < MetalMapWidth)
						{
							TotalMetal += MexArrayA[sy * MetalMapWidth + sx]; //get the metal from all pixels around the extractor radius  
						}
					} 
				}
			}
			// Quick calc test:		
			if(x > 0)
			{
				TotalMetal = TempAverage[y * MetalMapWidth + x -1];
				for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++)
				{
					if (sy >= 0 && sy < MetalMapHeight)
					{
						int addX = x+xend[a];
						int remX = x-xend[a] -1;
						if(addX < MetalMapWidth)
							TotalMetal += MexArrayA[sy * MetalMapWidth + addX];
						if(remX >= 0)
							TotalMetal -= MexArrayA[sy * MetalMapWidth + remX];
					}
				}
			} 
			else if(y > 0)
			{
				// x == 0 here
				TotalMetal = TempAverage[(y-1) * MetalMapWidth];
				// Remove the top half:
				int a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++)
				{
					if (sx < MetalMapWidth)
					{
						int remY = y-xend[a] -1;
						if(remY >= 0)
							TotalMetal -= MexArrayA[remY * MetalMapWidth + sx];
					}
				}
				// Add the bottom half:
				a = XtractorRadius;
				for (int sx=0;sx<=XtractorRadius;sx++,a++)
				{
					if (sx < MetalMapWidth)
					{
						int addY = y+xend[a];
						if(addY < MetalMapHeight)
							TotalMetal += MexArrayA[addY * MetalMapWidth + sx];
					}
				}

				 //TotalMetal = TotalMetal; // Comment out for debug
			}
			TempAverage[y * MetalMapWidth + x] = TotalMetal; //set that spots metal making ability (divide by cells to values are small)
			if (MaxMetal < TotalMetal)
				MaxMetal = TotalMetal;  //find the spot with the highest metal to set as the map's max
		}
	}
	// Make a list for the distribution of values
	int * valueDist = new int[256];
	for (int i = 0; i < 256; i++) // Clear the array (useless?)
	{
		valueDist[i] = 0;
	}

	for (int i = 0; i < TotalCells; i++) // this will get the total metal a mex placed at each spot would make
	{
		MexArrayB[i] = TempAverage[i] * 255 / MaxMetal;  //scale the metal so any map will have values 0-255, no matter how much metal it has
		int value = MexArrayB[i];
		valueDist[value]++;
	}
	
	// Find the current best value
	int bestValue = 0;
	int numberOfValues = 0;
	int usedSpots = 0;
	for (int i = 255; i >= 0; i--)
	{
		if(valueDist[i] != 0)
		{
			bestValue = i;
			numberOfValues = valueDist[i];
			break;
		}
	}

	// Make a list of the indexes of the best spots
	if(numberOfValues > 256) // Make shure that the list wont be too big
		numberOfValues = 256;
	int *bestSpotList = new int[numberOfValues];
	for (int i = 0; i < TotalCells; i++)
	{			
		if (MexArrayB[i] == bestValue)
		{
			// Add the index of this spot to the list.
			bestSpotList[usedSpots] = i;
			usedSpots++;
			if(usedSpots == numberOfValues)
			{
				// The list is filled, stop the loop.
				usedSpots = 0;
				break;
			}
		}
	}

	int printDebug1 = 100;
	for (int a = 0; a < MaxSpots; a++)
	{	
		if(!Stopme)
		{
			TempMetal = 0; //reset tempmetal so it can find new spots
			// Take the first spot
			int speedTempMetal_x;
			int speedTempMetal_y;
			int speedTempMetal = 0;
			bool found = false;
			while(!found)
			{				
				if(usedSpots == numberOfValues)
				{
					// The list is empty now, refill it:
					// Make a list of all the best spots:
					for (int i = 0; i < 256; i++) // Clear the array
					{
						valueDist[i] = 0;
					}
					// Find the metal distribution
					for (int i = 0; i < TotalCells; i++)
					{
						int value = MexArrayB[i];
						valueDist[value]++;
					}
					// Find the current best value
					bestValue = 0;
					numberOfValues = 0;
					usedSpots = 0;
					for (int i = 255; i >= 0; i--)
					{
						if(valueDist[i] != 0)
						{
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
					
					for (int i = 0; i < TotalCells; i++)
					{			
						if (MexArrayB[i] == bestValue)
						{
							// Add the index of this spot to the list.
							bestSpotList[usedSpots] = i;
							usedSpots++;
							if(usedSpots == numberOfValues)
							{
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

		if (!Stopme)
		{

			BufferSpot.x=(float)coordx * 16 + 8; // format metal coords to game-coords
			BufferSpot.z=(float)coordy * 16 + 8;
			BufferSpot.y=(float)TempMetal *aicb->GetMaxMetal() * (float)MaxMetal / 255; //Gets the actual amount of metal an extractor can make
			VectoredSpots.push_back(BufferSpot);
			NumSpotsFound += 1;
			
			// Small speedup of "wipes the metal around the spot so its not counted twice":
			for (int sy=coordy-XtractorRadius,a=0;sy<=coordy+XtractorRadius;sy++,a++)
			{
				if (sy >= 0 && sy < MetalMapHeight)
				{
					int clearXStart = coordx-xend[a];
					int clearXEnd = coordx+xend[a];
					if(clearXStart < 0)
						clearXStart = 0;
					if(clearXEnd >= MetalMapWidth)
						clearXEnd = MetalMapWidth -1;
					for(int xClear = clearXStart; xClear <= clearXEnd; xClear++)
					{
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
								for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++)
								{
									if (sy >= 0 && sy < MetalMapHeight)
									{
										for (int sx=x-xend[a];sx<=x+xend[a];sx++)
										{ 
											if (sx >= 0 && sx < MetalMapWidth)
											{
												TotalMetal += MexArrayA[sy * MetalMapWidth + sx]; //get the metal from all pixels around the extractor radius  
											}
										} 
									}
								}

							// Quick calc test:
							if(x > 0)
							{
								TotalMetal = TempAverage[y * MetalMapWidth + x -1];
								for (int sy=y-XtractorRadius,a=0;sy<=y+XtractorRadius;sy++,a++)
								{
									if (sy >= 0 && sy < MetalMapHeight)
									{
										int addX = x+xend[a];
										int remX = x-xend[a] -1;
										if(addX < MetalMapWidth)
											TotalMetal += MexArrayA[sy * MetalMapWidth + addX];
										if(remX >= 0)
											TotalMetal -= MexArrayA[sy * MetalMapWidth + remX];
									}
								}
							} 
							else if(y > 0)
							{
								// x == 0 here
								TotalMetal = TempAverage[(y-1) * MetalMapWidth];
								// Remove the top half:
								int a = XtractorRadius;
								for (int sx=0;sx<=XtractorRadius;sx++,a++)
								{
									if (sx < MetalMapWidth){
										int remY = y-xend[a] -1;
										if(remY >= 0)
											TotalMetal -= MexArrayA[remY * MetalMapWidth + sx];
									}
								}
								// Add the bottom half:
								a = XtractorRadius;
								for (int sx=0;sx<=XtractorRadius;sx++,a++)
								{
									if (sx < MetalMapWidth)
									{
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
	if (NumSpotsFound != 0)
	{
		AverageMetalPerSpot = (float)TotalMetalDouble * aicb->GetMaxMetal() / NumSpotsFound;
	}
	// if the spots cover more than 80% of the map, and the extractorradius is less than 60 (which is small),
	// we wil consider this map as a metalmap.
	if (NumSpotsFound > 0.8 * MaxSpots && aicb->GetExtractorRadius() < 60)
	{
		if(verbose) aicb->SendTextMsg("Metal Map Found",0);
		IsMetalMap		= true;
		NumSpotsFound	= 0; //no point in saving spots if the map is a metalmap
	}
}

void CMetalMap::SaveMetalMap()
{
	char buffer[1000];
	std::string filename = std::string("AI/HelperAI/MexData/") + std::string(aicb->GetMapName());
	filename.resize(filename.size()-3);
	filename += std::string("Mv");
	filename += std::string(M_CLASS_VERSION);

	strcpy(buffer, filename.c_str());
	aicb->GetValue(AIVAL_LOCATE_FILE_W, buffer);

	FILE *save_file = fopen(buffer, "wb");
	if(save_file)
	{
		fwrite(&NumSpotsFound, sizeof(int), 1, save_file);
		fwrite(&IsMetalMap, sizeof(bool), 1, save_file);
		fwrite(&AverageMetal, sizeof(float), 1, save_file);
		fwrite(&AverageMetalPerSpot, sizeof(float), 1, save_file);
		for(int i = 0; i < NumSpotsFound; i++){
			fwrite(&VectoredSpots[i], sizeof(float3), 1, save_file);
		}
		fclose(save_file);
		if(verbose) aicb->SendTextMsg("Metal Spots created and saved!",0);
	}
	else
	{
		if(verbose) aicb->SendTextMsg("Metal Spots couldnt be saved!",0);
	}
}

bool CMetalMap::LoadMetalMap()
{
	char buffer[1000];
	std::string filename = std::string("AI/HelperAI/MexData/") + std::string(aicb->GetMapName());
	filename.resize(filename.size()-3);
	filename += std::string("Mv");
	filename += std::string(M_CLASS_VERSION);

	strcpy(buffer, filename.c_str());
	aicb->GetValue(AIVAL_LOCATE_FILE_R, buffer);

	FILE *load_file;
	// load Spots if file exists 
	if((load_file = fopen(buffer, "rb")))
	{
		fread(&NumSpotsFound, sizeof(int), 1, load_file);
		VectoredSpots.resize(NumSpotsFound);
		fread(&IsMetalMap, sizeof(bool), 1, load_file);
		fread(&AverageMetal, sizeof(float), 1, load_file);
		fread(&AverageMetalPerSpot, sizeof(float), 1, load_file);
		for(int i = 0; i < NumSpotsFound; i++)
		{
			fread(&VectoredSpots[i], sizeof(float3), 1, load_file);
		}
		fclose(load_file);
		if(verbose) aicb->SendTextMsg("Metal Spots loaded from file",0);
		return true;
	}
	else
	{
		if(verbose) aicb->SendTextMsg("Metal Spots couldnt be loaded from file",0);
		return false;
	}
		
}
