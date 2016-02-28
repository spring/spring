/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <cstdio>

#include "Benchmark.h"

#include "Game.h"
#include "GlobalUnsynced.h"
#include "UI/GuiHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "System/TimeProfiler.h"

bool CBenchmark::enabled = false;
int CBenchmark::startFrame = 0;
int CBenchmark::endFrame = 5 * 60 * GAME_SPEED;


CBenchmark::CBenchmark()
	: CEventClient("[CBenchmark]", 271990, false)
{
	eventHandler.AddClient(this);
}

CBenchmark::~CBenchmark()
{
	eventHandler.RemoveClient(this);

	FILE* pFile = fopen("benchmark.data", "w");
	std::map<float, float>::const_iterator rit = realFPS.cbegin();
	std::map<float, float>::const_iterator dit = drawFPS.cbegin();
	std::map<int, float>::const_iterator   sit = simFPS.cbegin();
	std::map<int, size_t>::const_iterator  uit = units.cbegin();
	std::map<int, size_t>::const_iterator  fit = features.cbegin();
	std::map<int, float>::const_iterator   git = gameSpeed.cbegin();
	std::map<int, float>::const_iterator   lit = luaUsage.cbegin();

#ifdef _WIN64 //fprintf sometimes spews false warnings over %I64u
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#endif

	fprintf(pFile, "# GAME_FRAME effFPS drawFPS simFPS num_units num_features game_speed lua_usage\n");
	while (dit != drawFPS.cend() && sit != simFPS.cend()) {
		if (dit->first < sit->first) {
			fprintf(pFile, "%f %f %f %f " _STPF_ " " _STPF_ " %f %f\n", dit->first, rit->second, dit->second, sit->second, uit->second, fit->second, git->second, lit->second);
			++dit;
			++rit;
		} else {
			fprintf(pFile, "%f %f %f %f " _STPF_ " " _STPF_ " %f %f\n", (float)sit->first, rit->second, dit->second, sit->second, uit->second, fit->second, git->second, lit->second);
			++sit;
			++uit;
			++fit;
			++git;
			++lit;
		}
	}
	fclose(pFile);

#ifdef _WIN64
#pragma GCC diagnostic pop
#endif
}

void CBenchmark::GameFrame(int gameFrame)
{
	if (gameFrame == 0 && (startFrame - 15 * GAME_SPEED > 0)) {
		std::vector<string> cmds;
		cmds.push_back("@@setmaxspeed 100");
		cmds.push_back("@@setminspeed 100");
		guihandler->RunCustomCommands(cmds, false);
	}

	if (gameFrame == (startFrame - 15 * GAME_SPEED)) {
		std::vector<string> cmds;
		cmds.push_back("@@setminspeed 1");
		cmds.push_back("@@setmaxspeed 1");
		guihandler->RunCustomCommands(cmds, false);
	}

	if (gameFrame >= startFrame) {
		simFPS[gameFrame] = (gu->avgSimFrameTime == 0.0f)? 0.0f: 1000.0f / gu->avgSimFrameTime;
		units[gameFrame] = unitHandler->units.size();
		features[gameFrame] = featureHandler->GetActiveFeatures().size();
		gameSpeed[gameFrame] = GAME_SPEED * gs->wantedSpeedFactor;
		luaUsage[gameFrame] = profiler.GetPercent("Lua");
	}

	if (gameFrame == endFrame) {
		gu->globalQuit = true;
	}
}

void CBenchmark::DrawWorld()
{
	if (!simFPS.empty()) {
		realFPS[game->lastSimFrame + globalRendering->timeOffset] = globalRendering->FPS;
		drawFPS[game->lastSimFrame + globalRendering->timeOffset] = (gu->avgDrawFrameTime == 0.0f)? 0.0f: 1000.0f / gu->avgDrawFrameTime;
	}
}
