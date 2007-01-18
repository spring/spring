#include "MetalMap.h"


CMetalMap::CMetalMap(AIClasses* ai) {
	this -> ai = ai;

	// from 0-255, the minimum percentage of metal a spot needs to have
	// from the maximum to be saved, prevents crappier spots in between taken spaces
	// (they are still perfectly valid and will generate metal mind you!)
	MinMetalForSpot = 50;
	// if more spots than that are found the map is considered a metalmap, tweak this as needed
	MaxSpots = 10000;

	// metal map has 1/2 resolution of normal map
	MetalMapHeight = ai -> cb -> GetMapHeight() / 2;
	MetalMapWidth = ai -> cb -> GetMapWidth() / 2;

	TotalCells = MetalMapHeight * MetalMapWidth;
	XtractorRadius = int(ai -> cb -> GetExtractorRadius() / 16);
	DoubleRadius = XtractorRadius * 2;
	SquareRadius = XtractorRadius * XtractorRadius;
	DoubleSquareRadius = DoubleRadius * DoubleRadius;

	MexArrayA = new unsigned char [TotalCells];
	MexArrayB = new unsigned char [TotalCells];
	// used for drawing the TGA, not really needed with a couple of changes
	MexArrayC = new unsigned char [TotalCells];

	TempAverage = new int [TotalCells];
	TotalMetal = MaxMetal = NumSpotsFound = 0;
	Stopme = false;
	// L("Metal class logging works!");
}

CMetalMap::~CMetalMap() {
	delete[] MexArrayA;
	delete[] MexArrayB;
	delete[] MexArrayC;
	delete[] TempAverage;
}


float3 CMetalMap::GetNearestMetalSpot(int builderid, const UnitDef* extractor) {
	float Tempscore = 0;
	float MaxDivergence = 16;
	float distance;
	float3 spotcoords = ERRORVECTOR;
	float3 bestspot = ERRORVECTOR;

	// L("Getting Metal spot. Ave threat = " << ai -> tm -> GetAverageThreat());
	if (VectoredSpots.size()) {
		int* temparray = new int [MAXUNITS];

		for (unsigned int i = 0; i != VectoredSpots.size(); i++) {
			spotcoords = ai -> cb -> ClosestBuildSite(extractor, VectoredSpots[i], MaxDivergence, 2);

			if (spotcoords.x != -1) {
				distance = spotcoords.distance2D(ai -> cb -> GetUnitPos(builderid)) + 150;
				float mythreat = ai -> tm -> ThreatAtThisPoint(VectoredSpots[i]);
				// L("Spot number " << i << " Threat: " << mythreat);
				float spotscore = VectoredSpots[i].y / distance / (mythreat + 10);

				bool b1 = Tempscore < spotscore;
				bool b2 = ai -> cheat -> GetEnemyUnits(temparray, VectoredSpots[i], XtractorRadius);
				bool b3 = mythreat <= (ai -> tm -> GetAverageThreat() * 1.5);
				bool b4 = ai -> uh -> TaskPlanExist(spotcoords, extractor);

				if (b1 && !b2 && b3 && !b4) {
					Tempscore = spotscore;
					bestspot = spotcoords;
					bestspot.y = VectoredSpots[i].y;
				}
			}
		}

		delete[] temparray;
	}

//	if (Tempscore == 0)
//		L("No spot found: ");

	return bestspot;
}


void CMetalMap::Init() {
	// leave this line if you want to use this class in your AI
	ai -> cb -> SendTextMsg("KAI Metal Class by Krogothe", 0);

	// if there's no available load file, create one and save it
	if (!LoadMetalMap()) {
		GetMetalPoints();
		SaveMetalMap();

		string mapname =  string("Metal - ") + ai -> cb -> GetMapName();
		mapname.resize(mapname.size() - 4);
		// ai -> debug -> MakeBWTGA(MexArrayC, MetalMapWidth, MetalMapHeight, mapname);
	}

	char k[256];
	sprintf(k, "Metal Spots Found %i", NumSpotsFound);
	ai -> cb -> SendTextMsg(k, 0);
}


