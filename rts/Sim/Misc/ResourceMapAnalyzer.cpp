/*
	Copyright (c) 2005 Krogothe

	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	@author Krogothe
	@author Kloot
	@author hoijui
*/

#include <string>
#include <cstdio>

using std::fopen;
using std::fread;
using std::fwrite;
using std::fclose;

#include "ResourceMapAnalyzer.h"

#include "Sim/Misc/ResourceHandler.h"
#include "Sim/Misc/Resource.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "FileSystem/FileSystem.h"
#include "LogOutput.h"


static const float3 ERRORVECTOR(-1, 0, 0);
static std::string CACHE_BASE("");

CResourceMapAnalyzer::CResourceMapAnalyzer(int ResourceId):
	ResourceId(ResourceId),
	VectoredSpots()
{
	if (CACHE_BASE.empty()) {
		CACHE_BASE = filesystem.LocateDir("cache/analyzedResourceMaps/", FileSystem::WRITE | FileSystem::CREATE_DIRS);
	}

	// from 0-255, the minimum percentage of resources a spot needs to have from
	// the maximum to be saved, prevents crappier spots in between taken spaces
	// (they are still perfectly valid and will generate resources mind you!)
	MinIncomeForSpot = 50;
	// if more spots than that are found the map is considered a resource-map (eg. speed-metal), tweak this as needed
	MaxSpots = 10000;

	const CResource* resource = resourceHandler->GetResource(ResourceId);
	ExtractorRadius = resource->extractorRadius;

	// resource-map has 1/2 resolution of normal map
	MapHeight = resourceHandler->GetResourceMapWidth(ResourceId);
	MapWidth = resourceHandler->GetResourceMapHeight(ResourceId);

	TotalCells = MapHeight * MapWidth;
	XtractorRadius = static_cast<int>(ExtractorRadius / 16);
	DoubleRadius = XtractorRadius * 2;
	SquareRadius = XtractorRadius * XtractorRadius;
	DoubleSquareRadius = DoubleRadius * DoubleRadius;

	RexArrayA = new unsigned char [TotalCells];
	RexArrayB = new unsigned char [TotalCells];
	// used for drawing the TGA, not really needed with a couple of changes
	RexArrayC = new unsigned char [TotalCells];

	TempAverage = new int [TotalCells];
	TotalResources = MaxResource = NumSpotsFound = 0;
	StopMe = false;

	Init();
}

CResourceMapAnalyzer::~CResourceMapAnalyzer() {
	delete[] RexArrayA;
	delete[] RexArrayB;
	delete[] RexArrayC;
	delete[] TempAverage;
}



float3 CResourceMapAnalyzer::GetNearestSpot(int builderUnitId, const UnitDef* extractor) const {

	CUnit* builder = uh->units[builderUnitId];

	if (builder == NULL) {
		logOutput.Print("Invalid Unit ID: %i", builderUnitId);
		return ERRORVECTOR;
	}

	return GetNearestSpot(builder->midPos, builder->team, extractor);
}

// KLOOTNOTE: this needs to ignore spots already taken by allies
float3 CResourceMapAnalyzer::GetNearestSpot(float3 fromPos, int team, const UnitDef* extractor) const {
	float TempScore = 0.0f;
	float MaxDivergence = 16.0f;
	float3 spotCoords = ERRORVECTOR;
	float3 bestSpot = ERRORVECTOR;

	if (VectoredSpots.size()) {
		for (unsigned int i = 0; i != VectoredSpots.size(); i++) {
			if (extractor != NULL) {
				spotCoords = helper->ClosestBuildSite(team, extractor, VectoredSpots[i], MaxDivergence, 2);
			} else {
				spotCoords = VectoredSpots[i];
			}

			if (spotCoords.x >= 0.0f) {
				float distance = spotCoords.distance2D(fromPos) + 150;
				//float myThreat = ai->tm->ThreatAtThisPoint(VectoredSpots[i]);
				float spotScore = VectoredSpots[i].y / distance/* / (myThreat + 10)*/;
				//int numEnemies = ai->cheat->GetEnemyUnits(&ai->unitIDs[0], VectoredSpots[i], XtractorRadius * 2);

				// NOTE: threat at VectoredSpots[i] is determined
				// by presence of ARMED enemy units or buildings
				bool b1 = (TempScore < spotScore);
// 				bool b2 = (numEnemies == 0);
// 				bool b3 = (myThreat <= (ai->tm->GetAverageThreat() * 1.5));
// 				bool b4 = (ai->uh->TaskPlanExist(spotCoords, extractor));

				if (b1/* && b2 && b3 && !b4*/) {
					TempScore = spotScore;
					bestSpot = spotCoords;
					bestSpot.y = VectoredSpots[i].y;
				}
			}
		}
	}

	// no spot found if TempScore is zero
	return bestSpot;
}


