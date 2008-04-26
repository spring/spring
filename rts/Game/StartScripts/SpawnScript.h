#ifndef SPAWNSCRIPT_H
#define SPAWNSCRIPT_H

#include "Script.h"
#include <string>
#include <list>
#include <vector>
#include "FileSystem/FileHandler.h"

class CSpawnScript :
	public CScript
{
public:
	CSpawnScript(bool _autonomous);
	~CSpawnScript();
	void Update();

private:
	void LoadSpawns();

	bool autonomous;

	struct Spawn {
		int frame;
		std::string name;
	};

	std::list<Spawn> spawns;
	std::list<Spawn>::iterator curSpawn;
	int frameOffset;

	std::vector<float3> spawnPos;

	struct Unit{
		int id;
		int target;
		int team;
		float3 lastTargetPos;
	};
	std::list<Unit> myUnits;
	std::list<Unit>::iterator curUnit;
	std::string LoadToken(CFileHandler& file);
};

#endif
