/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "SDL_mouse.h"
#include "SDL_keyboard.h"
#include "mmgr.h"

#include "InMapDraw.h"
#include "Colors.h"
#include "glFont.h"
#include "GL/VertexArray.h"
#include "ExternalAI/AILegacySupport.h" // {Point, Line}Marker
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/PlayerHandler.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Net/UnpackPacket.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventHandler.h"
#include "System/BaseNetProtocol.h"
#include "System/NetProtocol.h"
#include "System/LogOutput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"
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
	drawAllMarks = false;
	allowSpecMapDrawing = true;
	allowLuaMapDrawing = true;
	lastDrawTime = 0;
	lastLeftClickTime = 0;
	lastPos = float3(1, 1, 1);

	drawQuadsX = gs->mapx / DRAW_QUAD_SIZE;
	drawQuadsY = gs->mapy / DRAW_QUAD_SIZE;
	drawQuads.resize(drawQuadsX * drawQuadsY);

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
	if (drawQuads.size() != (drawQuadsX * drawQuadsY)) {
		// For old savegames
		drawQuads.resize(drawQuadsX * drawQuadsY);
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
	const bool maySee = (gu->spectating || (!spec && allied) || imd->drawAllMarks);
	return maySee;
}

bool CInMapDraw::MapLine::MaySee(CInMapDraw* imd) const
{
	const int allyteam = senderAllyTeam;
	const bool spec = senderSpectator;
	const bool allied =
		(teamHandler->Ally(gu->myAllyTeam, allyteam) &&
		teamHandler->Ally(allyteam, gu->myAllyTeam));
	const bool maySee = (gu->spectating || (!spec && allied) || imd->drawAllMarks);
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
	const int drawQuadsX = imd->drawQuadsX;
	CInMapDraw::DrawQuad* dq = &imd->drawQuads[y * drawQuadsX + x];

	va->EnlargeArrays(dq->points.size()*12,0,VA_SIZE_TC);
	//! draw point markers
	for (std::list<CInMapDraw::MapPoint>::iterator pi = dq->points.begin(); pi != dq->points.end(); ++pi) {
		if (pi->MaySee(imd)) {
			const float3 pos = pi->pos;
			const float3 dif = (pos - camera->pos).ANormalize();
			const float3 dir1 = (dif.cross(UpVector)).ANormalize();
			const float3 dir2 = (dif.cross(dir1));

			const unsigned char col[4] = {
				pi->color[0],
				pi->color[1],
				pi->color[2],
				200
			};

			const float size = 6.0f;
			const float3 pos1(pos.x,  pos.y  +   5.0f, pos.z);
			const float3 pos2(pos1.x, pos1.y + 100.0f, pos1.z);

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

			if (!pi->label.empty()) {
				visLabels->push_back(&*pi);
			}
		}
	}

	lineva->EnlargeArrays(dq->lines.size() * 2, 0, VA_SIZE_C);
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

	if (!visibleLabels.empty()) {
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
	//TODO
}


void CInMapDraw::MouseMove(int x, int y, int dx,int dy, int button)
{
	float3 pos = GetMouseMapPos();
	if (pos.x < 0) {
		return;
	}
	if (mouse->buttons[SDL_BUTTON_LEFT].pressed && lastDrawTime < gu->gameTime - 0.05f) {
		SendLine(pos, lastPos, false);
		lastDrawTime = gu->gameTime;
		lastPos = pos;
	}
	if (mouse->buttons[SDL_BUTTON_RIGHT].pressed && lastDrawTime < gu->gameTime - 0.05f) {
		SendErase(pos);
		lastDrawTime = gu->gameTime;
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
		// drawAllMarks mode considerations
		return false;
	}

	return true;
}

int CInMapDraw::GotNetMsg(boost::shared_ptr<const netcode::RawPacket> &packet)
{
	int playerID = -1;

	try {
		netcode::UnpackPacket pckt(packet, 2);

		unsigned char uPlayerID;
		pckt >> uPlayerID;
		if (!playerHandler->IsValidPlayer(uPlayerID)) {
			throw netcode::UnpackPacketException("Invalid player number");
		}
		playerID = uPlayerID;

		unsigned char drawType;
		pckt >> drawType;

		switch (drawType) {
			case MAPDRAW_POINT: {
				short int x,z;
				pckt >> x;
				pckt >> z;
				const float3 pos(x, 0, z);
				unsigned char fromLua;
				pckt >> fromLua;
				string label;
				pckt >> label;
				if (!fromLua || allowLuaMapDrawing) {
					LocalPoint(pos, label, playerID);
				}
				break;
			}
			case MAPDRAW_LINE: {
				short int x1,z1,x2,z2;
				pckt >> x1;
				pckt >> z1;
				pckt >> x2;
				pckt >> z2;
				const float3 pos1(x1, 0, z1);
				const float3 pos2(x2, 0, z2);
				unsigned char fromLua;
				pckt >> fromLua;
				if (!fromLua || allowLuaMapDrawing) {
					LocalLine(pos1, pos2, playerID);
				}
				break;
			}
			case MAPDRAW_ERASE: {
				short int x,z;
				pckt >> x;
				pckt >> z;
				float3 pos(x, 0, z);
				LocalErase(pos, playerID);
				break;
			}
		}
	} catch (netcode::UnpackPacketException &e) {
		logOutput.Print("Got invalid MapDraw: %s", e.err.c_str());
		playerID = -1;
	}

	return playerID;
}

void CInMapDraw::LocalPoint(const float3& constPos, const std::string& label,
                            int playerID)
{
	if (!playerHandler->IsValidPlayer(playerID))
		return;

	GML_STDMUTEX_LOCK(inmap); // LocalPoint

	// GotNetMsg() alreadys checks validity of playerID
	const CPlayer* sender = playerHandler->Player(playerID);
	const bool allowed = AllowedMsg(sender);

	float3 pos = constPos;
	pos.CheckInBounds();
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z) + 2.0f;

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


	if (allowed || drawAllMarks) {
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
	if (!playerHandler->IsValidPlayer(playerID))
		return;

	GML_STDMUTEX_LOCK(inmap); // LocalLine

	const CPlayer* sender = playerHandler->Player(playerID);

	float3 pos1 = constPos1;
	float3 pos2 = constPos2;
	pos1.CheckInBounds();
	pos2.CheckInBounds();
	pos1.y = ground->GetHeightAboveWater(pos1.x, pos1.z) + 2.0f;
	pos2.y = ground->GetHeightAboveWater(pos2.x, pos2.z) + 2.0f;

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
	if (!playerHandler->IsValidPlayer(playerID))
		return;

	GML_STDMUTEX_LOCK(inmap); // LocalErase

	const CPlayer* sender = playerHandler->Player(playerID);

	float3 pos = constPos;
	pos.CheckInBounds();
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z) + 2.0f;

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

void CInMapDraw::SendWaitingInput(const std::string& label)
{
	SendPoint(waitingPoint, label, false);

	wantLabel = false;
	keyPressed = false;
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



unsigned int CInMapDraw::GetPoints(PointMarker* array, unsigned int maxPoints, const std::list<const unsigned char*>& colors)
{
	int numPoints = 0;

	std::list<MapPoint>* points = NULL;
	std::list<MapPoint>::const_iterator point;
	std::list<const unsigned char*>::const_iterator ic;

	for (size_t i = 0; i < (drawQuadsX * drawQuadsY) && numPoints < maxPoints; i++) {
		points = &(drawQuads[i].points);

		for (point = points->begin(); point != points->end() && numPoints < maxPoints; ++point) {
			for (ic = colors.begin(); ic != colors.end(); ++ic) {
				if (point->color == *ic) {
					array[numPoints].pos   = point->pos;
					array[numPoints].color = point->color;
					array[numPoints].label = point->label.c_str();
					numPoints++;
					break;
				}
			}
		}
	}

	return numPoints;
}

unsigned int CInMapDraw::GetLines(LineMarker* array, unsigned int maxLines, const std::list<const unsigned char*>& colors)
{
	int numLines = 0;

	std::list<MapLine>* lines = NULL;
	std::list<MapLine>::const_iterator line;
	std::list<const unsigned char*>::const_iterator ic;

	for (size_t i = 0; i < (drawQuadsX * drawQuadsY) && numLines < maxLines; i++) {
		lines = &(drawQuads[i].lines);

		for (line = lines->begin(); line != lines->end() && numLines < maxLines; ++line) {
			for (ic = colors.begin(); ic != colors.end(); ++ic) {
				if (line->color == *ic) {
					array[numLines].pos   = line->pos;
					array[numLines].pos2  = line->pos2;
					array[numLines].color = line->color;
					numLines++;
					break;
				}
			}
		}
	}

	return numLines;
}


void CInMapDraw::ClearMarks()
{
	for (unsigned int n = 0; n < drawQuads.size(); n++) {
		drawQuads[n].points.clear();
		drawQuads[n].lines.clear();
	}

	visibleLabels.clear();
}
