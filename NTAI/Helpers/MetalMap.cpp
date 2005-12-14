
#include "../helper.h"
MetalMap::MetalMap(IAICallback *cb)
{
	this->cb = cb;
	MinMetalForSpot = 40; // from 0-255, the minimum percentage of metal a spot needs to have
							//from the maximum to be saved. Prevents crappier spots in between taken spaces.
							//They are still perfectly valid and will generate metal mind you!
	MaxSpots = 4000; //If more spots than that are found the map is considered a metalmap, tweak this as needed

	MetalMapHeight = cb->GetMapHeight() / 2; //metal map has 1/2 resolution of normal map
	MetalMapWidth = cb->GetMapWidth() / 2;
	TotalCells = MetalMapHeight * MetalMapWidth;
	XtractorRadius = cb->GetExtractorRadius()/ 16;
	DoubleRadius = cb->GetExtractorRadius() / 8;
	SquareRadius = (cb->GetExtractorRadius() / 16) * (cb->GetExtractorRadius() / 16); //used to speed up loops so no recalculation needed
	DoubleSquareRadius = (cb->GetExtractorRadius() / 8) * (cb->GetExtractorRadius() / 8); // same as above 
	CellsInRadius = PI * XtractorRadius * XtractorRadius; //yadda yadda
	MexArrayA = new unsigned char [TotalCells];	
	MexArrayB = new unsigned char [TotalCells];
	MexArrayC = new unsigned char [TotalCells]; //used for drawing the TGA, not really needed with a couple of changes
	TempAverage = new int [TotalCells];
	//TempBestSpots = new MetalSpot [MaxSpots];
	TotalMetal = MaxMetal = Stopme = IsMetalMap = SpotsFound = 0; //clear variables just in case!
}
MetalMap::~MetalMap()
{
	delete [] MexArrayA;
	delete [] MexArrayB;
	delete [] MexArrayC;
	delete [] TempAverage;
	//delete [] TempBestSpots;
}

void MetalMap::init()
{
	cb->SendTextMsg("KAI Metal Class by Krogothe",0);
	if(!LoadMetalMap()) //if theres no available load file, create one and save it
	{
		GetMetalPoints();
		SaveMetalMap();
		MakeTGA(MexArrayC);
	}

}

void MetalMap::GetMetalPoints()
{	

	//Load up the metal Values in each pixel
	for (int i = 0; i != TotalCells - 1; i++)
	{
		MexArrayA[i] = *(cb->GetMetalMap() + i);
		TotalMetal += MexArrayA[i];		// Count the total metal so you can work out an average of the whole map
	}
	AverageMetal = TotalMetal / TotalCells;  //do the average

	// Now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y != MetalMapHeight; y++)
	{
		for (int x = 0; x != MetalMapWidth; x++)
		{
			TotalMetal = 0;
			for (int myx = x - XtractorRadius; myx != x + XtractorRadius; myx++)
			{
				if (myx >= 0 && myx < MetalMapWidth)
				{
					for (int myy = y - XtractorRadius; myy != y + XtractorRadius; myy++)
					{ 
						if (myy >= 0 && myy < MetalMapHeight && ((x - myx)*(x - myx) + (y - myy)*(y - myy)) <= SquareRadius)
						{
							TotalMetal += MexArrayA[myy * MetalMapWidth + myx]; //get the metal from all pixels around the extractor radius 
						}
					}
				}
			}
			TempAverage[y * MetalMapWidth + x] = TotalMetal; //set that spots metal making ability (divide by cells to values are small)
			if (MaxMetal < TotalMetal)
				MaxMetal = TotalMetal;  //find the spot with the highest metal to set as the map's max
		}
	}
	for (int i = 0; i != TotalCells; i++) // this will get the total metal a mex placed at each spot would make
	{
	MexArrayB[i] = TempAverage[i] * 255 / MaxMetal;  //scale the metal so any map will have values 0-255, no matter how much metal it has
	MexArrayC[i] = 0; // clear out the array since its never been used.
	}

	
	for (int a = 0; a != MaxSpots; a++)
	{	
		if(!Stopme)
			TempMetal = 0; //reset tempmetal so it can find new spots
		for (int i = 0; i != TotalCells; i++)
		{			//finds the best spot on the map and gets its coords
			if (MexArrayB[i] > TempMetal && !Stopme)
			{
				TempMetal = MexArrayB[i];
				coordx = i%MetalMapWidth;
				coordy = i/MetalMapWidth;
			}
		}		
		if (TempMetal < MinMetalForSpot)
			Stopme = 1; // if the spots get too crappy it will stop running the loops to speed it all up

		if (!Stopme)
		{

			TempBestSpots[a].x=coordx * 16; // format metal coords to game-coords
			TempBestSpots[a].y=coordy * 16;
			TempBestSpots[a].z=*(cb->GetHeightMap()+ coordy * MetalMapWidth + coordx);
			TempBestSpots[a].Amount=TempMetal * cb->GetMaxMetal() * MaxMetal / 255; //Gets the actual amount of metal an extractor can make
			MexArrayC[coordy * MetalMapWidth + coordx] = TempMetal; //plot TGA array (not necessary) for debug

			SpotsFound += 1;

			for (int myx = coordx - XtractorRadius; myx != coordx + XtractorRadius; myx++)
			{
				if (myx >= 0 && myx < MetalMapWidth)
				{
					for (int myy = coordy - XtractorRadius; myy != coordy + XtractorRadius; myy++)
					{
						if (myy >= 0 && myy < MetalMapHeight && ((coordx - myx)*(coordx - myx) + (coordy - myy)*(coordy - myy)) <= SquareRadius)
						{
							MexArrayA[myy * MetalMapWidth + myx] = 0; //wipes the metal around the spot so its not counted twice
							MexArrayB[myy * MetalMapWidth + myx] = 0;
						}
					}
				}
			}
		// Redo the whole averaging process around the picked spot so other spots can be found around it
			for (int y = coordy - DoubleRadius; y != coordy + DoubleRadius; y++)
			{
				if(y >=0 && y < MetalMapHeight)
				{
					for (int x = coordx - DoubleRadius; x != coordx + DoubleRadius; x++)
					{//funcion below is optimized so it will only update spots between r and 2r, greatly speeding it up
						if((coordx - x)*(coordx - x) + (coordy - y)*(coordy - y) <= DoubleSquareRadius && x >=0 && x < MetalMapWidth && MexArrayB[y * MetalMapWidth + x])
						{
							TotalMetal = 0;
							for (int myx = x - XtractorRadius; myx != x + XtractorRadius; myx++)
							{
								if (myx >= 0 && myx < MetalMapWidth)
								{
									for (int myy = y - XtractorRadius; myy != y + XtractorRadius; myy++)
									{ 
										if (myy >= 0 && myy < MetalMapHeight && ((x - myx)*(x - myx) + (y - myy)*(y - myy)) <= SquareRadius)
										{
											TotalMetal += MexArrayA[myy * MetalMapWidth + myx]; //recalculate nearby spots to account for deleted metal from chosen spot
										}
									}
								}
							}
							MexArrayB[y * MetalMapWidth + x] = TotalMetal * 255 / MaxMetal;; //set that spots metal amount 
						}
					}
				}
			}
		}
	}
	if (SpotsFound > MaxSpots * 0.95) // 0.95 used for for reliability, fucking with is bad juju
	{
		IsMetalMap = 1; 
		cb->SendTextMsg("Metal Map Found",0);
		SpotsFound = 0; //no point in saving spots if the map is a metalmap
	}

	//BestSpots = new MetalSpot [SpotsFound]; //save the bestspots in a non-temp array now that you know how many are there!
	for (int i = 0; i != SpotsFound; i++)
	BestSpots[i] = TempBestSpots[i];
	
}

