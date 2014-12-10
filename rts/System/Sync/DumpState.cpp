/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <fstream>
#include <vector>
#include <list>

#include "DumpState.h"

#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Net/GameServer.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/Util.h"
#include "System/Log/ILog.h"


void DumpState(int newMinFrameNum, int newMaxFrameNum, int newFramePeriod)
{
	#ifdef NDEBUG
	// must be in debug-mode for this
	return;
	#endif

	static std::fstream file;

	static int gMinFrameNum = -1;
	static int gMaxFrameNum = -1;
	static int gFramePeriod =  1;

	const int oldMinFrameNum = gMinFrameNum;
	const int oldMaxFrameNum = gMaxFrameNum;

	if (!gs->cheatEnabled) { return; }
	// check if the range is valid
	if (newMaxFrameNum < newMinFrameNum) { return; }
	// adjust the bounds if the new values are valid
	if (newMinFrameNum >= 0) { gMinFrameNum = newMinFrameNum; }
	if (newMaxFrameNum >= 0) { gMaxFrameNum = newMaxFrameNum; }
	if (newFramePeriod >= 1) { gFramePeriod = newFramePeriod; }

	if ((gMinFrameNum != oldMinFrameNum) || (gMaxFrameNum != oldMaxFrameNum)) {
		// bounds changed, open a new file
		if (file.is_open()) {
			file.flush();
			file.close();
		}

		std::string name = (gameServer != NULL)? "Server": "Client";
		name += "GameState-";
		name += IntToString(gu->RandInt());
		name += "-[";
		name += IntToString(gMinFrameNum);
		name += "-";
		name += IntToString(gMaxFrameNum);
		name += "].txt";

		file.open(name.c_str(), std::ios::out);

		if (file.is_open()) {
			file << "map name: " << gameSetup->mapName << "\n";
			file << "mod name: " << gameSetup->modName << "\n";
			file << "minFrame: " << gMinFrameNum << ", maxFrame: " << gMaxFrameNum << "\n";
			file << "randSeed: " << gs->GetRandSeed() << "\n";
			file << "initSeed: " << gs->GetInitRandSeed() << "\n";
		}

		LOG("[DumpState] using dump-file \"%s\"", name.c_str());
	}

	if (file.bad() || !file.is_open()) { return; }
	// check if the CURRENT frame lies within the bounds
	if (gs->frameNum < gMinFrameNum) { return; }
	if (gs->frameNum > gMaxFrameNum) { return; }
	if ((gs->frameNum % gFramePeriod) != 0) { return; }

	// we only care about the synced projectile data here
	const std::list<CUnit*>& units = unitHandler->activeUnits;
	const CFeatureSet& features = featureHandler->GetActiveFeatures();
	      ProjectileContainer& projectiles = projectileHandler->syncedProjectiles;

	std::list<CUnit*>::const_iterator unitsIt;
	CFeatureSet::const_iterator featuresIt;
	ProjectileContainer::iterator projectilesIt;
	std::vector<LocalModelPiece*>::const_iterator piecesIt;
	std::vector<CWeapon*>::const_iterator weaponsIt;

	file << "frame: " << gs->frameNum << ", seed: " << gs->GetRandSeed() << "\n";
	file << "\tunits: " << units.size() << "\n";

	#define DUMP_UNIT_DATA
	#define DUMP_UNIT_PIECE_DATA
	#define DUMP_UNIT_WEAPON_DATA
	#define DUMP_UNIT_COMMANDAI_DATA
	#define DUMP_UNIT_MOVETYPE_DATA
	#define DUMP_FEATURE_DATA
	#define DUMP_PROJECTILE_DATA
	#define DUMP_TEAM_DATA
	// #define DUMP_ALLYTEAM_DATA

	#ifdef DUMP_UNIT_DATA
	for (unitsIt = units.begin(); unitsIt != units.end(); ++unitsIt) {
		const CUnit* u = *unitsIt;
		const std::vector<CWeapon*>& weapons = u->weapons;
		const LocalModel* lm = u->localModel;
		const std::vector<LocalModelPiece*>& pieces = lm->pieces;
		const float3& pos = u->pos;
		const float3& xdir = u->rightdir;
		const float3& ydir = u->updir;
		const float3& zdir = u->frontdir;

		file << "\t\tunitID: " << u->id << " (name: " << u->unitDef->name << ")\n";
		file << "\t\t\tpos: <" << pos.x << ", " << pos.y << ", " << pos.z << "\n";
		file << "\t\t\txdir: <" << xdir.x << ", " << xdir.y << ", " << xdir.z << "\n";
		file << "\t\t\tydir: <" << ydir.x << ", " << ydir.y << ", " << ydir.z << "\n";
		file << "\t\t\tzdir: <" << zdir.x << ", " << zdir.y << ", " << zdir.z << "\n";
		file << "\t\t\theading: " << int(u->heading) << ", mapSquare: " << u->mapSquare << "\n";
		file << "\t\t\thealth: " << u->health << ", experience: " << u->experience << "\n";
		file << "\t\t\tisDead: " << u->isDead << ", activated: " << u->activated << "\n";
		file << "\t\t\tphysicalState: " << u->physicalState << "\n";
		file << "\t\t\tfireState: " << u->fireState << ", moveState: " << u->moveState << "\n";
		file << "\t\t\tpieces: " << pieces.size() << "\n";

		#ifdef DUMP_UNIT_PIECE_DATA
		for (piecesIt = pieces.begin(); piecesIt != pieces.end(); ++piecesIt) {
			const LocalModelPiece* lmp = *piecesIt;
			const S3DModelPiece* omp = lmp->original;
			const float3& ppos = lmp->GetPosition();
			const float3& prot = lmp->GetRotation();

			file << "\t\t\t\tname: " << omp->name << " (parentName: " << omp->parentName << ")\n";
			file << "\t\t\t\tpos: <" << ppos.x << ", " << ppos.y << ", " << ppos.z << ">\n";
			file << "\t\t\t\trot: <" << prot.x << ", " << prot.y << ", " << prot.z << ">\n";
			file << "\t\t\t\tvisible: " << lmp->scriptSetVisible << "\n";
			file << "\n";
		}
		#endif

		file << "\t\t\tweapons: " << weapons.size() << "\n";

		#ifdef DUMP_UNIT_WEAPON_DATA
		for (weaponsIt = weapons.begin(); weaponsIt != weapons.end(); ++weaponsIt) {
			const CWeapon* w = *weaponsIt;
			const float3& awp = w->weaponPos;
			const float3& rwp = w->relWeaponPos;
			const float3& amp = w->weaponMuzzlePos;
			const float3& rmp = w->relWeaponMuzzlePos;

			file << "\t\t\t\tweaponID: " << w->weaponNum << " (name: " << w->weaponDef->name << ")\n";
			file << "\t\t\t\tweaponDir: <" << w->weaponDir.x << ", " << w->weaponDir.y << ", " << w->weaponDir.z << ">\n";
			file << "\t\t\t\tabsWeaponPos: <" << awp.x << ", " << awp.y << ", " << awp.z << ">\n";
			file << "\t\t\t\trelWeaponPos: <" << rwp.x << ", " << rwp.y << ", " << rwp.z << ">\n";
			file << "\t\t\t\tabsWeaponMuzzlePos: <" << amp.x << ", " << amp.y << ", " << amp.z << ">\n";
			file << "\t\t\t\trelWeaponMuzzlePos: <" << rmp.x << ", " << rmp.y << ", " << rmp.z << ">\n";
			file << "\n";
		}
		#endif

		#ifdef DUMP_UNIT_COMMANDAI_DATA
		const CCommandAI* cai = u->commandAI;
		const CCommandQueue& cq = cai->commandQue;

		file << "\t\t\tcommandAI:\n";
		file << "\t\t\t\torderTarget->id: " << ((cai->orderTarget != NULL)? cai->orderTarget->id: -1) << "\n";
		file << "\t\t\t\tcommandQue.size(): " << cq.size() << "\n";

		for (CCommandQueue::const_iterator cit = cq.begin(); cit != cq.end(); ++cit) {
			const Command& c = *cit;

			file << "\t\t\t\t\tcommandID: " << c.GetID() << "\n";
			file << "\t\t\t\t\ttag: " << c.tag << ", options: " << c.options << "\n";
			file << "\t\t\t\t\tparams: " << c.GetParamsCount() << "\n";

			for (unsigned int n = 0; n < c.GetParamsCount(); n++) {
				file << "\t\t\t\t\t\t" << c.GetParam(n) << "\n";
			}
		}
		#endif

		#ifdef DUMP_UNIT_MOVETYPE_DATA
		const AMoveType* amt = u->moveType;
		const float3& goalPos = amt->goalPos;
		const float3& oldUpdatePos = amt->oldPos;
		const float3& oldSlowUpPos = amt->oldSlowUpdatePos;

		file << "\t\t\tmoveType:\n";
		file << "\t\t\t\tgoalPos: <" << goalPos.x << ", " << goalPos.y << ", " << goalPos.z << ">\n";
		file << "\t\t\t\toldUpdatePos: <" << oldUpdatePos.x << ", " << oldUpdatePos.y << ", " << oldUpdatePos.z << ">\n";
		file << "\t\t\t\toldSlowUpPos: <" << oldSlowUpPos.x << ", " << oldSlowUpPos.y << ", " << oldSlowUpPos.z << ">\n";
		file << "\t\t\t\tmaxSpeed: " << amt->GetMaxSpeed() << ", maxWantedSpeed: " << amt->GetMaxWantedSpeed() << "\n";
		file << "\t\t\t\tprogressState: " << amt->progressState << "\n";
		#endif
	}
	#endif

	file << "\tfeatures: " << features.size() << "\n";

	#ifdef DUMP_FEATURE_DATA
	for (featuresIt = features.begin(); featuresIt != features.end(); ++featuresIt) {
		const CFeature* f = *featuresIt;

		file << "\t\tfeatureID: " << f->id << " (name: " << f->def->name << ")\n";
		file << "\t\t\tpos: <" << f->pos.x << ", " << f->pos.y << ", " << f->pos.z << ">\n";
		file << "\t\t\thealth: " << f->health << ", reclaimLeft: " << f->reclaimLeft << "\n";
	}
	#endif

	file << "\tprojectiles: " << projectiles.size() << "\n";

	#ifdef DUMP_PROJECTILE_DATA
	for (projectilesIt = projectiles.begin(); projectilesIt != projectiles.end(); ++projectilesIt) {
		const CProjectile* p = *projectilesIt;

		file << "\t\tprojectileID: " << p->id << "\n";
		file << "\t\t\tpos: <" << p->pos.x << ", " << p->pos.y << ", " << p->pos.z << ">\n";
		file << "\t\t\tdir: <" << p->dir.x << ", " << p->dir.y << ", " << p->dir.z << ">\n";
		file << "\t\t\tspeed: <" << p->speed.x << ", " << p->speed.y << ", " << p->speed.z << ">\n";
		file << "\t\t\tweapon: " << p->weapon << ", piece: " << p->piece << "\n";
		file << "\t\t\tcheckCol: " << p->checkCol << ", deleteMe: " << p->deleteMe << "\n";
	}
	#endif

	file << "\tteams: " << teamHandler->ActiveTeams() << "\n";

	#ifdef DUMP_TEAM_DATA
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		const CTeam* t = teamHandler->Team(a);

		file << "\t\tteamID: " << t->teamNum << " (controller: " << t->GetControllerName() << ")\n";
		file << "\t\t\tmetal: " << float(t->res.metal) << ", energy: " << float(t->res.energy) << "\n";
		file << "\t\t\tmetalPull: " << t->resPull.metal << ", energyPull: " << t->resPull.energy << "\n";
		file << "\t\t\tmetalIncome: " << t->resIncome.metal << ", energyIncome: " << t->resIncome.energy << "\n";
		file << "\t\t\tmetalExpense: " << t->resExpense.metal << ", energyExpense: " << t->resExpense.energy << "\n";
	}
	#endif

	file << "\tallyteams: " << teamHandler->ActiveAllyTeams() << "\n";

	#ifdef DUMP_ALLYTEAM_DATA
	for (int a = 0; a < teamHandler->ActiveAllyTeams(); ++a) {
		file << "\t\tallyteamID: " << a << ", LOS-map:" << "\n";

		for (int y = 0; y < losHandler->losSizeY; ++y) {
			file << " ";

			for (int x = 0; x < losHandler->losSizeX; ++x) {
				file << "\t\t\t" << losHandler->losMaps[a][y * losHandler->losSizeX + x] << " ";
			}

			file << "\n";
		}
	}
	#endif

	file.flush();
}
