/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LINE_DRAWER_H
#define _LINE_DRAWER_H

#include <vector>

#include "Game/UI/CursorIcons.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "System/Color.h"

class CLineDrawer {
public:
	CLineDrawer() {
		for (size_t i = 0; i < regularLines.size(); i++) {
			regularLines[i].reserve(64);
			stippleLines[i].reserve(64);
		}
	}

	enum LineWidth {
		Default   = 0,
		QueuedCmd = 1
	};


	void Configure(
		bool useColorRestarts,
		bool useRestartColor,
		const float* restartColor,
		float restartAlpha
	);
	void SetWidth(LineWidth w) { width = w; }

	void DrawAll(bool onMiniMap);
	void SetupLineStipple();
	void UpdateLineStipple();

	void StartPath(const float3& pos, const float* color);
	void DrawLine(const float3& endPos, const float* color);
	void DrawLineAndIcon(int cmdID, const float3& endPos, const float* color);
	void DrawIconAtLastPos(int cmdID);
	void Break(const float3& endPos, const float* color);
	void Restart();
	/// same as restart; only way for this to work would be using glGet so it's left broken
	void RestartSameColor() { Restart(); }
	void RestartWithColor(const float* color);

	const float3& GetLastPos() const { return lastPos; }

	bool HaveRegularLines() const { for (auto& v: regularLines) { if (!v.empty()) return true; } return false; }
	bool HaveStippleLines() const { for (auto& v: stippleLines) { if (!v.empty()) return true; } return false; }

private:
	bool lineStipple = false;
	bool useColorRestarts = false;
	bool useRestartColor = false;

	float restartAlpha = 0.0f;
	float stippleTimer = 0.0f;

	LineWidth width = Default;

	float3 lastPos;

	const float* restartColor = nullptr;
	const float* lastColor = nullptr;

	typedef std::vector<VA_TYPE_C> Line;

	// queue all lines and draw them in one go later
	// even := GL_LINE_LOOP, odd := GL_LINES (useColorRestarts)
	// width:
	// [0,1] := 1.0f or 2.5f on minimap
	// [2,3] := cmdColors.QueuedLineWidth() or 2.5f on minimap

	std::array<std::vector<Line>, 4> regularLines;
	std::array<std::vector<Line>, 4> stippleLines;
};


extern CLineDrawer lineDrawer;


/******************************************************************************/
//
//  Inlines
//

inline void CLineDrawer::Configure(bool ucr, bool urc, const float* rc, float ra)
{
	restartAlpha = ra;
	restartColor = rc;

	useRestartColor = urc;
	useColorRestarts = ucr;
}


inline void CLineDrawer::Break(const float3& endPos, const float* color)
{
	lastPos = endPos;
	lastColor = color;
}


inline void CLineDrawer::RestartWithColor(const float *color)
{
	lastColor = color;
	Restart();
}


inline void CLineDrawer::StartPath(const float3& pos, const float* color)
{
	lastPos = pos;
	lastColor = color;
	Restart();
}


inline void CLineDrawer::DrawLine(const float3& endPos, const float* color)
{
	const int idx = width * 2 + useColorRestarts;
	Line& line = (lineStipple)? stippleLines[idx].back(): regularLines[idx].back();

	if (!useColorRestarts) {
		line.push_back({endPos, color});
	} else {
		line.push_back({lastPos, useRestartColor? restartColor: SColor{color[0], color[1], color[2], color[3] * restartAlpha}});
		line.push_back({endPos, color});
	}

	lastPos = endPos;
	lastColor = color;
}


inline void CLineDrawer::DrawLineAndIcon(int cmdID, const float3& endPos, const float* color)
{
	cursorIcons.AddIcon(cmdID, endPos);
	DrawLine(endPos, color);
}


inline void CLineDrawer::DrawIconAtLastPos(int cmdID)
{
	cursorIcons.AddIcon(cmdID, lastPos);
}


#endif // _LINE_DRAWER_H
