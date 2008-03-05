#ifndef __GAME_SETUP_H__
#define __GAME_SETUP_H__

#include <string>

#include "GameSetupData.h"

class TdfParser;

class CGameSetup : public CGameSetupData
{
public:
	CGameSetup();
	~CGameSetup();
	bool Init(std::string setupFile);

	bool Init(const char* buf, int size);

	void LoadStartPositions();

private:
	void LoadStartPositionsFromMap();
	void LoadUnitRestrictions(const TdfParser& file);
	void LoadPlayers(const TdfParser& file);
	void LoadTeams(const TdfParser& file);
	void LoadAllyTeams(const TdfParser& file);

	void RemapPlayers();
	void RemapTeams();
	void RemapAllyteams();
};

extern CGameSetup* gameSetup;

#endif // __GAME_SETUP_H__
