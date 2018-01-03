#include "LuaWater.h"
#include "Game/Game.h"
#include "Map/ReadMap.h"
#include "System/EventHandler.h"

void CLuaWater::UpdateWater(CGame* game)
{
	if (!readMap->HasVisibleWater())
		return;

	DrawReflection(game);
	DrawRefraction(game);
}


void CLuaWater::Draw()
{
	if (!readMap->HasVisibleWater())
		return;

	eventHandler.DrawWater();
}

void CLuaWater::DrawReflection(CGame* game)
{
	drawReflection = true;

	game->SetDrawMode(Game::ReflectionDraw);
	eventHandler.DrawWorldReflection();
	game->SetDrawMode(Game::NormalDraw);

	drawReflection = false;
}

void CLuaWater::DrawRefraction(CGame* game)
{
	drawRefraction = true;

	game->SetDrawMode(Game::RefractionDraw);
	eventHandler.DrawWorldRefraction();
	game->SetDrawMode(Game::NormalDraw);

	drawRefraction = false;
}

