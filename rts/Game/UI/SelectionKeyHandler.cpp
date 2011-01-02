/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <fstream>
#include <SDL_keysym.h>
#include "mmgr.h"

#include "Game/Camera/CameraController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/TeamHandler.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "MouseHandler.h"
#include "FileSystem/FileSystem.h"
#include "SelectionKeyHandler.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "KeyBindings.h"
#include "myMath.h"
#include <boost/cstdint.hpp>

CSelectionKeyHandler* selectionKeys;

CSelectionKeyHandler::CSelectionKeyHandler()
{
	LoadSelectionKeys();
}

CSelectionKeyHandler::~CSelectionKeyHandler()
{
}

void CSelectionKeyHandler::LoadSelectionKeys()
{
	std::ifstream ifs(filesystem.LocateFile("selectkeys.txt").c_str());

	selectNumber = 0;

	while (ifs.peek() != EOF && !ifs.eof()) {
		std::string key, sel;
		ifs >> key;

		if (ifs.peek() == EOF || ifs.eof())
			break;

		ifs >> sel;

		bool shift=false;
		bool control=false;
		bool alt=false;
		unsigned char keyname=0;

		while(true){
			std::string s=ReadToken(key);

			if(s=="Shift"){
				shift=true;
			} else if (s=="Control"){
				control=true;
			} else if (s=="Alt"){
				alt=true;
			} else {
				char c=s[0];
				if(c>='A' && c<='Z')
					keyname=SDLK_a + (c - 'A');

				if(c>='0' && c<='9')
					keyname=SDLK_0 + (c -'0');

				break;
			}
			ReadDelimiter(key);
		}
		std::string keybindstring;
		if ( alt ) keybindstring += "Alt";
		if ( control ) {
			if ( keybindstring.size() != 0 ) keybindstring += "+";
			keybindstring += "Ctrl";
		}
		if ( shift ) {
			if ( keybindstring.size() != 0 ) keybindstring += "+";
			keybindstring += "Shift";
		}
		if ( keybindstring.size() != 0 ) keybindstring += "+";
		keybindstring += keyname;
		keyBindings->Command( "bind " + keybindstring + " select " + sel );
	}
}

std::string CSelectionKeyHandler::ReadToken(std::string& s)
{
	std::string ret;
	// change made.  avoiding repeated substr calls...
/*	int index = 0;
	//char c=s[index];

	while ( index < s.length() && s[index] != '_' && s[index++] != '+' );

	ret = s.substr(0, index);
	if ( index == s.length() )
		s.clear();
	else
		s = s.substr(index, std::string::npos);
*/
	if (s.empty())
		return std::string();

	char c=s[0];
	while(c && c!='_' && c!='+'){
		s=s.substr(1,std::string::npos);
		ret+=c;
		c=s[0];
	}

	return ret;
}


std::string CSelectionKeyHandler::ReadDelimiter(std::string& s)
{
	std::string ret = s.substr(0, 1);
	if (s.size() >= 1) {
		s = s.substr(1, std::string::npos);
	} else {
		s = "";
	}
	return ret;
}


namespace
{
	struct Filter
	{
	public:
		typedef std::map<std::string, Filter*> Map;

		/// Contains all existing filter singletons.
		static Map& all() {
			static Map instance;
			return instance;
		}

		/// Called immediately before the filter is used.
		virtual void Prepare() {}

		/// Called immediately before the filter is used for every parameter.
		virtual void SetParam(int index, const std::string& value) {
			assert(false);
		}

		/// Actual filtering, should return false if unit should be removed
		/// from proposed selection.
		virtual bool ShouldIncludeUnit(const CUnit* unit) const = 0;

		/// Number of arguments this filter has.
		const int numArgs;

	protected:
		Filter(const std::string& name, int args) : numArgs(args) {
			all().insert(Map::value_type(name, this));
		}
	};

	// prototype / factory based approach might be better at some point?
	// for now these singleton filters seem ok. (they are not reentrant tho!)

#define DECLARE_FILTER_EX(name, args, condition, extra, init) \
	struct name ## _Filter : public Filter { \
		name ## _Filter() : Filter(#name, args) { init; } \
		bool ShouldIncludeUnit(const CUnit* unit) const { return condition; } \
		extra \
	} name ## _filter_instance; \

