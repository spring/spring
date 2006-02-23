#include "../helper.h"


MetalMap::MetalMap(IAICallback* ai)
{
	this->cb = ai;

	MinMetalForSpot = 30; // from 0-255, the minimum percentage of metal a spot needs to have
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
	MexArrayA = new unsigned char [TotalCells];	
	MexArrayB = new unsigned char [TotalCells];
	MexArrayC = new unsigned char [TotalCells]; //used for drawing the TGA, not really needed with a couple of changes
	TempAverage = new int [TotalCells];
	TotalMetal = MaxMetal = NumSpotsFound = 0; //clear variables just in case!
	Stopme = false;
}

MetalMap::~MetalMap()
{
	delete [] MexArrayA;
	delete [] MexArrayB;
	delete [] MexArrayC;
	delete [] TempAverage;
}

void MetalMap::init()
{
	//cb->SendTextMsg("KAI Metal Class by Krogothe",0);	//If you are going to use this class or part of it, please leave this line as credit
														//it wont eat up CPU cycles, cause cancer or degrade your AI, so theres no reason to remove it
														//but by keeping it youll give appropriate credit for the time and effort ive put into 
														//making this class and then commenting it for the good of the community.
	if(!LoadMetalMap()) //if theres no available load file, create one and save it
	{
		GetMetalPoints();
		SaveMetalMap();
		MakeTGA(MexArrayC);
	}
	//char k[200];
	//sprintf(k,"Metal Spots Found %i",NumSpotsFound);
	//cb->SendTextMsg(k,0);
	LastTryAtSpot.assign(VectoredSpots.size(),0);
}

