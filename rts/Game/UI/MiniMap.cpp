// MiniMap.cpp: implementation of the CMiniMap class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include "StdAfx.h"
#include "MiniMap.h"
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>
#include "SDL_keysym.h"
#include "SDL_mouse.h"
#include "SDL_types.h"
#include "CommandColors.h"
#include "GuiHandler.h"
#include "InfoConsole.h"
#include "MouseHandler.h"
#include "ExternalAI/Group.h"
#include "Game/CameraController.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/IconHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/Sound.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CMiniMap* minimap;

extern Uint8* keys;


CMiniMap::CMiniMap()
: xpos(2),
	ypos(gu->screeny - 2),
	height(200),
	width(200),
	fullProxy(false),
	proxyMode(false),
	selecting(false),
	mouseMove(false),
	mouseResize(false),
	mouseLook(false),
	maximized(false),
	minimized(false),
	useIcons(true)
{
	float hw=sqrt(float(gs->mapx)/gs->mapy);
	if(gu->dualScreenMode) {
		width = gu->screenx;
		height = gu->screeny;
		xpos = gu->screenx - gu->screenxPos;
		ypos = 0;
	}
	else {
		width = (int) (width*hw);
		height = (int) (height/hw);
		ypos = gu->screeny - height - 2;
	}

	fullProxy = !!configHandler.GetInt("MiniMapFullProxy", 0);

	cursorScale =
		atof(configHandler.GetString("MiniMapCursorScale", "-0.5").c_str());

	useIcons = !!configHandler.GetInt("MiniMapIcons", 1);
	
	simpleColors=!!configHandler.GetInt("SimpleMiniMapColors", 0);
	myColor[0]   =(unsigned char)(0.2f*255);
	myColor[1]   =(unsigned char)(0.9f*255);
	myColor[2]   =(unsigned char)(0.2f*255);
	myColor[3]   =(unsigned char)(1.0f*255);
	allyColor[0] =(unsigned char)(0.3f*255);
	allyColor[1] =(unsigned char)(0.3f*255);
	allyColor[2] =(unsigned char)(0.9f*255);
	allyColor[3] =(unsigned char)(1.0f*255);
	enemyColor[0]=(unsigned char)(0.9f*255);
	enemyColor[1]=(unsigned char)(0.2f*255);
	enemyColor[2]=(unsigned char)(0.2f*255);
	enemyColor[3]=(unsigned char)(1.0f*255);
}


CMiniMap::~CMiniMap()
{
}


/******************************************************************************/

bool CMiniMap::MousePress(int x, int y, int button)
{
	if (minimized) {
		if (x<10 && y<10) {
			minimized = false;
			return true;
		} else {
			return false;
		}
	}

	if ((x <= xpos) || (x >= (xpos + width)) ||
	    (y <= (gu->screeny - ypos - height)) || (y >= (gu->screeny - ypos))) {
		return false; // outside the box
	}
	
	if (button == SDL_BUTTON_LEFT){
		if (guihandler->inCommand >= 0) {
			proxyMode = true;
			ProxyMousePress(x, y, button);
			return true;
		}
		else if ((x > (xpos + width - 10)) && (y > (gu->screeny - ypos - 10))) {
			mouseResize = true;
			return true;
		}
		else if ((x < (xpos + 10)) && (y < (gu->screeny - ypos - height + 10))) {
			mouseMove = true;
			return true;
		}
		else if (!mouse->buttons[SDL_BUTTON_LEFT].chorded) {
			selecting = true;
			return true;
		}
	}
	else if ((fullProxy && (button == SDL_BUTTON_MIDDLE)) ||
					 (!fullProxy && (button == SDL_BUTTON_RIGHT))) {
		MoveView(x, y);
		mouseLook = true;
		return true;
	}
	else if (fullProxy && (button == SDL_BUTTON_RIGHT)) {
		proxyMode = true;
		ProxyMousePress(x, y, button);
		return true;
	}
	
	return false;
}


