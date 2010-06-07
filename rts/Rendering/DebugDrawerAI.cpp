/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/bitops.h"
#include "System/GlobalUnsynced.h"

static const float3 GRAPH_MIN_SCALE = float3( 1e9,  1e9, 0.0f);
static const float3 GRAPH_MAX_SCALE = float3(-1e9, -1e9, 0.0f);

DebugDrawerAI::DebugDrawerAI(): draw(false) {
	graphs.resize(teamHandler->ActiveTeams(), Graph(GRAPH_MIN_SCALE, GRAPH_MAX_SCALE));
	texsets.resize(teamHandler->ActiveTeams(), TexSet());
}
DebugDrawerAI::~DebugDrawerAI() {
	for (std::vector<Graph>::iterator it = graphs.begin(); it != graphs.end(); ++it) {
		(*it).Clear();
	}
	for (std::vector<TexSet>::iterator it = texsets.begin(); it != texsets.end(); ++it) {
		(*it).Clear();
	}

	graphs.clear();
	texsets.clear();
}

DebugDrawerAI* DebugDrawerAI::GetInstance() {
	static DebugDrawerAI ddAI;
	return &ddAI;
}



void DebugDrawerAI::Draw() {
	if (!draw || !gu->spectating) {
		return;
	}

	assert(gu->myTeam < graphs.size());

	if (!skirmishAIHandler.GetSkirmishAIsInTeam(gu->myTeam).empty()) {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);

		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);

		// draw data for the (AI) team being spectated
		graphs[gu->myTeam].Draw();
		texsets[gu->myTeam].Draw();

		glPopAttrib();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
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
	if (x < -1.0f || x > 1.0f) { return; }
	if (y < -1.0f || y > 1.0f) { return; }

	assert(teamNum < graphs.size());
	graphs[teamNum].SetPos(x, y);
}

void DebugDrawerAI::SetGraphSize(int teamNum, float w, float h) {
	if (w < 0.0f || w > 2.0f) { return; }
	if (h < 0.0f || h > 2.0f) { return; }

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
	if (x < -1.0f || x > 1.0f) { return; }
	if (y < -1.0f || y > 1.0f) { return; }

	assert(teamNum < texsets.size());
	texsets[teamNum].SetTexturePos(texHandle, x, y);
}
void DebugDrawerAI::SetOverlayTextureSize(int teamNum, int texHandle, float w, float h) {
	if (w < 0.0f || w > 2.0f) { return; }
	if (h < 0.0f || h > 2.0f) { return; }

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
	if (lines.find(lineNum) == lines.end()) {
		lines[lineNum] = Graph::GraphLine(GRAPH_MIN_SCALE, GRAPH_MAX_SCALE);
	}

	Graph::GraphLine* line = &lines[lineNum];

	line->lineData.push_back(float3(x, y, 0.0f));
	line->lineMin.x = std::min(line->lineMin.x, x);
	line->lineMin.y = std::min(line->lineMin.y, y);
	line->lineMax.x = std::max(line->lineMax.x, x);
	line->lineMax.y = std::max(line->lineMax.y, y);

	minScale.x = std::min(minScale.x, x);
	minScale.y = std::min(minScale.y, y);
	maxScale.x = std::max(maxScale.x, x);
	maxScale.y = std::max(maxScale.y, y);

	scale = float3(maxScale.x - minScale.x, maxScale.y - minScale.y, 0.0);
}

void DebugDrawerAI::Graph::DelPoints(int lineNum, int numPoints) {
	typedef std::map<int, Graph::GraphLine>::iterator LineIt;
	typedef std::list<float3>::const_iterator ListIt;

	LineIt lit = lines.find(lineNum);

	if (lit == lines.end()) {
		return;
	}

	Graph::GraphLine* line = &(lit->second);

	line->lineMin = GRAPH_MIN_SCALE;
	line->lineMax = GRAPH_MAX_SCALE;

	minScale = GRAPH_MIN_SCALE;
	maxScale = GRAPH_MAX_SCALE;

	for (int i = 0; (i < numPoints && !line->lineData.empty()); i++) {
		line->lineData.pop_front();
	}

	// recalculate the line scales
	for (ListIt dit = line->lineData.begin(); dit != line->lineData.end(); ++dit) {
		line->lineMin.x = std::min(line->lineMin.x, (*dit).x);
		line->lineMin.y = std::min(line->lineMin.y, (*dit).y);
		line->lineMax.x = std::max(line->lineMax.x, (*dit).x);
		line->lineMax.y = std::max(line->lineMax.y, (*dit).y);
	}

	// recalculate the graph scales
	for (lit = lines.begin(); lit != lines.end(); ++lit) {
		minScale.x = std::min((lit->second).lineMin.x, minScale.x); 
		minScale.y = std::min((lit->second).lineMin.y, minScale.y);
		maxScale.x = std::max((lit->second).lineMax.x, maxScale.x);
		maxScale.y = std::max((lit->second).lineMax.y, maxScale.y);
	}

	scale.x = maxScale.x - minScale.x;
	scale.y = maxScale.y - minScale.y;
}

