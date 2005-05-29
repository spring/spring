#include "StdAfx.h"
#include "SelectionKeyHandler.h"
#include <fstream>
#include "windows.h"
#include "InfoConsole.h"
#include "Team.h"
#include "SelectedUnits.h"
#include "Unit.h"
#include "Camera.h"
#include "MouseHandler.h"
#include "CameraController.h"
#include "Ground.h"
#include "UnitDef.h"
#include "Building.h"
#include "CategoryHandler.h"
#include "CommandAI.h"
//#include "mmgr.h"

CSelectionKeyHandler selectionKeys;

extern bool keys[256];

CSelectionKeyHandler::CSelectionKeyHandler(void)
{
	selectNumber=0;
	std::ifstream ifs("selectkeys.txt");

	char buf[10000];

	while(ifs.peek()!=EOF && !ifs.eof()){
		ifs >> buf;
		string key(buf);

		if(ifs.peek()==EOF || ifs.eof())
			break;

		ifs >> buf;
		string sel(buf);

		HotKey hk;

		hk.select=sel;
		hk.shift=false;
		hk.control=false;
		hk.alt=false;

		while(true){
			string s=ReadToken(key);

			if(s=="Shift"){
				hk.shift=true;
			} else if (s=="Control"){
				hk.control=true;
			} else if (s=="Alt"){
				hk.alt=true;
			} else {
				char c=s[0];
				if(c>='A' && c<='Z')
					hk.key=c;

				if(c>='0' && c<='9')
					hk.key=c+48-'0';

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

bool CSelectionKeyHandler::KeyPressed(unsigned char key)
{
	for(vector<HotKey>::iterator hi=hotkeys.begin();hi!=hotkeys.end();++hi){
		if(key==hi->key && hi->shift==keys[VK_SHIFT] && hi->control==keys[VK_CONTROL] && hi->alt==keys[VK_MENU]){
			DoSelection(hi->select);
			return true;
		}
	}
	return false;
}

bool CSelectionKeyHandler::KeyReleased(unsigned char key)
{
	return false;
}

string CSelectionKeyHandler::ReadToken(string& s)
{
	string ret;
	char c=s[0];

	while(c && c!='_' && c!='+'){
		s=s.substr(1,string::npos);
		ret+=c;

		c=s[0];
	}
	return ret;
}

string CSelectionKeyHandler::ReadDelimiter(string& s)
{
	string ret=s.substr(0,1);
	s=s.substr(1,string::npos);
	return ret;
}

void CSelectionKeyHandler::DoSelection(string selectString)
{
	list<CUnit*> selection;

	string s=ReadToken(selectString);

	if(s=="AllMap"){
		set<CUnit*>* tu=&gs->teams[gu->myTeam]->units;
		for(set<CUnit*>::iterator ui=tu->begin();ui!=tu->end();++ui){
			selection.push_back(*ui);
		}
	} else if(s=="Visible"){
		set<CUnit*>* tu=&gs->teams[gu->myTeam]->units;
		for(set<CUnit*>::iterator ui=tu->begin();ui!=tu->end();++ui){
			if(camera->InView((*ui)->midPos,(*ui)->radius))
				selection.push_back(*ui);
		}

	} else if(s=="FromMouse"){
		ReadDelimiter(selectString);
		float maxDist=atof(ReadToken(selectString).c_str());

		float dist=ground->LineGroundCol(camera->pos,camera->pos+mouse->dir*8000);
		float3 mp=camera->pos+mouse->dir*dist;

		set<CUnit*>* tu=&gs->teams[gu->myTeam]->units;
		for(set<CUnit*>::iterator ui=tu->begin();ui!=tu->end();++ui){
			if(mp.distance((*ui)->pos)<maxDist)
				selection.push_back(*ui);
		}
	} else if(s=="PrevSelection"){
		set<CUnit*>* tu=&selectedUnits.selectedUnits;
		for(set<CUnit*>::iterator ui=tu->begin();ui!=tu->end();++ui){
			selection.push_back(*ui);
		}
	} else {
		info->AddLine("Unknown source token %s",s.c_str());
		return;
	}

	ReadDelimiter(selectString);

	while(true){
		string s=ReadDelimiter(selectString);
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
			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->unitDef->buildSpeed>0){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Building"){
			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if(dynamic_cast<CBuilding*>(*ui)){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Commander"){
			unsigned int comCat=CCategoryHandler::Instance()->GetCategory("COMMANDER");

			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->category & comCat){	//fix with better test for commander
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Transport"){
			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->unitDef->transportCapacity>0){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Aircraft"){
			unsigned int acCat=CCategoryHandler::Instance()->GetCategory("VTOL");

			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->category & acCat){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Weapons"){
			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if(!(*ui)->weapons.empty()){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="WeaponRange"){
			ReadDelimiter(selectString);
			float minRange=atof(ReadToken(selectString).c_str());

			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->maxRange>minRange){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="AbsoluteHealth"){
			ReadDelimiter(selectString);
			float minHealth=atof(ReadToken(selectString).c_str());

			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->health>minHealth){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="RelativeHealth"){
			ReadDelimiter(selectString);
			float minHealth=atof(ReadToken(selectString).c_str())*0.01;//convert from percent

			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->health/(*ui)->maxHealth > minHealth){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="InPrevSel"){
			set<int> prevTypes;
			set<CUnit*>* tu=&selectedUnits.selectedUnits;
			for(set<CUnit*>::iterator si=tu->begin();si!=tu->end();++si){
				prevTypes.insert((*si)->aihint);
			}
			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if(prevTypes.find((*ui)->aihint)!=prevTypes.end()){		//should move away from aihint
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="NameContain"){
			ReadDelimiter(selectString);
			string name=ReadToken(selectString);

			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->unitDef->humanName.find(name)!=string::npos){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Idle"){
			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->commandAI->commandQue.empty()){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Radar"){
			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->radarRadius || (*ui)->sonarRadius || (*ui)->jammerRadius){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}
		} else if(s=="Category"){
			ReadDelimiter(selectString);
			string catname=ReadToken(selectString);
			unsigned int cat=CCategoryHandler::Instance()->GetCategory(catname);

			list<CUnit*>::iterator ui=selection.begin();
			while(ui!=selection.end()){
				bool filterTrue=false;
				if((*ui)->category==cat){
					filterTrue=true;
				}
				if(filterTrue ^ _not){
					++ui;
				} else {
					list<CUnit*>::iterator prev=ui++;
					selection.erase(prev);
				}
			}

		} else {
			info->AddLine("Unknown token in filter %s",s.c_str());
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
		for(list<CUnit*>::iterator ui=selection.begin();ui!=selection.end();++ui)
			selectedUnits.AddUnit(*ui);
	} else if(s=="SelectOne"){
		if(selection.empty())
			return;
		if(++selectNumber>=selection.size())
			selectNumber=0;

		CUnit* sel;
		int a=0;
		for(list<CUnit*>::iterator ui=selection.begin();ui!=selection.end() && a<=selectNumber;++ui,++a)
			sel=*ui;

		selectedUnits.AddUnit(sel);
		mouse->inStateTransit=true;
		mouse->transitSpeed=0.7;
		if(mouse->currentCamController!=mouse->camControllers[0]){
			mouse->currentCamController->SetPos(sel->pos);
		} else {	//fps camera
			
			if(camera->rot.x>-1)
				camera->rot.x=-1;

			float3 wantedCamDir;
			wantedCamDir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
			wantedCamDir.y=(float)(sin(camera->rot.x));
			wantedCamDir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
			wantedCamDir.Normalize();

			((CFPSController*)mouse->camControllers[0])->pos=sel->pos - wantedCamDir*800;
		}
	} else if(s=="SelectNum"){
		ReadDelimiter(selectString);
		int num=atoi(ReadToken(selectString).c_str());

		if(selection.empty())
			return;

		if(selectNumber>=selection.size())
			selectNumber=0;

		list<CUnit*>::iterator ui=selection.begin();
		for(int a=0;a<selectNumber;++a)
			++ui;
		for(int a=0;a<num;++ui,++a){
			if(ui==selection.end())
				ui=selection.begin();
			selectedUnits.AddUnit(*ui);
		}

		selectNumber+=num;
	} else if(s=="SelectPart"){
		ReadDelimiter(selectString);
		float part=atof(ReadToken(selectString).c_str())*0.01;//convert from percent
		int num=(int)(selection.size()*part);

		if(selection.empty())
			return;

		if(selectNumber>=selection.size())
			selectNumber=0;

		list<CUnit*>::iterator ui=selection.begin();
		for(int a=0;a<selectNumber;++a)
			++ui;
		for(int a=0;a<num;++ui,++a){
			if(ui==selection.end())
				ui=selection.begin();
			selectedUnits.AddUnit(*ui);
		}

		selectNumber+=num;
	} else {
		info->AddLine("Unknown token in conclusion %s",s.c_str());
		return;
	}
}