void CMiniMap::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (mouseMove) {
		xpos += dx;
		ypos -= dy;
		xpos = max(0, xpos);
		if (gu->dualScreenMode) {
			xpos = min((2 * gu->screenx) - width, xpos);
		} else {
			xpos = min(gu->screenx - width, xpos);
		}
		ypos = min(gu->screeny - height, ypos);
		ypos = max(0, ypos);
		return;
	}

	if (mouseResize) {
		ypos-=dy;
		height+=dy;
		width+=dx;
		height = min(gu->screeny, height);
		if (gu->dualScreenMode) {
			width = min(2*gu->screenx, width);
		} else {
			width = min(gu->screenx, width);
		}
		if (keys[SDLK_LSHIFT]) {
			width = (height * gs->mapx) / gs->mapy;
		}
		return;
	}
	
	if (mouseLook) {
		if ((x > xpos) && (x < (xpos + width)) &&
		    (y > (gu->screeny - ypos - height)) && (y < (gu->screeny - ypos))) {
			MoveView(x,y);
		}
		return;
	}
}


void CMiniMap::MouseRelease(int x, int y, int button)
{
	if (mouseMove || mouseResize || mouseLook) {
		mouseMove=false;
		mouseResize=false;
		mouseLook=false;
		return;
	}
	
	if (proxyMode) {
		ProxyMouseRelease(x, y, button);
		proxyMode = false;
		return;
	}
	
	if (selecting) {
		SelectUnits(x, y);
		selecting = false;
		return;
	}

	if (button == SDL_BUTTON_LEFT) {
		if (x>xpos+width-10 && y<gu->screeny-ypos-height+10){
			if (maximized) {
				maximized = false;
				xpos = oldxpos;
				ypos = oldypos;
				width = oldwidth;
				height = oldheight;
			} else {
				maximized = true;
				oldxpos = xpos;
				oldypos = ypos;
				oldwidth = width;
				oldheight = height;
				width = height;
				height = gu->screeny;
				xpos = (gu->screenx - gu->screeny) / 2;
				ypos = 0;
			}
			return;
		}

		if((x < (xpos + 10)) && (y > (gu->screeny - ypos - 10))){
			minimized = true;
			return;
		}
	}
}


/******************************************************************************/

void CMiniMap::MoveView(int x, int y)
{
	float dist=ground->LineGroundCol(camera->pos,camera->pos+camera->forward*gu->viewRange*1.4f);
	float3 dif(0,0,0);
	if(dist>0){
		dif=camera->forward*dist;
	}
	float camHeight=camera->pos.y-ground->GetHeight(camera->pos.x,camera->pos.z);
	float3 clickPos;
	clickPos.x=(float(x-xpos))/width*gs->mapx*8;
	clickPos.z=(float(y-(gu->screeny-ypos-height)))/height*gs->mapy*8;
	mouse->currentCamController->SetPos(clickPos);
}


void CMiniMap::SelectUnit(CUnit* unit, int button) const
{
	CMouseHandler::ButtonPress& press = mouse->buttons[button];

	if(!keys[SDLK_LSHIFT]) {
		selectedUnits.ClearSelected();
	}

	if(press.lastRelease < (gu->gameTime - mouse->doubleClickTime)){
		if(keys[SDLK_LCTRL] &&
		   (selectedUnits.selectedUnits.find(unit) !=
		    selectedUnits.selectedUnits.end())) {
			selectedUnits.RemoveUnit(unit);
		} else {
			selectedUnits.AddUnit(unit);
		}
	} else {
		//double click
		if (unit->group && !keys[SDLK_LCTRL]){
			//select the current unit's group if it has one
			selectedUnits.SelectGroup(unit->group->id);
		} else {
			//select all units of same type
			set<CUnit*>::iterator ui;
			set<CUnit*>& myTeamUnits = gs->Team(gu->myTeam)->units;
			for (ui = myTeamUnits.begin(); ui != myTeamUnits.end(); ++ui) {
				if ((*ui)->aihint == unit->aihint) {
					selectedUnits.AddUnit(*ui);
				}
			}
		}
	}

	press.lastRelease = gu->gameTime;

	if (unit->unitDef->sounds.select.id) {
		sound->PlayUnitReply(unit->unitDef->sounds.select.id, unit,
		                     unit->unitDef->sounds.select.volume);
	}
}