void DebugDrawerAI::Graph::SetColor(int lineNum, const float3& c) {
	if (lines.find(lineNum) == lines.end()) {
		lines[lineNum] = Graph::GraphLine(GRAPH_MIN_SCALE, GRAPH_MAX_SCALE);
	}

	lines[lineNum].lineColor = c;
}

void DebugDrawerAI::Graph::SetLabel(int lineNum, const std::string& s) {
	if (lines.find(lineNum) == lines.end()) {
		lines[lineNum] = Graph::GraphLine(GRAPH_MIN_SCALE, GRAPH_MAX_SCALE);
	}

	lines[lineNum].lineLabel = s;
	lines[lineNum].lineLabelSize = s.size();
	lines[lineNum].lineLabelWidth = font->GetSize() * font->GetTextWidth(s);
	lines[lineNum].lineLabelHeight = font->GetSize() * font->GetTextHeight(s);

	minLabelSize  = std::min(minLabelSize,  lines[lineNum].lineLabelSize);
	maxLabelSize  = std::max(maxLabelSize,  lines[lineNum].lineLabelSize);
	minLabelWidth = std::min(minLabelWidth, lines[lineNum].lineLabelWidth);
	maxLabelWidth = std::max(maxLabelWidth, lines[lineNum].lineLabelWidth);
}

void DebugDrawerAI::Graph::Clear() {
	typedef std::map<int, Graph::GraphLine>::iterator LineIt;

	for (LineIt it = lines.begin(); it != lines.end(); ++it) {
		(it->second).lineData.clear();
	}
}



