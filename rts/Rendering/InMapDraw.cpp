/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "SDL_mouse.h"
#include "SDL_keyboard.h"
#include "mmgr.h"

#include "InMapDraw.h"
#include "glFont.h"
#include "GL/VertexArray.h"
#include "Colors.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/PlayerHandler.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventHandler.h"
#include "System/BaseNetProtocol.h"
#include "System/NetProtocol.h"
#include "System/LogOutput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/AudioChannel.h"
#include "System/creg/STL_List.h"


#define DRAW_QUAD_SIZE 32

CInMapDraw* inMapDrawer = NULL;

CR_BIND(CInMapDraw, );

CR_REG_METADATA(CInMapDraw, (
	CR_MEMBER(drawQuads),
	CR_RESERVED(4)
));

CR_BIND(CInMapDraw::MapPoint, );

CR_REG_METADATA_SUB(CInMapDraw, MapPoint, (
	CR_MEMBER(pos),
	CR_MEMBER(label),
	CR_MEMBER(senderAllyTeam),
	CR_MEMBER(senderSpectator),
	CR_RESERVED(4)
));

CR_BIND(CInMapDraw::MapLine, );

CR_REG_METADATA_SUB(CInMapDraw, MapLine, (
	CR_MEMBER(pos),
	CR_MEMBER(pos2),
	CR_MEMBER(senderAllyTeam),
	CR_MEMBER(senderSpectator),
	CR_RESERVED(4)
));

CR_BIND(CInMapDraw::DrawQuad, );

CR_REG_METADATA_SUB(CInMapDraw, DrawQuad, (
	CR_MEMBER(points),
	CR_MEMBER(lines),
	CR_RESERVED(4)
));


const float quadScale = 1.0f / (DRAW_QUAD_SIZE * SQUARE_SIZE);


CInMapDraw::CInMapDraw(void)
{
	keyPressed = false;
	wantLabel = false;
	drawAll = false;
	allowSpecMapDrawing = true;
	allowLuaMapDrawing = true;
	lastLineTime = 0;
	lastLeftClickTime = 0;
	lastPos = float3(1, 1, 1);

	drawQuadsX = gs->mapx / DRAW_QUAD_SIZE;
	drawQuadsY = gs->mapy / DRAW_QUAD_SIZE;
	numQuads = drawQuadsX * drawQuadsY;
	drawQuads.resize(numQuads);

	unsigned char tex[64][128][4];
	for (int y = 0; y < 64; y++) {
		for (int x = 0; x < 128; x++) {
			tex[y][x][0] = 0;
			tex[y][x][1] = 0;
			tex[y][x][2] = 0;
			tex[y][x][3] = 0;
		}
	}

	#define SMOOTHSTEP(x,y,a) (unsigned char)(x * (1.0f - a) + y * a)

	for (int y = 0; y < 64; y++) {
		// circular thingy
		for (int x = 0; x < 64; x++) {
			float dist = sqrt((float)(x - 32) * (x - 32) + (y - 32) * (y - 32));
			if (dist > 31.0f) {
				// do nothing - leave transparent
			} else if (dist > 30.0f) {
				// interpolate (outline -> nothing)
				float a = (dist - 30.0f);
				tex[y][x][3] = SMOOTHSTEP(255,0,a);
			} else if (dist > 24.0f) {
				// black outline
				tex[y][x][3] = 255;
			} else if (dist > 23.0f) {
				// interpolate (inner -> outline)
				float a = (dist - 23.0f);
				tex[y][x][0] = SMOOTHSTEP(255,0,a);
				tex[y][x][1] = SMOOTHSTEP(255,0,a);
				tex[y][x][2] = SMOOTHSTEP(255,0,a);
				tex[y][x][3] = SMOOTHSTEP(200,255,a);
			} else {
				tex[y][x][0] = 255;
				tex[y][x][1] = 255;
				tex[y][x][2] = 255;
				tex[y][x][3] = 200;
			}
		}
	}
	for (int y = 0; y < 64; y++) {
		// linear falloff
		for (int x = 64; x < 128; x++) {
			float dist = abs(y - 32);
			if (dist > 31.0f) {
				// do nothing - leave transparent
			} else if (dist > 30.0f) {
				// interpolate (outline -> nothing)
				float a = (dist - 30.0f);
				tex[y][x][3] = SMOOTHSTEP(255,0,a);
			} else if (dist > 24.0f) {
				// black outline
				tex[y][x][3] = 255;
			} else if (dist > 23.0f) {
				// interpolate (inner -> outline)
				float a = (dist - 23.0f);
				tex[y][x][0] = SMOOTHSTEP(255,0,a);
				tex[y][x][1] = SMOOTHSTEP(255,0,a);
				tex[y][x][2] = SMOOTHSTEP(255,0,a);
				tex[y][x][3] = SMOOTHSTEP(200,255,a);
			} else {
				tex[y][x][0] = 255;
				tex[y][x][1] = 255;
				tex[y][x][2] = 255;
				tex[y][x][3] = 200;
			}
		}
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, 128, 64, GL_RGBA, GL_UNSIGNED_BYTE, tex[0]);

	blippSound = sound->GetSoundId("MapPoint", false);
}


