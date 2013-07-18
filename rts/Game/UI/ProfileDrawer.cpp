/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>

#include "ProfileDrawer.h"
#include "System/ThreadPool.h"
#include "System/TimeProfiler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/GlobalConstants.h" // for GAME_SPEED

ProfileDrawer* ProfileDrawer::instance = NULL;

void ProfileDrawer::SetEnabled(bool enable)
{
	if (enable) {
		assert(instance == NULL);
		instance = new ProfileDrawer();
		GML_STDMUTEX_LOCK_NOPROF(time); // SetEnabled
		// reset peak indicators each time the drawer is restarted
		std::map<std::string, CTimeProfiler::TimeRecord>::iterator pi;
		for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi)
			(*pi).second.peak = 0.0f;
	} else {
		ProfileDrawer* tmpInstance = instance;
		instance = NULL;
		delete tmpInstance;
	}
}

bool ProfileDrawer::IsEnabled()
{
	return (instance != NULL);
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
		const float fStartY = start_y - y * 0.018f;
#else
		const float fStartY = start_y - y * 0.024f;
#endif
		const float s = pi->second.total.toSecsf();
		const float p = pi->second.percent * 100;
		float fStartX = start_x + 0.005f + 0.015f + 0.005f;

		// print total-time running since application start
		fStartX += 0.09f;
		font->glFormat(fStartX, fStartY, 0.7f, FONT_BASELINE | FONT_SCALE | FONT_NORM | FONT_RIGHT, "%.2fs", s);

		// print percent of CPU time used within the last 500ms
		fStartX += 0.04f;
		font->glFormat(fStartX, fStartY, 0.7f, FONT_BASELINE | FONT_SCALE | FONT_NORM | FONT_RIGHT, "%.2f%%", p);
		fStartX += 0.04f;
		font->glFormat(fStartX, fStartY, 0.7f, FONT_BASELINE | FONT_SCALE | FONT_NORM | FONT_RIGHT, "\xff\xff%c%c%.2f%%", pi->second.newpeak?1:255, pi->second.newpeak?1:255, pi->second.peak * 100);

		// print timer name
		fStartX += 0.01f;
		font->glFormat(fStartX, fStartY, 0.7f, FONT_BASELINE | FONT_SCALE | FONT_NORM, pi->first);
	}
	font->End();

	// draw the Timer selection boxes
	glPushMatrix();
	glTranslatef(start_x + 0.005f, start_y, 0);
	glScalef(0.015f, 0.02f, 0.02f);
		glBegin(GL_QUADS);
			int i = 0;
			for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi, ++i){
				glColorf3((float3)pi->second.color);
				glVertex3f(0, 0 - i * 1.2f, 0);
				glVertex3f(1, 0 - i * 1.2f, 0);
				glVertex3f(1, 1 - i * 1.2f, 0);
				glVertex3f(0, 1 - i * 1.2f, 0);
			}
		glEnd();
		// draw the 'graph view disabled' cross
		glBegin(GL_LINES);
			i = 0;
			glColor3f(1,0,0);
			for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi, ++i){
				if (!pi->second.showGraph) {
					glVertex3f(0, 0 - i * 1.2f, 0);
					glVertex3f(1, 1 - i * 1.2f, 0);
					glVertex3f(1, 0 - i * 1.2f, 0);
					glVertex3f(0, 1 - i * 1.2f, 0);
				}
			}
		glEnd();
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
			const float p = pi->second.frames[a].toSecsf() * GAME_SPEED;
			const float x = start_x + (a * steps_x);
			const float y = 0.02f + (p * 0.4f);
			va->AddVertex0(float3(x, y, 0.0f));
		}
		glColorf3((float3)pi->second.color);
		va->DrawArray0(GL_LINE_STRIP);
	}

	const float maxHist = 4.0f;
	const auto curTime = spring_gettime();
	const float r = std::fmod(curTime.toSecsf(), maxHist) / maxHist;

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	auto& coreProf = profiler.profileCore;
	for (int i = 0; i < coreProf.size(); ++i) {
		if (coreProf[i].empty())
			continue;

		const float y1 = 0.1f * i;
		const float y2 = 0.1f * (i+1);

		while (!coreProf[i].empty() && (curTime - coreProf[i].front().second) > spring_secs(maxHist))
			coreProf[i].pop_front();

		for (const auto& data: coreProf[i]) {
			float x1 = std::fmod(data.first.toSecsf(), maxHist)  / maxHist;
			float x2 = std::fmod(data.second.toSecsf(), maxHist) / maxHist;
			x2 = std::max(x1 + globalRendering->pixelX, x2);

			va->AddVertex0(float3(x1, y1, 0.0f));
			va->AddVertex0(float3(x1, y2, 0.0f));

			va->AddVertex0(float3(x2, y2, 0.0f));
			va->AddVertex0(float3(x2, y1, 0.0f));
		}
	}

	const float y1 = 0.0f;
	const float y2 = 0.1f * (ThreadPool::GetNumThreads());
	va->AddVertex0(float3(r, y1, 0.0f));
	va->AddVertex0(float3(r, y2, 0.0f));
	va->AddVertex0(float3(r + 10 * globalRendering->pixelX, y2, 0.0f));
	va->AddVertex0(float3(r + 10 * globalRendering->pixelX, y1, 0.0f));

	glColor3f(1.0f,0.0f,0.0f);
	va->DrawArray0(GL_QUADS);

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
