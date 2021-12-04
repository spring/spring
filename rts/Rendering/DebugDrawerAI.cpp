/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/WideLineAdapter.hpp"
#include "Sim/Misc/TeamHandler.h"
#include "System/bitops.h"

#include <algorithm>

static constexpr float3 GRAPH_MIN_SCALE( 1e9,  1e9, 0.0f);
static constexpr float3 GRAPH_MAX_SCALE(-1e9, -1e9, 0.0f);

DebugDrawerAI::DebugDrawerAI() {
	graphs.resize(teamHandler.ActiveTeams(), Graph(GRAPH_MIN_SCALE, GRAPH_MAX_SCALE));
	texsets.resize(teamHandler.ActiveTeams(), TexSet());
}
DebugDrawerAI::~DebugDrawerAI() {
	for (Graph& graph: graphs) {
		graph.Clear();
	}
	for (TexSet& texSet: texsets) {
		texSet.Clear();
	}

	graphs.clear();
	texsets.clear();
}

DebugDrawerAI* DebugDrawerAI::GetInstance() {
	static DebugDrawerAI ddAI;
	return &ddAI;
}



void DebugDrawerAI::Draw() {
	if (!enabled || !gu->spectating)
		return;

	assert(gu->myTeam < graphs.size());

	if (skirmishAIHandler.GetSkirmishAIsInTeam(gu->myTeam).empty())
		return;

	glAttribStatePtr->PushBits(GL_ENABLE_BIT);
	glAttribStatePtr->DisableDepthTest();

	// draw data for the (AI) team being spectated
	GL::RenderDataBufferC* bufferC = GL::GetRenderBufferC();
	GL::RenderDataBufferT* bufferT = GL::GetRenderBufferT();

	graphs[gu->myTeam].Draw(bufferC, bufferC->GetShader());
	texsets[gu->myTeam].Draw(bufferT, bufferT->GetShader());

	glAttribStatePtr->PopBits();
}



// add a datapoint to the <lineNum>'th line of the graph for team <teamNum>
void DebugDrawerAI::AddGraphPoint(int teamNum, int lineNum, float x, float y) {
	assert(teamNum < graphs.size());
	graphs[teamNum].AddPoint(lineNum, x, y);
}
// delete the first <N> datapoints from the <lineNum>'th line of the graph for team <teamNum>
void DebugDrawerAI::DelGraphPoints(int teamNum, int lineNum, int numPoints) {
	assert(teamNum < graphs.size());
	graphs[teamNum].DelPoints(lineNum, numPoints);
}

void DebugDrawerAI::SetGraphPos(int teamNum, float x, float y) {
	if (x < -1.0f || x > 1.0f) return;
	if (y < -1.0f || y > 1.0f) return;

	assert(teamNum < graphs.size());
	graphs[teamNum].SetPos(x, y);
}

void DebugDrawerAI::SetGraphSize(int teamNum, float w, float h) {
	if (w < 0.0f || w > 2.0f) return;
	if (h < 0.0f || h > 2.0f) return;

	assert(teamNum < graphs.size());
	graphs[teamNum].SetSize(w, h);
}

void DebugDrawerAI::SetGraphLineColor(int teamNum, int lineNum, const float3& c) {
	assert(teamNum < graphs.size());
	graphs[teamNum].SetColor(lineNum, c);
}

void DebugDrawerAI::SetGraphLineLabel(int teamNum, int lineNum, const std::string& s) {
	assert(teamNum < graphs.size());
	graphs[teamNum].SetLabel(lineNum, s);
}



int DebugDrawerAI::AddOverlayTexture(int teamNum, const float* data, int w, int h) {
	assert(teamNum < texsets.size());
	return (texsets[teamNum].AddTexture(data, w, h));
}

void DebugDrawerAI::UpdateOverlayTexture(int teamNum, int texHandle, const float* data, int x, int y, int w, int h) {
	assert(teamNum < texsets.size());
	texsets[teamNum].UpdateTexture(texHandle, data, x, y, w, h);
}

void DebugDrawerAI::DelOverlayTexture(int teamNum, int texHandle) {
	assert(teamNum < texsets.size());
	texsets[teamNum].DelTexture(texHandle);
}

