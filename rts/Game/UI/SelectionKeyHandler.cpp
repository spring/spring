/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <fstream>

#include "Game/Camera/CameraController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "MouseHandler.h"
#include "SelectionKeyHandler.h"
#include "Map/Ground.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/FileSystem/DataDirsAccess.h"
#include <boost/cstdint.hpp>

CSelectionKeyHandler* selectionKeys;

CSelectionKeyHandler::CSelectionKeyHandler()
	: selectNumber(0)
{
}

CSelectionKeyHandler::~CSelectionKeyHandler()
{
}

std::string CSelectionKeyHandler::ReadToken(std::string& str)
{
	std::string ret;

	size_t index = 0;
	while ((index < str.length()) && (str[index] != '_') && (str[index] != '+')) {
		index++;
	}

	ret = str.substr(0, index);
	str = str.substr(index, std::string::npos);

	return ret;
}


std::string CSelectionKeyHandler::ReadDelimiter(std::string& str)
{
	std::string ret = str.substr(0, 1);
	if (str.size() >= 1) {
		str = str.substr(1, std::string::npos);
	} else {
		str = "";
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

		virtual ~Filter() {}

		/// Called immediately before the filter is used.
		virtual void Prepare() {}

		/// Called immediately before the filter is used for every parameter.
		virtual void SetParam(int index, const std::string& value) {
			assert(false);
		}

		/**
		 * Actual filtering, should return false if unit should be removed
		 * from proposed selection.
		 */
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

	DECLARE_FILTER(Builder, unit->unitDef->buildSpeed > 0)
	DECLARE_FILTER(Building, dynamic_cast<const CBuilding*>(unit) != NULL)
	DECLARE_FILTER(Transport, unit->unitDef->transportCapacity > 0)
	DECLARE_FILTER(Aircraft, unit->unitDef->canfly)
	DECLARE_FILTER(Weapons, !unit->weapons.empty())
	DECLARE_FILTER(Idle, unit->commandAI->commandQue.empty())
	DECLARE_FILTER(Waiting, !unit->commandAI->commandQue.empty() &&
	               (unit->commandAI->commandQue.front().GetID() == CMD_WAIT))
	DECLARE_FILTER(InHotkeyGroup, unit->group != NULL)
	DECLARE_FILTER(Radar, unit->radarRadius || unit->sonarRadius || unit->jammerRadius)
	DECLARE_FILTER(ManualFireUnit, unit->unitDef->canManualFire)

	DECLARE_FILTER_EX(WeaponRange, 1, unit->maxRange > minRange,
		float minRange;
		void SetParam(int index, const std::string& value) {
			minRange = atof(value.c_str());
		},
		minRange=0.0f;
	)

	DECLARE_FILTER_EX(AbsoluteHealth, 1, unit->health > minHealth,
		float minHealth;
		void SetParam(int index, const std::string& value) {
			minHealth = atof(value.c_str());
		},
		minHealth=0.0f;
	)

	DECLARE_FILTER_EX(RelativeHealth, 1, unit->health / unit->maxHealth > minHealth,
		float minHealth;
		void SetParam(int index, const std::string& value) {
			minHealth = atof(value.c_str()) * 0.01f; // convert from percent
		},
		minHealth=0.0f;
	)

	DECLARE_FILTER_EX(InPrevSel, 0, prevTypes.find(unit->unitDef->id) != prevTypes.end(),
		std::set<int> prevTypes;
		void Prepare() {
			prevTypes.clear();
			const CUnitSet& tu = selectedUnitsHandler.selectedUnits;
			for (CUnitSet::const_iterator si = tu.begin(); si != tu.end(); ++si) {
				prevTypes.insert((*si)->unitDef->id);
			}
		},
	)

	DECLARE_FILTER_EX(NameContain, 1, unit->unitDef->humanName.find(name) != std::string::npos,
		std::string name;
		void SetParam(int index, const std::string& value) {
			name = value;
		},
	)

	DECLARE_FILTER_EX(Category, 1, unit->category == cat,
		unsigned int cat;
		void SetParam(int index, const std::string& value) {
			cat = CCategoryHandler::Instance()->GetCategory(value);
		},
		cat=0;
	)
//FIXME: std::strtof is in C99 which M$ doesn't bother to support.
#ifdef _MSC_VER
	#define STRTOF strtod
#else
	#define STRTOF strtof
#endif

	DECLARE_FILTER_EX(RulesParamEquals, 2, unit->modParams.find(param) != unit->modParams.end() &&
			((wantedValueStr.empty()) ? unit->modParams.find(param)->second.valueInt == wantedValue
			: unit->modParams.find(param)->second.valueString == wantedValueStr),
		std::string param;
		float wantedValue;
		std::string wantedValueStr;
		void SetParam(int index, const std::string& value) {
			switch (index) {
				case 0: {
					param = value;
				} break;
				case 1: {
					const char* cstr = value.c_str();
					char* endNumPos = NULL;
					wantedValue = STRTOF(cstr, &endNumPos);
					if (endNumPos == cstr) wantedValueStr = value;
				} break;
			}
		},
		wantedValue=0.0f;
	)

#undef DECLARE_FILTER_EX
#undef DECLARE_FILTER
#undef STRTOF
}



void CSelectionKeyHandler::DoSelection(std::string selectString)
{
	std::list<CUnit*> selection;

//	guicontroller->AddText(selectString.c_str());
	std::string s=ReadToken(selectString);

	if(s=="AllMap"){
		if (!gu->spectatingFullSelect) {
			// team units
			for(CUnit* unit: teamHandler->Team(gu->myTeam)->units){
				selection.push_back(unit);
			}
		} else {
			// all units
			for (CUnit *unit: unitHandler->activeUnits){
				selection.push_back(unit);
			}
		}
	} else if(s=="Visible"){
		if (!gu->spectatingFullSelect) {
			// team units in viewport
			for(CUnit* unit: teamHandler->Team(gu->myTeam)->units){
				if (camera->InView(unit->midPos,unit->radius)){
					selection.push_back(unit);
				}
			}
		} else {
		  // all units in viewport
			for (CUnit *unit: unitHandler->activeUnits){
				if (camera->InView(unit->midPos,unit->radius)){
					selection.push_back(unit);
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

		float dist = CGround::LineGroundCol(camera->GetPos(), camera->GetPos() + mouse->dir * 8000, false);
		float3 mp=camera->GetPos()+mouse->dir*dist;
		if (cylindrical) {
			mp.y = 0;
		}

		if (!gu->spectatingFullSelect) {
		  // team units in mouse range
			for(CUnit* unit: teamHandler->Team(gu->myTeam)->units){
				float3 up = unit->pos;
				if (cylindrical) {
					up.y = 0;
				}
				if(mp.SqDistance(up) < Square(maxDist)){
					selection.push_back(unit);
				}
			}
		} else {
		  // all units in mouse range
			for(CUnit *unit: unitHandler->activeUnits){
				float3 up = unit->pos;
				if (cylindrical) {
					up.y = 0;
				}
				if(mp.SqDistance(up)<Square(maxDist)){
					selection.push_back(unit);
				}
			}
		}
	} else if(s=="PrevSelection"){
		CUnitSet* su=&selectedUnitsHandler.selectedUnits;
		for(CUnitSet::iterator ui=su->begin();ui!=su->end();++ui){
			selection.push_back(*ui);
		}
	} else {
		LOG_L(L_WARNING, "Unknown source token %s", s.c_str());
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
			LOG_L(L_WARNING, "Unknown token in filter %s", s.c_str());
			return;
		}
	}

	ReadDelimiter(selectString);
	s=ReadToken(selectString);

	if(s=="ClearSelection"){
		selectedUnitsHandler.ClearSelected();

		ReadDelimiter(selectString);
		s=ReadToken(selectString);
	}

	if(s=="SelectAll"){
		for (std::list<CUnit*>::iterator ui=selection.begin();ui!=selection.end();++ui)
			selectedUnitsHandler.AddUnit(*ui);
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

		selectedUnitsHandler.AddUnit(sel);
		camHandler->CameraTransition(0.8f);
		if (camHandler->GetCurrentControllerNum() != CCameraHandler::CAMERA_MODE_FIRSTPERSON) {
			camHandler->GetCurrentController().SetPos(sel->pos);
		} else {
			//fps camera
			if (camera->GetRot().x > -1.f)
				camera->SetRotX(-1.f);

			camHandler->GetCurrentController().SetPos(sel->pos - camera->GetDir() * 800);
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
			selectedUnitsHandler.AddUnit(*ui);
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
			selectedUnitsHandler.AddUnit(*ui);
		}

		selectNumber+=num;
	} else {
		LOG_L(L_WARNING, "Unknown token in conclusion %s", s.c_str());
	}
}
