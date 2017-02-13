/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LINE_DRAWER_H
#define _LINE_DRAWER_H

#include <vector>

#include "Game/UI/CursorIcons.h"
#include "Rendering/GL/myGL.h"

class CLineDrawer {
	public:
		CLineDrawer();

		void Configure(bool useColorRestarts, bool useRestartColor,
		               const float* restartColor, float restartAlpha);

		void SetupLineStipple();
		void UpdateLineStipple();
		               
		void StartPath(const float3& pos, const float* color);
		void FinishPath() const;
		void DrawLine(const float3& endPos, const float* color);
		void DrawLineAndIcon(int cmdID, const float3& endPos, const float* color);
		void DrawIconAtLastPos(int cmdID);
		void Break(const float3& endPos, const float* color);
		void Restart();
		/// now same as restart
		void RestartSameColor();
		void RestartWithColor(const float* color);
		const float3& GetLastPos() const { return lastPos; }

		void DrawAll();

	private:
		bool lineStipple;
		bool useColorRestarts;
		bool useRestartColor;
		float restartAlpha;
		const float* restartColor;
		
		float3 lastPos;
		const float* lastColor;
		
		float stippleTimer;

		// queue all lines and draw them in one go later
		struct LinePair {
			GLenum type;
			std::vector<GLfloat> verts;
			std::vector<GLfloat> colors;
		};

		std::vector<LinePair> lines;
		std::vector<LinePair> stippled;
};


extern CLineDrawer lineDrawer;


/******************************************************************************/
//
//  Inlines
//

inline void CLineDrawer::Configure(bool ucr, bool urc,
                                   const float* rc, float ra)
{
	restartAlpha = ra;
	restartColor = rc;
	useRestartColor = urc;
	useColorRestarts = ucr;
}


inline void CLineDrawer::FinishPath() const
{
	// noop, left for compatibility
}


inline void CLineDrawer::Break(const float3& endPos, const float* color)
{
	lastPos = endPos;
	lastColor = color;
}


inline void CLineDrawer::Restart()
{
	LinePair *ptr;
	if (lineStipple) {
		stippled.push_back(LinePair());
		ptr = &stippled.back();
	} else {
		lines.push_back(LinePair());
		ptr = &lines.back();
	}
	LinePair& p = *ptr;

	if (!useColorRestarts)	 {
		p.type = GL_LINE_STRIP;
		p.colors.push_back(lastColor[0]);
		p.colors.push_back(lastColor[1]);
		p.colors.push_back(lastColor[2]);
		p.colors.push_back(lastColor[3]);
		p.verts.push_back(lastPos[0]);
		p.verts.push_back(lastPos[1]);
		p.verts.push_back(lastPos[2]);
	} else {
		p.type = GL_LINES;
	}
}


inline void CLineDrawer::RestartWithColor(const float *color)
{
	lastColor = color;
	Restart();
}


inline void CLineDrawer::RestartSameColor()
{
	// the only way for this to work would be using glGet AFAIK
	// so it's left broken
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
	LinePair *ptr;
	if (lineStipple) {
		ptr = &stippled.back();
	} else {
		ptr = &lines.back();
	}
	LinePair& p = *ptr;

	if (!useColorRestarts) {
		p.colors.push_back(color[0]);
		p.colors.push_back(color[1]);
		p.colors.push_back(color[2]);
		p.colors.push_back(color[3]);
		p.verts.push_back(endPos.x);
		p.verts.push_back(endPos.y);
		p.verts.push_back(endPos.z);
	} else {
		if (useRestartColor) {
			p.colors.push_back(restartColor[0]);
			p.colors.push_back(restartColor[1]);
			p.colors.push_back(restartColor[2]);
			p.colors.push_back(restartColor[3]);
		} else {
			p.colors.push_back(color[0]);
			p.colors.push_back(color[1]);
			p.colors.push_back(color[2]);
			p.colors.push_back(color[3] * restartAlpha);
		}
		p.verts.push_back(lastPos.x);
		p.verts.push_back(lastPos.y);
		p.verts.push_back(lastPos.z);

		p.colors.push_back(color[0]);
		p.colors.push_back(color[1]);
		p.colors.push_back(color[2]);
		p.colors.push_back(color[3]);
		p.verts.push_back(endPos.x);
		p.verts.push_back(endPos.y);
		p.verts.push_back(endPos.z);
	}

	lastPos = endPos;
	lastColor = color;
}


inline void CLineDrawer::DrawLineAndIcon(
                         int cmdID, const float3& endPos, const float* color)
{
	cursorIcons.AddIcon(cmdID, endPos);
	DrawLine(endPos, color);
}


inline void CLineDrawer::DrawIconAtLastPos(int cmdID)
{
	cursorIcons.AddIcon(cmdID, lastPos);
}


#endif // _LINE_DRAWER_H
