/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <map>
#include <stdio.h>
#include "Benchmark.h"

#include "Game.h"
#include "GlobalUnsynced.h"
#include "UI/GuiHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "System/TimeProfiler.h"

static std::map<float, float> realFPS;
static std::map<float, float> drawFPS;
static std::map<int, float>   simFPS;
static std::map<int, size_t>  units;
static std::map<int, size_t>  features;
static std::map<int, float>   gameSpeed;
static std::map<int, float>   luaUsage;

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
	FILE* pFile = fopen("benchmark.data", "w");
	std::map<float, float>::const_iterator rit = realFPS.begin();
	std::map<float, float>::const_iterator dit = drawFPS.begin();
	std::map<int, float>::const_iterator   sit = simFPS.begin();
	std::map<int, size_t>::const_iterator  uit = units.begin();
	std::map<int, size_t>::const_iterator  fit = features.begin();
	std::map<int, float>::const_iterator   git = gameSpeed.begin();
	std::map<int, float>::const_iterator   lit = luaUsage.begin();

	fprintf(pFile, "# GAME_FRAME effFPS drawFPS simFPS num_units num_features game_speed lua_usage\n");
	while (dit != drawFPS.end() && sit != simFPS.end())
	{
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
}

void CBenchmark::GameFrame(int gameFrame)
{
	if (gameFrame == 0 && (startFrame - 45 * GAME_SPEED > 0)) {
		std::vector<string> cmds;
		cmds.push_back("@@setmaxspeed 100");
		cmds.push_back("@@setminspeed 100");
		guihandler->RunCustomCommands(cmds, false);
	}

	if (gameFrame == (startFrame - 45 * GAME_SPEED)) {
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