void CMiniMap::SelectUnits(int x, int y) const
{
	if (!keys[SDLK_LSHIFT] && !keys[SDLK_LCTRL]) {
		selectedUnits.ClearSelected();
	}

	CMouseHandler::ButtonPress& bp = mouse->buttons[SDL_BUTTON_LEFT];

	if (fullProxy && (bp.movement > 4)) {
		// use a selection box
		const float3 newPos = GetMapPosition(x, y);
		const float3 oldPos = GetMapPosition(bp.x, bp.y);
		const float xmin = min(oldPos.x, newPos.x);
		const float xmax = max(oldPos.x, newPos.x);
		const float zmin = min(oldPos.z, newPos.z);
		const float zmax = max(oldPos.z, newPos.z);
		
		set<CUnit*>::iterator ui;
		set<CUnit*>& selection = selectedUnits.selectedUnits;
		set<CUnit*>& myUnits = gs->Team(gu->myTeam)->units;

		CUnit* unit;
		int addedunits = 0;
		for (ui = myUnits.begin(); ui != myUnits.end(); ++ui) {
			const float3& midPos = (*ui)->midPos;
			if ((midPos.x > xmin) && (midPos.x < xmax) &&
			    (midPos.z > zmin) && (midPos.z < zmax)) {
				if (keys[SDLK_LCTRL] && (selection.find(*ui) != selection.end())) {
					selectedUnits.RemoveUnit(*ui);
				} else {
					selectedUnits.AddUnit(*ui);
					unit = *ui;
					addedunits++;
				}
			}
		}

		if (addedunits >= 2) {
			sound->PlaySample(mouse->soundMultiselID);
		}
		else if (addedunits == 1) {
			if (unit->unitDef->sounds.select.id) {
				sound->PlayUnitReply(unit->unitDef->sounds.select.id, unit,
														 unit->unitDef->sounds.select.volume);
			}
		}
	}
	else {
		// Single unit
		float size = 0.2f / sqrt((float)(width + height)) * gs->mapx * SQUARE_SIZE;
		float3 pos = GetMapPosition(x, y);
		CUnit* unit = helper->GetClosestFriendlyUnit(pos, size, gu->myAllyTeam);

		set<CUnit*>::iterator ui;
		set<CUnit*>& selection = selectedUnits.selectedUnits;
		set<CUnit*>& myUnits = gs->Team(gu->myTeam)->units;
		
		if (unit && (unit->team == gu->myTeam)){
			if (bp.lastRelease < (gu->gameTime - mouse->doubleClickTime)) {
				if (keys[SDLK_LCTRL] && (selection.find(unit) != selection.end())) {
					selectedUnits.RemoveUnit(unit);
				} else {
					selectedUnits.AddUnit(unit);
				}
			} else {
				//double click
				if (unit->group && !keys[SDLK_LCTRL]) {
					//select the current units group if it has one
					selectedUnits.SelectGroup(unit->group->id);
				} else {
					//select all units of same type
					for (ui = myUnits.begin(); ui != myUnits.end(); ++ui) {
						if ((*ui)->aihint == unit->aihint) {
							selectedUnits.AddUnit(*ui);
						}
					}
				}
			}
			bp.lastRelease = gu->gameTime;

			if (unit->unitDef->sounds.select.id) {
				sound->PlayUnitReply(unit->unitDef->sounds.select.id, unit,
														 unit->unitDef->sounds.select.volume);
			}
		}
	}
}