void CMetalMap::GetMetalPoints() {
	// time stuff
	ai -> math -> TimerStart();

	int* xend = new int[DoubleRadius + 1];

	for (int a = 0; a < DoubleRadius + 1; a++) {
		float z = a - XtractorRadius;
		float floatsqrradius = SquareRadius;
		xend[a] = int(sqrtf(floatsqrradius - z * z));
	}

	// load up the metal values in each pixel
	const unsigned char* metalMapArray = ai -> cb -> GetMetalMap();
	double TotalMetalDouble  = 0;

	for (int i = 0; i < TotalCells; i++) {
		// count the total metal so you can work out an average of the whole map
		TotalMetalDouble +=  MexArrayA[i] = metalMapArray[i];
	}

	// do the average
	AverageMetal = TotalMetalDouble / TotalCells;

	// quick test for no metal map:
	if (TotalMetalDouble < 0.9) {
		// the map doesn't have any metal, just stop
		NumSpotsFound = 0;
		delete[] xend;
		//("Time taken to generate spots: " << ai -> math -> TimerSecs() << " seconds.");
		return;
	}

	// Now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y < MetalMapHeight; y++) {
		for (int x = 0; x < MetalMapWidth; x++) {
			TotalMetal = 0;

			// first spot needs full calculation
			if (x == 0 && y == 0)
				for (int sy = y - XtractorRadius, a = 0;  sy <= y + XtractorRadius;  sy++, a++) {
					if (sy >= 0 && sy < MetalMapHeight){
						for (int sx = x - xend[a]; sx <= x + xend[a]; sx++) {
							if (sx >= 0 && sx < MetalMapWidth) {
								// get the metal from all pixels around the extractor radius
								TotalMetal += MexArrayA[sy * MetalMapWidth + sx];
							}
						} 
					}
				}

			// quick calc test
			if (x > 0) {
				TotalMetal = TempAverage[y * MetalMapWidth + x - 1];
				for (int sy = y - XtractorRadius, a = 0;  sy <= y + XtractorRadius;  sy++, a++) {
					if (sy >= 0 && sy < MetalMapHeight) {
						int addX = x + xend[a];
						int remX = x - xend[a] - 1;

						if (addX < MetalMapWidth)
							TotalMetal += MexArrayA[sy * MetalMapWidth + addX];
						if (remX >= 0)
							TotalMetal -= MexArrayA[sy * MetalMapWidth + remX];
					}
				}
			}
			else if (y > 0) {
				// x == 0 here
				TotalMetal = TempAverage[(y - 1) * MetalMapWidth];
				// remove the top half
				int a = XtractorRadius;

				for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
					if (sx < MetalMapWidth) {
						int remY = y - xend[a] - 1;

						if (remY >= 0)
							TotalMetal -= MexArrayA[remY * MetalMapWidth + sx];
					}
				}

				// add the bottom half
				a = XtractorRadius;

				for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
					if (sx < MetalMapWidth) {
						int addY = y + xend[a];

						if (addY < MetalMapHeight)
							TotalMetal += MexArrayA[addY * MetalMapWidth + sx];
					}
				}

				// comment out for debug
				TotalMetal = TotalMetal;
			}

			//set that spot's metal making ability (divide by cells to values are small)
			TempAverage[y * MetalMapWidth + x] = TotalMetal;

			if (MaxMetal < TotalMetal) {
				// find the spot with the highest metal to set as the map's max
				MaxMetal = TotalMetal;
			}
		}
	}

	// make a list for the distribution of values
	int* valueDist = new int[256];

	for (int i = 0; i < 256; i++) {
		// clear the array (useless?)
		valueDist[i] = 0;
	}

	// this will get the total metal a mex placed at each spot would make
	for (int i = 0; i < TotalCells; i++) {
		// scale the metal so any map will have values 0-255, no matter how much metal it has
		MexArrayB[i] = TempAverage[i] * 255 / MaxMetal;
		// clear out the array since it has never been used
		MexArrayC[i] = 0;

		int value = MexArrayB[i];
		valueDist[value]++;
	}

	// find the current best value
	int bestValue = 0;
	int numberOfValues = 0;
	int usedSpots = 0;

	for (int i = 255; i >= 0; i--) {
		if (valueDist[i] != 0) {
			bestValue = i;
			numberOfValues = valueDist[i];
			break;
		}
	}

	// make a list of the indexes of the best spots
	// (make sure that the list wont be too big)
	if (numberOfValues > 256)
		numberOfValues = 256;

	int* bestSpotList = new int[numberOfValues];

	for (int i = 0; i < TotalCells; i++) {
		if (MexArrayB[i] == bestValue) {
			// add the index of this spot to the list
			bestSpotList[usedSpots] = i;
			usedSpots++;

			if (usedSpots == numberOfValues) {
				// the list is filled, stop the loop
				usedSpots = 0;
				break;
			}
		}
	}

	for (int a = 0; a < MaxSpots; a++) {
		if (!Stopme) {
			// reset tempmetal so it can find new spots
			TempMetal = 0;
			// take the first spot
			int speedTempMetal_x;
			int speedTempMetal_y;
			int speedTempMetal = 0;
			bool found = false;

			while (!found) {
				if (usedSpots == numberOfValues) {
					// the list is empty now, refill it

					// make a list of all the best spots
					for (int i = 0; i < 256; i++) {
						// clear the array
						valueDist[i] = 0;
					}

					// find the metal distribution
					for (int i = 0; i < TotalCells; i++) {
						int value = MexArrayB[i];
						valueDist[value]++;
					}

					// find the current best value
					bestValue = 0;
					numberOfValues = 0;
					usedSpots = 0;

					for (int i = 255; i >= 0; i--) {
						if (valueDist[i] != 0) {
							bestValue = i;
							numberOfValues = valueDist[i];
							break;
						}
					}

					// make a list of the indexes of the best spots
					// (make sure that the list wont be too big)
					if (numberOfValues > 256)
						numberOfValues = 256;

					delete[] bestSpotList;
					bestSpotList = new int[numberOfValues];

					for (int i = 0; i < TotalCells; i++) {
						if (MexArrayB[i] == bestValue) {
							// add the index of this spot to the list
							bestSpotList[usedSpots] = i;
							usedSpots++;

							if (usedSpots == numberOfValues) {
								// the list is filled, stop the loop
								usedSpots = 0;
								break;
							}
						}
					}
				}

				// The list is not empty now.
				int spotIndex = bestSpotList[usedSpots];

				if (MexArrayB[spotIndex] == bestValue) {
					// the spot is still valid, so use it
					speedTempMetal_x = spotIndex % MetalMapWidth;
					speedTempMetal_y = spotIndex / MetalMapWidth;
					speedTempMetal = bestValue;
					found = true;
				}

				// update the bestSpotList index
				usedSpots++;
			}

			coordx = speedTempMetal_x;
			coordy = speedTempMetal_y;
			TempMetal = speedTempMetal;
		}

		if (TempMetal < MinMetalForSpot) {
			// if the spots get too crappy it will stop running the loops to speed it all up
			Stopme = 1;
		}

		if (!Stopme) {
			// format metal coords to game-coords
			BufferSpot.x = coordx * 16 + 8;
			BufferSpot.z = coordy * 16 + 8;
			// gets the actual amount of metal an extractor can make
			BufferSpot.y = TempMetal * (ai -> cb -> GetMaxMetal()) * MaxMetal / 255;
			VectoredSpots.push_back(BufferSpot);

			// plot TGA array (not necessary) for debug
			MexArrayC[coordy * MetalMapWidth + coordx] = TempMetal;
			NumSpotsFound += 1;
			
			// small speedup of "wipes the metal around the spot so its not counted twice"
			for (int sy = coordy - XtractorRadius, a = 0;  sy <= coordy + XtractorRadius;  sy++, a++) {
				if (sy >= 0 && sy < MetalMapHeight) {
					int clearXStart = coordx - xend[a];
					int clearXEnd = coordx + xend[a];

					if (clearXStart < 0)
						clearXStart = 0;
					if (clearXEnd >= MetalMapWidth)
						clearXEnd = MetalMapWidth - 1;

					for (int xClear = clearXStart; xClear <= clearXEnd; xClear++) {
						// wipes the metal around the spot so it's not counted twice
						MexArrayA[sy * MetalMapWidth + xClear] = 0;
						MexArrayB[sy * MetalMapWidth + xClear] = 0;
						TempAverage[sy * MetalMapWidth + xClear] = 0;
					}
				}
			}

			// redo the whole averaging process around the picked spot so other spots can be found around it
			for (int y = coordy - DoubleRadius; y <= coordy + DoubleRadius; y++) {
				if (y >=0 && y < MetalMapHeight) {
					for (int x = coordx - DoubleRadius; x <= coordx + DoubleRadius; x++) {
						if (x >=0 && x < MetalMapWidth) {
							TotalMetal = 0;

							// comment out for debug
							if (x == 0 && y == 0)
								for (int sy = y - XtractorRadius, a = 0;  sy <= y + XtractorRadius;  sy++, a++) {
									if (sy >= 0 && sy < MetalMapHeight) {
										for (int sx = x - xend[a]; sx <= x + xend[a]; sx++) {
											if (sx >= 0 && sx < MetalMapWidth) {
												// get the metal from all pixels around the extractor radius
												TotalMetal += MexArrayA[sy * MetalMapWidth + sx];
											}
										}
									}
								}

							// quick calc test
							if (x > 0) {
								TotalMetal = TempAverage[y * MetalMapWidth + x - 1];

								for (int sy = y - XtractorRadius, a = 0;  sy <= y + XtractorRadius;  sy++, a++) {
									if (sy >= 0 && sy < MetalMapHeight) {
										int addX = x + xend[a];
										int remX = x - xend[a] - 1;

										if (addX < MetalMapWidth)
											TotalMetal += MexArrayA[sy * MetalMapWidth + addX];
										if (remX >= 0)
											TotalMetal -= MexArrayA[sy * MetalMapWidth + remX];
									}
								}
							}
							else if (y > 0) {
								// x == 0 here
								TotalMetal = TempAverage[(y - 1) * MetalMapWidth];
								// remove the top half
								int a = XtractorRadius;

								for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
									if (sx < MetalMapWidth) {
										int remY = y - xend[a] - 1;

										if (remY >= 0)
											TotalMetal -= MexArrayA[remY * MetalMapWidth + sx];
									}
								}

								// add the bottom half
								a = XtractorRadius;

								for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
									if (sx < MetalMapWidth) {
										int addY = y + xend[a];

										if (addY < MetalMapHeight)
											TotalMetal += MexArrayA[addY * MetalMapWidth + sx];
									}
								}
							}

							TempAverage[y * MetalMapWidth + x] = TotalMetal;
							// set that spot's metal amount
							MexArrayB[y * MetalMapWidth + x] = TotalMetal * 255 / MaxMetal;
						}
					}
				}
			}
		}
	}

	// kill the lists
	delete[] bestSpotList;
	delete[] valueDist;
	delete[] xend;

	// 0.95 used for for reliability
	if (NumSpotsFound > MaxSpots * 0.95) {
		ai -> cb -> SendTextMsg("Metal Map Found", 0);
	}

	// L("Time taken to generate spots: " << ai -> math -> TimerSecs() << " seconds.");
	// ai -> cb -> SendTextMsg(c, 0);
}


