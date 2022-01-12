/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "InMapDrawView.h"
#include "Rendering/Colors.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/WideLineAdapter.hpp"

#include "Game/Camera.h"
#include "Game/InMapDrawModel.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"

CInMapDrawView* inMapDrawerView = nullptr;


CInMapDrawView::CInMapDrawView()
{
	unsigned char tex[64][128][4];

	std::memset(tex, 0, sizeof(tex));

	for (int y = 0; y < 64; y++) {
		// circular thingy
		for (int x = 0; x < 64; x++) {
			const float dist = std::sqrt((float)(x - 32) * (x - 32) + (y - 32) * (y - 32));

			if (dist > 31.0f) {
				// do nothing - leave transparent
			} else if (dist > 30.0f) {
				// interpolate (outline -> nothing)
				tex[y][x][3] = mix(255,   0, dist - 30.0f);
			} else if (dist > 24.0f) {
				// black outline
				tex[y][x][3] = 255;
			} else if (dist > 23.0f) {
				// interpolate (inner -> outline)
				tex[y][x][0] = mix(255,   0, dist - 23.0f);
				tex[y][x][1] = mix(255,   0, dist - 23.0f);
				tex[y][x][2] = mix(255,   0, dist - 23.0f);
				tex[y][x][3] = mix(200, 255, dist - 23.0f);
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
				tex[y][x][3] = mix(255,   0, dist - 30.0f);
			} else if (dist > 24.0f) {
				// black outline
				tex[y][x][3] = 255;
			} else if (dist > 23.0f) {
				// interpolate (inner -> outline)
				tex[y][x][0] = mix(255,   0, dist - 23.0f);
				tex[y][x][1] = mix(255,   0, dist - 23.0f);
				tex[y][x][2] = mix(255,   0, dist - 23.0f);
				tex[y][x][3] = mix(200, 255, dist - 23.0f);
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
}


CInMapDrawView::~CInMapDrawView()
{
	glDeleteTextures(1, &texture);
}


struct InMapDraw_QuadDrawer: public CReadMap::IQuadDrawer
{
	std::vector<const CInMapDrawModel::MapPoint*>* visibleLabels = nullptr;

	GL::RenderDataBufferTC* pointBuffer = nullptr;
	GL::WideLineAdapterC* linesAdapter = nullptr;

	void ResetState() override {
		pointBuffer = nullptr;
		linesAdapter = nullptr;
	}
	void DrawQuad(int x, int y) override ;

private:
	void DrawPoint(const CInMapDrawModel::MapPoint* point, GL::RenderDataBufferTC* buffer) const;
	void DrawLine(const CInMapDrawModel::MapLine* line, GL::WideLineAdapterC* wla) const;
};



void InMapDraw_QuadDrawer::DrawPoint(const CInMapDrawModel::MapPoint* point, GL::RenderDataBufferTC* buffer) const
{
	const float3& pos = point->GetPos();
	const float3 zdir = (pos - camera->GetPos()).ANormalize();
	const float3 xdir = (zdir.cross(UpVector)).ANormalize();
	const float3 ydir = (zdir.cross(xdir));


	const unsigned char* pcolor = point->IsBySpectator() ? color4::white : teamHandler.Team(point->GetTeamID())->color;
	const unsigned char color[4] = {pcolor[0], pcolor[1], pcolor[2], 200};

	constexpr float size = 6.0f;
	const float3 pos1(pos.x,  pos.y  +   5.0f, pos.z);
	const float3 pos2(pos1.x, pos1.y + 100.0f, pos1.z);

	{
		buffer->SafeAppend({pos1 - xdir * size,               0.25f, 0.0f, color}); // tl
		buffer->SafeAppend({pos1 + xdir * size,               0.25f, 1.0f, color}); // tr
		buffer->SafeAppend({pos1 + xdir * size + ydir * size, 0.00f, 1.0f, color}); // br

		buffer->SafeAppend({pos1 + xdir * size + ydir * size, 0.00f, 1.0f, color}); // br
		buffer->SafeAppend({pos1 - xdir * size + ydir * size, 0.00f, 0.0f, color}); // bl
		buffer->SafeAppend({pos1 - xdir * size,               0.25f, 0.0f, color}); // tl
	}
	{
		buffer->SafeAppend({pos1 - xdir * size,               0.75f, 0.0f, color});
		buffer->SafeAppend({pos1 + xdir * size,               0.75f, 1.0f, color});
		buffer->SafeAppend({pos2 + xdir * size,               0.75f, 1.0f, color});

		buffer->SafeAppend({pos2 + xdir * size,               0.75f, 1.0f, color});
		buffer->SafeAppend({pos2 - xdir * size,               0.75f, 0.0f, color});
		buffer->SafeAppend({pos1 - xdir * size,               0.75f, 0.0f, color});
	}
	{
		buffer->SafeAppend({pos2 - xdir * size,               0.25f, 0.0f, color});
		buffer->SafeAppend({pos2 + xdir * size,               0.25f, 1.0f, color});
		buffer->SafeAppend({pos2 + xdir * size - ydir * size, 0.00f, 1.0f, color});

		buffer->SafeAppend({pos2 + xdir * size - ydir * size, 0.00f, 1.0f, color});
		buffer->SafeAppend({pos2 - xdir * size - ydir * size, 0.00f, 0.0f, color});
		buffer->SafeAppend({pos2 - xdir * size,               0.25f, 0.0f, color});
	}

	if (point->GetLabel().empty())
		return;

	visibleLabels->push_back(point);
}

void InMapDraw_QuadDrawer::DrawLine(const CInMapDrawModel::MapLine* line, GL::WideLineAdapterC* wla) const
{
	const unsigned char* color = line->IsBySpectator() ? color4::white : teamHandler.Team(line->GetTeamID())->color;
	wla->SafeAppend({line->GetPos1() - (line->GetPos1() - camera->GetPos()).ANormalize() * 26, color});
	wla->SafeAppend({line->GetPos2() - (line->GetPos2() - camera->GetPos()).ANormalize() * 26, color});
}



void InMapDraw_QuadDrawer::DrawQuad(int x, int y)
{
	const CInMapDrawModel::DrawQuad* dq = inMapDrawerModel->GetDrawQuad(x, y);

	//! draw point markers
	for (const CInMapDrawModel::MapPoint& pi: dq->points) {
		if (pi.IsVisibleToPlayer(inMapDrawerModel->GetAllMarksVisible())) {
			DrawPoint(&pi, pointBuffer);
		}
	}

	//! draw line markers
	for (const CInMapDrawModel::MapLine& li: dq->lines) {
		if (li.IsVisibleToPlayer(inMapDrawerModel->GetAllMarksVisible())) {
			DrawLine(&li, linesAdapter);
		}
	}
}



void CInMapDrawView::Draw()
{
	GL::RenderDataBufferTC* pointBuffer = GL::GetRenderBufferTC();
	GL::RenderDataBufferC* linesBuffer = GL::GetRenderBufferC();
	GL::WideLineAdapterC* wla = GL::GetWideLineAdapterC();

	Shader::IProgramObject* pointShader = pointBuffer->GetShader();
	Shader::IProgramObject* linesShader = linesBuffer->GetShader();
	wla->Setup(linesBuffer, globalRendering->viewSizeX, globalRendering->viewSizeY, 3.0f, camera->GetViewProjectionMatrix());

	InMapDraw_QuadDrawer drawer;
	drawer.visibleLabels = &visibleLabels;
	drawer.pointBuffer = pointBuffer;
	drawer.linesAdapter = wla;

	readMap->GridVisibility(nullptr, &drawer, 1e9, CInMapDrawModel::DRAW_QUAD_SIZE);


	glAttribStatePtr->DisableDepthMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->EnableBlendMask();


	{
		// draw lines
		linesShader->Enable();
		linesShader->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
		linesShader->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
		wla->Submit(GL_LINES);
		linesShader->Disable();
	}
	{
		// draw points
		glBindTexture(GL_TEXTURE_2D, texture);
		pointShader->Enable();
		pointShader->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
		pointShader->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
		pointBuffer->Submit(GL_TRIANGLES);
		pointShader->Disable();
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	if (!visibleLabels.empty()) {
		font->SetColors(); // default
		font->glWorldBegin();

		// draw point labels
		for (const CInMapDrawModel::MapPoint* point: visibleLabels) {
			const float3 pos = point->GetPos() + UpVector * 111.0f;

			const CTeam* team = teamHandler.Team(point->GetTeamID());
			const unsigned char* color = point->IsBySpectator() ? color4::white : team->color;

			font->SetTextColor(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, 1.0f);
			font->glWorldPrint(pos, 26.0f, point->GetLabel());
		}

		font->glWorldEnd();
		visibleLabels.clear();
	}

	glAttribStatePtr->EnableDepthMask();
}