CInMapDraw::~CInMapDraw(void)
{
	glDeleteTextures (1, &texture);
}

void CInMapDraw::PostLoad()
{
	if (drawQuads.size() != numQuads) {
		// For old savegames
		drawQuads.resize(numQuads);
	}
}

unsigned char DefaultColor[4] = {0, 0, 0, 255};

void SerializeColor(creg::ISerializer &s, unsigned char **color)
{
	if (s.IsWriting()) {
		char ColorType = 0;
		char ColorId = 0;
		if (!ColorType)
			for (int a = 0; a < teamHandler->ActiveTeams(); ++a)
				if (*color==teamHandler->Team(a)->color) {
					ColorType = 1;
					ColorId = a;
					break;
				}
		s.Serialize(&ColorId,sizeof(ColorId));
		s.Serialize(&ColorType,sizeof(ColorType));
	} else {
		char ColorType;
		char ColorId;
		s.Serialize(&ColorId,sizeof(ColorId));
		s.Serialize(&ColorType,sizeof(ColorType));
		switch (ColorType) {
			case 0:{
				*color = DefaultColor;
				break;
			}
			case 1:{
				*color = teamHandler->Team(ColorId)->color;
				break;
			}
		}
	}
}

bool CInMapDraw::MapPoint::MaySee(CInMapDraw* imd) const
{
	const int allyteam = senderAllyTeam;
	const bool spec = senderSpectator;
	const bool allied =
		(teamHandler->Ally(gu->myAllyTeam, allyteam) &&
		teamHandler->Ally(allyteam, gu->myAllyTeam));
	const bool maySee = (gu->spectating || (!spec && allied) || imd->drawAll);
	return maySee;
}

bool CInMapDraw::MapLine::MaySee(CInMapDraw* imd) const
{
	const int allyteam = senderAllyTeam;
	const bool spec = senderSpectator;
	const bool allied =
		(teamHandler->Ally(gu->myAllyTeam, allyteam) &&
		teamHandler->Ally(allyteam, gu->myAllyTeam));
	const bool maySee = (gu->spectating || (!spec && allied) || imd->drawAll);
	return maySee;
}



struct InMapDraw_QuadDrawer: public CReadMap::IQuadDrawer
{
	CVertexArray *va, *lineva;
	CInMapDraw* imd;
	std::vector<CInMapDraw::MapPoint*>* visLabels;

	void DrawQuad(int x, int y);
};

