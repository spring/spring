/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"

#include <stdexcept>

static constexpr float3 ERRORVECTOR(-1, 0, 0);
static std::string CACHE_BASE("");

CResourceMapAnalyzer::CResourceMapAnalyzer(int resourceId)
	: resourceId(resourceId)
	, numSpotsFound(-1)

	, extractorRadius(-1.0f)
	, averageIncome(0.0f)

	, stopMe(false)

	, maxSpots(10000)
	, mapHeight(0)
	, mapWidth(0)
	, totalCells(0)
	, squareRadius(0)
	, doubleSquareRadius(0)
	, totalResources(0)
	, maxResource(0)
	, tempResources(0)
	, coordX(0)
	, coordZ(0)
	, minIncomeForSpot(50)
	, xtractorRadius(0)
	, doubleRadius(0)
{
	if (CACHE_BASE.empty())
		CACHE_BASE = dataDirsAccess.LocateDir(FileSystem::GetCacheDir() + "/analyzedResourceMaps/", FileQueryFlags::WRITE | FileQueryFlags::CREATE_DIRS);
}



float3 CResourceMapAnalyzer::GetNearestSpot(int builderUnitId, const UnitDef* extractor) const {

	const CUnit* builder = unitHandler.GetUnit(builderUnitId);

	if (builder == nullptr) {
		LOG_L(L_WARNING, "GetNearestSpot: Invalid unit ID: %i", builderUnitId);
		return ERRORVECTOR;
	}

	return GetNearestSpot(builder->midPos, builder->team, extractor);
}

float3 CResourceMapAnalyzer::GetNearestSpot(float3 fromPos, int team, const UnitDef* extractor) const {

	float tempScore = 0.0f;
	float maxDivergence = 16.0f;
	float3 spotCoords = ERRORVECTOR;
	float3 bestSpot = ERRORVECTOR;

	for (size_t i = 0; i < vectoredSpots.size(); ++i) {
		if (extractor != nullptr) {
			spotCoords = CGameHelper::ClosestBuildPos(team, extractor, vectoredSpots[i], maxDivergence, 2);
		} else {
			spotCoords = vectoredSpots[i];
		}

		if (spotCoords.x >= 0.0f) {
			float distance = spotCoords.distance2D(fromPos) + 150;
			//float myThreat = ai->tm->ThreatAtThisPoint(vectoredSpots[i]);
			float spotScore = vectoredSpots[i].y / distance/* / (myThreat + 10)*/;
			//int numEnemies = ai->cheat->GetEnemyUnits(&ai->unitIDs[0], vectoredSpots[i], xtractorRadius * 2);

			// NOTE: threat at vectoredSpots[i] is determined
			// by presence of ARMED enemy units or buildings
			bool b1 = (tempScore < spotScore);
// 			bool b2 = (numEnemies == 0);
// 			bool b3 = (myThreat <= (ai->tm->GetAverageThreat() * 1.5));
// 			bool b4 = (ai->unitHandler.TaskPlanExist(spotCoords, extractor));

			if (b1/* && b2 && b3 && !b4*/) {
				tempScore = spotScore;
				bestSpot = spotCoords;
				bestSpot.y = vectoredSpots[i].y;
			}
		}
	}

	// no spot found if tempScore is zero
	return bestSpot;
}


void CResourceMapAnalyzer::Init() {
	const CResourceDescription* resource = resourceHandler->GetResource(resourceId);

	mapWidth = resourceHandler->GetResourceMapWidth(resourceId);
	mapHeight = resourceHandler->GetResourceMapHeight(resourceId);

	totalCells = mapHeight * mapWidth;
	extractorRadius = resource->extractorRadius;
	xtractorRadius = static_cast<int>(extractorRadius / (SQUARE_SIZE * 2));
	doubleRadius = xtractorRadius * 2;
	squareRadius = xtractorRadius * xtractorRadius;
	doubleSquareRadius = doubleRadius * doubleRadius;

	rexArrayA.resize(totalCells);
	rexArrayB.resize(totalCells);
	// used for drawing the TGA, not really needed with a couple of changes
	rexArrayC.resize(totalCells);

	tempAverage.resize(totalCells);

	// if there's no available load file, create one and save it
	if (!LoadResourceMap()) {
		GetResourcePoints();
		SaveResourceMap();
	}
}

