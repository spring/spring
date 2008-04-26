#include "StdAfx.h"
#include <fstream>
#include <SDL_keysym.h>
#include <SDL_types.h>
#include "Game/Camera/CameraController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "MouseHandler.h"
#include "Platform/FileSystem.h"
#include "SelectionKeyHandler.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "mmgr.h"

CSelectionKeyHandler *selectionKeys;

extern Uint8 *keys;

CSelectionKeyHandler::CSelectionKeyHandler(void)
{
	std::ifstream ifs(filesystem.LocateFile("selectkeys.txt").c_str());

	char buf[10000];

	selectNumber=0;

	while(ifs.peek()!=EOF && !ifs.eof()){
		ifs >> buf;
		std::string key(buf);

		if(ifs.peek()==EOF || ifs.eof())
			break;

		ifs >> buf;
		std::string sel(buf);

		HotKey hk;

		hk.select=sel;
		hk.shift=false;
		hk.control=false;
		hk.alt=false;

		while(true){
			std::string s=ReadToken(key);

			if(s=="Shift"){
				hk.shift=true;
			} else if (s=="Control"){
				hk.control=true;
			} else if (s=="Alt"){
				hk.alt=true;
			} else {
				char c=s[0];
				if(c>='A' && c<='Z')
					hk.key=SDLK_a + (c - 'A');

				if(c>='0' && c<='9')
					hk.key=SDLK_0 + (c -'0');

				break;
			}
			ReadDelimiter(key);
		}

		hotkeys.push_back(hk);
	}
}

CSelectionKeyHandler::~CSelectionKeyHandler(void)
{
}

bool CSelectionKeyHandler::KeyPressed(unsigned short key, bool isRepeat)
{
	// TODO: sort the vector, and do key-based fast lookup
	for(vector<HotKey>::iterator hi=hotkeys.begin();hi!=hotkeys.end();++hi){
		if(key==hi->key && hi->shift==!!keys[SDLK_LSHIFT] && hi->control==!!keys[SDLK_LCTRL] && hi->alt==!!keys[SDLK_LALT]){
			DoSelection(hi->select);
			return true;
		}
	}
	return false;
}

bool CSelectionKeyHandler::KeyReleased(unsigned short key)
{
	return false;
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


void CSelectionKeyHandler::DoSelection(std::string selectString)
{
	std::list<CUnit*> selection;

//	guicontroller->AddText(selectString.c_str());
	std::string s=ReadToken(selectString);

	if(s=="AllMap"){
		if (!gu->spectatingFullSelect) {
			// team units
			CUnitSet* tu=&gs->Team(gu->myTeam)->units;
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
			CUnitSet* tu=&gs->Team(gu->myTeam)->units;
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
	} else if(s=="FromMouse"){
		ReadDelimiter(selectString);
		float maxDist=atof(ReadToken(selectString).c_str());

		float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*8000);
		float3 mp=camera->pos+mouse->dir*dist;

		if (!gu->spectatingFullSelect) {
		  // team units in mouse range
			CUnitSet* tu=&gs->Team(gu->myTeam)->units;
			for(CUnitSet::iterator ui=tu->begin();ui!=tu->end();++ui){
				if(mp.distance((*ui)->pos)<maxDist){
					selection.push_back(*ui);
				}
			}
		} else {
		  // all units in mouse range
			std::list<CUnit*>* au=&uh->activeUnits;
			for(std::list<CUnit*>::iterator ui=au->begin();ui!=au->end();++ui){
				if(mp.distance((*ui)->pos)<maxDist){
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

		if(s=="Builder"){
			std::list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->unitDef->buildSpeed>0){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Building"){
			std::list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if(dynamic_cast<CBuilding*>(*ui)){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Commander"){
			unsigned int comCat=CCategoryHandler::Instance()->GetCategory("COMMANDER");

			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui!=selection.end()) {
				bool filterTrue=false;
				if ((*ui)->category & comCat){	//fix with better test for commander
					filterTrue=true;
				}
				if (filterTrue ^ _not) {
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Transport"){
			std::list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->unitDef->transportCapacity>0){
					filterTrue=true;
				}
				if (filterTrue ^ _not) {
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Aircraft"){
			unsigned int acCat=CCategoryHandler::Instance()->GetCategory("VTOL");

			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui != selection.end()) {
				bool filterTrue=false;
				if ((*ui)->category & acCat){
					filterTrue=true;
				}
				if (filterTrue ^ _not) {
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Weapons"){
			std::list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if(!(*ui)->weapons.empty()){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="WeaponRange"){
			ReadDelimiter(selectString);
			float minRange=atof(ReadToken(selectString).c_str());

			std::list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if ((*ui)->maxRange>minRange) {
					filterTrue=true;
				}
				if (filterTrue ^ _not) {
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="AbsoluteHealth"){
			ReadDelimiter(selectString);
			float minHealth=atof(ReadToken(selectString).c_str());

			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui != selection.end()) {
				bool filterTrue=false;
				if ((*ui)->health>minHealth){
					filterTrue=true;
				}
				if (filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="RelativeHealth"){
			ReadDelimiter(selectString);
			float minHealth=atof(ReadToken(selectString).c_str())*0.01f;//convert from percent

			std::list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->health/(*ui)->maxHealth > minHealth){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="InPrevSel"){
			std::set<int> prevTypes;
			CUnitSet* tu=&selectedUnits.selectedUnits;
			for (CUnitSet::iterator si=tu->begin();si!=tu->end();++si){
				prevTypes.insert((*si)->aihint);
			}
			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui != selection.end()) {
				bool filterTrue=false;
				if(prevTypes.find((*ui)->aihint)!=prevTypes.end()){		//should move away from aihint
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="NameContain"){
			ReadDelimiter(selectString);
			std::string name=ReadToken(selectString);

			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui != selection.end()) {
				bool filterTrue=false;
				if ((*ui)->unitDef->humanName.find(name)!=std::string::npos){
					filterTrue=true;
				}
				if (filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Idle"){
			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui != selection.end()) {
				bool filterTrue=false;
				if((*ui)->commandAI->commandQue.empty()){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Waiting"){
			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui != selection.end()) {
				bool filterTrue=false;
				if(!(*ui)->commandAI->commandQue.empty() &&
				   ((*ui)->commandAI->commandQue.front().id == CMD_WAIT)){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="InHotkeyGroup"){
			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui != selection.end()) {
				bool filterTrue=false;
				if((*ui)->group){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if (s == "Radar") {
			std::list<CUnit*>::iterator ui=selection.begin();
			while (ui != selection.end()) {
				bool filterTrue=false;
				if((*ui)->radarRadius || (*ui)->sonarRadius || (*ui)->jammerRadius){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Category"){
			ReadDelimiter(selectString);
			std::string catname=ReadToken(selectString);
			unsigned int cat=CCategoryHandler::Instance()->GetCategory(catname);

			std::list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->category==cat){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					std::list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}

		} else {
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
			wantedCamDir.Normalize();

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