void DebugDrawerAI::SetOverlayTexturePos(int teamNum, int texHandle, float x, float y) {
	if (x < -1.0f || x > 1.0f) return;
	if (y < -1.0f || y > 1.0f) return;

	assert(teamNum < texsets.size());
	texsets[teamNum].SetTexturePos(texHandle, x, y);
}
void DebugDrawerAI::SetOverlayTextureSize(int teamNum, int texHandle, float w, float h) {
	if (w < 0.0f || w > 2.0f) return;
	if (h < 0.0f || h > 2.0f) return;

	assert(teamNum < texsets.size());
	texsets[teamNum].SetTextureSize(texHandle, w, h);
}
void DebugDrawerAI::SetOverlayTextureLabel(int teamNum, int texHandle, const std::string& label) {
	assert(teamNum < texsets.size());
	texsets[teamNum].SetTextureLabel(texHandle, label);
}



DebugDrawerAI::Graph::Graph(const float3& mins, const float3& maxs):
	pos(ZeroVector),
	size(ZeroVector),
	minScale(mins),
	maxScale(maxs),
	minLabelSize(1000),
	minLabelWidth(1000.0f),
	maxLabelSize(0),
	maxLabelWidth(0.0f) {
}

void DebugDrawerAI::Graph::AddPoint(int lineNum, float x, float y) {
	if (lines.find(lineNum) == lines.end())
		lines[lineNum] = Graph::GraphLine(GRAPH_MIN_SCALE, GRAPH_MAX_SCALE);

	Graph::GraphLine& line = lines[lineNum];

	line.lineData.emplace_back(x, y, 0.0f);
	line.lineMin.x = std::min(line.lineMin.x, x);
	line.lineMin.y = std::min(line.lineMin.y, y);
	line.lineMax.x = std::max(line.lineMax.x, x);
	line.lineMax.y = std::max(line.lineMax.y, y);

	minScale.x = std::min(minScale.x, x);
	minScale.y = std::min(minScale.y, y);
	maxScale.x = std::max(maxScale.x, x);
	maxScale.y = std::max(maxScale.y, y);

	scale = float3(maxScale.x - minScale.x, maxScale.y - minScale.y, 0.0);
}

void DebugDrawerAI::Graph::DelPoints(int lineNum, int numPoints) {
	auto lit = lines.find(lineNum);

	if (lit == lines.end())
		return;

	Graph::GraphLine& line = lit->second;

	line.lineMin = GRAPH_MIN_SCALE;
	line.lineMax = GRAPH_MAX_SCALE;

	minScale = GRAPH_MIN_SCALE;
	maxScale = GRAPH_MAX_SCALE;

	for (int i = 0; (i < numPoints && !line.lineData.empty()); i++) {
		line.lineData.pop_front();
	}

	// recalculate the line scales
	for (const float3& coor: line.lineData) {
		line.lineMin.x = std::min(line.lineMin.x, coor.x);
		line.lineMin.y = std::min(line.lineMin.y, coor.y);
		line.lineMax.x = std::max(line.lineMax.x, coor.x);
		line.lineMax.y = std::max(line.lineMax.y, coor.y);
	}

	// recalculate the graph scales
	for (const auto& pair: lines) {
		minScale.x = std::min((pair.second).lineMin.x, minScale.x);
		minScale.y = std::min((pair.second).lineMin.y, minScale.y);
		maxScale.x = std::max((pair.second).lineMax.x, maxScale.x);
		maxScale.y = std::max((pair.second).lineMax.y, maxScale.y);
	}

	scale.x = maxScale.x - minScale.x;
	scale.y = maxScale.y - minScale.y;
}

void DebugDrawerAI::Graph::SetColor(int lineNum, const float3& c) {
	if (lines.find(lineNum) == lines.end())
		lines[lineNum] = Graph::GraphLine(GRAPH_MIN_SCALE, GRAPH_MAX_SCALE);

	lines[lineNum].lineColor = c;
}

void DebugDrawerAI::Graph::SetLabel(int lineNum, const std::string& s) {
	if (lines.find(lineNum) == lines.end())
		lines[lineNum] = Graph::GraphLine(GRAPH_MIN_SCALE, GRAPH_MAX_SCALE);

	Graph::GraphLine& line = lines[lineNum];

	line.lineLabel = s;
	line.lineLabelSize = s.size();
	line.lineLabelWidth = font->GetSize() * font->GetTextWidth(s);
	line.lineLabelHeight = font->GetSize() * font->GetTextHeight(s);

	minLabelSize  = std::min(minLabelSize,  line.lineLabelSize);
	maxLabelSize  = std::max(maxLabelSize,  line.lineLabelSize);
	minLabelWidth = std::min(minLabelWidth, line.lineLabelWidth);
	maxLabelWidth = std::max(maxLabelWidth, line.lineLabelWidth);
}

