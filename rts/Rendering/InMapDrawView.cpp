/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "InMapDrawView.h"
#include "Rendering/Colors.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"

#include "Game/Camera.h"
#include "Game/InMapDrawModel.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"

CInMapDrawView* inMapDrawerView = nullptr;


/**
 * how far on the way between x and y [0.0f, 1.0f]
 */
static inline unsigned char smoothStep(int x, int y, int a) {
	return (unsigned char)((x * (1.0f - a)) + (y * a));
}

CInMapDrawView::CInMapDrawView()
{
	uint8_t tex[64][128][4];
	std::memset(tex, 0, sizeof(tex));

	for (int y = 0; y < 64; y++) {
		// circular thingy
		for (int x = 0; x < 64; x++) {
			const float dist = std::sqrt((float)(x - 32) * (x - 32) + (y - 32) * (y - 32));
			if (dist > 31.0f) {
				// do nothing - leave transparent
			} else if (dist > 30.0f) {
				// interpolate (outline -> nothing)
				const float a = (dist - 30.0f);
				tex[y][x][3] = smoothStep(255,   0, a);
			} else if (dist > 24.0f) {
				// black outline
				tex[y][x][3] = 255;
			} else if (dist > 23.0f) {
				// interpolate (inner -> outline)
				const float a = (dist - 23.0f);
				tex[y][x][0] = smoothStep(255,   0, a);
				tex[y][x][1] = smoothStep(255,   0, a);
				tex[y][x][2] = smoothStep(255,   0, a);
				tex[y][x][3] = smoothStep(200, 255, a);
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
			const float dist = std::abs(y - 32);
			if (dist > 31.0f) {
				// do nothing - leave transparent
			} else if (dist > 30.0f) {
				// interpolate (outline -> nothing)
				const float a = (dist - 30.0f);
				tex[y][x][3] = smoothStep(255,   0, a);
			} else if (dist > 24.0f) {
				// black outline
				tex[y][x][3] = 255;
			} else if (dist > 23.0f) {
				// interpolate (inner -> outline)
				const float a = (dist - 23.0f);
				tex[y][x][0] = smoothStep(255,   0, a);
				tex[y][x][1] = smoothStep(255,   0, a);
				tex[y][x][2] = smoothStep(255,   0, a);
				tex[y][x][3] = smoothStep(200, 255, a);
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, 128, 64, GL_RGBA, GL_UNSIGNED_BYTE, tex[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
}


CInMapDrawView::~CInMapDrawView()
{
	glDeleteTextures(1, &texture);
}


struct InMapDraw_QuadDrawer: public CReadMap::IQuadDrawer
{
	TypedRenderBuffer<VA_TYPE_TC>* rbp = nullptr;
	TypedRenderBuffer<VA_TYPE_C >* rbl = nullptr;

	std::vector<const CInMapDrawModel::MapPoint*>* visibleLabels;

	void ResetState() {}
	void DrawQuad(int x, int y);

private:
	void DrawPoint(const CInMapDrawModel::MapPoint* point) const;
	void DrawLine(const CInMapDrawModel::MapLine* line) const;
};

void InMapDraw_QuadDrawer::DrawPoint(const CInMapDrawModel::MapPoint* point) const
{
	const float3& pos = point->GetPos();
	const float3 dif = (pos - camera->GetPos()).ANormalize();
	const float3 dir1 = (dif.cross(UpVector)).ANormalize();
	const float3 dir2 = (dif.cross(dir1));


	SColor color = point->IsBySpectator() ? color4::white : SColor{ teamHandler.Team(point->GetTeamID())->color };
	color.a = 200;

	const float size = 6.0f;
	const float3 pos1(pos.x,  pos.y  +   5.0f, pos.z);
	const float3 pos2(pos1.x, pos1.y + 100.0f, pos1.z);

	rbp->AddQuadTriangles(
		{ pos1 - dir1 * size,               0.25f, 0, color },
		{ pos1 + dir1 * size,               0.25f, 1, color },
		{ pos1 + dir1 * size + dir2 * size, 0.00f, 1, color },
		{ pos1 - dir1 * size + dir2 * size, 0.00f, 0, color }
	);
	rbp->AddQuadTriangles(
		{ pos1 - dir1 * size,               0.75f, 0, color },
		{ pos1 + dir1 * size,               0.75f, 1, color },
		{ pos2 + dir1 * size,               0.75f, 1, color },
		{ pos2 - dir1 * size,               0.75f, 0, color }
	);
	rbp->AddQuadTriangles(
		{ pos2 - dir1 * size,               0.25f, 0, color },
		{ pos2 + dir1 * size,               0.25f, 1, color },
		{ pos2 + dir1 * size - dir2 * size, 0.00f, 1, color },
		{ pos2 - dir1 * size - dir2 * size, 0.00f, 0, color }
	);

	if (!point->GetLabel().empty()) {
		visibleLabels->push_back(point);
	}
}

void InMapDraw_QuadDrawer::DrawLine(const CInMapDrawModel::MapLine* line) const
{
	const SColor color = line->IsBySpectator() ? color4::white : SColor{ teamHandler.Team(line->GetTeamID())->color };
	rbl->AddVertices({
		{ line->GetPos1() - (line->GetPos1() - camera->GetPos()).ANormalize() * 26, color },
		{ line->GetPos2() - (line->GetPos2() - camera->GetPos()).ANormalize() * 26, color }
	});
}

void InMapDraw_QuadDrawer::DrawQuad(int x, int y)
{
	const CInMapDrawModel::DrawQuad* dq = inMapDrawerModel->GetDrawQuad(x, y);

	//! draw point markers
	for (const CInMapDrawModel::MapPoint& pi: dq->points) {
		if (pi.IsVisibleToPlayer(inMapDrawerModel->GetAllMarksVisible())) {
			DrawPoint(&pi);
		}
	}

	//! draw line markers
	for (const CInMapDrawModel::MapLine& li: dq->lines) {
		if (li.IsVisibleToPlayer(inMapDrawerModel->GetAllMarksVisible())) {
			DrawLine(&li);
		}
	}
}



void CInMapDrawView::Draw()
{
	InMapDraw_QuadDrawer drawer;
	drawer.visibleLabels = &visibleLabels;
	drawer.rbl = &rbl;
	drawer.rbp = &rbp;

	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	readMap->GridVisibility(nullptr, &drawer, 1e9, CInMapDrawModel::DRAW_QUAD_SIZE);

	{
		glLineWidth(3.0f);
		auto& sh = rbl.GetShader();
		sh.Enable();
		if (globalRendering->amdHacks) {
			rbl.DrawArrays(GL_LINES, false); //! draw lines
			glLineWidth(1.0f);
			rbl.DrawArrays(GL_LINES, true ); // width greater than 2 causes GUI flicker on ATI hardware as of driver version 9.3
		}
		else {
			rbl.DrawArrays(GL_LINES, true ); //! draw lines
		}
		sh.Disable();
		glLineWidth(1.0f);
	}

	// draw points

	{
		glBindTexture(GL_TEXTURE_2D, texture);
		auto& sh = rbp.GetShader();
		sh.Enable();
		rbp.DrawElements(GL_TRIANGLES); //! draw point markers
		sh.Disable();
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (!visibleLabels.empty()) {
		font->SetColors(); // default

		// draw point labels
		for (const CInMapDrawModel::MapPoint* point: visibleLabels) {
			const float3 pos = point->GetPos() + UpVector * 111.0f;

			const CTeam* team = teamHandler.Team(point->GetTeamID());
			const SColor color = point->IsBySpectator() ? color4::white : SColor{ team->color };

			font->SetTextColor(color);
			font->glWorldPrint(pos, 26.0f, point->GetLabel(), true);
		}

		font->DrawWorldBuffered();
		visibleLabels.clear();
		font->SetColors(); // default
	}

	glDepthMask(GL_TRUE);
}
