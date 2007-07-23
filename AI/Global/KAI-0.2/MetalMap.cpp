#include "MetalMap.h"


CMetalMap::CMetalMap(AIClasses* ai) {
	this -> ai = ai;

	// from 0-255, the minimum percentage of metal a spot needs to have
	// from the maximum to be saved to prevent crappier spots in between taken spaces
	// (they are still perfectly valid and will generate metal mind you!)
	MinMetalForSpot = 64;
	// if more spots than that are found the map is considered a metalmap, tweak this as needed
	MaxSpots = 5000;

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
	MexArrayD = new unsigned char [TotalCells];

	TempAverage = new int [TotalCells];
	TotalMetal = MaxMetal = NumSpotsFound = 0;
	Stopme = false;

	L("Metal class logging works!");
}

CMetalMap::~CMetalMap() {
	delete[] MexArrayA;
	delete[] MexArrayB;
	delete[] MexArrayC;
	delete[] MexArrayD;
	delete[] TempAverage;
}



float3 CMetalMap::GetNearestMetalSpot(int builderid, const UnitDef* extractor) {
	if (extractor == 0x00000000) {
		L("CMetalMap::GetNearestMetalSpot called with parameters builderid:"<<builderid<<" extractor:"<<extractor);
		return ERRORVECTOR;
	}

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
				bool b2 = ai -> cheat -> GetEnemyUnits(temparray, VectoredSpots[i],XtractorRadius)==0;
				bool b3 = mythreat <= ai -> tm -> GetAverageThreat() * 1.3;
				bool b4 = !ai -> uh -> TaskPlanExist(spotcoords, extractor);

				if (b1 && b2 && b3 && b4) {
					Tempscore = spotscore;
					bestspot = spotcoords;
					bestspot.y = VectoredSpots[i].y;
				}
			}
		}

		delete[] temparray;
	}
	else if (AverageMetal > 0) {
		spotcoords = ai -> cb -> ClosestBuildSite(extractor, ai -> cb -> GetUnitPos(builderid), DEFCBS_RADIUS, DEFCBS_SEPARATION);
	}

	if (Tempscore == 0)
		L("No spot found");

	return bestspot;
}


int CMetalMap::FindMetalSpotUpgrade(int builderid, const UnitDef* extractor) {
	float Tempscore = 0;
//	float distance;
//	float3 spotcoords = ERRORVECTOR;
	int bestspot = -1;
	float3 bestfreemetalpos = GetNearestMetalSpot(builderid, extractor);
	float bestfreemetal = bestfreemetalpos.y;
	float bestfreedistance = ai->uh->Distance2DToNearestFactory(bestfreemetalpos.x,bestfreemetalpos.z);
	if (bestfreedistance>DEFCBS_RADIUS) bestfreemetal/=10;

	if (VectoredSpots.size()) {
		int* temparray = new int [MAXUNITS];

		for (list<int>::iterator i = ai -> uh -> AllUnitsByCat[CAT_MEX] . begin(); i != ai -> uh -> AllUnitsByCat[CAT_MEX] . end(); i++) {
			float3 spotcoords = ai -> MyUnits[*i] -> pos();
			if (spotcoords.x == -1) continue;
			float oldextracts = ai -> cb -> GetUnitDef(*i) -> extractsMetal;
			float newextracts = extractor -> extractsMetal;
			if (newextracts<oldextracts*1.2) continue;
			if (ai -> uh -> TaskPlanExist(spotcoords, extractor)) continue;
			float metalatpos = GetMetalAtThisPoint(spotcoords);
			L("Metal at upgrade "<<metalatpos<<"At "<<spotcoords.x<<"."<<spotcoords.z<<" (free "<<bestfreemetal<<")");
			float dist = spotcoords.distance2D(ai -> cb -> GetUnitPos(builderid));
			if (dist>DEFCBS_RADIUS) metalatpos/=10;
			float addmetal = metalatpos*(newextracts-oldextracts);
			float spotscore = addmetal;
			if (Tempscore<spotscore) {
				Tempscore = spotscore;
				bestspot = *i;
			}
/*
			distance = sqrt(spotcoords.distance2D(ai -> cb -> GetUnitPos(builderid)) + 150);
			// L("Spot number " << i << " Threat: " << mythreat);
			if (extractor -> extractsMetal/ai -> cb -> GetUnitDef(*i) -> extractsMetal<1.2) continue;
			float metaldifference = extractor -> extractsMetal - ai -> cb -> GetUnitDef(*i) -> extractsMetal;
			float spotscore = (GetMetalAtThisPoint(spotcoords) * metaldifference) / sqrt(distance + 500);

			bool b1 = Tempscore < spotscore;
			bool b2 = ai -> uh -> TaskPlanExist(spotcoords, extractor);
			bool b3 = bestfreemetal < (GetMetalAtThisPoint(spotcoords) * (metaldifference / extractor -> extractsMetal));

			if (b1 && !b2 && b3) {
				Tempscore = spotscore;
				bestspot = *i;
			}
*/
		}

		delete[] temparray;
	}

	if (Tempscore <= 0)
		L("No spot found for upgrade");
	else {
		// draw green arrow pointing upward above metal extractor
//		spotcoords = ai -> cb -> GetUnitPos(bestspot);
//		ai -> cb -> CreateLineFigure(spotcoords, float3(spotcoords.x, spotcoords.y + 100, spotcoords.z), 10, 1, 10000, 98);
	}
	return bestspot;
}