float CResourceMapAnalyzer::GetAverageIncome() const {
	return averageIncome;
}


void CResourceMapAnalyzer::GetResourcePoints() {
	std::vector<int> xend(doubleRadius + 1);

	for (int a = 0; a < doubleRadius + 1; a++) {
		float z = a - xtractorRadius;
		float floatsqrradius = squareRadius;
		xend[a] = int(math::sqrt(floatsqrradius - z * z));
	}

	// load up the resource values in each pixel
	const unsigned char* resourceMapArray = resourceHandler->GetResourceMap(resourceId);
	double totalResourcesDouble  = 0;

	for (int i = 0; i < totalCells; i++) {
		// count the total resources so you can work out
		// an average of the whole map
		totalResourcesDouble +=  rexArrayA[i] = resourceMapArray[i];
	}

	// do the average
	averageIncome = totalResourcesDouble / totalCells;
	numSpotsFound = 0;

	// if the map does not have any resource (quick test), just stop
	if (totalResourcesDouble < 0.9)
		return;

	// Now work out how much resources each spot can make
	// by adding up the resources from nearby spots
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			totalResources = 0;

			// first spot needs full calculation
			if (x == 0 && y == 0)
				for (int sy = y - xtractorRadius, a = 0;  sy <= y + xtractorRadius;  sy++, a++) {
					if (sy >= 0 && sy < mapHeight){
						for (int sx = x - xend[a]; sx <= x + xend[a]; sx++) {
							if (sx >= 0 && sx < mapWidth) {
								// get the resources from all pixels around the extractor radius
								totalResources += rexArrayA[sy * mapWidth + sx];
							}
						}
					}
				}

			// quick calc test
			if (x > 0) {
				totalResources = tempAverage[y * mapWidth + x - 1];
				for (int sy = y - xtractorRadius, a = 0;  sy <= y + xtractorRadius;  sy++, a++) {
					if (sy >= 0 && sy < mapHeight) {
						const int addX = x + xend[a];
						const int remX = x - xend[a] - 1;

						if (addX < mapWidth) {
							totalResources += rexArrayA[sy * mapWidth + addX];
						}
						if (remX >= 0) {
							totalResources -= rexArrayA[sy * mapWidth + remX];
						}
					}
				}
			} else if (y > 0) {
				// x == 0 here
				totalResources = tempAverage[(y - 1) * mapWidth];
				// remove the top half
				int a = xtractorRadius;

				for (int sx = 0; sx <= xtractorRadius;  sx++, a++) {
					if (sx < mapWidth) {
						const int remY = y - xend[a] - 1;

						if (remY >= 0) {
							totalResources -= rexArrayA[remY * mapWidth + sx];
						}
					}
				}

				// add the bottom half
				a = xtractorRadius;

				for (int sx = 0; sx <= xtractorRadius;  sx++, a++) {
					if (sx < mapWidth) {
						const int addY = y + xend[a];

						if (addY < mapHeight) {
							totalResources += rexArrayA[addY * mapWidth + sx];
						}
					}
				}
			}

			// set that spot's resource making ability
			// (divide by cells to values are small)
			tempAverage[y * mapWidth + x] = totalResources;

			if (maxResource < totalResources) {
				// find the spot with the highest resource value to set as the map's max
				maxResource = totalResources;
			}
		}
	}

	// make a list for the distribution of values
	std::vector<int> valueDist(256, 0);

	// this will get the total resources a rex placed at each spot would make
	for (int i = 0; i < totalCells; i++) {
		// scale the resources so any map will have values 0-255,
		// no matter how much resources it has
		rexArrayB[i] = tempAverage[i] * 255 / maxResource;
		// clear out the array since it has never been used
		rexArrayC[i] = 0;

		int value = rexArrayB[i];
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
	if (numberOfValues > 256) {
		numberOfValues = 256;
	}

	std::vector<int> bestSpotList(numberOfValues);

	for (int i = 0; i < totalCells; i++) {
		if (rexArrayB[i] == bestValue) {
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

	for (int a = 0; a < maxSpots; a++) {
		if (!stopMe) {
			// reset temporary resources so it can find new spots
			tempResources = 0;
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
					for (int i = 0; i < totalCells; i++) {
						int value = rexArrayB[i];
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
					if (numberOfValues > 256) {
						numberOfValues = 256;
					}

					bestSpotList.clear();
					bestSpotList.resize(numberOfValues);

					for (int i = 0; i < totalCells; i++) {
						if (rexArrayB[i] == bestValue) {
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

				if (rexArrayB[spotIndex] == bestValue) {
					// the spot is still valid, so use it
					speedTempResources_x = spotIndex % mapWidth;
					speedTempResources_y = spotIndex / mapWidth;
					speedTempResources = bestValue;
					found = true;
				}

				// update the bestSpotList index
				usedSpots++;
			}

			coordX = speedTempResources_x;
			coordZ = speedTempResources_y;
			tempResources = speedTempResources;
		}

		if (tempResources < minIncomeForSpot) {
			// if the spots get too crappy it will stop running the loops to speed it all up
			stopMe = true;
		}

		if (!stopMe) {
			// format resource coords to game-coords
			bufferSpot.x = coordX * (SQUARE_SIZE * 2) + SQUARE_SIZE;
			bufferSpot.z = coordZ * (SQUARE_SIZE * 2) + SQUARE_SIZE;
			// gets the actual amount of resource an extractor can make
			const CResourceDescription* resource = resourceHandler->GetResource(resourceId);
			bufferSpot.y = tempResources * (resource->maxWorth) * maxResource / 255;
			vectoredSpots.push_back(bufferSpot);

			// plot TGA array (not necessary) for debug
			rexArrayC[coordZ * mapWidth + coordX] = tempResources;
			numSpotsFound += 1;

			// small speedup of "wipes the resources around the spot so it is not counted twice"
			for (int sy = coordZ - xtractorRadius, a = 0;  sy <= coordZ + xtractorRadius;  sy++, a++) {
				if (sy >= 0 && sy < mapHeight) {
					int clearXStart = coordX - xend[a];
					int clearXEnd = coordX + xend[a];

					if (clearXStart < 0) {
						clearXStart = 0;
					}
					if (clearXEnd >= mapWidth) {
						clearXEnd = mapWidth - 1;
					}

					for (int xClear = clearXStart; xClear <= clearXEnd; xClear++) {
						// wipes the resources around the spot so it is not counted twice
						rexArrayA[sy * mapWidth + xClear] = 0;
						rexArrayB[sy * mapWidth + xClear] = 0;
						tempAverage[sy * mapWidth + xClear] = 0;
					}
				}
			}

			// redo the whole averaging process around the picked spot so other spots can be found around it
			for (int y = coordZ - doubleRadius; y <= coordZ + doubleRadius; y++) {
				if (y >=0 && y < mapHeight) {
					for (int x = coordX - doubleRadius; x <= coordX + doubleRadius; x++) {
						if (x >=0 && x < mapWidth) {
							totalResources = 0;

							// comment out for debug
							if (x == 0 && y == 0) {
								for (int sy = y - xtractorRadius, a = 0;  sy <= y + xtractorRadius;  sy++, a++) {
									if (sy >= 0 && sy < mapHeight) {
										for (int sx = x - xend[a]; sx <= x + xend[a]; sx++) {
											if (sx >= 0 && sx < mapWidth) {
												// get the resources from all pixels around the extractor radius
												totalResources += rexArrayA[sy * mapWidth + sx];
											}
										}
									}
								}
							}

							// quick calc test
							if (x > 0) {
								totalResources = tempAverage[y * mapWidth + x - 1];

								for (int sy = y - xtractorRadius, a = 0;  sy <= y + xtractorRadius;  sy++, a++) {
									if (sy >= 0 && sy < mapHeight) {
										int addX = x + xend[a];
										int remX = x - xend[a] - 1;

										if (addX < mapWidth) {
											totalResources += rexArrayA[sy * mapWidth + addX];
										}
										if (remX >= 0) {
											totalResources -= rexArrayA[sy * mapWidth + remX];
										}
									}
								}
							} else if (y > 0) {
								// x == 0 here
								totalResources = tempAverage[(y - 1) * mapWidth];
								// remove the top half
								int a = xtractorRadius;

								for (int sx = 0; sx <= xtractorRadius;  sx++, a++) {
									if (sx < mapWidth) {
										int remY = y - xend[a] - 1;

										if (remY >= 0) {
											totalResources -= rexArrayA[remY * mapWidth + sx];
										}
									}
								}

								// add the bottom half
								a = xtractorRadius;

								for (int sx = 0; sx <= xtractorRadius;  sx++, a++) {
									if (sx < mapWidth) {
										int addY = y + xend[a];

										if (addY < mapHeight) {
											totalResources += rexArrayA[addY * mapWidth + sx];
										}
									}
								}
							}

							tempAverage[y * mapWidth + x] = totalResources;
							// set that spot's resource amount
							rexArrayB[y * mapWidth + x] = totalResources * 255 / maxResource;
						}
					}
				}
			}
		}
	}
}


template<typename T>
static inline void writeToFile(const T& value, FILE* file) {

	if (fwrite(&value, sizeof(T), 1, file) != 1) {
		throw std::runtime_error("failed to write value to file");
	}
}

void CResourceMapAnalyzer::SaveResourceMap() {

	const std::string cacheFileName = GetCacheFileName();
	FILE* saveFile = fopen(cacheFileName.c_str(), "wb");

	try {
		if (saveFile == nullptr)
			throw std::runtime_error("failed to open file for writing");

		assert(numSpotsFound != -1);
		writeToFile(numSpotsFound, saveFile);
		writeToFile(averageIncome, saveFile);
		for (int i = 0; i < numSpotsFound; i++) {
			writeToFile(vectoredSpots[i], saveFile);
		}
	} catch (const std::runtime_error& err) {
		LOG_L(L_WARNING, "Failed to save the analyzed resource-map to file %s, reason: %s", cacheFileName.c_str(), err.what());
	}

	fclose(saveFile);
}

static void fileReadChecked(void* buf, size_t size, size_t count, FILE* fstream) {

	if (fread(buf, size, count, fstream) != count) {
		throw std::runtime_error("Failed to read the required number of items");
	}
}

bool CResourceMapAnalyzer::LoadResourceMap() {

	bool loaded = false;

	const std::string cacheFileName = GetCacheFileName();

	FILE* cacheFile = fopen(cacheFileName.c_str(), "rb");

	if (cacheFile != nullptr) {
		try {
			fileReadChecked(&numSpotsFound, sizeof(int), 1, cacheFile);
			vectoredSpots.resize(numSpotsFound);
			fileReadChecked(&averageIncome, sizeof(float), 1, cacheFile);
			for (int i = 0; i < numSpotsFound; i++) {
				fileReadChecked(&vectoredSpots[i], sizeof(float3), 1, cacheFile);
			}
			loaded = true;
		} catch (const std::runtime_error& err) {
			LOG_L(L_WARNING, "Failed to load the resource map cache from file %s: %s", cacheFileName.c_str(), err.what());
		}
		fclose(cacheFile);
	}

	return loaded;
}


std::string CResourceMapAnalyzer::GetCacheFileName() const {

	const CResourceDescription* resource = resourceHandler->GetResource(resourceId);
	std::string absFile = CACHE_BASE + gameSetup->mapName + resource->name;

	return absFile;
}