void CResourceMapAnalyzer::Init() {

	// leave this line if you want to use this class
	const CResource* resource = resourceHandler->GetResource(ResourceId);
	logOutput.Print("ResourceMapAnalyzer by Krogothe, initialized for resource %i(%s)",
			ResourceId, resource->name.c_str());

	// if there's no available load file, create one and save it
	if (!LoadResourceMap()) {
		GetResourcePoints();
		SaveResourceMap();
	}

	// "Resource Spots Found: %i", NumSpotsFound);
}

float CResourceMapAnalyzer::GetAverageIncome() const {
	return AverageIncome;
}

const std::vector<float3>& CResourceMapAnalyzer::GetSpots() const {
	return VectoredSpots;
}

void CResourceMapAnalyzer::GetResourcePoints() {
	int* xend = new int[DoubleRadius + 1];

	for (int a = 0; a < DoubleRadius + 1; a++) {
		float z = a - XtractorRadius;
		float floatsqrradius = SquareRadius;
		xend[a] = int(streflop::sqrtf(floatsqrradius - z * z));
	}

	// load up the resource values in each pixel
	const unsigned char* resourceMapArray = resourceHandler->GetResourceMap(ResourceId);
	double TotalResourcesDouble  = 0;

	for (int i = 0; i < TotalCells; i++) {
		// count the total resources so you can work out an average of the whole map
		TotalResourcesDouble +=  RexArrayA[i] = resourceMapArray[i];
	}

	// do the average
	AverageIncome = TotalResourcesDouble / TotalCells;

	// quick test for no-resources-map:
	if (TotalResourcesDouble < 0.9) {
		// the map does not have any resource, just stop
		NumSpotsFound = 0;
		delete[] xend;
		return;
	}

	// Now work out how much resources each spot can make by adding up the resources from nearby spots
	for (int y = 0; y < MapHeight; y++) {
		for (int x = 0; x < MapWidth; x++) {
			TotalResources = 0;

			// first spot needs full calculation
			if (x == 0 && y == 0)
				for (int sy = y - XtractorRadius, a = 0;  sy <= y + XtractorRadius;  sy++, a++) {
					if (sy >= 0 && sy < MapHeight){
						for (int sx = x - xend[a]; sx <= x + xend[a]; sx++) {
							if (sx >= 0 && sx < MapWidth) {
								// get the resources from all pixels around the extractor radius
								TotalResources += RexArrayA[sy * MapWidth + sx];
							}
						}
					}
				}

			// quick calc test
			if (x > 0) {
				TotalResources = TempAverage[y * MapWidth + x - 1];
				for (int sy = y - XtractorRadius, a = 0;  sy <= y + XtractorRadius;  sy++, a++) {
					if (sy >= 0 && sy < MapHeight) {
						int addX = x + xend[a];
						int remX = x - xend[a] - 1;

						if (addX < MapWidth)
							TotalResources += RexArrayA[sy * MapWidth + addX];
						if (remX >= 0)
							TotalResources -= RexArrayA[sy * MapWidth + remX];
					}
				}
			}
			else if (y > 0) {
				// x == 0 here
				TotalResources = TempAverage[(y - 1) * MapWidth];
				// remove the top half
				int a = XtractorRadius;

				for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
					if (sx < MapWidth) {
						int remY = y - xend[a] - 1;

						if (remY >= 0)
							TotalResources -= RexArrayA[remY * MapWidth + sx];
					}
				}

				// add the bottom half
				a = XtractorRadius;

				for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
					if (sx < MapWidth) {
						int addY = y + xend[a];

						if (addY < MapHeight)
							TotalResources += RexArrayA[addY * MapWidth + sx];
					}
				}

				// comment out for debug
				TotalResources = TotalResources;
			}

			// set that spot's resource making ability (divide by cells to values are small)
			TempAverage[y * MapWidth + x] = TotalResources;

			if (MaxResource < TotalResources) {
				// find the spot with the highest resource value to set as the map's max
				MaxResource = TotalResources;
			}
		}
	}

	// make a list for the distribution of values
	int* valueDist = new int[256];

	for (int i = 0; i < 256; i++) {
		// clear the array (useless?)
		valueDist[i] = 0;
	}

	// this will get the total resources a rex placed at each spot would make
	for (int i = 0; i < TotalCells; i++) {
		// scale the resources so any map will have values 0-255, no matter how much resources it has
		RexArrayB[i] = TempAverage[i] * 255 / MaxResource;
		// clear out the array since it has never been used
		RexArrayC[i] = 0;

		int value = RexArrayB[i];
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
		if (RexArrayB[i] == bestValue) {
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
		if (!StopMe) {
			// reset temporary resources so it can find new spots
			TempResources = 0;
			// take the first spot
			int speedTempResources_x = 0;
			int speedTempResources_y = 0;
			int speedTempResources = 0;
			bool found = false;

			while (!found) {
				if (usedSpots == numberOfValues) {
					// the list is empty now, refill it

					// make a list of all the best spots
					for (int i = 0; i < 256; i++) {
						// clear the array
						valueDist[i] = 0;
					}

					// find the resource distribution
					for (int i = 0; i < TotalCells; i++) {
						int value = RexArrayB[i];
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
						if (RexArrayB[i] == bestValue) {
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

				if (RexArrayB[spotIndex] == bestValue) {
					// the spot is still valid, so use it
					speedTempResources_x = spotIndex % MapWidth;
					speedTempResources_y = spotIndex / MapWidth;
					speedTempResources = bestValue;
					found = true;
				}

				// update the bestSpotList index
				usedSpots++;
			}

			coordX = speedTempResources_x;
			coordZ = speedTempResources_y;
			TempResources = speedTempResources;
		}

		if (TempResources < MinIncomeForSpot) {
			// if the spots get too crappy it will stop running the loops to speed it all up
			StopMe = 1;
		}

		if (!StopMe) {
			// format resource coords to game-coords
			BufferSpot.x = coordX * 16 + 8;
			BufferSpot.z = coordZ * 16 + 8;
			// gets the actual amount of resource an extractor can make
			const CResource* resource = resourceHandler->GetResource(ResourceId);
			BufferSpot.y = TempResources * (resource->maxWorth) * MaxResource / 255;
			VectoredSpots.push_back(BufferSpot);

			// plot TGA array (not necessary) for debug
			RexArrayC[coordZ * MapWidth + coordX] = TempResources;
			NumSpotsFound += 1;

			// small speedup of "wipes the resources around the spot so it is not counted twice"
			for (int sy = coordZ - XtractorRadius, a = 0;  sy <= coordZ + XtractorRadius;  sy++, a++) {
				if (sy >= 0 && sy < MapHeight) {
					int clearXStart = coordX - xend[a];
					int clearXEnd = coordX + xend[a];

					if (clearXStart < 0)
						clearXStart = 0;
					if (clearXEnd >= MapWidth)
						clearXEnd = MapWidth - 1;

					for (int xClear = clearXStart; xClear <= clearXEnd; xClear++) {
						// wipes the resources around the spot so it is not counted twice
						RexArrayA[sy * MapWidth + xClear] = 0;
						RexArrayB[sy * MapWidth + xClear] = 0;
						TempAverage[sy * MapWidth + xClear] = 0;
					}
				}
			}

			// redo the whole averaging process around the picked spot so other spots can be found around it
			for (int y = coordZ - DoubleRadius; y <= coordZ + DoubleRadius; y++) {
				if (y >=0 && y < MapHeight) {
					for (int x = coordX - DoubleRadius; x <= coordX + DoubleRadius; x++) {
						if (x >=0 && x < MapWidth) {
							TotalResources = 0;

							// comment out for debug
							if (x == 0 && y == 0)
								for (int sy = y - XtractorRadius, a = 0;  sy <= y + XtractorRadius;  sy++, a++) {
									if (sy >= 0 && sy < MapHeight) {
										for (int sx = x - xend[a]; sx <= x + xend[a]; sx++) {
											if (sx >= 0 && sx < MapWidth) {
												// get the resources from all pixels around the extractor radius
												TotalResources += RexArrayA[sy * MapWidth + sx];
											}
										}
									}
								}

							// quick calc test
							if (x > 0) {
								TotalResources = TempAverage[y * MapWidth + x - 1];

								for (int sy = y - XtractorRadius, a = 0;  sy <= y + XtractorRadius;  sy++, a++) {
									if (sy >= 0 && sy < MapHeight) {
										int addX = x + xend[a];
										int remX = x - xend[a] - 1;

										if (addX < MapWidth)
											TotalResources += RexArrayA[sy * MapWidth + addX];
										if (remX >= 0)
											TotalResources -= RexArrayA[sy * MapWidth + remX];
									}
								}
							}
							else if (y > 0) {
								// x == 0 here
								TotalResources = TempAverage[(y - 1) * MapWidth];
								// remove the top half
								int a = XtractorRadius;

								for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
									if (sx < MapWidth) {
										int remY = y - xend[a] - 1;

										if (remY >= 0)
											TotalResources -= RexArrayA[remY * MapWidth + sx];
									}
								}

								// add the bottom half
								a = XtractorRadius;

								for (int sx = 0; sx <= XtractorRadius;  sx++, a++) {
									if (sx < MapWidth) {
										int addY = y + xend[a];

										if (addY < MapHeight)
											TotalResources += RexArrayA[addY * MapWidth + sx];
									}
								}
							}

							TempAverage[y * MapWidth + x] = TotalResources;
							// set that spot's resource amount
							RexArrayB[y * MapWidth + x] = TotalResources * 255 / MaxResource;
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

	// 0.95 used for reliability
	// bool isResourceMap = (NumSpotsFound > MaxSpots * 0.95);
}


void CResourceMapAnalyzer::SaveResourceMap() {
	std::string map = GetCacheFileName();
	FILE* saveFile = fopen(map.c_str(), "wb");

	assert(saveFile != NULL);

	fwrite(&NumSpotsFound, sizeof(int), 1, saveFile);
	fwrite(&AverageIncome, sizeof(float), 1, saveFile);

	for (int i = 0; i < NumSpotsFound; i++) {
		fwrite(&VectoredSpots[i], sizeof(float3), 1, saveFile);
	}

	fclose(saveFile);
}

bool CResourceMapAnalyzer::LoadResourceMap() {
	std::string map = GetCacheFileName();
	FILE* loadFile = fopen(map.c_str(), "rb");

	if (loadFile != NULL) {
		fread(&NumSpotsFound, sizeof(int), 1, loadFile);
		VectoredSpots.resize(NumSpotsFound);
		fread(&AverageIncome, sizeof(float), 1, loadFile);

		for (int i = 0; i < NumSpotsFound; i++) {
			fread(&VectoredSpots[i], sizeof(float3), 1, loadFile);
		}

		fclose(loadFile);
		return true;
	}

	return false;
}


std::string CResourceMapAnalyzer::GetCacheFileName() const {

	const CResource* resource = resourceHandler->GetResource(ResourceId);
	std::string absFile = CACHE_BASE + gameSetup->mapName + resource->name;

	return absFile;
}
