#include "StdAfx.h"
#include "InMapDraw.h"
#include "glFont.h"
#include "GL/myGL.h"
#include "GL/VertexArray.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/Player.h"
#include "Game/Team.h"
#include "Game/UI/LuaUI.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "NetProtocol.h"
#include "LogOutput.h"
#include "Sound.h"
#include "SDL_mouse.h"
#include "SDL_keyboard.h"
#include "creg/STL_List.h"


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
	CR_RESERVED(4),
	CR_SERIALIZER(Serialize)
));

CR_BIND(CInMapDraw::MapLine, );

CR_REG_METADATA_SUB(CInMapDraw, MapLine, (
	CR_MEMBER(pos),
	CR_MEMBER(pos2),
	CR_MEMBER(senderAllyTeam),
	CR_MEMBER(senderSpectator),
	CR_RESERVED(4),
	CR_SERIALIZER(Serialize)
));

CR_BIND(CInMapDraw::DrawQuad, );

CR_REG_METADATA_SUB(CInMapDraw, DrawQuad, (
	CR_MEMBER(points),
	CR_MEMBER(lines),
	CR_RESERVED(4)
));



CInMapDraw::CInMapDraw(void)
{
	keyPressed = false;
	wantLabel = false;
	drawAll = false;
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
	for (int y = 0; y < 64; y++) {
		// circular thingy
		for (int x = 0; x < 64; x++) {
			float dist = sqrt((float)(x - 32) * (x - 32) + (y - 32) * (y - 32));
			if (dist > 31.875f) {
				// do nothing - leave transparent
			} else if (dist > 24.5f) {
				// black outline
				tex[y][x][3] = 255;
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
		for (int x = 0; x < 64; x++) {
			float dist = abs(y - 32);
			if (dist > 24.5f) {
				// black outline
				tex[y][x + 64][3] = 255;
			} else {
				tex[y][x + 64][0] = 255;
				tex[y][x + 64][1] = 255;
				tex[y][x + 64][2] = 255;
				tex[y][x + 64][3] = 200;
			}
		}
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, 128, 64, GL_RGBA, GL_UNSIGNED_BYTE, tex[0]);

	blippSound=sound->GetWaveId("sounds/beep6.wav");
}


CInMapDraw::~CInMapDraw(void)
{
	glDeleteTextures (1, &texture);
}

void CInMapDraw::PostLoad()
{
	if (drawQuads.size() != numQuads) {
		//For old savegames
		drawQuads.resize(numQuads);
	}
}

unsigned char DefaultColor[4] = {0, 0, 0, 255};

void SerializeColor(creg::ISerializer &s, unsigned char **color)
{
	if (s.IsWriting()) {
		char ColorType = 0;
		char ColorId = 0;
		if (!ColorType) for (int a=0;a<MAX_TEAMS;a++) 
			if (*color==gs->Team(a)->color) {
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
				*color = gs->Team(ColorId)->color;
				break;
			}
		}
	}
}

void CInMapDraw::MapPoint::Serialize(creg::ISerializer &s)
{
	SerializeColor(s,&color);
}

void CInMapDraw::MapLine::Serialize(creg::ISerializer &s)
{
	SerializeColor(s,&color);
}



struct InMapDraw_QuadDrawer: public CReadMap::IQuadDrawer
{
	CVertexArray *va, *lineva;
	CInMapDraw* imd;
	unsigned int texture;

	void DrawQuad(int x, int y);
};

void InMapDraw_QuadDrawer::DrawQuad(int x, int y)
{
	int drawQuadsX = imd->drawQuadsX;
	CInMapDraw::DrawQuad* dq = &imd->drawQuads[y * drawQuadsX + x];

	// draw point markers
	for (std::list<CInMapDraw::MapPoint>::iterator pi = dq->points.begin(); pi != dq->points.end(); ++pi) {
		const int allyteam = pi->senderAllyTeam;
		const bool spec = pi->senderSpectator;
		const bool allied = (gs->Ally(gu->myAllyTeam, allyteam) && gs->Ally(allyteam, gu->myAllyTeam));
		const bool maySee = (gu->spectating || (!spec && allied) || imd->drawAll);

		if (maySee) {
			float3 pos = pi->pos;
			float3 dif(pos - camera->pos);
			float camDist = dif.Length();
			dif /= camDist;
			float3 dir1(dif.cross(UpVector));
			dir1.Normalize();
			float3 dir2(dif.cross(dir1));

			unsigned char col[4];
			col[0] = pi->color[0];
			col[1] = pi->color[1];
			col[2] = pi->color[2];
			col[3] = 200;

			float size = 6;
			float3 pos1 = pos;
			float3 pos2 = pos1;
			pos2.y += 100;

			va->AddVertexTC(pos1 - dir1 * size,               0.25f, 0, col);
			va->AddVertexTC(pos1 + dir1 * size,               0.25f, 1, col);
			va->AddVertexTC(pos1 + dir1 * size + dir2 * size, 0.00f, 1, col);
			va->AddVertexTC(pos1 - dir1 * size + dir2 * size, 0.00f, 0, col);

			va->AddVertexTC(pos1 - dir1 * size,               0.75f, 0, col);
			va->AddVertexTC(pos1 + dir1 * size,               0.75f, 1, col);
			va->AddVertexTC(pos2 + dir1 * size,               0.75f, 1, col);
			va->AddVertexTC(pos2 - dir1 * size,               0.75f, 0, col);

			va->AddVertexTC(pos2 - dir1 * size,               0.25f, 0, col);
			va->AddVertexTC(pos2 + dir1 * size,               0.25f, 1, col);
			va->AddVertexTC(pos2 + dir1 * size - dir2 * size, 0.00f, 1, col);
			va->AddVertexTC(pos2 - dir1 * size - dir2 * size, 0.00f, 0, col);

			if (pi->label.size() > 0) {
				glPushMatrix();
				glTranslatef3(pi->pos + UpVector * 105);
				glScalef(1200, 1200, 1200);
				glColor4ub(pi->color[0], pi->color[1], pi->color[2], 250);
				font->glWorldPrint(pi->label.c_str());
				glPopMatrix();
				glBindTexture(GL_TEXTURE_2D, texture);
			}
		}
	}

	// draw line markers
	for (std::list<CInMapDraw::MapLine>::iterator li = dq->lines.begin(); li != dq->lines.end(); ++li) {
		const int allyteam = li->senderAllyTeam;
		const bool spec = li->senderSpectator;
		const bool allied = (gs->Ally(gu->myAllyTeam, allyteam) && gs->Ally(allyteam, gu->myAllyTeam));
		const bool maySee = (gu->spectating || (!spec && allied) || imd->drawAll);

		if (maySee) {
			lineva->AddVertexC(li->pos - (li->pos - camera->pos).Normalize() * 26, li->color);
			lineva->AddVertexC(li->pos2 - (li->pos2 - camera->pos).Normalize() * 26, li->color);
		}
	}
}



void CInMapDraw::Draw(void)
{
	glDepthMask(0);

	CVertexArray* va = GetVertexArray();
	va->Initialize();

	CVertexArray* lineva = GetVertexArray();
	lineva->Initialize();

	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, texture);

	InMapDraw_QuadDrawer drawer;
	drawer.imd = this;
	drawer.lineva = lineva;
	drawer.va = va;
	drawer.texture = texture;

	readmap->GridVisibility(camera, DRAW_QUAD_SIZE, 3000.0f, &drawer);

	glDisable(GL_TEXTURE_2D);
	glLineWidth(3);
	lineva->DrawArrayC(GL_LINES);
	glLineWidth(1);
	glEnable(GL_TEXTURE_2D);
	va->DrawArrayTC(GL_QUADS);
	glDepthMask(1);
}



