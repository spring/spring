#include "StdAfx.h"
#include <assert.h>
#include "mmgr.h"

#include "ProfileDrawer.h"
#include "TimeProfiler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/GlobalConstants.h" // for GAME_SPEED

ProfileDrawer* ProfileDrawer::instance = NULL;

void ProfileDrawer::Enable()
{
	assert(instance == NULL);
	instance = new ProfileDrawer();
}


void ProfileDrawer::Disable()
{
	if (instance) {
		delete instance;
		instance = NULL;
	}
}

static const float start_x = 0.6f;
static const float end_x   = 0.99f;
static const float end_y   = 0.99f;
static const float start_y = 0.965f;

void ProfileDrawer::Draw()
{
	GML_STDMUTEX_LOCK_NOPROF(time); // Draw

	// draw the background of the window
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.0f, 0.0f, 0.5f, 0.5f);
	if(!profiler.profile.empty()){
		glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(start_x, end_y,                                      0);
		glVertex3f(end_x,   end_y,                                      0);
		glVertex3f(start_x, end_y-profiler.profile.size()*0.024f-0.01f, 0);
		glVertex3f(end_x,   end_y-profiler.profile.size()*0.024f-0.01f, 0);
		glEnd();
	}

	std::map<std::string, CTimeProfiler::TimeRecord>::iterator pi;

	// draw the textual info (total-time, short-time percentual time, timer-name)
	int y = 0;
	font->Begin();
	for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi, ++y) {
#if GML_MUTEX_PROFILER
		if (pi->first.size()<5 || pi->first.substr(pi->first.size()-5,5).compare("Mutex")!=0) { --y; continue; }
#endif
		const float s = ((float)pi->second.total) / 1000.0f;
		const float p = pi->second.percent * 100;
		float fStartX = start_x + 0.005f + 0.015f + 0.005f;
		const float fStartY = start_y - y * 0.024f;

		// print total-time running since application start
		fStartX += 0.09f;
		font->glFormat(fStartX, fStartY, 0.7f, FONT_BASELINE | FONT_SCALE | FONT_NORM | FONT_RIGHT, "%.2fs", s);

		// print percent of CPU time used within the last 500ms
		fStartX += 0.08f;
		font->glFormat(fStartX, fStartY, 0.7f, FONT_BASELINE | FONT_SCALE | FONT_NORM | FONT_RIGHT, "%.2f%%", p);

		// print timer name
		fStartX += 0.01f;
		font->glFormat(fStartX, fStartY, 0.7f, FONT_BASELINE | FONT_SCALE | FONT_NORM, "%s", pi->first.c_str());
	}
	font->End();

	// draw the Timer selection boxes
	glPushMatrix();
	glTranslatef(start_x + 0.005f, start_y, 0);
	glScalef(0.015f, 0.02f, 0.02f);
	for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi){
		glColorf3((float3)pi->second.color);
		glBegin(GL_QUADS);
		glVertex3f(0,0,0);
		glVertex3f(1,0,0);
		glVertex3f(1,1,0);
		glVertex3f(0,1,0);
		glEnd();
		// draw the 'graph view disabled' cross
		if (!pi->second.showGraph) {
			glColor3f(1,0,0);
			glBegin(GL_LINES);
			glVertex3f(0,0,0);
			glVertex3f(1,1,0);
			glVertex3f(1,0,0);
			glVertex3f(0,1,0);
			glEnd();
		}
		glTranslatef(0,-1.2f,0);
	}
	glPopMatrix();

	// draw the graph
	for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi) {
		if (!pi->second.showGraph) {
			continue;
		}
		CVertexArray* va = GetVertexArray();
		va->Initialize();
		const float steps_x = (end_x - start_x) / CTimeProfiler::TimeRecord::frames_size;
		for (size_t a=0; a < CTimeProfiler::TimeRecord::frames_size; ++a) {
			// profile runtime; eg 0.5f means: uses 50% of a CPU (during that frame)
			// This may be more then 1.0f, in case an operation
			// which ran over many frames, ended in this one.
			const float p = ((float)pi->second.frames[a]) / 1000.0f * GAME_SPEED;
			const float x = start_x + (a * steps_x);
			const float y = 0.02f + (p * 0.4f);
			va->AddVertex0(float3(x, y, 0.0f));
		}
		glColorf3((float3)pi->second.color);
		va->DrawArray0(GL_LINE_STRIP);
	}
	glEnable(GL_TEXTURE_2D);
}

bool ProfileDrawer::MousePress(int x, int y, int button)
{
	GML_STDMUTEX_LOCK_NOPROF(time); // MousePress

	const float mx = MouseX(x);
	const float my = MouseY(y);

	// check if a Timer selection box was hit
	if (mx<start_x || mx>end_x || my<end_y-profiler.profile.size()*0.024f-0.01f || my>end_y) {
		return false;
	}

	const int selIndex = (int) ((end_y - my) / 0.024f);

	// switch the selected Timers showGraph value
	if ((selIndex >= 0) && (selIndex < profiler.profile.size())) {
		std::map<std::string, CTimeProfiler::TimeRecord>::iterator pi = profiler.profile.begin();
		for (int i = 0; i < selIndex; i++) {
			++pi;
		}
		pi->second.showGraph = !pi->second.showGraph;
	}

	return false;
}

bool ProfileDrawer::IsAbove(int x, int y)
{
	GML_STDMUTEX_LOCK_NOPROF(time); // IsAbove

	const float mx = MouseX(x);
	const float my = MouseY(y);

	// check if a Timer selection box was hit
	if (mx<start_x || mx>end_x || my<end_y - profiler.profile.size()*0.024f-0.01f || my>end_y) {
		return false;
	}

	return true;
}

ProfileDrawer::ProfileDrawer()
{
}

ProfileDrawer::~ProfileDrawer()
{
}