void DebugDrawerAI::Graph::Draw() {
	static unsigned char color[4] = {0.75f * 255, 0.75f * 255, 0.75f * 255, 0.5f * 255};

	CVertexArray* va = GetVertexArray();

	{
		color[0] = 0.25f * 255;
		color[1] = 0.25f * 255;
		color[2] = 0.25f * 255;
		color[3] = 1.00f * 255;

		// label-box
		va->Initialize();
		va->AddVertexC(pos,                                                                               color);
		va->AddVertexC(pos + float3(-(((maxLabelWidth * 1.33f) / globalRendering->viewSizeX) * size.x),   0.0f, 0.0f), color);
		va->AddVertexC(pos + float3(-(((maxLabelWidth * 1.33f) / globalRendering->viewSizeX) * size.x), size.y, 0.0f), color);
		va->AddVertexC(pos + float3(                                                0.0f,  size.y, 0.0f), color);
		va->DrawArrayC(GL_LINE_STRIP);

		if (scale.y > 0.0f && scale.x > 0.0f) {
			font->Begin();
			font->SetTextColor(0.25f, 0.25f, 0.25f, 1.0f);

			// horizontal grid lines
			for (float s = 0.0f; s <= (scale.y + 0.01f); s += (scale.y * 0.1f)) {
				va->Initialize();
				va->AddVertexC(pos + float3(  0.0f, (s / scale.y) * size.y, 0.0f), color);
				va->AddVertexC(pos + float3(size.x, (s / scale.y) * size.y, 0.0f), color);
				va->DrawArrayC(GL_LINES);

				const float tx = (pos.x + size.x) + (size.x * 0.025f);
				const float ty = pos.y + (s / scale.y) * size.y;

				font->glFormat(tx, ty, 1.0f, FONT_SCALE | FONT_NORM, "%2.1e", s + minScale.y);
			}

			// vertical grid lines
			for (float s = 0.0f; s <= (scale.x + 0.01f); s += (scale.x * 0.1f)) {
				va->Initialize();
				va->AddVertexC(pos + float3((s / scale.x) * size.x,   0.0f, 0.0f), color);
				va->AddVertexC(pos + float3((s / scale.x) * size.x, size.y, 0.0f), color);
				va->DrawArrayC(GL_LINES);

				const float tx = (pos.x + (s / scale.x) * size.x) - (size.x * 0.05f);
				const float ty = pos.y - size.y * 0.1f;

				font->glFormat(tx, ty, 1.0f, FONT_SCALE | FONT_NORM, "%2.1e", s + minScale.x);
			}

			font->End();
		}
	}

	{
		typedef std::map<int, Graph::GraphLine>::const_iterator LineIt;
		typedef std::list<float3>::const_iterator ListIt;

		if (!lines.empty()) {
			font->Begin();

			int lineNum = 0;
			float linePad = (1.0f / lines.size()) * 0.5f;

			for (LineIt lit = lines.begin(); lit != lines.end(); ++lit) {
				const Graph::GraphLine& line = lit->second;
				const std::list<float3>& data = line.lineData;

				if (data.empty()) {
					continue;
				}

				// right-outline the labels
				const float sx = (maxLabelWidth / globalRendering->viewSizeX) * size.x;
				const float tx = pos.x - ((line.lineLabelWidth / maxLabelWidth) * 1.33f) * sx;
				const float ty = pos.y + ((lineNum * linePad * 2.0f) + linePad) * size.y;

				font->SetTextColor(line.lineColor.x, line.lineColor.y, line.lineColor.z, 1.0f);
				font->glFormat(tx, ty, 1.0f, FONT_SCALE | FONT_NORM, "%s", (line.lineLabel).c_str());

				color[0] = line.lineColor.x * 255;
				color[1] = line.lineColor.y * 255;
				color[2] = line.lineColor.z * 255;
				color[3] = 255;

				glLineWidth(line.lineWidth);
				va->Initialize();

				for (ListIt pit = data.begin(); pit != data.end(); ++pit) {
					ListIt npit = pit; ++npit;

					const float px1 = (((*pit).x - minScale.x) / scale.x) * size.x;
					const float py1 = (((*pit).y - minScale.y) / scale.y) * size.y;
					const float px2 = ((npit == data.end() || px2 < px1)? px1: (((*npit).x - minScale.x) / scale.x) * size.x);
					const float py2 = ((npit == data.end()             )? py1: (((*npit).y - minScale.y) / scale.y) * size.y);

					va->AddVertexC(pos + float3(px1, py1, 0.0f), color);
					va->AddVertexC(pos + float3(px2, py2, 0.0f), color);
				}

				va->DrawArrayC(GL_LINE_STRIP);
				glLineWidth(1.0f);

				lineNum += 1;
			}

			font->End();
		}
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}






void DebugDrawerAI::TexSet::Clear() {
	for (std::map<int, TexSet::Texture*>::iterator it = textures.begin(); it != textures.end(); ++it) {
		delete (it->second);
	}

	textures.clear();
}

int DebugDrawerAI::TexSet::AddTexture(const float* data, int w, int h) {
	if (!globalRendering->supportNPOTs && (w != next_power_of_2(w) || h != next_power_of_2(h))) {
		return 0;
	}

	textures[curTexHandle] = new TexSet::Texture(w, h, data);
	return (curTexHandle++);
}

void DebugDrawerAI::TexSet::UpdateTexture(int texHandle, const float* data, int x, int y, int w, int h) {
	std::map<int, TexSet::Texture*>::iterator it = textures.find(texHandle);

	if (it == textures.end()) {
		return;
	}

	if ((x + w) > (it->second)->GetWidth()) { return; }
	if ((y + h) > (it->second)->GetHeight()) { return; }

	glBindTexture(GL_TEXTURE_2D, (it->second)->GetID());
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_FLOAT, data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void DebugDrawerAI::TexSet::DelTexture(int texHandle) {
	std::map<int, TexSet::Texture*>::iterator it = textures.find(texHandle);

	if (it != textures.end()) {
		delete (it->second);
		textures.erase(it->first);
	}
}

void DebugDrawerAI::TexSet::SetTexturePos(int texHandle, float x, float y) {
	std::map<int, TexSet::Texture*>::iterator it = textures.find(texHandle);

	if (it != textures.end()) {
		(it->second)->SetPos(float3(x, y, 0.0f));
	}
}
void DebugDrawerAI::TexSet::SetTextureSize(int texHandle, float w, float h) {
	std::map<int, TexSet::Texture*>::iterator it = textures.find(texHandle);

	if (it != textures.end()) {
		(it->second)->SetSize(float3(w, h, 0.0f));
	}
}
void DebugDrawerAI::TexSet::SetTextureLabel(int texHandle, const std::string& label) {
	std::map<int, TexSet::Texture*>::iterator it = textures.find(texHandle);

	if (it != textures.end()) {
		(it->second)->SetLabel(label);
	}
}



void DebugDrawerAI::TexSet::Draw() {
	if (!textures.empty()) {
		glEnable(GL_TEXTURE_2D);
		font->Begin();
		font->SetTextColor(0.0f, 0.0f, 0.0f, 1.0f);

		CVertexArray* va = GetVertexArray();

		for (std::map<int, TexSet::Texture*>::iterator it = textures.begin(); it != textures.end(); ++it) {
			const TexSet::Texture* tex = it->second;
			const float3& pos = tex->GetPos();
			const float3& size = tex->GetSize();

			glBindTexture(GL_TEXTURE_2D, tex->GetID());
			va->Initialize();
			va->AddVertexT(pos,                                0.0f, 1.0f);
			va->AddVertexT(pos + float3(size.x,   0.0f, 0.0f), 1.0f, 1.0f);
			va->AddVertexT(pos + float3(size.x, size.y, 0.0f), 1.0f, 0.0f);
			va->AddVertexT(pos + float3(  0.0f, size.y, 0.0f), 0.0f, 0.0f);
			va->DrawArrayT(GL_QUADS);
			glBindTexture(GL_TEXTURE_2D, 0);

			const float tx = pos.x + size.x * 0.5f - ((tex->GetLabelWidth() * 0.5f) / globalRendering->viewSizeX) * size.x;
			const float ty = pos.y + size.y        + ((tex->GetLabelHeight() * 0.5f) / globalRendering->viewSizeY) * size.y;

			font->glFormat(tx, ty, 1.0f, FONT_SCALE | FONT_NORM, "%s", tex->GetLabel().c_str());
		}

		font->End();
		glDisable(GL_TEXTURE_2D);
	}
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