void CInMapDraw::MousePress(int x, int y, int button)
{
	float3 pos = GetMouseMapPos();
	if (pos.x < 0)
		return;

	switch (button) {
	case SDL_BUTTON_LEFT:
		if (lastLeftClickTime > gu->gameTime - 0.3f) {
			PromptLabel(pos);
		}
		lastLeftClickTime = gu->gameTime;
		break;
	case SDL_BUTTON_RIGHT:
		ErasePos(pos);
		break;
	case SDL_BUTTON_MIDDLE:{
		CreatePoint(pos, "");
		break;}
	}

	lastPos = pos;
}


void CInMapDraw::MouseRelease(int x, int y, int button)
{

}


void CInMapDraw::MouseMove(int x, int y, int dx,int dy, int button)
{
	float3 pos = GetMouseMapPos();
	if (pos.x < 0)
		return;

	if (mouse->buttons[SDL_BUTTON_LEFT].pressed && lastLineTime < gu->gameTime - 0.05f) {
		AddLine(pos, lastPos);
		lastLineTime = gu->gameTime;
		lastPos = pos;
	}
	if (mouse->buttons[SDL_BUTTON_RIGHT].pressed)
		ErasePos(pos);

}


float3 CInMapDraw::GetMouseMapPos(void)
{
	float dist = ground->LineGroundCol(camera->pos, camera->pos + mouse->dir * gu->viewRange * 1.4f);
	if (dist < 0) {
		return float3(-1, 1, -1);
	}
	float3 pos = camera->pos + mouse->dir * dist;
	pos.CheckInBounds();
	return pos;
}