void InMapDraw_QuadDrawer::DrawQuad(int x, int y)
{
	int drawQuadsX = imd->drawQuadsX;
	CInMapDraw::DrawQuad* dq = &imd->drawQuads[y * drawQuadsX + x];

	va->EnlargeArrays(dq->points.size()*12,0,VA_SIZE_TC);
	//! draw point markers
	for (std::list<CInMapDraw::MapPoint>::iterator pi = dq->points.begin(); pi != dq->points.end(); ++pi) {
		if (pi->MaySee(imd)) {
			float3 pos = pi->pos;
			float3 dif(pos - camera->pos);
			dif.ANormalize();
			float3 dir1(dif.cross(UpVector));
			dir1.ANormalize();
			float3 dir2(dif.cross(dir1));

			unsigned char col[4];
			col[0] = pi->color[0];
			col[1] = pi->color[1];
			col[2] = pi->color[2];
			col[3] = 200;

			float size = 6;
			float3 pos1 = pos;
			pos1.y += 5.0f;
			float3 pos2 = pos1;
			pos2.y += 100.0f;

			va->AddVertexQTC(pos1 - dir1 * size,               0.25f, 0, col);
			va->AddVertexQTC(pos1 + dir1 * size,               0.25f, 1, col);
			va->AddVertexQTC(pos1 + dir1 * size + dir2 * size, 0.00f, 1, col);
			va->AddVertexQTC(pos1 - dir1 * size + dir2 * size, 0.00f, 0, col);

			va->AddVertexQTC(pos1 - dir1 * size,               0.75f, 0, col);
			va->AddVertexQTC(pos1 + dir1 * size,               0.75f, 1, col);
			va->AddVertexQTC(pos2 + dir1 * size,               0.75f, 1, col);
			va->AddVertexQTC(pos2 - dir1 * size,               0.75f, 0, col);

			va->AddVertexQTC(pos2 - dir1 * size,               0.25f, 0, col);
			va->AddVertexQTC(pos2 + dir1 * size,               0.25f, 1, col);
			va->AddVertexQTC(pos2 + dir1 * size - dir2 * size, 0.00f, 1, col);
			va->AddVertexQTC(pos2 - dir1 * size - dir2 * size, 0.00f, 0, col);

			if (pi->label.size() > 0) {
				visLabels->push_back(&*pi);
			}
		}
	}

	lineva->EnlargeArrays(dq->lines.size()*2,0,VA_SIZE_C);
	//! draw line markers
	for (std::list<CInMapDraw::MapLine>::iterator li = dq->lines.begin(); li != dq->lines.end(); ++li) {
		if (li->MaySee(imd)) {
			lineva->AddVertexQC(li->pos - (li->pos - camera->pos).ANormalize() * 26, li->color);
			lineva->AddVertexQC(li->pos2 - (li->pos2 - camera->pos).ANormalize() * 26, li->color);
		}
	}
}



void CInMapDraw::Draw(void)
{
	GML_STDMUTEX_LOCK(inmap); //! Draw

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	CVertexArray* lineva = GetVertexArray();
	lineva->Initialize();

	InMapDraw_QuadDrawer drawer;
	drawer.imd = this;
	drawer.lineva = lineva;
	drawer.va = va;
	drawer.visLabels = &visibleLabels;

	glDepthMask(0);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texture);

	readmap->GridVisibility(camera, DRAW_QUAD_SIZE, 3000.0f, &drawer);

	glDisable(GL_TEXTURE_2D);
	glLineWidth(3.f);
	lineva->DrawArrayC(GL_LINES); //! draw lines
	// XXX hopeless drivers, retest in a year or so...
	// width greater than 2 causes GUI flicker on ATI hardware as of driver version 9.3
	// so redraw lines with width 1
	if (globalRendering->atiHacks) {
		glLineWidth(1.f);
		lineva->DrawArrayC(GL_LINES);
	}
	// draw points
	glLineWidth(1);
	glEnable(GL_TEXTURE_2D);
	va->DrawArrayTC(GL_QUADS); //! draw point markers 

	if (visibleLabels.size() > 0) {
		font->SetColors(); //! default

		//! draw point labels
		for (std::vector<MapPoint*>::iterator pi = visibleLabels.begin(); pi != visibleLabels.end(); ++pi) {
			float3 pos = (*pi)->pos;
			pos.y += 111.0f;

			font->SetTextColor((*pi)->color[0]/255.0f, (*pi)->color[1]/255.0f, (*pi)->color[2]/255.0f, 1.0f); //FIXME (overload!)
			font->glWorldPrint(pos, 26.0f, (*pi)->label);
		}

		visibleLabels.clear();
	}

	glDepthMask(1);
}



void CInMapDraw::MousePress(int x, int y, int button)
{
	float3 pos = GetMouseMapPos();
	if (pos.x < 0)
		return;

	switch (button) {
		case SDL_BUTTON_LEFT: {
			if (lastLeftClickTime > gu->gameTime - 0.3f) {
				PromptLabel(pos);
			}
			lastLeftClickTime = gu->gameTime;
			break;
		}
		case SDL_BUTTON_RIGHT: {
			SendErase(pos);
			break;
		}
		case SDL_BUTTON_MIDDLE:{
			SendPoint(pos, "", false);
			break;
		}
	}

	lastPos = pos;
}


void CInMapDraw::MouseRelease(int x, int y, int button)
{
}


void CInMapDraw::MouseMove(int x, int y, int dx,int dy, int button)
{
	float3 pos = GetMouseMapPos();
	if (pos.x < 0) {
		return;
	}
	if (mouse->buttons[SDL_BUTTON_LEFT].pressed && lastLineTime < gu->gameTime - 0.05f) {
		SendLine(pos, lastPos, false);
		lastLineTime = gu->gameTime;
		lastPos = pos;
	}
	if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) {
		SendErase(pos);
	}

}