float CMetalMap::GetMetalAtThisPoint(float3 pos) {
	int x = int((pos.x - 7) / METALMAP2POS);
	int y = int((pos.z - 7) / METALMAP2POS);
	return MexArrayD[y * MetalMapWidth + x] * (ai -> cb -> GetMaxMetal());
}


void CMetalMap::Init() {
	// Leave this line if you want to use this class in your AI
	ai -> cb -> SendTextMsg("KAI Metal Class by Krogothe", 0);

	// if theres no available load file, create one and save it
	if (!LoadMetalMap()) {
		GetMetalPoints();
		SaveMetalMap();
		string mapname =  string("Metal - ") + ai -> cb -> GetMapName();
		mapname.resize(mapname.size() - 4);
		// KLOOTNOTE: this does show metalspots!
		ai -> debug -> MakeBWTGA(MexArrayC, MetalMapWidth, MetalMapHeight, mapname);
	}

	// KLOOTNOTE: this is alledgedly zero
	char k[256];
	sprintf(k, "Metal Spots Found %i", NumSpotsFound);
	ai -> cb -> SendTextMsg(k, 0);
}


void CMetalMap::GetMetalPoints() {
	// Time stuff:
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

	// Quick test for no metal map:
	if (TotalMetalDouble < 0.9) {
		// The map doesn't have any metal, just stop
		NumSpotsFound = 0;
		delete[] xend;
		L("Time taken to generate spots: " << ai -> math -> TimerSecs() << " seconds.");
		return;
	}

	// now work out how much metal each spot can make by adding up the metal from nearby spots
	for (int y = 0; y < MetalMapHeight; y++) {
		for (int x = 0; x < MetalMapWidth; x++) {
			TotalMetal = 0;

			// first spot needs full calculation
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
						int addX = x+xend[a];
						int remX = x-xend[a] - 1;

						if (addX < MetalMapWidth)
							TotalMetal += MexArrayA[sy * MetalMapWidth + addX];
						if (remX >= 0)
							TotalMetal -= MexArrayA[sy * MetalMapWidth + remX];
					}
				}
			}
			else if (y > 0) {
				// x == 0 here
				TotalMetal = TempAverage[(y-1) * MetalMapWidth];
				// remove the top half
				int a = XtractorRadius;
				for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
					if (sx < MetalMapWidth) {
						int remY = y-xend[a] - 1;

						if (remY >= 0)
							TotalMetal -= MexArrayA[remY * MetalMapWidth + sx];
					}
				}

				// add the bottom half
				a = XtractorRadius;
				for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
					if (sx < MetalMapWidth) {
						int addY = y+xend[a];

						if (addY < MetalMapHeight)
							TotalMetal += MexArrayA[addY * MetalMapWidth + sx];
					}
				}

				// comment out for debug
				TotalMetal = TotalMetal;
			}

			// set that spots metal making ability (divide by cells to values are small)
			TempAverage[y * MetalMapWidth + x] = TotalMetal;

			if (MaxMetal < TotalMetal) {
				// find the spot with the highest metal to set as the map's max
				MaxMetal = TotalMetal;
			}
		}
	}


	// make a list for the distribution of values
	int * valueDist = new int[256];
	for (int i = 0; i < 256; i++) {
		// clear the array (useless?)
		valueDist[i] = 0;
	}

	// this will get the total metal a mex placed at each spot would make
	for (int i = 0; i < TotalCells; i++) {
		// scale the metal so any map will have values 0-255, no matter how much metal it has
		MexArrayB[i] = TempAverage[i] * 255 / MaxMetal;
		MexArrayD[i] = TempAverage[i];
		// clear out this array since it has never been used
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
	// (make sure that the list won't be too big)
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
						if (MexArrayB[i] == bestValue){
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

				// the list is not empty now
				int spotIndex = bestSpotList[usedSpots];

				if (MexArrayB[spotIndex] == bestValue) {
					// the spot is still valid, so use it
					speedTempMetal_x = spotIndex%MetalMapWidth;
					speedTempMetal_y = spotIndex/MetalMapWidth;
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

			// small speedup of "wipes the metal around the spot so it's not counted twice"
			for (int sy = coordy - XtractorRadius, a = 0;  sy <= coordy + XtractorRadius;  sy++, a++) {
				if (sy >= 0 && sy < MetalMapHeight) {
					int clearXStart = coordx - xend[a];
					int clearXEnd = coordx + xend[a];

					if (clearXStart < 0)
						clearXStart = 0;
					if (clearXEnd >= MetalMapWidth)
						clearXEnd = MetalMapWidth - 1;

					for (int xClear = clearXStart; xClear <= clearXEnd; xClear++) {
						// wipes the metal around the spot so its not counted twice
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

							// Comment out for debug
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
										int addX = x+xend[a];
										int remX = x-xend[a] - 1;

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

								for (int sx = 0; sx <= XtractorRadius;  sx++, a++){
									if (sx < MetalMapWidth) {
										int remY = y-xend[a] - 1;

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
	// KLOOTNOTE: disabled because AI
	// regards all maps as metal maps!

// 	if (NumSpotsFound > (MaxSpots * 0.95)) {
// 		ai -> cb -> SendTextMsg("Metal Map Found", 0);
// 		// no point in saving spots if the map is a metalmap (!)
// 		NumSpotsFound = 0;
// 		VectoredSpots.clear();
// 	}

	L ("Time taken to generate spots: " << ai -> math -> TimerSecs() << " seconds.");
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
	L("Spots found: " << NumSpotsFound << " AverageMetal: " << AverageMetal);
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

	// load spots if file exists
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
