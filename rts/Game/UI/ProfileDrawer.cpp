/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include <deque>

#include "ProfileDrawer.h"
#include "InputReceiver.h"
#include "Game/GlobalUnsynced.h"
#include "System/EventHandler.h"
#include "System/Rectangle.h"
#include "System/ThreadPool.h"
#include "System/TimeProfiler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/GlobalConstants.h" // for GAME_SPEED
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "lib/lua/include/LuaUser.h"

ProfileDrawer* ProfileDrawer::instance = NULL;

static const float start_x = 0.6f;
static const float end_x   = 0.99f;
static const float end_y   = 0.99f;
static const float start_y = 0.965f;
static const float lineHeight = 0.017f;

static const auto DBG_FONT_FLAGS = (FONT_SCALE | FONT_NORM | FONT_SHADOW);

typedef std::pair<spring_time,spring_time> TimeSlice;
static std::deque<TimeSlice> vidFrames;
static std::deque<TimeSlice> simFrames;
static std::deque<TimeSlice> lgcFrames;
static std::deque<TimeSlice> swpFrames;
static std::deque<TimeSlice> uusFrames;


ProfileDrawer::ProfileDrawer()
: CEventClient("[ProfileDrawer]", 199991, false)
{
	autoLinkEvents = true;
	RegisterLinkedEvents(this);
	eventHandler.AddClient(this);
}


ProfileDrawer::~ProfileDrawer()
{
}


