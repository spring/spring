#ifndef RADARHANDLER_H
#define RADARHANDLER_H

#include <boost/noncopyable.hpp>

#include "Object.h"
#include "Sim/Units/Unit.h"

#define RADAR_SIZE 8

class CRadarHandler : public boost::noncopyable
{
	CR_DECLARE(CRadarHandler);

public:
	CRadarHandler(bool circularRadar);
	~CRadarHandler();

	void MoveUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);

	bool InRadar(const float3& pos, int allyTeam) {
		const int gx = (int) pos.x / (SQUARE_SIZE * RADAR_SIZE);
		const int gz = (int) pos.z / (SQUARE_SIZE * RADAR_SIZE);
		const int rowIdx = std::max(0, std::min(ysize - 1, gz)) * xsize;
		const int colIdx = std::max(0, std::min(xsize - 1, gx));
		const int square = rowIdx + colIdx;
		if (pos.y < -0.5f) {
			#ifdef DEBUG
			assert(square < sonarMaps[allyTeam].size());
			#endif
			return (sonarMaps[allyTeam][square] && !commonSonarJammerMap[square]);
		}
		else if (!circularRadar && pos.y > 0.5f) {
			#ifdef DEBUG
			assert(square < airRadarMaps[allyTeam].size());
			#endif
			return (airRadarMaps[allyTeam][square] && !commonJammerMap[square]);
		}
		else {
			#ifdef DEBUG
			assert(square < radarMaps[allyTeam].size());
			#endif
			return (radarMaps[allyTeam][square] && !commonJammerMap[square]);
		}
	}

	bool InRadar(const CUnit* unit, int allyTeam) {
		if (unit->stealth)
			return false;

		const int rowIdx = std::max(0, std::min(ysize - 1, (int) unit->pos.z / (SQUARE_SIZE * RADAR_SIZE))) * xsize;
		const int colIdx = std::max(0, std::min(xsize - 1, (int) unit->pos.x / (SQUARE_SIZE * RADAR_SIZE)));
		const int square = rowIdx + colIdx;

		if (unit->isUnderWater) {
			#ifdef DEBUG
			assert(square < sonarMaps[allyTeam].size());
			#endif
			return (!!sonarMaps[allyTeam][square]) && !commonSonarJammerMap[square];
		}
		if (!circularRadar && unit->useAirLos) {
			#ifdef DEBUG
			assert(square < airRadarMaps[allyTeam].size());
			#endif
			return airRadarMaps[allyTeam][square] && !commonJammerMap[square];
		} else {
			#ifdef DEBUG
			assert(square < radarMaps[allyTeam].size() && square < sonarMaps[allyTeam].size());
			#endif
			return (radarMaps[allyTeam][square] || (unit->pos.y <= 1 && sonarMaps[allyTeam][square])) && !commonJammerMap[square];
		}
	}

	bool InSeismicDistance(const CUnit* unit, int allyTeam) {
		const int rowIdx = std::max(0, std::min(ysize - 1, (int) unit->pos.z / (SQUARE_SIZE * RADAR_SIZE))) * xsize;
		const int colIdx = std::max(0, std::min(xsize - 1, (int) unit->pos.x / (SQUARE_SIZE * RADAR_SIZE)));
		const int square = rowIdx + colIdx;
		#ifdef DEBUG
		assert(square < seismicMaps[allyTeam].size());
		#endif
		return !!seismicMaps[allyTeam][square];
	}

	bool circularRadar;

	std::vector<unsigned short> radarMaps[MAX_TEAMS];
	std::vector<unsigned short> airRadarMaps[MAX_TEAMS];
	std::vector<unsigned short> sonarMaps[MAX_TEAMS];
	std::vector<unsigned short> jammerMaps[MAX_TEAMS];
	std::vector<unsigned short> seismicMaps[MAX_TEAMS];
	std::vector<unsigned short> commonJammerMap;
	std::vector<unsigned short> commonSonarJammerMap;
	float radarErrorSize[MAX_TEAMS];
	float baseRadarErrorSize;

	int xsize;
	int ysize;

	float targFacEffect;

protected:
	void AddMapArea(int2 pos, int radius, std::vector<unsigned short>& map, int amount);

	void SafeLosRadarAdd(CUnit* unit);

private:
	void Serialize(creg::ISerializer& s);
};

extern CRadarHandler* radarhandler;

#endif /* RADARHANDLER_H */