float3 CInMapDraw::GetMouseMapPos(void)
{
	float dist = ground->LineGroundCol(camera->pos, camera->pos + mouse->dir * globalRendering->viewRange * 1.4f);
	if (dist < 0) {
		return float3(-1, 1, -1);
	}
	float3 pos = camera->pos + mouse->dir * dist;
	pos.CheckInBounds();
	return pos;
}



bool CInMapDraw::AllowedMsg(const CPlayer* sender) const {
	const int  team      = sender->team;
	const int  allyteam  = teamHandler->AllyTeam(team);
	const bool alliedMsg = (teamHandler->Ally(gu->myAllyTeam, allyteam) &&
	                        teamHandler->Ally(allyteam, gu->myAllyTeam));

	if (!gu->spectating && (sender->spectator || !alliedMsg)) {
		// we are playing and the guy sending the
		// net-msg is a spectator or not an ally;
		// cannot just ignore the message due to
		// drawAll mode considerations
		return false;
	}

	return true;
}

void CInMapDraw::GotNetMsg(const unsigned char* msg)
{
	const int playerID = msg[2];

	if ((playerID < 0) || (playerID >= playerHandler->ActivePlayers())) {
		return;
	}
	const CPlayer* sender = playerHandler->Player(playerID);
	if (sender == NULL) {
		return;
	}

	switch (msg[3]) {
		case MAPDRAW_POINT: {
			const float3 pos(*(short*) &msg[4], 0, *(short*) &msg[6]);
			const bool fromLua = msg[8];
			const string label = (char*) &msg[9];
			if (!fromLua || allowLuaMapDrawing) {
				LocalPoint(pos, label, playerID);
			}
			break;
		}
		case MAPDRAW_LINE: {
			const float3 pos1(*(short*) &msg[4], 0, *(short*) &msg[6]);
			const float3 pos2(*(short*) &msg[8], 0, *(short*) &msg[10]);
			const bool fromLua = msg[12];
			if (!fromLua || allowLuaMapDrawing) {
				LocalLine(pos1, pos2, playerID);
			}
			break;
		}
		case MAPDRAW_ERASE: {
			float3 pos(*(short*) &msg[4], 0, *(short*) &msg[6]);
			LocalErase(pos, playerID);
			break;
		}
	}
}

void CInMapDraw::LocalPoint(const float3& constPos, const std::string& label,
                            int playerID)
{
	if ((playerID < 0) || (playerID >= playerHandler->ActivePlayers()))
		return;

	GML_STDMUTEX_LOCK(inmap); // LocalPoint

	// GotNetMsg() alreadys checks validity of playerID
	const CPlayer* sender = playerHandler->Player(playerID);
	const bool allowed = AllowedMsg(sender);

	float3 pos = constPos;
	pos.CheckInBounds();
	pos.y = ground->GetHeight(pos.x, pos.z) + 2.0f;

	// event clients may process the point
	// if their owner is allowed to see it
	if (allowed && eventHandler.MapDrawCmd(playerID, MAPDRAW_POINT, &pos, NULL, &label)) {
		return;
	}

	// let the engine handle it (disallowed
	// points added here are filtered while
	// rendering the quads)
	MapPoint point;
	point.pos = pos;
	point.color = sender->spectator ? color4::white : teamHandler->Team(sender->team)->color;
	point.label = label;
	point.senderAllyTeam = teamHandler->AllyTeam(sender->team);
	point.senderSpectator = sender->spectator;

	const int quad = int(pos.z * quadScale) * drawQuadsX +
	                 int(pos.x * quadScale);
	drawQuads[quad].points.push_back(point);


	if (allowed || drawAll) {
		// if we happen to be in drawAll mode, notify us now
		// even if this message is not intented for our ears
		logOutput.Print("%s added point: %s",
		                sender->name.c_str(), point.label.c_str());
		logOutput.SetLastMsgPos(pos);
		Channels::UserInterface.PlaySample(blippSound, pos);
		minimap->AddNotification(pos, float3(1.0f, 1.0f, 1.0f), 1.0f);
	}
}