void CInMapDraw::GotNetMsg(const unsigned char* msg)
{
	const int playerID = msg[2];
	if ((playerID < 0) || (playerID >= MAX_PLAYERS)) {
		return;
	}
	const CPlayer* sender = gs->players[playerID];
	if (sender == NULL) {
		return;
	}
	const int team = sender->team;
	const int allyteam = gs->AllyTeam(team);
	const bool alliedMsg = (gs->Ally(gu->myAllyTeam, allyteam) && gs->Ally(allyteam, gu->myAllyTeam));
	bool allowed = true;

	if (!gu->spectating && (sender->spectator || !alliedMsg)) {
		// we are playing and the guy sending the
		// net-msg is a spectator or not an ally;
		// cannot just ignore the message due to
		// drawAll mode considerations
		allowed = false;
	}

	const float quadScale = 1.0f / (DRAW_QUAD_SIZE * SQUARE_SIZE);

	switch (msg[3]) {
		case NET_POINT: {
			float3 pos(*(short*) &msg[4], 0, *(short*) &msg[6]);
			pos.CheckInBounds();
			pos.y = ground->GetHeight(pos.x, pos.z) + 2.0f;

			const string label = (char*) &msg[8];
			if (luaUI && luaUI->MapDrawCmd(playerID, NET_POINT, &pos, NULL, &label)) {
				return;
			}

			MapPoint p;
			p.pos = pos;
			p.color = gs->Team(team)->color;
			p.label = label;
			p.senderAllyTeam = allyteam;
			p.senderSpectator = sender->spectator;

			const int quad = int(pos.z * quadScale) * drawQuadsX + int(pos.x * quadScale);
			drawQuads[quad].points.push_back(p);

			if (allowed || drawAll) {
				// if we happen to be in drawAll mode, notify us now
				// even if this message is not intented for our ears
				logOutput.Print("%s added point: %s", sender->playerName.c_str(), p.label.c_str());
				logOutput.SetLastMsgPos(pos);
				sound->PlaySample(blippSound);
				minimap->AddNotification(pos, float3(1.0f, 1.0f, 1.0f), 1.0f);
			}

			break;
		}

		case NET_ERASE: {
			float3 pos(*(short*) &msg[4], 0, *(short*) &msg[6]);
			pos.CheckInBounds();
			pos.y = ground->GetHeight(pos.x, pos.z) + 2.0f;
			if (luaUI && luaUI->MapDrawCmd(playerID, NET_ERASE, &pos, NULL, NULL)) {
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
						if (pi->pos.distance2D(pos) < radius) {
							pi = dq->points.erase(pi);
						} else {
							++pi;
						}
					}
					std::list<MapLine>::iterator li;
					for (li = dq->lines.begin(); li != dq->lines.end(); /* none */) {
						if (li->pos.distance2D(pos) < radius) {
							li = dq->lines.erase(li);
						} else {
							++li;
						}
					}
				}
			}

			break;
		}

		case NET_LINE: {
			float3 pos(*(short*) &msg[4], 0, *(short*) &msg[6]);
			float3 pos2(*(short*) &msg[8], 0, *(short*) &msg[10]);
			pos.CheckInBounds();
			pos2.CheckInBounds();
			pos.y = ground->GetHeight(pos.x, pos.z) + 2.0f;
			pos2.y = ground->GetHeight(pos2.x, pos2.z) + 2.0f;

			if (luaUI && luaUI->MapDrawCmd(playerID, NET_LINE, &pos, &pos2, NULL)) {
				return;
			}

			MapLine l;
			l.pos = pos;
			l.pos2 = pos2;
			l.color = gs->Team(team)->color;
			l.senderAllyTeam = allyteam;
			l.senderSpectator = sender->spectator;

			const int quad = int(pos.z * quadScale) * drawQuadsX + int(pos.x * quadScale);
			drawQuads[quad].lines.push_back(l);

			break;
		}
	}
}


void CInMapDraw::ErasePos(const float3& pos)
{
	net->SendMapErase(gu->myPlayerNum, (short)pos.x, (short)pos.z);
}


void CInMapDraw::CreatePoint(const float3& pos, std::string label)
{
	net->SendMapDrawPoint(gu->myPlayerNum, (short)pos.x, (short)pos.z, label);
}


void CInMapDraw::AddLine(const float3& pos, const float3& pos2)
{
	net->SendMapDrawLine(gu->myPlayerNum, (short)pos.x, (short)pos.z, (short)pos2.x, (short)pos2.z);
}


void CInMapDraw::PromptLabel (const float3& pos)
{
	waitingPoint = pos;
	game->userWriting = true;
	wantLabel = true;
	game->userPrompt = "Label: ";
	game->ignoreChar = '\xA7';		//should do something better here
}