void ProfileDrawer::SetEnabled(bool enable)
{
	if (enable) {
		assert(instance == NULL);
		instance = new ProfileDrawer();
		// reset peak indicators each time the drawer is restarted
		for (auto& p: profiler.profile)
			p.second.peak = 0.0f;
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



static void DrawTimeSlice(std::deque<TimeSlice>& frames, const spring_time curTime, const spring_time maxHist, const float drawArea[4])
{
	// remove old entries
	while (!frames.empty() && (curTime - frames.front().second) > maxHist) {
		frames.pop_front();
	}

	const float y1 = drawArea[1];
	const float y2 = drawArea[3];

	// render
	CVertexArray* va = GetVertexArray();
	va->Initialize();
	for (TimeSlice& ts: frames) {
		float x1 = (ts.first % maxHist).toSecsf() / maxHist.toSecsf();
		float x2 = (ts.second % maxHist).toSecsf() / maxHist.toSecsf();
		x2 = std::max(x1 + globalRendering->pixelX, x2);

		x1 = drawArea[0] + x1 * (drawArea[2] - drawArea[0]);
		x2 = drawArea[0] + x2 * (drawArea[2] - drawArea[0]);

		va->AddVertex0(x1, y1, 0.0f);
		va->AddVertex0(x1, y2, 0.0f);
		va->AddVertex0(x2, y2, 0.0f);
		va->AddVertex0(x2, y1, 0.0f);

		const float mx1 = x1 + 3 * globalRendering->pixelX;
		const float mx2 = x2 - 3 * globalRendering->pixelX;
		if (mx1 < mx2) {
			va->AddVertex0(mx1, y1 + 3 * globalRendering->pixelX, 0.0f);
			va->AddVertex0(mx1, y2 - 3 * globalRendering->pixelX, 0.0f);
			va->AddVertex0(mx2, y2 - 3 * globalRendering->pixelX, 0.0f);
			va->AddVertex0(mx2, y1 + 3 * globalRendering->pixelX, 0.0f);
		}
	}

	va->DrawArray0(GL_QUADS);
}


static void DrawThreadBarcode()
{
	const float maxHist_f = 4.0f;
	const spring_time curTime = spring_now();
	const spring_time maxHist = spring_secs(maxHist_f);
	auto& coreProf = profiler.profileCore;
	const auto numThreads = coreProf.size();

	const float drawArea[4] = {0.01f, 0.30f, (start_x / 2), 0.35f};

	// background
	CVertexArray* va = GetVertexArray();
	va->Initialize();
		va->AddVertex0(drawArea[0] - 10 * globalRendering->pixelX, drawArea[1] - 10 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(drawArea[0] - 10 * globalRendering->pixelX, drawArea[3] + 10 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(drawArea[2] + 10 * globalRendering->pixelX, drawArea[3] + 10 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(drawArea[2] + 10 * globalRendering->pixelX, drawArea[1] - 10 * globalRendering->pixelY, 0.0f);
	glColor4f(0.0f,0.0f,0.0f, 0.5f);
	va->DrawArray0(GL_QUADS);

	// title
	font->glFormat(drawArea[0], drawArea[3], 0.7f, FONT_TOP | DBG_FONT_FLAGS, "ThreadPool (%.0fsec)", maxHist_f);

	// bars
	glColor4f(1.0f,0.0f,0.0f, 0.6f);
	int i = 0;
	for (auto& frames: coreProf) {
		float drawArea2[4] = {drawArea[0], 0.f, drawArea[2], 0.f};
		drawArea2[1] = drawArea[1] + ((drawArea[3] - drawArea[1]) / numThreads) * i++;
		drawArea2[3] = drawArea[1] + ((drawArea[3] - drawArea[1]) / numThreads) * i - 4 * globalRendering->pixelY;
		DrawTimeSlice(frames, curTime, maxHist, drawArea2);
	}

	// feeder
	//const float y1 = 0.0f;
	//const float y2 = 0.1f * numThreads;
	//CVertexArray* va = GetVertexArray();
	va->Initialize();
		const float r = (curTime % maxHist).toSecsf() / maxHist_f;
		const float xf = drawArea[0] + r * (drawArea[2] - drawArea[0]);
		va->AddVertex0(xf, drawArea[1], 0.0f);
		va->AddVertex0(xf, drawArea[3], 0.0f);
		va->AddVertex0(xf + 5 * globalRendering->pixelX, drawArea[3], 0.0f);
		va->AddVertex0(xf + 5 * globalRendering->pixelX, drawArea[1], 0.0f);
	glColor3f(1.0f,0.0f,0.0f);
	va->DrawArray0(GL_QUADS);
}


static void DrawFrameBarcode()
{
	const float maxHist_f = 0.5f;
	const spring_time curTime = spring_now();
	const spring_time maxHist = spring_secs(maxHist_f);
	float drawArea[4] = {0.01f, 0.2f, start_x - 0.05f, 0.25f};

	// background
	CVertexArray* va = GetVertexArray();
	va->Initialize();
		va->AddVertex0(drawArea[0] - 10 * globalRendering->pixelX, drawArea[1] - 10 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(drawArea[0] - 10 * globalRendering->pixelX, drawArea[3] + 20 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(drawArea[2] + 10 * globalRendering->pixelX, drawArea[3] + 20 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(drawArea[2] + 10 * globalRendering->pixelX, drawArea[1] - 10 * globalRendering->pixelY, 0.0f);
	glColor4f(0.0f,0.0f,0.0f, 0.5f);
	va->DrawArray0(GL_QUADS);

	// title and legend
	font->glFormat(drawArea[0], drawArea[3] + 10 * globalRendering->pixelY, 0.7f, FONT_TOP | DBG_FONT_FLAGS,
			"Frame Grapher (%.2fsec)"
			"\xff\xff\x80\xff  GC"
			"\xff\xff\xff\x01  Unsynced"
			"\xff\x01\x01\xff  Swap"
			"\xff\x01\xff\x01  Video"
			"\xff\xff\x01\x01  Sim"
			, maxHist_f);

	// gc frames
	glColor4f(1.0f,0.5f,1.0f, 0.55f);
	DrawTimeSlice(lgcFrames, curTime, maxHist, drawArea);

	// updateunsynced frames
	glColor4f(1.0f,1.0f,0.0f, 0.9f);
	DrawTimeSlice(uusFrames, curTime, maxHist, drawArea);

	// video swap frames
	glColor4f(0.0f,0.0f,1.0f, 0.55f);
	DrawTimeSlice(swpFrames, curTime, maxHist, drawArea);

	// video frames
	glColor4f(0.0f,1.0f,0.0f, 0.55f);
	DrawTimeSlice(vidFrames, curTime, maxHist, drawArea);

	// sim frames
	glColor4f(1.0f,0.0f,0.0f, 0.55f);
	DrawTimeSlice(simFrames, curTime, maxHist, drawArea);

	// draw `feeder` indicating current time pos
	//CVertexArray* va = GetVertexArray();
	va->Initialize();
		// draw feeder
		const float r = (curTime % maxHist).toSecsf() / maxHist_f;
		const float xf = drawArea[0] + r * (drawArea[2] - drawArea[0]);
		va->AddVertex0(xf, drawArea[1], 0.0f);
		va->AddVertex0(xf, drawArea[3], 0.0f);
		va->AddVertex0(xf + 10 * globalRendering->pixelX, drawArea[3], 0.0f);
		va->AddVertex0(xf + 10 * globalRendering->pixelX, drawArea[1], 0.0f);

		// draw scale (horizontal bar that indicates 30FPS timing length)
		const float xs1 = drawArea[2] - 1.f/(30.f*maxHist_f) * (drawArea[2] - drawArea[0]);
		const float xs2 = drawArea[2] +               0.0f * (drawArea[2] - drawArea[0]);
		va->AddVertex0(xs1, drawArea[3] +  2 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(xs1, drawArea[3] + 10 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(xs2, drawArea[3] + 10 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(xs2, drawArea[3] +  2 * globalRendering->pixelY, 0.0f);
	glColor4f(1.0f,0.0f,0.0f, 1.0f);
	va->DrawArray0(GL_QUADS);
}


static void DrawProfiler()
{
	font->SetTextColor(1,1,1,1);
	CVertexArray* va  = GetVertexArray();
	CVertexArray* va2 = GetVertexArray();

	// draw the background of the window
	if(!profiler.profile.empty()){
		va->Initialize();
			va->AddVertex0(start_x, end_y,                                      0);
			va->AddVertex0(end_x,   end_y,                                      0);
			va->AddVertex0(start_x, end_y-profiler.profile.size()*lineHeight-0.01f, 0);
			va->AddVertex0(end_x,   end_y-profiler.profile.size()*lineHeight-0.01f, 0);
		glColor4f(0.0f, 0.0f, 0.5f, 0.5f);
		va->DrawArray0(GL_TRIANGLE_STRIP);
	}

	std::map<std::string, CTimeProfiler::TimeRecord>::iterator pi;

	const float textSize = 0.5f;

	// draw the textual info (total-time, short-time percentual time, timer-name)
	int y = 0;
	for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi, ++y) {
		const float fStartY = start_y - y * lineHeight;
		float fStartX = start_x + 0.005f + 0.015f + 0.005f;

		// print total-time running since application start
		fStartX += 0.04f;
		font->glFormat(fStartX, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT, "%.2fs", pi->second.total.toSecsf());

		// print percent of CPU time used within the last 500ms
		fStartX += 0.04f;
		font->glFormat(fStartX, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT, "%.2f%%", pi->second.percent * 100);
		fStartX += 0.04f;
		font->glFormat(fStartX, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT, "\xff\xff%c%c%.2f%%", pi->second.newPeak?1:255, pi->second.newPeak?1:255, pi->second.peak * 100);
		fStartX += 0.04f;
		font->glFormat(fStartX, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT, "\xff\xff%c%c%.0fms", pi->second.newLagPeak?1:255, pi->second.newLagPeak?1:255, pi->second.maxLag);

		// print timer name
		fStartX += 0.01f;
		font->glFormat(fStartX, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM, pi->first);
	}


	// draw the Timer selection boxes
	const float boxSize = lineHeight*0.9;
	const float selOffset = boxSize*0.2;
	glPushMatrix();
	glTranslatef(start_x + 0.005f, start_y + boxSize, 0); // we are now at upper left of first box
		va->Initialize();
		va2->Initialize();
			int i = 0;
			for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi, ++i){

				auto& fc = pi->second.color;
				SColor c(fc[0], fc[1], fc[2]);
				va->AddVertexC(float3(0, -i*lineHeight, 0), c); // upper left
				va->AddVertexC(float3(0, -i*lineHeight-boxSize, 0), c); // lower left
				va->AddVertexC(float3(boxSize, -i*lineHeight-boxSize, 0), c); // lower right
				va->AddVertexC(float3(boxSize, -i*lineHeight, 0), c); // upper right

				if (pi->second.showGraph) {
					va2->AddVertex0(lineHeight+selOffset, -i*lineHeight-selOffset, 0); // upper left
					va2->AddVertex0(lineHeight+selOffset, -i*lineHeight-boxSize+selOffset, 0); // lower left
					va2->AddVertex0(lineHeight+boxSize-selOffset, -i*lineHeight-boxSize+selOffset, 0); // lower right
					va2->AddVertex0(lineHeight+boxSize-selOffset, -i*lineHeight-selOffset, 0); // upper right
				}
			}
		// draw the boxes
		va->DrawArrayC(GL_QUADS);
		// draw the 'graph view disabled' cross
		glColor3f(1,0,0);
		va2->DrawArray0(GL_QUADS);
	glPopMatrix();

	// draw the graph
	glLineWidth(3.0f);
	for (pi = profiler.profile.begin(); pi != profiler.profile.end(); ++pi) {
		if (!pi->second.showGraph) {
			continue;
		}
		va->Initialize();
		const float steps_x = (end_x - start_x) / CTimeProfiler::TimeRecord::frames_size;
		for (size_t a=0; a < CTimeProfiler::TimeRecord::frames_size; ++a) {
			// profile runtime; eg 0.5f means: uses 50% of a CPU (during that frame)
			// This may be more then 1.0f, in case an operation
			// which ran over many frames, ended in this one.
			const float p = pi->second.frames[a].toSecsf() * GAME_SPEED;
			const float x = start_x + (a * steps_x);
			const float y = 0.02f + (p * 0.96f);
			va->AddVertex0(x, y, 0.0f);
		}

		glColorf3((float3)pi->second.color);
		va->DrawArray0(GL_LINE_STRIP);
	}
	glLineWidth(1.0f);
}


static void DrawInfoText()
{
	// background
	CVertexArray* va = GetVertexArray();
	va->Initialize();
		va->AddVertex0(0.01f - 10 * globalRendering->pixelX, 0.02f - 10 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(0.01f - 10 * globalRendering->pixelX, 0.16f + 20 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(start_x - 0.05f + 10 * globalRendering->pixelX, 0.16f + 20 * globalRendering->pixelY, 0.0f);
		va->AddVertex0(start_x - 0.05f + 10 * globalRendering->pixelX, 0.02f - 10 * globalRendering->pixelY, 0.0f);
	glColor4f(0.0f,0.0f,0.0f, 0.5f);
	va->DrawArray0(GL_QUADS);

	//print some infos (fps,gameframe,particles)
	font->SetTextColor(1,1,0.5f,0.8f);

	font->glFormat(0.01f, 0.02f, 1.0f, DBG_FONT_FLAGS, "FPS: %0.1f SimFPS: %0.1f SimFrame: %d Speed: %2.2f (%2.2f) Particles: %d (%d)",
	    globalRendering->FPS, gu->simFPS, gs->frameNum, gs->speedFactor, gs->wantedSpeedFactor, projectileHandler->syncedProjectiles.size() + projectileHandler->unsyncedProjectiles.size(), projectileHandler->currentParticles);

	// 16ms := 60fps := 30simFPS + 30drawFPS
	font->glFormat(0.01f, 0.07f, 0.7f, DBG_FONT_FLAGS, "avgFrame: %s%2.1fms\b avgDrawFrame: %s%2.1fms\b avgSimFrame: %s%2.1fms\b",
	   (gu->avgFrameTime     > 30) ? "\xff\xff\x01\x01" : "", gu->avgFrameTime,
	   (gu->avgDrawFrameTime > 16) ? "\xff\xff\x01\x01" : "", gu->avgDrawFrameTime,
	   (gu->avgSimFrameTime  > 16) ? "\xff\xff\x01\x01" : "", gu->avgSimFrameTime
	);

	const int2 pfsUpdates = pathManager->GetNumQueuedUpdates();
	const char* fmtString = "[%s-PFS] queued updates: %i %i";

	switch (pathManager->GetPathFinderType()) {
		case PFS_TYPE_DEFAULT: {
			font->glFormat(0.01f, 0.12f, 0.7f, DBG_FONT_FLAGS, fmtString, "DEFAULT", pfsUpdates.x, pfsUpdates.y);
		} break;
		case PFS_TYPE_QTPFS: {
			font->glFormat(0.01f, 0.12f, 0.7f, DBG_FONT_FLAGS, fmtString, "QT", pfsUpdates.x, pfsUpdates.y);
		} break;
	}

	SLuaInfo luaInfo = {0, 0, 0, 0};
	spring_lua_alloc_get_stats(&luaInfo);

	font->glFormat(
		0.01f, 0.15f, 0.7f, DBG_FONT_FLAGS,
		"Lua-allocated memory: %.1fMB (%.5uK allocs : %.5u usecs : %.1u states)",
		luaInfo.allocedBytes / 1024.0f / 1024.0f,
		luaInfo.numLuaAllocs / 1000,
		luaInfo.luaAllocTime,
		luaInfo.numLuaStates
	);
}



void ProfileDrawer::DrawScreen()
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0,1,0,1);

	glDisable(GL_TEXTURE_2D);
	font->Begin();
	font->SetTextColor(1,1,0.5f,0.8f);

	DrawThreadBarcode();
	DrawFrameBarcode();
	DrawProfiler();
	DrawInfoText();

	font->End();
	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

bool ProfileDrawer::MousePress(int x, int y, int button)
{
	const float mx = CInputReceiver::MouseX(x);
	const float my = CInputReceiver::MouseY(y);

	// check if a Timer selection box was hit
	if (mx<start_x || mx>end_x || my<end_y-profiler.profile.size()*lineHeight-0.01f || my>end_y) {
		return false;
	}

	const int selIndex = (int) ((end_y - 0.5*lineHeight - my) / lineHeight);

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
	const float mx = CInputReceiver::MouseX(x);
	const float my = CInputReceiver::MouseY(y);

	// check if a Timer selection box was hit
	if (mx<start_x || mx>end_x || my<end_y - profiler.profile.size()*lineHeight-0.01f || my>end_y) {
		return false;
	}

	return true;
}


void ProfileDrawer::DbgTimingInfo(DbgTimingInfoType type, const spring_time start, const spring_time end)
{
	if (!IsEnabled())
		return;

	switch (type) {
		case TIMING_VIDEO: {
			vidFrames.emplace_back(start, end);
		} break;
		case TIMING_SIM: {
			simFrames.emplace_back(start, end);
		} break;
		case TIMING_GC: {
			lgcFrames.emplace_back(start, end);
		} break;
		case TIMING_SWAP: {
			swpFrames.emplace_back(start, end);
		} break;
		case TIMING_UNSYNCED: {
			uusFrames.emplace_back(start, end);
		} break;
	}
}
