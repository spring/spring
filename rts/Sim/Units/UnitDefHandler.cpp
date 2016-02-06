/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <locale>
#include <cctype>

#include "UnitDefHandler.h"
#include "UnitDef.h"
#include "UnitDefImage.h"
#include "Lua/LuaParser.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/SideParser.h"
#include "Sim/Misc/Team.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Sound/ISound.h"


CUnitDefHandler* unitDefHandler = NULL;


#if defined(_MSC_VER) && (_MSC_VER < 1800)
bool isblank(int c) {
	return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}
#endif


CUnitDefHandler::CUnitDefHandler(LuaParser* defsParser) : noCost(false)
{
	const LuaTable rootTable = defsParser->GetRoot().SubTable("UnitDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading UnitDefs");
	}

	vector<string> unitDefNames;
	rootTable.GetKeys(unitDefNames);

	unitDefs.reserve(unitDefNames.size() + 1);
	unitDefs.emplace_back();

	for (unsigned int a = 0; a < unitDefNames.size(); ++a) {
		const string& unitName = unitDefNames[a];
		LuaTable udTable = rootTable.SubTable(unitName);

		// parse the unitdef data (but don't load buildpics, etc...)
		PushNewUnitDef(StringToLower(unitName), udTable);
	}

	CleanBuildOptions();
	FindStartUnits();
	ProcessDecoys();
	AssignTechLevels();
}


CUnitDefHandler::~CUnitDefHandler()
{ }


int CUnitDefHandler::PushNewUnitDef(const std::string& unitName, const LuaTable& udTable)
{
	if (std::find_if(unitName.begin(), unitName.end(), isblank) != unitName.end()) {
		LOG_L(L_WARNING,
				"UnitDef name \"%s\" contains white-spaces, "
				"which will likely cause problems. "
				"Please contact the Game/Mod developers.",
				unitName.c_str());
	}

	int defid = unitDefs.size();

	try {
		unitDefs.emplace_back(udTable, unitName, defid);
		UnitDef& newDef = unitDefs.back();
		UnitDefLoadSounds(&newDef, udTable);

		if (!newDef.decoyName.empty()) {
			decoyNameMap[unitName] = StringToLower(newDef.decoyName);
		}

		// force-initialize the real* members
		newDef.SetNoCost(true);
		newDef.SetNoCost(noCost);
	} catch (const content_error& err) {
		LOG_L(L_ERROR, "%s", err.what());
		return 0;
	}

	unitDefIDsByName[unitName] = defid;
	return defid;
}


void CUnitDefHandler::CleanBuildOptions()
{
	// remove invalid build options
	for (int i = 1; i < unitDefs.size(); i++) {
		UnitDef& ud = unitDefs[i];
		map<int, string>& bo = ud.buildOptions;
		map<int, string>::iterator it = bo.begin();
		while (it != bo.end()) {
			bool erase = false;

			const UnitDef* bd = GetUnitDefByName(it->second);
			if (bd == NULL) {
				LOG_L(L_WARNING,
						"removed the \"%s\" entry from the \"%s\" build menu",
						it->second.c_str(), ud.name.c_str());
				erase = true;
			}
			/*
			else if (bd->maxThisUnit <= 0) {
				// don't remove, just grey out the icon
				erase = true; // silent removal
			}
			*/

			if (erase) {
				map<int, string>::iterator tmp = it;
				++it;
				bo.erase(tmp);
			} else {
				++it;
			}
		}
	}
}


void CUnitDefHandler::ProcessDecoys()
{
	// assign the decoy pointers, and build the decoy map
	map<string, string>::const_iterator mit;
	for (mit = decoyNameMap.begin(); mit != decoyNameMap.end(); ++mit) {
		map<string, int>::iterator fakeIt, realIt;
		fakeIt = unitDefIDsByName.find(mit->first);
		realIt = unitDefIDsByName.find(mit->second);
		if ((fakeIt != unitDefIDsByName.end()) && (realIt != unitDefIDsByName.end())) {
			UnitDef& fake = unitDefs[fakeIt->second];
			UnitDef& real = unitDefs[realIt->second];
			fake.decoyDef = &real;
			decoyMap[real.id].insert(fake.id);
		}
	}
	decoyNameMap.clear();
}


void CUnitDefHandler::FindStartUnits()
{
	for (unsigned int i = 0; i < sideParser.GetCount(); i++) {
		const std::string& startUnit = sideParser.GetStartUnit(i);
		if (!startUnit.empty()) {
			std::map<std::string, int>::iterator it = unitDefIDsByName.find(startUnit);
			if (it != unitDefIDsByName.end()) {
				startUnitIDs.insert(it->second);
			}
		}
	}
}


void CUnitDefHandler::UnitDefLoadSounds(UnitDef* ud, const LuaTable& udTable)
{
	LuaTable soundsTable = udTable.SubTable("sounds");

	LoadSounds(soundsTable, ud->sounds.ok,          "ok");      // eg. "ok1", "ok2", ...
	LoadSounds(soundsTable, ud->sounds.select,      "select");  // eg. "select1", "select2", ...
	LoadSounds(soundsTable, ud->sounds.arrived,     "arrived"); // eg. "arrived1", "arrived2", ...
	LoadSounds(soundsTable, ud->sounds.build,       "build");
	LoadSounds(soundsTable, ud->sounds.activate,    "activate");
	LoadSounds(soundsTable, ud->sounds.deactivate,  "deactivate");
	LoadSounds(soundsTable, ud->sounds.cant,        "cant");
	LoadSounds(soundsTable, ud->sounds.underattack, "underattack");
}