void MetalMap::SaveMetalMap()
{
	string filename;
	//char buffer[80];
	//strcpy(buffer, "\\MetalSpots\\");
	filename.append("\\MetalSpots\\");
	//strcat(buffer, cb->GetMapName());
	filename.append(cb->GetMapName());
	filename.append(".dat");
	//ReplaceExtension (buffer, filename, sizeof(filename), "dat");

	FILE *save_file = fopen(filename.c_str(), "wb");

	fprintf(save_file, "%i %f %i\n",IsMetalMap,AverageMetal,SpotsFound);
	for(int i = 0; i != SpotsFound; i++)
		fprintf(save_file, "%i %i %i %f\n",BestSpots[i].x,BestSpots[i].y,BestSpots[i].z,BestSpots[i].Amount);
	
	fclose(save_file);
	cb->SendTextMsg("Metal Spots created and saved!",0);
}

bool MetalMap::LoadMetalMap()
{
	/*char filename[100];
	char buffer[80];
	strcpy(buffer, "\\MetalSpots\\");
	strcat(buffer, cb->GetMapName());
	ReplaceExtension (buffer, filename, sizeof(filename), "dat");*/
	string filename;
	//char buffer[80];
	//strcpy(buffer, "\\MetalSpots\\");
	filename.append("\\MetalSpots\\");
	//strcat(buffer, cb->GetMapName());
	filename.append(cb->GetMapName());
	filename.append(".dat");
	//ReplaceExtension (buffer, filename, sizeof(filename), "dat");
	FILE *load_file;

	// load Spots if file exists 
	if(load_file = fopen(filename.c_str(), "r"))
	{
		fscanf(load_file, "%i %f %i\n",&IsMetalMap,&AverageMetal,&SpotsFound);
		//BestSpots = new MetalSpot [SpotsFound];
		for(int i = 0; i != SpotsFound; i++)
		{
			fscanf(load_file, "%i %i %i %f\n",&BestSpots[i].x,&BestSpots[i].y,&BestSpots[i].z,&BestSpots[i].Amount);
			BestSpots[i].Occupied = 0;
		}
		fclose(load_file);
		cb->SendTextMsg("Metal Spots loaded from file",0);
		return true;
	}
	else
		return false;

}

void MetalMap::MakeTGA(unsigned char* data)
{
	// open file
	FILE *fp=fopen("debugmetalspots.tga", "wb");
	// fill & write header
	char Header[18];
	memset(Header, 0, sizeof(Header));

	Header[2] = 3;		// uncompressed gray-scale
	Header[12] = (char) (MetalMapWidth & 0xFF);
	Header[13] = (char) (MetalMapWidth >> 8);
	Header[14] = (char) (MetalMapHeight & 0xFF);
	Header[15] = (char) (MetalMapHeight >> 8);
	Header[16] = 8; // 8 bits/pixel
	Header[17] = 0x20;	

	fwrite(Header, 18, 1, fp);

	uchar out[1];

	for (int y=0;y<MetalMapHeight;y++)
		for (int x=0;x<MetalMapWidth;x++)
		{
			out[0]=data[y * MetalMapWidth + x];

			fwrite (out, 1, 1, fp);
			}
	fclose(fp);
}