/******************************************************************************/

CUnit* CMiniMap::GetSelectUnit(const float3& pos) const
{
	const float mapX = gs->mapx * SQUARE_SIZE;
	const float size = 0.2f / sqrt((float)(width + height)) * mapX;
	CUnit* unit = helper->GetClosestUnit(pos, size);
	if (unit != NULL) {
		const int losMask = (LOS_INLOS | LOS_INRADAR);
		if ((unit->losStatus[gu->myAllyTeam] & losMask) || gu->spectating) {
			return unit;
		} else {
			return NULL;
		}
	}
	return unit;
}


float3 CMiniMap::GetMapPosition(int x, int y) const
{
	const float mapHeight = readmap->maxheight + 1000.0f;
	const float mapX = gs->mapx * SQUARE_SIZE;
	const float mapY = gs->mapy * SQUARE_SIZE;
	const float3 pos(mapX * float(x - xpos) / width, mapHeight,
	                 mapY * float(y - (gu->screeny - ypos - height)) / height);
	return pos;
}


void CMiniMap::ProxyMousePress(int x, int y, int button)
{
	float3 mapPos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(mapPos);
	if (unit) {
		mapPos = helper->GetUnitErrorPos(unit, gu->myAllyTeam);
		mapPos.y = readmap->maxheight + 1000.0f;
	}

	CMouseHandler::ButtonPress& bp = mouse->buttons[button];
	bp.camPos = mapPos;
	bp.dir = float3(0.0f, -1.0f, 0.0f);
	
	guihandler->MousePress(x, y, -button);
}


void CMiniMap::ProxyMouseRelease(int x, int y, int button)
{
	CCamera c = *camera;
	const float3 tmpMouseDir = mouse->dir;

	float3 mapPos = GetMapPosition(x, y);
	const CUnit* unit = GetSelectUnit(mapPos);
	if (unit) {
		mapPos = helper->GetUnitErrorPos(unit, gu->myAllyTeam);
		mapPos.y = readmap->maxheight + 1000.0f;
	}

	mouse->dir = float3(0.0f, -1.0f, 0.0f);
	camera->pos = mapPos;
	camera->forward = mouse->dir;

	guihandler->MouseRelease(x, y, -button);
	
	mouse->dir = tmpMouseDir;
	*camera = c;
}


/******************************************************************************/

bool CMiniMap::IsAbove(int x, int y)
{
	if(minimized && x<10 && y<10){
		return true;
	}
	if(minimized)
		return false;

	if((x>xpos) && (x<xpos+width) && (y>gu->screeny-ypos-height) && (y<gu->screeny-ypos)){
		return true;
	}
	return false;
}


