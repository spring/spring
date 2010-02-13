#ifndef __LINE_DRAWER_H__
#define __LINE_DRAWER_H__

#include "Rendering/GL/myGL.h"
#include "GlobalUnsynced.h"
#include "Game/UI/CursorIcons.h"

class CLineDrawer {
	public:
		CLineDrawer();
		~CLineDrawer() {};

		void Configure(bool useColorRestarts, bool useRestartColor,
		               const float* restartColor, float restartAlpha);

		void SetupLineStipple();
		void UpdateLineStipple();
		               
		void StartPath(const float3& pos, const float* color);
		void FinishPath();
		void DrawLine(const float3& endPos, const float* color);
		void DrawLineAndIcon(int cmdID, const float3& endPos, const float* color);
		void DrawIconAtLastPos(int cmdID);
		void Break(const float3& endPos, const float* color);
		void Restart();
		void RestartSameColor();
		const float3& GetLastPos() const { return lastPos; }

	private:
		bool lineStipple;
		bool useColorRestarts;
		bool useRestartColor;
		float restartAlpha;
		const float* restartColor;
		
		float3 lastPos;
		const float* lastColor;
		
		float stippleTimer;
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


inline void CLineDrawer::FinishPath()
{
	glEnd();
	if (lineStipple) {
		glDisable(GL_LINE_STIPPLE);
	}
}


inline void CLineDrawer::Break(const float3& endPos, const float* color)
{
	lastPos = endPos;
	lastColor = color;
	glEnd();
	if (lineStipple) {
		glDisable(GL_LINE_STIPPLE);
	}
}


inline void CLineDrawer::Restart()
{
	// GL_LINE_STIPPLE apparently leaks memory in ATI drivers
	if (lineStipple && !gu->atiHacks) {
		glEnable(GL_LINE_STIPPLE);
	}
	if (!useColorRestarts) {
		glBegin(GL_LINE_STRIP);
		glColor4fv(lastColor);
		glVertexf3(lastPos);
	} else {
		glBegin(GL_LINES);
	}
}


inline void CLineDrawer::RestartSameColor()
{
	if (lineStipple && !gu->atiHacks) {
		glEnable(GL_LINE_STIPPLE);
	}
	if (!useColorRestarts) {
		glBegin(GL_LINE_STRIP);
		glVertexf3(lastPos);
	} else {
		glBegin(GL_LINES);
	}
}


inline void CLineDrawer::StartPath(const float3& pos, const float* color)
{
	lastPos = pos;
	lastColor = color;
	Restart();
}


inline void CLineDrawer::DrawLine(const float3& endPos, const float* color)
{
	if (!useColorRestarts) {
		glColor4fv(color);
		glVertexf3(endPos);
	} else {
		if (useRestartColor) {
			glColor4fv(restartColor);
		} else {
			glColor4f(color[0], color[1], color[2], color[3] * restartAlpha);
		}
		glVertexf3(lastPos);
		glColor4fv(color);
		glVertexf3(endPos);
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


#endif // __LINE_DRAWER_H__