void CInMapDraw::LocalLine(const float3& constPos1, const float3& constPos2,
                           int playerID)
{
	GML_STDMUTEX_LOCK(inmap); // LocalLine

	const CPlayer* sender = playerHandler->Player(playerID);

	float3 pos1 = constPos1;
	float3 pos2 = constPos2;
	pos1.CheckInBounds();
	pos2.CheckInBounds();
	pos1.y = ground->GetHeight(pos1.x, pos1.z) + 2.0f;
	pos2.y = ground->GetHeight(pos2.x, pos2.z) + 2.0f;

	if (AllowedMsg(sender) && eventHandler.MapDrawCmd(playerID, MAPDRAW_LINE, &pos1, &pos2, NULL)) {
		return;
	}

	MapLine line;
	line.pos  = pos1;
	line.pos2 = pos2;
	line.color = sender->spectator ? color4::white : teamHandler->Team(sender->team)->color;
	line.senderAllyTeam = teamHandler->AllyTeam(sender->team);
	line.senderSpectator = sender->spectator;

	const int quad = int(pos1.z * quadScale) * drawQuadsX +
	                 int(pos1.x * quadScale);
	drawQuads[quad].lines.push_back(line);
}


void CInMapDraw::LocalErase(const float3& constPos, int playerID)
{
	GML_STDMUTEX_LOCK(inmap); // LocalErase

	const CPlayer* sender = playerHandler->Player(playerID);

	float3 pos = constPos;
	pos.CheckInBounds();
	pos.y = ground->GetHeight(pos.x, pos.z) + 2.0f;

	if (AllowedMsg(sender) && eventHandler.MapDrawCmd(playerID, MAPDRAW_ERASE, &pos, NULL, NULL)) {
		return;
	}

	const float radius = 100.0f;
	const int maxY = drawQuadsY - 1;
	const int maxX = drawQuadsX - 1;
	const int yStart = (int) std::max(0,    int((pos.z - radius) * quadScale));
	const int xStart = (int) std::max(0,    int((pos.x - radius) * quadScale));
	const int yEnd   = (int) std::min(maxY, int((pos.z + radius) * quadScale));
	const int xEnd   = (int) std::min(maxX, int((pos.x + radius) * quadScale));

	for (int y = yStart; y <= yEnd; ++y) {
		for (int x = xStart; x <= xEnd; ++x) {
			DrawQuad* dq = &drawQuads[(y * drawQuadsX) + x];

			std::list<MapPoint>::iterator pi;
			for (pi = dq->points.begin(); pi != dq->points.end(); /* none */) {
				if (pi->pos.SqDistance2D(pos) < (radius*radius) && (pi->senderSpectator == sender->spectator)) {
					pi = dq->points.erase(pi);
				} else {
					++pi;
				}
			}
			std::list<MapLine>::iterator li;
			for (li = dq->lines.begin(); li != dq->lines.end(); /* none */) {
				if (li->pos.SqDistance2D(pos) < (radius*radius) && (li->senderSpectator == sender->spectator)) {
					li = dq->lines.erase(li);
				} else {
					++li;
				}
			}
		}
	}
}


void CInMapDraw::SendErase(const float3& pos)
{
	if (!gu->spectating || allowSpecMapDrawing)
		net->Send(CBaseNetProtocol::Get().SendMapErase(gu->myPlayerNum, (short)pos.x, (short)pos.z));
}


void CInMapDraw::SendPoint(const float3& pos, const std::string& label, bool fromLua)
{
	if (!gu->spectating || allowSpecMapDrawing)
		net->Send(CBaseNetProtocol::Get().SendMapDrawPoint(gu->myPlayerNum, (short)pos.x, (short)pos.z, label, fromLua));
}


void CInMapDraw::SendLine(const float3& pos, const float3& pos2, bool fromLua)
{
	if (!gu->spectating || allowSpecMapDrawing)
		net->Send(CBaseNetProtocol::Get().SendMapDrawLine(gu->myPlayerNum, (short)pos.x, (short)pos.z, (short)pos2.x, (short)pos2.z, fromLua));
}


void CInMapDraw::PromptLabel(const float3& pos)
{
	waitingPoint = pos;
	game->userWriting = true;
	wantLabel = true;
	game->userPrompt = "Label: ";
	game->ignoreChar = '\xA7'; // should do something better here
}


void CInMapDraw::SetSpecMapDrawingAllowed(bool state)
{
	allowSpecMapDrawing = state;
	logOutput.Print("Spectator map drawing is %s", allowSpecMapDrawing? "disabled": "enabled");
}

void CInMapDraw::SetLuaMapDrawingAllowed(bool state)
{
	allowLuaMapDrawing = state;
	logOutput.Print("Lua map drawing is %s", allowLuaMapDrawing? "disabled": "enabled");
}