std::string CMiniMap::GetTooltip(int x, int y)
{
	if(minimized)
		return "Unminimize map";

	if(x<xpos+10 && y>gu->screeny-ypos-10)
		return "Minimize map";

	if(x>xpos+width-10 && y<gu->screeny-ypos-height+10)
		return "Maximize map";

	if(x>xpos+width-10 && y>gu->screeny-ypos-10)
		return "Resize map";

	if (x<xpos+10 && y<gu->screeny-ypos-height+10)
		return "Move map";

	const string buildTip = guihandler->GetBuildTooltip();
	if (!buildTip.empty()) {
		return buildTip;
	}
	
	CUnit* unit = GetSelectUnit(GetMapPosition(x, y));
	
	if (unit) {
		// don't show the tooltip if it's a radar dot
		if(!gu->spectating && (gs->AllyTeam(unit->team) != gu->myAllyTeam) &&
		   !loshandler->InLos(unit,gu->myAllyTeam)){
			return "Enemy unit";
		}
		// show the player name instead of unit name if it has FBI tag showPlayerName
		string s;
		if(unit->unitDef->showPlayerName){
			s=gs->players[gs->Team(unit->team)->leader]->playerName.c_str();
		} else {
			s=unit->tooltip;
		}
		// don't show the unit health and other info if it has
		// FBI tag hideDamage and isn't on our ally team
		if(!(!gu->spectating && unit->unitDef->hideDamage &&
		     (gs->AllyTeam(unit->team) != gu->myAllyTeam))){
			char tmp[500];

			sprintf(tmp,"\nHealth %.0f/%.0f",unit->health,unit->maxHealth);
			s+=tmp;

			if(unit->unitDef->maxFuel>0){
				sprintf(tmp," Fuel %.0f/%.0f",unit->currentFuel,unit->unitDef->maxFuel);
				s+=tmp;
			}

			sprintf(tmp,"\nExperience %.2f Cost %.0f Range %.0f \n\xff\xd3\xdb\xffMetal: \xff\x50\xff\x50%.1f\xff\x90\x90\x90/\xff\xff\x50\x01-%.1f\xff\xd3\xdb\xff Energy: \xff\x50\xff\x50%.1f\xff\x90\x90\x90/\xff\xff\x50\x01-%.1f",
				unit->experience,unit->metalCost+unit->energyCost/60,unit->maxRange, unit->metalMake, unit->metalUse, unit->energyMake, unit->energyUse);
			s+=tmp;
		}

		if (gs->cheatEnabled) {
			char buf[32];
			SNPRINTF(buf, 32, "\xff\xc0\xc0\xff  [TechLevel %i]", unit->unitDef->techLevel);
			s += buf;
		}
		return s;
	}
	
	float3 pos(float(x-xpos)/width*gs->mapx*SQUARE_SIZE,500,float(y-(gu->screeny-ypos-height))/height*gs->mapx*SQUARE_SIZE);

	char tmp[500];

	CReadMap::TerrainType* tt=&readmap->terrainTypes[readmap->typemap[min(gs->hmapx*gs->hmapy-1,max(0,((int)pos.z/16)*gs->hmapx+((int)pos.x/16)))]];
	string ttype=tt->name;
	sprintf(tmp, "Pos %.0f %.0f Elevation %.0f\n"
	             "Terrain type: %s\n"
	             "Speeds T/K/H/S %.2f %.2f %.2f %.2f\n"
	             "Hardness %.0f Metal %.1f",
	             pos.x, pos.z, ground->GetHeight2(pos.x,pos.z), ttype.c_str(),
	             tt->tankSpeed, tt->kbotSpeed, tt->hoverSpeed, tt->shipSpeed,
	             tt->hardness * mapDamage->mapHardness,
	             readmap->metalMap->getMetalAmount((int)(pos.x/16), (int)(pos.z/16)));

	return tmp;
}


void CMiniMap::AddNotification(float3 pos, float3 color, float alpha)
{
	Notification n;
	n.pos=pos;
	n.color=color;
	n.alpha=alpha;
	n.creationTime=gu->gameTime;

	notes.push_back(n);
}


/******************************************************************************/

inline void CMiniMap::DrawInMap(const float3& pos)
{
	glVertex2f(pos.x/(gs->mapx*SQUARE_SIZE),1-pos.z/(gs->mapy*SQUARE_SIZE));
}