void CMetalMap::SaveMetalMap() {
	string filename = string(METALFOLDER) + string(ai -> cb -> GetMapName());
	filename.resize(filename.size() - 3);
	filename += string("Metal");

	char filename_buf[1024];
	strcpy(filename_buf, filename.c_str());
	ai -> cb -> GetValue(AIVAL_LOCATE_FILE_W, filename_buf);

	FILE* save_file = fopen(filename_buf, "wb");
	fwrite(&NumSpotsFound, sizeof(int), 1, save_file);

	// L("Spots found: " << NumSpotsFound << " AverageMetal: " << AverageMetal);
	fwrite(&AverageMetal, sizeof(float), 1, save_file);

	for (int i = 0; i < NumSpotsFound; i++) {
		// L("Loaded i: " << i << ", x; " << VectoredSpots[i].x << ", y; " << VectoredSpots[i].z << ", value: " << VectoredSpots[i].y);
		fwrite(&VectoredSpots[i], sizeof(float3), 1, save_file);
	}

	fclose(save_file);
	ai -> cb -> SendTextMsg("Metal Spots created and saved!", 0);
}


bool CMetalMap::LoadMetalMap() {
	string filename = string(METALFOLDER) + string(ai -> cb -> GetMapName());
	filename.resize(filename.size() - 3);
	filename += string("Metal");

	char filename_buf[1024];
	strcpy(filename_buf, filename.c_str());
	ai -> cb -> GetValue(AIVAL_LOCATE_FILE_R, filename_buf);

	FILE* load_file;

	// load Spots if file exists
	if ((load_file = fopen(filename_buf, "rb"))) {
		fread(&NumSpotsFound, sizeof(int), 1, load_file);
		VectoredSpots.resize(NumSpotsFound);
		fread(&AverageMetal, sizeof(float), 1, load_file);

		for (int i = 0; i < NumSpotsFound; i++) {
			fread(&VectoredSpots[i], sizeof(float3), 1, load_file);
			// L("Loaded i: " << i << ", x; " << VectoredSpots[i].x << ", y; " << VectoredSpots[i].z << ", value: " << VectoredSpots[i].y);
		}

		fclose(load_file);
		ai -> cb -> SendTextMsg("Metal Spots loaded from file", 0);
		return true;
	}

	return false;
}