#define DECLARE_FILTER(name, condition) \
	DECLARE_FILTER_EX(name, 0, condition, ,)

	DECLARE_FILTER(Builder, unit->unitDef->buildSpeed > 0);
	DECLARE_FILTER(Building, dynamic_cast<const CBuilding*>(unit) != NULL);
	DECLARE_FILTER(Commander, unit->unitDef->isCommander);
	DECLARE_FILTER(Transport, unit->unitDef->transportCapacity > 0);
	DECLARE_FILTER(Aircraft, unit->unitDef->canfly);
	DECLARE_FILTER(Weapons, !unit->weapons.empty());
	DECLARE_FILTER(Idle, unit->commandAI->commandQue.empty());
	DECLARE_FILTER(Waiting, !unit->commandAI->commandQue.empty() &&
	               (unit->commandAI->commandQue.front().id == CMD_WAIT));
	DECLARE_FILTER(InHotkeyGroup, unit->group != NULL);
	DECLARE_FILTER(Radar, unit->radarRadius || unit->sonarRadius || unit->jammerRadius);

	DECLARE_FILTER_EX(WeaponRange, 1, unit->maxRange > minRange,
		float minRange;
		void SetParam(int index, const std::string& value) {
			minRange = atof(value.c_str());
		},
		minRange=0.0f;
	);

	DECLARE_FILTER_EX(AbsoluteHealth, 1, unit->health > minHealth,
		float minHealth;
		void SetParam(int index, const std::string& value) {
			minHealth = atof(value.c_str());
		},
		minHealth=0.0f;
	);

	DECLARE_FILTER_EX(RelativeHealth, 1, unit->health / unit->maxHealth > minHealth,
		float minHealth;
		void SetParam(int index, const std::string& value) {
			minHealth = atof(value.c_str()) * 0.01f; // convert from percent
		},
		minHealth=0.0f;
	);

	DECLARE_FILTER_EX(InPrevSel, 0, prevTypes.find(unit->unitDef->id) != prevTypes.end(),
		std::set<int> prevTypes;
		void Prepare() {
			prevTypes.clear();
			const CUnitSet& tu = selectedUnits.selectedUnits;
			for (CUnitSet::const_iterator si = tu.begin(); si != tu.end(); ++si) {
				prevTypes.insert((*si)->unitDef->id);
			}
		},
	);

	DECLARE_FILTER_EX(NameContain, 1, unit->unitDef->humanName.find(name) != std::string::npos,
		std::string name;
		void SetParam(int index, const std::string& value) {
			name = value;
		},
	);

	DECLARE_FILTER_EX(Category, 1, unit->category == cat,
		unsigned int cat;
		void SetParam(int index, const std::string& value) {
			cat = CCategoryHandler::Instance()->GetCategory(value);
		},
		cat=0;
	);

	DECLARE_FILTER_EX(RulesParamEquals, 2, unit->modParamsMap.find(param) != unit->modParamsMap.end() &&
	                  unit->modParams[unit->modParamsMap.find(param)->second].value == wantedValue,
		std::string param;
		float wantedValue;
		void SetParam(int index, const std::string& value) {
			switch (index) {
				case 0: param = value; break;
				case 1: wantedValue = atof(value.c_str()); break;
			}
		},
		wantedValue=0.0f;
	);

#undef DECLARE_FILTER_EX
#undef DECLARE_FILTER
};



