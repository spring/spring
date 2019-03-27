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
		for (int i = 0; i < 2; i++) {
			regularLines[i].reserve(64);
			stippleLines[i].reserve(64);
		}
	}

	void Configure(
		bool useColorRestarts,
		bool useRestartColor,
		const float* restartColor,
		float restartAlpha
	);

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

	bool HaveRegularLines() const { return (!regularLines[0].empty() || !regularLines[1].empty()); }
	bool HaveStippleLines() const { return (!stippleLines[0].empty() || !stippleLines[1].empty()); }

private:
	bool lineStipple = false;
	bool useColorRestarts = false;
	bool useRestartColor = false;

	float restartAlpha = 0.0f;
	float stippleTimer = 0.0f;

	float3 lastPos;

	const float* restartColor = nullptr;
	const float* lastColor = nullptr;

	typedef std::vector<VA_TYPE_C> Line;

	// queue all lines and draw them in one go later
	// [0] := GL_LINE_LOOP, [1] := GL_LINES (useColorRestarts)
	std::vector<Line> regularLines[2];
	std::vector<Line> stippleLines[2];
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
	Line& line = (lineStipple)? stippleLines[useColorRestarts].back(): regularLines[useColorRestarts].back();

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