void DebugDrawerAI::Graph::Clear() {
	for (auto& pair: lines) {
		(pair.second).lineData.clear();
	}
}



void DebugDrawerAI::Graph::Draw(GL::RenderDataBufferC* buffer, Shader::IProgramObject* shader) {
	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

	unsigned char color[4];

	{
		color[0] = 0.25f * 255;
		color[1] = 0.25f * 255;
		color[2] = 0.25f * 255;
		color[3] = 1.00f * 255;

		// label-box
		buffer->SafeAppend({pos,                                                                                            {color}});
		buffer->SafeAppend({pos + float3(-(((maxLabelWidth * 1.33f) / globalRendering->viewSizeX) * size.x),   0.0f, 0.0f), {color}});
		buffer->SafeAppend({pos + float3(-(((maxLabelWidth * 1.33f) / globalRendering->viewSizeX) * size.x), size.y, 0.0f), {color}});
		buffer->SafeAppend({pos + float3(                                                             0.0f,  size.y, 0.0f), {color}});
		buffer->Submit(GL_LINE_STRIP);

		if (scale.y > 0.0f && scale.x > 0.0f) {
			font->SetTextColor(0.25f, 0.25f, 0.25f, 1.0f);

			// horizontal grid lines
			for (float s = 0.0f; s <= (scale.y + 0.01f); s += (scale.y * 0.1f)) {
				buffer->SafeAppend({pos + float3(  0.0f, (s / scale.y) * size.y, 0.0f), {color}});
				buffer->SafeAppend({pos + float3(size.x, (s / scale.y) * size.y, 0.0f), {color}});

				const float tx = (pos.x + size.x) + (size.x * 0.025f);
				const float ty = pos.y + (s / scale.y) * size.y;

				font->glFormat(tx, ty, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%2.1e", s + minScale.y);
			}

			// vertical grid lines
			for (float s = 0.0f; s <= (scale.x + 0.01f); s += (scale.x * 0.1f)) {
				buffer->SafeAppend({pos + float3((s / scale.x) * size.x,   0.0f, 0.0f), {color}});
				buffer->SafeAppend({pos + float3((s / scale.x) * size.x, size.y, 0.0f), {color}});

				const float tx = (pos.x + (s / scale.x) * size.x) - (size.x * 0.05f);
				const float ty = pos.y - size.y * 0.1f;

				font->glFormat(tx, ty, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%2.1e", s + minScale.x);
			}

			buffer->Submit(GL_LINES);
		}
	}

	{
		if (!lines.empty()) {
			GL::WideLineAdapterC* wla = GL::GetWideLineAdapterC();
			wla->Setup(buffer, globalRendering->viewSizeX, globalRendering->viewSizeY, 1.0f, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

			int lineNum = 0;
			float linePad = (1.0f / lines.size()) * 0.5f;

			for (const auto& pair: lines) {
				const Graph::GraphLine& line = pair.second;
				const std::deque<float3>& data = line.lineData;

				if (data.empty())
					continue;

				// right-outline the labels
				const float sx = (maxLabelWidth / globalRendering->viewSizeX) * size.x;
				const float tx = pos.x - ((line.lineLabelWidth / maxLabelWidth) * 1.33f) * sx;
				const float ty = pos.y + ((lineNum * linePad * 2.0f) + linePad) * size.y;

				font->SetTextColor(line.lineColor.x, line.lineColor.y, line.lineColor.z, 1.0f);
				font->glPrint(tx, ty, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, line.lineLabel);

				color[0] = line.lineColor.x * 255;
				color[1] = line.lineColor.y * 255;
				color[2] = line.lineColor.z * 255;
				color[3] = 255;

				wla->SetWidth(line.lineWidth);

				for (auto pit = data.begin(); pit != data.end(); ++pit) {
					auto npit = pit; ++npit;

					const float px1 = ((pit->x - minScale.x) / scale.x) * size.x;
					const float py1 = ((pit->y - minScale.y) / scale.y) * size.y;
					const float px2 = (npit == data.end()) ? px1 : ((npit->x - minScale.x) / scale.x) * size.x;
					const float py2 = (npit == data.end()) ? py1 : ((npit->y - minScale.y) / scale.y) * size.y;

					wla->SafeAppend({pos + float3(px1, py1, 0.0f), {color}});
					wla->SafeAppend({pos + float3(px2, py2, 0.0f), {color}});
				}

				wla->Submit(GL_LINE_STRIP);

				lineNum += 1;
			}
		}
	}

	shader->Disable();
	font->DrawBufferedGL4();
}






void DebugDrawerAI::TexSet::UpdateTexture(int texHandle, const float* data, int x, int y, int w, int h) {
	auto it = textures.find(texHandle);

	if (it == textures.end())
		return;

	if ((x + w) > (it->second).GetWidth())
		return;
	if ((y + h) > (it->second).GetHeight())
		return;

	glBindTexture(GL_TEXTURE_2D, (it->second).GetID());
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_FLOAT, data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void DebugDrawerAI::TexSet::DelTexture(int texHandle) {
	auto it = textures.find(texHandle);

	if (it == textures.end())
		return;

	textures.erase(it->first);
}

void DebugDrawerAI::TexSet::SetTexturePos(int texHandle, float x, float y) {
	auto it = textures.find(texHandle);

	if (it == textures.end())
		return;

	(it->second).SetPos(float3(x, y, 0.0f));
}
void DebugDrawerAI::TexSet::SetTextureSize(int texHandle, float w, float h) {
	auto it = textures.find(texHandle);

	if (it == textures.end())
		return;

	(it->second).SetSize(float3(w, h, 0.0f));
}
void DebugDrawerAI::TexSet::SetTextureLabel(int texHandle, const std::string& label) {
	auto it = textures.find(texHandle);

	if (it == textures.end())
		return;

	(it->second).SetLabel(label);
}



void DebugDrawerAI::TexSet::Draw(GL::RenderDataBufferT* buffer, Shader::IProgramObject* shader) {
	if (textures.empty())
		return;

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

	font->SetTextColor(0.0f, 0.0f, 0.0f, 1.0f);

	for (const auto& pair: textures) {
		const TexSet::Texture& tex = pair.second;
		const float3& pos = tex.GetPos();
		const float3& size = tex.GetSize();

		glBindTexture(GL_TEXTURE_2D, tex.GetID());
		buffer->SafeAppend({pos,                                0.0f, 1.0f}); // tl
		buffer->SafeAppend({pos + float3(size.x,   0.0f, 0.0f), 1.0f, 1.0f}); // tr
		buffer->SafeAppend({pos + float3(size.x, size.y, 0.0f), 1.0f, 0.0f}); // br

		buffer->SafeAppend({pos + float3(size.x, size.y, 0.0f), 1.0f, 0.0f}); // br
		buffer->SafeAppend({pos + float3(  0.0f, size.y, 0.0f), 0.0f, 0.0f}); // bl
		buffer->SafeAppend({pos,                                0.0f, 1.0f}); // tl
		buffer->Submit(GL_TRIANGLES);
		glBindTexture(GL_TEXTURE_2D, 0);

		const float tx = pos.x + size.x * 0.5f - ((tex.GetLabelWidth () * 0.5f) / globalRendering->viewSizeX) * size.x;
		const float ty = pos.y + size.y        + ((tex.GetLabelHeight() * 0.5f) / globalRendering->viewSizeY) * size.y;

		font->glFormat(tx, ty, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%s", (tex.GetLabel()).c_str());
	}

	shader->Disable();
	font->DrawBufferedGL4();
}



DebugDrawerAI::TexSet::Texture::Texture(int w, int h, const float* data):
	id(0),
	xsize(w),
	ysize(h),
	pos(ZeroVector),
	size(ZeroVector),
	label(""),
	labelWidth(0.0f),
	labelHeight(0.0f)
{
	const int intFormat = GL_RGBA;  // note: data only holds the red component
	const int extFormat = GL_RED;
	const int dataType  = GL_FLOAT;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, intFormat, w, h, 0, extFormat, dataType, data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

DebugDrawerAI::TexSet::Texture::~Texture() {
	glDeleteTextures(1, &id);
}

void DebugDrawerAI::TexSet::Texture::SetLabel(const std::string& s) {
	label = s;
	labelWidth = font->GetSize() * font->GetTextWidth(s);
	labelHeight = font->GetSize() * font->GetTextHeight(s);
}