void CSelectionKeyHandler::DoSelection(std::string selectString)
{
	GML_RECMUTEX_LOCK(sel); // DoSelection

	std::list<CUnit*> selection;

//	guicontroller->AddText(selectString.c_str());
	std::string s=ReadToken(selectString);

	if(s=="AllMap"){
		if (!gu->spectatingFullSelect) {
			// team units
			CUnitSet* tu=&teamHandler->Team(gu->myTeam)->units;
			for(CUnitSet::iterator ui=tu->begin();ui!=tu->end();++ui){
				selection.push_back(*ui);
			}
		} else {
			// all units
			std::list<CUnit*>* au=&uh->activeUnits;
			for (std::list<CUnit*>::iterator ui=au->begin();ui!=au->end();++ui){
				selection.push_back(*ui);
			}
		}
	} else if(s=="Visible"){
		if (!gu->spectatingFullSelect) {
			// team units in viewport
			CUnitSet* tu=&teamHandler->Team(gu->myTeam)->units;
			for (CUnitSet::iterator ui=tu->begin();ui!=tu->end();++ui){
				if (camera->InView((*ui)->midPos,(*ui)->radius)){
					selection.push_back(*ui);
				}
			}
		} else {
		  // all units in viewport
			std::list<CUnit*>* au=&uh->activeUnits;
			for (std::list<CUnit*>::iterator ui=au->begin();ui!=au->end();++ui){
				if (camera->InView((*ui)->midPos,(*ui)->radius)){
					selection.push_back(*ui);
				}
			}
		}
	} else if(s=="FromMouse" || s=="FromMouseC"){
		// FromMouse uses distance from a point on the ground,
		// so essentially a selection sphere.
		// FromMouseC uses a cylinder shaped volume for selection,
		// so the heights of the units do not matter.
		const bool cylindrical = (s == "FromMouseC");
		ReadDelimiter(selectString);
		float maxDist=atof(ReadToken(selectString).c_str());

		float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*8000);
		float3 mp=camera->pos+mouse->dir*dist;
		if (cylindrical) {
			mp.y = 0;
		}

		if (!gu->spectatingFullSelect) {
		  // team units in mouse range
			CUnitSet* tu=&teamHandler->Team(gu->myTeam)->units;
			for(CUnitSet::iterator ui=tu->begin();ui!=tu->end();++ui){
				float3 up = (*ui)->pos;
				if (cylindrical) {
					up.y = 0;
				}
				if(mp.SqDistance(up) < Square(maxDist)){
					selection.push_back(*ui);
				}
			}
		} else {
		  // all units in mouse range
			std::list<CUnit*>* au=&uh->activeUnits;
			for(std::list<CUnit*>::iterator ui=au->begin();ui!=au->end();++ui){
				float3 up = (*ui)->pos;
				if (cylindrical) {
					up.y = 0;
				}
				if(mp.SqDistance(up)<Square(maxDist)){
					selection.push_back(*ui);
				}
			}
		}
	} else if(s=="PrevSelection"){
		CUnitSet* su=&selectedUnits.selectedUnits;
		for(CUnitSet::iterator ui=su->begin();ui!=su->end();++ui){
			selection.push_back(*ui);
		}
	} else {
		logOutput.Print("Unknown source token %s",s.c_str());
		return;
	}

	ReadDelimiter(selectString);

	while(true){
		std::string s=ReadDelimiter(selectString);
		if(s=="+")
			break;

		s=ReadToken(selectString);

		bool _not=false;

		if(s=="Not"){
			_not=true;
			ReadDelimiter(selectString);
			s=ReadToken(selectString);
		}

		Filter::Map& filters = Filter::all();
		Filter::Map::iterator f = filters.find(s);

		if (f != filters.end()) {
			f->second->Prepare();
			for (int i = 0; i < f->second->numArgs; ++i) {
				ReadDelimiter(selectString);
				f->second->SetParam(i, ReadToken(selectString));
			}
			std::list<CUnit*>::iterator ui = selection.begin();
			while (ui != selection.end()) {
				if (f->second->ShouldIncludeUnit(*ui) ^ _not) {
					++ui;
				}
				else {
					std::list<CUnit*>::iterator prev = ui++;
					selection.erase(prev);
				}
			}
		}
		else {
			logOutput.Print("Unknown token in filter %s",s.c_str());
			return;
		}
	}

	ReadDelimiter(selectString);
	s=ReadToken(selectString);

	if(s=="ClearSelection"){
		selectedUnits.ClearSelected();

		ReadDelimiter(selectString);
		s=ReadToken(selectString);
	}

	if(s=="SelectAll"){
		for (std::list<CUnit*>::iterator ui=selection.begin();ui!=selection.end();++ui)
			selectedUnits.AddUnit(*ui);
	} else if(s=="SelectOne"){
		if(selection.empty())
			return;
		if(++selectNumber>=selection.size())
			selectNumber=0;

		CUnit* sel = NULL;
		int a=0;
		for (std::list<CUnit*>::iterator ui=selection.begin();ui!=selection.end() && a<=selectNumber;++ui,++a)
			sel=*ui;

		if (sel == NULL)
			return;

		selectedUnits.AddUnit(sel);
		camHandler->CameraTransition(0.8f);
		if(camHandler->GetCurrentControllerNum() != 0){
			camHandler->GetCurrentController().SetPos(sel->pos);
		} else {	//fps camera

			if(camera->rot.x>-1)
				camera->rot.x=-1;

			float3 wantedCamDir;
			wantedCamDir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
			wantedCamDir.y=(float)(sin(camera->rot.x));
			wantedCamDir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
			wantedCamDir.ANormalize();

			camHandler->GetCurrentController().SetPos(sel->pos - wantedCamDir*800);
		}
	} else if(s=="SelectNum"){
		ReadDelimiter(selectString);
		int num=atoi(ReadToken(selectString).c_str());

		if(selection.empty())
			return;

		if(selectNumber>=selection.size())
			selectNumber=0;

		std::list<CUnit*>::iterator ui=selection.begin();
		for (int a=0;a<selectNumber;++a)
			++ui;
		for (int a=0;a<num;++ui,++a){
			if(ui==selection.end())
				ui=selection.begin();
			selectedUnits.AddUnit(*ui);
		}

		selectNumber+=num;
	} else if(s=="SelectPart"){
		ReadDelimiter(selectString);
		float part=atof(ReadToken(selectString).c_str())*0.01f;//convert from percent
		int num=(int)(selection.size()*part);

		if(selection.empty())
			return;

		if(selectNumber>=selection.size())
			selectNumber=0;

		std::list<CUnit*>::iterator ui = selection.begin();
		for (int a = 0; a < selectNumber; ++a)
			++ui;
		for(int a=0;a<num;++ui,++a){
			if(ui==selection.end())
				ui=selection.begin();
			selectedUnits.AddUnit(*ui);
		}

		selectNumber+=num;
	} else {
		logOutput.Print("Unknown token in conclusion %s",s.c_str());
	}
}