void CUnitDefHandler::LoadSounds(const LuaTable& soundsTable, GuiSoundSet& gsound, const string& soundName)
{
	string fileName = soundsTable.GetString(soundName, "");
	if (!fileName.empty()) {
		LoadSound(gsound, fileName, 1.0f);
		return;
	}

	LuaTable sndTable = soundsTable.SubTable(soundName);
	for (int i = 1; true; i++) {
		LuaTable sndFileTable = sndTable.SubTable(i);
		if (sndFileTable.IsValid()) {
			fileName = sndFileTable.GetString("file", "");
			if (!fileName.empty()) {
				const float volume = sndFileTable.GetFloat("volume", 1.0f);
				if (volume > 0.0f) {
					LoadSound(gsound, fileName, volume);
				}
			}
		} else {
			fileName = sndTable.GetString(i, "");
			if (fileName.empty()) {
				break;
			}
			LoadSound(gsound, fileName, 1.0f);
		}
	}
}


void CUnitDefHandler::LoadSound(GuiSoundSet& gsound, const string& fileName, const float volume)
{
	const int id = LoadSoundFile(fileName);
	if (id > 0)
	{
		GuiSoundSet::Data soundData(fileName, id, volume);
		gsound.sounds.push_back(soundData);
	}
}


const UnitDef* CUnitDefHandler::GetUnitDefByName(std::string name)
{
	StringToLowerInPlace(name);

	std::map<std::string, int>::iterator it = unitDefIDsByName.find(name);
	if (it == unitDefIDsByName.end()) {
		return NULL;
	}

	return &unitDefs[it->second];
}


const UnitDef* CUnitDefHandler::GetUnitDefByID(int defid)
{
	if ((defid <= 0) || (defid >= unitDefs.size())) {
		return NULL;
	}
	return &unitDefs[defid];
}


static bool LoadBuildPic(const string& filename, CBitmap& bitmap)
{
	CFileHandler bfile(filename);
	if (bfile.FileExists()) {
		bitmap.Load(filename);
		return true;
	}
	return false;
}


unsigned int CUnitDefHandler::GetUnitDefImage(const UnitDef* unitDef)
{
	if (unitDef->buildPic != NULL) {
		return (unitDef->buildPic->textureID);
	}
	SetUnitDefImage(unitDef, unitDef->buildPicName);
	return unitDef->buildPic->textureID;
}


void CUnitDefHandler::SetUnitDefImage(const UnitDef* unitDef, const std::string& texName)
{
	if (unitDef->buildPic == NULL) {
		unitDef->buildPic = new UnitDefImage();
	} else {
		unitDef->buildPic->Free();
	}

	CBitmap bitmap;

	if (!texName.empty()) {
		bitmap.Load("unitpics/" + texName);
	}
	else {
		if (!LoadBuildPic("unitpics/" + unitDef->name + ".dds", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitDef->name + ".png", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitDef->name + ".pcx", bitmap) &&
		    !LoadBuildPic("unitpics/" + unitDef->name + ".bmp", bitmap)) {
			bitmap.AllocDummy(SColor(255, 0, 0, 255));
		}
	}

	const unsigned int texID = bitmap.CreateTexture();

	UnitDefImage* unitImage = unitDef->buildPic;
	unitImage->textureID = texID;
	unitImage->imageSizeX = bitmap.xsize;
	unitImage->imageSizeY = bitmap.ysize;
}


void CUnitDefHandler::SetUnitDefImage(const UnitDef* unitDef,
                                      unsigned int texID, int xsize, int ysize)
{
	if (unitDef->buildPic == NULL) {
		unitDef->buildPic = new UnitDefImage();
	} else {
		unitDef->buildPic->Free();
	}

	UnitDefImage* unitImage = unitDef->buildPic;
	unitImage->textureID = texID;
	unitImage->imageSizeX = xsize;
	unitImage->imageSizeY = ysize;
}


bool CUnitDefHandler::ToggleNoCost()
{
	noCost = !noCost;

	for (int i = 1; i < unitDefs.size(); ++i) {
		unitDefs[i].SetNoCost(noCost);
	}

	return noCost;
}


void CUnitDefHandler::AssignTechLevels()
{
	set<int>::iterator it;
	for (it = startUnitIDs.begin(); it != startUnitIDs.end(); ++it) {
		AssignTechLevel(unitDefs[*it], 0);
	}
}


void CUnitDefHandler::AssignTechLevel(UnitDef& ud, int level)
{
	if ((ud.techLevel >= 0) && (ud.techLevel <= level)) {
		return;
	}

	ud.techLevel = level;

	level++;

	map<int, std::string>::const_iterator bo_it;
	for (bo_it = ud.buildOptions.begin(); bo_it != ud.buildOptions.end(); ++bo_it) {
		std::map<std::string, int>::const_iterator ud_it = unitDefIDsByName.find(bo_it->second);
		if (ud_it != unitDefIDsByName.end()) {
			AssignTechLevel(unitDefs[ud_it->second], level);
		}
	}
}
