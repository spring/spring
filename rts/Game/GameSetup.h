#ifndef __GAME_SETUP_H__
#define __GAME_SETUP_H__

#include <string>
#include <map>

#include "GameSetupData.h"

class TdfParser;

class CGameSetup : public CGameSetupData
{
public:
	CGameSetup();
	~CGameSetup();
	bool Init(std::string setupFile);
	bool Update();

	bool Init(const char* buf, int size);

private:
	void LoadMap();
	void LoadStartPositionsFromMap();

	void LoadStartPositions(const TdfParser& file);
	void LoadUnitRestrictions(const TdfParser& file);
	void LoadPlayers(const TdfParser& file);
	void LoadTeams(const TdfParser& file);
	void LoadAllyTeams(const TdfParser& file);
};

extern CGameSetup* gameSetup;

#endif // __GAME_SETUP_H__