void MetalMap::GetMetalPoints()
{	

	int timetaken = time (NULL);
	
	int* xend = new int[DoubleRadius]; 
	for (int a=0;a<DoubleRadius;a++) 
	{ 
		float z=a-XtractorRadius;
		float floatsqrradius = SquareRadius;
		xend[a]=sqrt(floatsqrradius-z*z);
	}
	//Load up the metal Values in each pixel
	for (int i = 0; i < TotalCells; i++)
	{
		MexArrayA[i] = *(cb->GetMetalMap() + i);
		TotalMetal += MexArrayA[i];		// Count the total metal so you can work out an average of the whole map
	}
	AverageMetal = TotalMetal / TotalCells;  //do the average

	// Now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y < MetalMapHeight; y++)
	{
		for (int x = 0; x < MetalMapWidth; x++)
		{
			TotalMetal = 0;
			for (int sy=y-XtractorRadius,a=0;sy<y+XtractorRadius;sy++,a++)
			{ 
				if (sy >= 0 && sy < MetalMapHeight)
				{
					for (int sx=x-xend[a];sx<x+xend[a];sx++)
					{ 
						if (sx >= 0 && sx < MetalMapWidth)
							TotalMetal += MexArrayA[sy * MetalMapWidth + sx]; //get the metal from all pixels around the extractor radius  
					} 
				}
			} 
			TempAverage[y * MetalMapWidth + x] = TotalMetal; //set that spots metal making ability (divide by cells to values are small)
			if (MaxMetal < TotalMetal)
				MaxMetal = TotalMetal;  //find the spot with the highest metal to set as the map's max
		}
	}
	for (int i = 0; i < TotalCells; i++) // this will get the total metal a mex placed at each spot would make
	{
		MexArrayB[i] = TempAverage[i] * 255 / MaxMetal;  //scale the metal so any map will have values 0-255, no matter how much metal it has
		MexArrayC[i] = 0; // clear out the array since its never been used.
	}

	
	for (int a = 0; a < MaxSpots; a++)
	{	
		if(!Stopme)
			TempMetal = 0; //reset tempmetal so it can find new spots
		for (int i = 0; i < TotalCells; i++)
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

			BufferSpot.x=coordx * 16 + 8; // format metal coords to game-coords
			BufferSpot.z=coordy * 16 + 8;
			BufferSpot.y=TempMetal * cb->GetMaxMetal() * MaxMetal / 255; //Gets the actual amount of metal an extractor can make
			VectoredSpots.push_back(BufferSpot);
			MexArrayC[coordy * MetalMapWidth + coordx] = TempMetal; //plot TGA array (not necessary) for debug

			NumSpotsFound += 1;

			for (int myx = coordx - XtractorRadius; myx < coordx + XtractorRadius; myx++)
			{
				if (myx >= 0 && myx < MetalMapWidth)
				{
					for (int myy = coordy - XtractorRadius; myy < coordy + XtractorRadius; myy++)
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
			for (int y = coordy - DoubleRadius; y < coordy + DoubleRadius; y++)
			{
				if(y >=0 && y < MetalMapHeight)
				{
					for (int x = coordx - DoubleRadius; x < coordx + DoubleRadius; x++)
					{//funcion below is optimized so it will only update spots between r and 2r, greatly speeding it up
						if((coordx - x)*(coordx - x) + (coordy - y)*(coordy - y) <= DoubleSquareRadius && x >=0 && x < MetalMapWidth && MexArrayB[y * MetalMapWidth + x])
						{
							TotalMetal = 0;
							for (int sy=y-XtractorRadius,a=0;sy<y+XtractorRadius;sy++,a++)
							{ 
								if (sy >= 0 && sy < MetalMapHeight)
								{
									for (int sx=x-xend[a];sx<x+xend[a];sx++)
									{ 
										if (sx >= 0 && sx < MetalMapWidth)
											TotalMetal += MexArrayA[sy * MetalMapWidth + sx]; //get the metal from all pixels around the extractor radius  
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
	if (NumSpotsFound > MaxSpots * 0.95) // 0.95 used for for reliability, fucking with is bad juju
	{
		cb->SendTextMsg("Metal Map Found",0);
		NumSpotsFound = 0; //no point in saving spots if the map is a metalmap
	}

	timetaken = time (NULL) - timetaken;
	char c[200];
	sprintf(c,"Time taken to generate spots: %i seconds.",timetaken);
	cb->SendTextMsg(c,0);
	
}

void MetalMap::SaveMetalMap(){
	char file[100];
	sprintf(file,"aidll/globalai/%s.met",cb->GetMapName()); //Youll need a folder to save stuff to exist else it will crash

	FILE *save_file = fopen(file, "wb");

	fprintf(save_file, "%i %i\n",AverageMetal,NumSpotsFound);
	for(int i = 0; i < NumSpotsFound; i++)
		fprintf(save_file, "%f %f %f\n",VectoredSpots[i].x,VectoredSpots[i].z,VectoredSpots[i].y);
	
	fclose(save_file);
	cb->SendTextMsg("Metal Spots created and saved!",0);
}

bool MetalMap::LoadMetalMap()
{
	char file[100];
	sprintf(file,"aidll/globalai/%s.met",cb->GetMapName());//have this folder exist or crashes are imminent
	FILE *load_file;

	// load Spots if file exists 
	if(load_file = fopen(file, "r")){
		fscanf(load_file, "%i %i\n",&AverageMetal,&NumSpotsFound);
		VectoredSpots.resize(NumSpotsFound);
		for(int i = 0; i < NumSpotsFound; i++)
			fscanf(load_file, "%f %f %f\n",&VectoredSpots[i].x,&VectoredSpots[i].z,&VectoredSpots[i].y);
		fclose(load_file);
		//cb->SendTextMsg("Metal Spots loaded from file",0);
		return true;
	}
	else
		return false;

}

void MetalMap::MakeTGA(unsigned char* data){ //heavily based on Zaphods MakeTGA, thanks!
	// open file
	FILE *fp=fopen("debugVectoredSpots.tga", "wb");
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
		for (int x=0;x<MetalMapWidth;x++){
			out[0]=data[y * MetalMapWidth + x];

			fwrite (out, 1, 1, fp);
			}
	fclose(fp);
}