void CMiniMap::Draw()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(minimized){
		glColor4f(1,1,1,0.5f);
		glDisable(GL_TEXTURE_2D);
		glViewport(gu->screenxPos,gu->screeny-10,10,10);
		glBegin(GL_QUADS);
		glVertex2f(0,0);
		glVertex2f(1,0);
		glVertex2f(1,1);
		glVertex2f(0,1);
		glEnd();
		glViewport(gu->screenxPos,0,gu->screenx,gu->screeny);
		return;
	}

	glViewport(xpos,ypos,width,height);
	glLoadIdentity();             // Reset The Current Modelview Matrix
	glMatrixMode(GL_PROJECTION);  // Select The Projection Matrix
	glLoadIdentity();             // Reset The Projection Matrix
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);   // Select The Modelview Matrix

	glColor4f(0.6f,0.6f,0.6f,1);

	readmap->DrawMinimap();

	glEnable(GL_BLEND);
	float size=0.25f/sqrt((float)width+height);

	// draw the units
	list<CUnit*>::iterator ui;
	for(ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();ui++){
		DrawUnit(*ui, size);
	}
	
	// highlight the selected unit
	CUnit* unit = GetSelectUnit(GetMapPosition(mouse->lastx, mouse->lasty));
	if (unit != NULL) {
		DrawUnitHighlight(unit, size);
	}

	glDisable(GL_TEXTURE_2D);

	left.clear();

	//Add restraints for camera sides
	GetFrustumSide(cam2->bottom);
	GetFrustumSide(cam2->top);
	GetFrustumSide(cam2->rightside);
	GetFrustumSide(cam2->leftside);

	std::vector<fline>::iterator fli,fli2;
	for(fli=left.begin();fli!=left.end();fli++){
	  for(fli2=left.begin();fli2!=left.end();fli2++){
			if(fli==fli2)
				continue;
			float colz=0;
			if(fli->dir-fli2->dir==0)
				continue;
			colz=-(fli->base-fli2->base)/(fli->dir-fli2->dir);
			if(fli2->left*(fli->dir-fli2->dir)>0){
				if(colz>fli->minz && colz<400096)
					fli->minz=colz;
			} else {
				if(colz<fli->maxz && colz>-10000)
					fli->maxz=colz;
			}
		}
	}
	glColor4f(1,1,1,0.5f);
	glBegin(GL_LINES);
	for(fli=left.begin();fli!=left.end();fli++){
		if(fli->minz<fli->maxz){
			DrawInMap(float3(fli->base+fli->dir*fli->minz,0,fli->minz));
			DrawInMap(float3(fli->base+fli->dir*fli->maxz,0,fli->maxz));
		}
	}
	glEnd();

	glColor4f(1,1,1,0.0002f*(width+height));
	glBegin(GL_POINTS);
		Projectile_List::iterator psi;
		for(psi=ph->ps.begin();psi != ph->ps.end();++psi){
			CProjectile* p=*psi;
			if((p->owner && p->owner->allyteam==gu->myAllyTeam) || loshandler->InLos(p,gu->myAllyTeam)){
				DrawInMap((*psi)->pos);
			}
		}
	glEnd();

	for(set<CUnit*>::iterator si=selectedUnits.selectedUnits.begin();si!=selectedUnits.selectedUnits.end();++si){
		if((*si)->radarRadius && !(*si)->beingBuilt && (*si)->activated){
			glColor3f(0.3f,1,0.3f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrt((float)height)*(*si)->radarRadius*(SQUARE_SIZE*RADAR_SIZE)/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*(*si)->radarRadius*(SQUARE_SIZE*RADAR_SIZE));
			}
			glEnd();
		}
		if((*si)->sonarRadius && !(*si)->beingBuilt && (*si)->activated){
			glColor3f(0.3f,1,0.3f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrt((float)height)*(*si)->sonarRadius*(SQUARE_SIZE*RADAR_SIZE)/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*(*si)->sonarRadius*(SQUARE_SIZE*RADAR_SIZE));
			}
			glEnd();
		}
		if((*si)->jammerRadius && !(*si)->beingBuilt && (*si)->activated){
			glColor3f(1.0f,0.2f,0.2f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrt((float)height)*(*si)->jammerRadius*(SQUARE_SIZE*RADAR_SIZE)/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*(*si)->jammerRadius*(SQUARE_SIZE*RADAR_SIZE));
			}
			glEnd();
		}
		if((*si)->stockpileWeapon && (*si)->stockpileWeapon->weaponDef->interceptor){		//change if someone someday create a non stockpiled interceptor
			CWeapon* w=(*si)->stockpileWeapon;
			if(w->numStockpiled)
				glColor4f(1.0f,1.0f,1.0f,1.0f);
			else
				glColor4f(0.0f,0.0f,0.0f,0.6f);
			glBegin(GL_LINE_STRIP);
			int numSegments=(int)(sqrt((float)height)*w->weaponDef->coverageRange/2000)+8;
			for(int a=0;a<=numSegments;++a){
				DrawInMap((*si)->pos+float3(sin(a*2*PI/numSegments),0,cos(a*2*PI/numSegments))*w->weaponDef->coverageRange);
			}
			glEnd();
		}
	}

	// selection box
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	CMouseHandler::ButtonPress& bp = mouse->buttons[SDL_BUTTON_LEFT];
	if (selecting && fullProxy && (bp.movement > 4)) {
		const float3 oldPos = GetMapPosition(bp.x, bp.y);
		const float3 newPos = GetMapPosition(mouse->lastx, mouse->lasty);
		glColor4fv(cmdColors.mouseBox);
		glBlendFunc((GLenum)cmdColors.MouseBoxBlendSrc(),
		            (GLenum)cmdColors.MouseBoxBlendDst());
		glLineWidth(cmdColors.MouseBoxLineWidth());
		glBegin(GL_LINE_LOOP);
			DrawInMap(float3(oldPos.x, 0.0f, oldPos.z));
			DrawInMap(float3(newPos.x, 0.0f, oldPos.z));
			DrawInMap(float3(newPos.x, 0.0f, newPos.z));
			DrawInMap(float3(oldPos.x, 0.0f, newPos.z));
		glEnd();
		glLineWidth(1.0f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
	DrawNotes();

	glColor4f(1,1,1,0.5f);
	glViewport(xpos,ypos,10,10);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();
	glViewport(xpos+width-10,ypos,10,10);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();
	glViewport(xpos+width-10,ypos+height-10,10,10);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();
	glViewport(xpos,ypos+height-10,10,10);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();

	// outline
	glLineWidth(1.51f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glViewport(xpos - 1, ypos - 1, width + 2, height + 2);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	glViewport(xpos - 2, ypos - 2, width + 4, height + 4);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glRectf(0.0f, 0.0f, 1.0f, 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth(1.0f);
	
	glViewport(gu->screenxPos,0,gu->screenx,gu->screeny);
}


void CMiniMap::GetFrustumSide(float3& side)
{
	fline temp;
	float3 up(0,1,0);

	float3 b=up.cross(side);		//get vector for collision between frustum and horizontal plane
	if(fabs(b.z)<0.0001f)
		b.z=0.00011f;
	if(fabs(b.z)>0.0001f){
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(side);			//get vector from camera to collision line
		float3 colpoint;				//a point on the collision line

		if(side.y>0)
			colpoint=cam2->pos-c*((cam2->pos.y)/c.y);
		else
			colpoint=cam2->pos-c*((cam2->pos.y)/c.y);


		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		temp.left=-1;
		if(b.z>0)
			temp.left=1;
		temp.maxz=gs->mapy*SQUARE_SIZE;
		temp.minz=0;
		left.push_back(temp);
	}

}


inline CIcon* CMiniMap::GetUnitIcon(CUnit* unit, float& scale) const
{
	if (!useIcons) {
		if (unit->losStatus[gu->myAllyTeam] & LOS_INRADAR) {
			return iconHandler->GetIcon("default");
		} else {
			return NULL;
		}
	}

	if ((unit->allyteam == gu->myAllyTeam) ||
	    (unit->losStatus[gu->myAllyTeam] & LOS_INLOS) ||
	    ((unit->losStatus[gu->myAllyTeam] & LOS_PREVLOS) &&
	     (unit->losStatus[gu->myAllyTeam] & LOS_CONTRADAR)) ||
	    gu->spectating) {
		CIcon* icon = iconHandler->GetIcon(unit->unitDef->iconType);
		if (icon->radiusAdjust) {
			scale *= (unit->radius / 30.0f);
		}
		return icon;
	}

	if (unit->losStatus[gu->myAllyTeam] & LOS_INRADAR) {
		return iconHandler->GetIcon("default");
	}
	
	return NULL;
}


void CMiniMap::DrawUnit(CUnit* unit, float size)
{
	if(unit->lastDamage>gs->frameNum-90 && gs->frameNum&8)
		return;

	float3 pos=helper->GetUnitErrorPos(unit,gu->myAllyTeam);
/*	if(pos.z<0 || pos.z>gs->mapy*SQUARE_SIZE){
		logOutput.Print("Errenous position in minimap::drawunit %f %f %f",pos.x,pos.y,pos.z);
		return;
	}*/

	CIcon* icon = GetUnitIcon(unit, size);
	if (icon == NULL) {
		return;
	}

	size *= icon->size;

	if(unit->commandAI->selected) {
		glColor3f(1,1,1);
	} else {
		if(simpleColors){
			if(unit->team==gu->myTeam){
				glColor3ubv(myColor);
			} else if (gs->Ally(gu->myAllyTeam,unit->allyteam)){
				glColor3ubv(allyColor);
			} else {
				glColor3ubv(enemyColor);
			}
		} else {
			glColor3ubv(gs->Team(unit->team)->color);
		}
	}

	float x=pos.x/(gs->mapx*SQUARE_SIZE);
	float y=1-pos.z/(gs->mapy*SQUARE_SIZE);

	float aspectRatio=float(width)/float(height);

	glBindTexture(GL_TEXTURE_2D,icon->texture);
	glBegin(GL_QUADS);
	glTexCoord2f(1,0);
	glVertex2f(x+size,y+size*aspectRatio);
	glTexCoord2f(1,1);
	glVertex2f(x+size,y-size*aspectRatio);
	glTexCoord2f(0,1);
	glVertex2f(x-size,y-size*aspectRatio);
	glTexCoord2f(0,0);
	glVertex2f(x-size,y+size*aspectRatio);
	glEnd();
}


void CMiniMap::DrawUnitHighlight(CUnit* unit, float size)
{
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_COPY_INVERTED);

	DrawUnit(unit, size * 2.0f);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_COLOR_LOGIC_OP);

	DrawUnit(unit, size);

	return;
}


void CMiniMap::DrawNotes(void)
{
	float baseSize=gs->mapx*SQUARE_SIZE;
	glBegin(GL_LINES);
	for(list<Notification>::iterator ni=notes.begin();ni!=notes.end();){
		float age=gu->gameTime-ni->creationTime;
		if(age>2){
			ni=notes.erase(ni);
			continue;
		}
		glColor4f(ni->color.x,ni->color.y,ni->color.z,ni->alpha);
		for(int a=0;a<3;++a){
			float modage=age+a*0.1f;
			float rot=modage*3;
			float size=baseSize-modage*baseSize*0.9f;
			if(size<0){
				if(size<-baseSize*0.4f)
					continue;
				else if(size>-baseSize*0.2f)
					size=modage*baseSize*0.9f-baseSize;
				else
					size=baseSize*1.4f-modage*baseSize*0.9f;
			}
			DrawInMap(ni->pos+float3(sin(rot),0,cos(rot))*size);
			DrawInMap(ni->pos+float3(cos(rot),0,-sin(rot))*size);

			DrawInMap(ni->pos+float3(cos(rot),0,-sin(rot))*size);
			DrawInMap(ni->pos+float3(-sin(rot),0,-cos(rot))*size);

			DrawInMap(ni->pos+float3(-sin(rot),0,-cos(rot))*size);
			DrawInMap(ni->pos+float3(-cos(rot),0,sin(rot))*size);

			DrawInMap(ni->pos+float3(-cos(rot),0,sin(rot))*size);
			DrawInMap(ni->pos+float3(sin(rot),0,cos(rot))*size);
		}
		++ni;
	}
	glEnd();
}


/******************************************************************************/
