#include "LuaWater.h"
#include "Game/Game.h"
#include "System/EventHandler.h"

void CLuaWater::UpdateWater(CGame* game)
{
	DrawReflection(game);
	DrawRefraction(game);
}


void CLuaWater::Draw()
{
	eventHandler.DrawWater();
}

void CLuaWater::DrawReflection(CGame* game)
{
	drawReflection = true;

	game->SetDrawMode(CGame::gameReflectionDraw);
	eventHandler.DrawWorldReflection();
	game->SetDrawMode(CGame::gameNormalDraw);

	drawReflection = false;
}

void CLuaWater::DrawRefraction(CGame* game)
{
	drawRefraction = true;

	game->SetDrawMode(CGame::gameRefractionDraw);
	eventHandler.DrawWorldRefraction();
	game->SetDrawMode(CGame::gameNormalDraw);

	drawRefraction = false;
}

