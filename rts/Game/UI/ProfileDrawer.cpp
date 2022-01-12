/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <deque>

#include "ProfileDrawer.h"
#include "InputReceiver.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaAllocState.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/WideLineAdapter.hpp"
#include "Sim/Features/FeatureMemPool.h"
#include "Sim/Misc/GlobalConstants.h" // for GAME_SPEED
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Units/UnitMemPool.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Weapons/WeaponMemPool.h"
#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"
#include "lib/lua/include/LuaUser.h" // spring_lua_alloc_get_stats

ProfileDrawer* ProfileDrawer::instance = nullptr;

static constexpr float MAX_THREAD_HIST_TIME = 0.5f; // secs
static constexpr float MAX_FRAMES_HIST_TIME = 0.5f; // secs

static constexpr float  MIN_X_COOR = 0.6f;
static constexpr float  MAX_X_COOR = 0.99f;
static constexpr float  MIN_Y_COOR = 0.95f;
static constexpr float LINE_HEIGHT = 0.017f;

static constexpr unsigned int DBG_FONT_FLAGS = (FONT_SCALE | FONT_NORM | FONT_SHADOW);

typedef std::pair<spring_time, spring_time> TimeSlice;
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

void ProfileDrawer::SetEnabled(bool enable)
{
	if (!enable) {
		spring::SafeDelete(instance);
		return;
	}

	assert(instance == nullptr);
	instance = new ProfileDrawer();

	// reset peak indicators each time the drawer is restarted
	profiler.ResetPeaks();
}



static void DrawBufferStats(const float2 pos)
{
	const float4 drawArea = {pos.x, pos.y + 0.02f, 0.2f, pos.y - (0.23f + 0.02f)};

	GL::RenderDataBuffer0* rdb0 = GL::GetRenderBuffer0();
	GL::RenderDataBufferC* rdbC = GL::GetRenderBufferC();
	GL::RenderDataBufferT* rdbT = GL::GetRenderBufferT();

	GL::RenderDataBufferT4* rdbT4 = GL::GetRenderBufferT4();
	GL::RenderDataBufferTN* rdbTN = GL::GetRenderBufferTN();
	GL::RenderDataBufferTC* rdbTC = GL::GetRenderBufferTC();

	GL::RenderDataBuffer2D0* rdb2D0 = GL::GetRenderBuffer2D0();
	GL::RenderDataBuffer2DT* rdb2DT = GL::GetRenderBuffer2DT();

	GL::RenderDataBufferL* rdbL = GL::GetRenderBufferL();

	// background
	rdbC->SafeAppend({{drawArea.x - 10.0f * globalRendering->pixelX, drawArea.y - 10.0f * globalRendering->pixelY, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // TL
	rdbC->SafeAppend({{drawArea.x - 10.0f * globalRendering->pixelX, drawArea.w + 10.0f * globalRendering->pixelY, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // BL
	rdbC->SafeAppend({{drawArea.z + 10.0f * globalRendering->pixelX, drawArea.w + 10.0f * globalRendering->pixelY, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // BR

	rdbC->SafeAppend({{drawArea.z + 10.0f * globalRendering->pixelX, drawArea.w + 10.0f * globalRendering->pixelY, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // BR
	rdbC->SafeAppend({{drawArea.z + 10.0f * globalRendering->pixelX, drawArea.y - 10.0f * globalRendering->pixelY, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // TR
	rdbC->SafeAppend({{drawArea.x - 10.0f * globalRendering->pixelX, drawArea.y - 10.0f * globalRendering->pixelY, 0.0f}, {0.0f, 0.0f, 0.0f, 0.5f}}); // TL
	rdbC->Submit(GL_TRIANGLES);

	#define FMT "{elems=" _STPF_ " indcs=" _STPF_ " submits{e,i}={" _STPF_ "," _STPF_ "}}"
	font->SetTextColor(1.0f, 1.0f, 0.5f, 0.8f);
	font->glFormat(pos.x, pos.y - 0.00f, 0.7f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "RenderBuffers (%u/%u)", globalRendering->drawFrame % GL::NUM_RENDER_BUFFERS, GL::NUM_RENDER_BUFFERS);
	font->glFormat(pos.x, pos.y - 0.025f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\t0=" FMT, rdb0->SumElems(), rdb0->SumIndcs(), rdb0->NumSubmits(false), rdb0->NumSubmits(true));
	font->glFormat(pos.x, pos.y - 0.045f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\tC=" FMT, rdbC->SumElems(), rdbC->SumIndcs(), rdbC->NumSubmits(false), rdbC->NumSubmits(true));
	font->glFormat(pos.x, pos.y - 0.065f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\tT=" FMT, rdbT->SumElems(), rdbT->SumIndcs(), rdbT->NumSubmits(false), rdbT->NumSubmits(true));

	font->glFormat(pos.x, pos.y - 0.095f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\tT4=" FMT, rdbT4->SumElems(), rdbT4->SumIndcs(), rdbT4->NumSubmits(false), rdbT4->NumSubmits(true));
	font->glFormat(pos.x, pos.y - 0.115f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\tTN=" FMT, rdbTN->SumElems(), rdbTN->SumIndcs(), rdbTN->NumSubmits(false), rdbTN->NumSubmits(true));
	font->glFormat(pos.x, pos.y - 0.135f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\tTC=" FMT, rdbTC->SumElems(), rdbTC->SumIndcs(), rdbTC->NumSubmits(false), rdbTC->NumSubmits(true));

	font->glFormat(pos.x, pos.y - 0.165f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\t2D0=" FMT, rdb2D0->SumElems(), rdb2D0->SumIndcs(), rdb2D0->NumSubmits(false), rdb2D0->NumSubmits(true));
	font->glFormat(pos.x, pos.y - 0.185f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\t2DT=" FMT, rdb2DT->SumElems(), rdb2DT->SumIndcs(), rdb2DT->NumSubmits(false), rdb2DT->NumSubmits(true));

	font->glFormat(pos.x, pos.y - 0.215f, 0.5f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "\tL=" FMT, rdbL->SumElems(), rdbL->SumIndcs(), rdbL->NumSubmits(false), rdbL->NumSubmits(true));
	#undef FMT
}

static void DrawTimeSlices(
	std::deque<TimeSlice>& frames,
	const spring_time curTime,
	const spring_time maxTime,
	const float4& drawArea,
	const float4& sliceColor
) {
	// remove old entries
	while (!frames.empty() && (curTime - frames.front().second) > maxTime) {
		frames.pop_front();
	}

	const float y1 = drawArea.y;
	const float y2 = drawArea.w;

	// render
	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();

	for (const TimeSlice& ts: frames) {
		float x1 = (ts.first  % maxTime).toSecsf() / maxTime.toSecsf();
		float x2 = (ts.second % maxTime).toSecsf() / maxTime.toSecsf();

		x2 = std::max(x1 + globalRendering->pixelX, x2);

		x1 = drawArea.x + x1 * (drawArea.z - drawArea.x);
		x2 = drawArea.x + x2 * (drawArea.z - drawArea.x);

		buffer->SafeAppend({{x1, y1, 0.0f}, {sliceColor}}); // tl
		buffer->SafeAppend({{x1, y2, 0.0f}, {sliceColor}}); // bl
		buffer->SafeAppend({{x2, y2, 0.0f}, {sliceColor}}); // br

		buffer->SafeAppend({{x2, y2, 0.0f}, {sliceColor}}); // br
		buffer->SafeAppend({{x2, y1, 0.0f}, {sliceColor}}); // tr
		buffer->SafeAppend({{x1, y1, 0.0f}, {sliceColor}}); // tl

		const float mx1 = x1 + 3.0f * globalRendering->pixelX;
		const float mx2 = x2 - 3.0f * globalRendering->pixelX;

		if (mx1 >= mx2)
			continue;

		buffer->SafeAppend({{mx1, y1 + 3.0f * globalRendering->pixelX, 0.0f}, {sliceColor}}); // bl
		buffer->SafeAppend({{mx1, y2 - 3.0f * globalRendering->pixelX, 0.0f}, {sliceColor}}); // tl
		buffer->SafeAppend({{mx2, y2 - 3.0f * globalRendering->pixelX, 0.0f}, {sliceColor}}); // tr

		buffer->SafeAppend({{mx2, y2 - 3.0f * globalRendering->pixelX, 0.0f}, {sliceColor}}); // tr
		buffer->SafeAppend({{mx2, y1 + 3.0f * globalRendering->pixelX, 0.0f}, {sliceColor}}); // br
		buffer->SafeAppend({{mx1, y1 + 3.0f * globalRendering->pixelX, 0.0f}, {sliceColor}}); // bl
	}
}


static void DrawThreadBarcode(GL::RenderDataBufferC* buffer)
{
	constexpr float    barColor[4] = {0.0f, 0.0f, 0.0f, 0.5f};
	constexpr float feederColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};

	const float drawArea[4] = {0.01f, 0.30f, (MIN_X_COOR * 0.5f), 0.35f};

	const spring_time curTime = spring_now();
	const spring_time maxTime = spring_secs(MAX_THREAD_HIST_TIME);

	const size_t numThreads = profiler.GetNumThreadProfiles();

	{
		// background
		buffer->SafeAppend({{drawArea[0] - 10.0f * globalRendering->pixelX, drawArea[1] - 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // tl
		buffer->SafeAppend({{drawArea[0] - 10.0f * globalRendering->pixelX, drawArea[3] + 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // bl
		buffer->SafeAppend({{drawArea[2] + 10.0f * globalRendering->pixelX, drawArea[3] + 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // br

		buffer->SafeAppend({{drawArea[2] + 10.0f * globalRendering->pixelX, drawArea[3] + 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // br
		buffer->SafeAppend({{drawArea[2] + 10.0f * globalRendering->pixelX, drawArea[1] - 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // tr
		buffer->SafeAppend({{drawArea[0] - 10.0f * globalRendering->pixelX, drawArea[1] - 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // tl
	}
	{
		// title
		font->glFormat(drawArea[0], drawArea[3], 0.7f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED, "ThreadPool (%.1f seconds :: " _STPF_ " threads)", MAX_THREAD_HIST_TIME, numThreads);
	}
	{
		// need to lock; DrawTimeSlice pop_front()'s old entries from
		// threadProf while ~ScopedMtTimer can modify it concurrently
		profiler.ToggleLock(true);

		// bars for each pool-thread profile
		int i = 0;
		for (auto& threadProf: profiler.GetThreadProfiles()) {
			float drawArea2[4] = {drawArea[0], 0.0f, drawArea[2], 0.0f};
			drawArea2[1] = drawArea[1] + ((drawArea[3] - drawArea[1]) / numThreads) * i++;
			drawArea2[3] = drawArea[1] + ((drawArea[3] - drawArea[1]) / numThreads) * i - (4 * globalRendering->pixelY);
			DrawTimeSlices(threadProf, curTime, maxTime, drawArea2, {1.0f, 0.0f, 0.0f, 0.6f});
		}

		profiler.ToggleLock(false);
	}
	{
		// feeder
		const float r = (curTime % maxTime).toSecsf() / MAX_THREAD_HIST_TIME;
		const float xf = drawArea[0] + r * (drawArea[2] - drawArea[0]);

		buffer->SafeAppend({{xf                                 , drawArea[1], 0.0f}, {feederColor}}); // tl
		buffer->SafeAppend({{xf                                 , drawArea[3], 0.0f}, {feederColor}}); // bl
		buffer->SafeAppend({{xf + 5.0f * globalRendering->pixelX, drawArea[3], 0.0f}, {feederColor}}); // br

		buffer->SafeAppend({{xf + 5.0f * globalRendering->pixelX, drawArea[3], 0.0f}, {feederColor}}); // br
		buffer->SafeAppend({{xf + 5.0f * globalRendering->pixelX, drawArea[1], 0.0f}, {feederColor}}); // tr
		buffer->SafeAppend({{xf                                 , drawArea[1], 0.0f}, {feederColor}}); // tl
	}
}


static void DrawFrameBarcode(GL::RenderDataBufferC* buffer)
{
	constexpr float    barColor[4] = {0.0f, 0.0f, 0.0f, 0.5f};
	constexpr float feederColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};

	const float drawArea[4] = {0.01f, 0.21f, MIN_X_COOR - 0.05f, 0.26f};

	const spring_time curTime = spring_now();
	const spring_time maxTime = spring_secs(MAX_FRAMES_HIST_TIME);

	// background
	buffer->SafeAppend({{drawArea[0] - 10.0f * globalRendering->pixelX, drawArea[1] - 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // tl
	buffer->SafeAppend({{drawArea[0] - 10.0f * globalRendering->pixelX, drawArea[3] + 20.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // bl
	buffer->SafeAppend({{drawArea[2] + 10.0f * globalRendering->pixelX, drawArea[3] + 20.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // br

	buffer->SafeAppend({{drawArea[2] + 10.0f * globalRendering->pixelX, drawArea[3] + 20.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // br
	buffer->SafeAppend({{drawArea[2] + 10.0f * globalRendering->pixelX, drawArea[1] - 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // tr
	buffer->SafeAppend({{drawArea[0] - 10.0f * globalRendering->pixelX, drawArea[1] - 10.0f * globalRendering->pixelY, 0.0f}, {barColor}}); // tl

	// title and legend
	font->glFormat(drawArea[0], drawArea[3] + 10 * globalRendering->pixelY, 0.7f, FONT_TOP | DBG_FONT_FLAGS | FONT_BUFFERED,
		"Frame Grapher (%.2fsec)"
		"\xff\xff\x80\xff  GC"
		"\xff\xff\xff\x01  Unsynced"
		"\xff\x01\x01\xff  Swap"
		"\xff\x01\xff\x01  Video"
		"\xff\xff\x01\x01  Sim"
		, MAX_FRAMES_HIST_TIME
	);

	DrawTimeSlices(lgcFrames, curTime, maxTime, drawArea, {1.0f, 0.5f, 1.0f, 0.55f}); // gc frames
	DrawTimeSlices(uusFrames, curTime, maxTime, drawArea, {1.0f, 1.0f, 0.0f, 0.90f}); // unsynced-update frames
	DrawTimeSlices(swpFrames, curTime, maxTime, drawArea, {0.0f, 0.0f, 1.0f, 0.55f}); // video swap frames
	DrawTimeSlices(vidFrames, curTime, maxTime, drawArea, {0.0f, 1.0f, 0.0f, 0.55f}); // video frames
	DrawTimeSlices(simFrames, curTime, maxTime, drawArea, {1.0f, 0.0f, 0.0f, 0.55f}); // sim frames

	{
		// draw 'feeder' (indicates current time pos)
		const float r = (curTime % maxTime).toSecsf() / MAX_FRAMES_HIST_TIME;
		const float xf = drawArea[0] + r * (drawArea[2] - drawArea[0]);

		buffer->SafeAppend({{xf                               , drawArea[1], 0.0f}, {feederColor}}); // tl
		buffer->SafeAppend({{xf                               , drawArea[3], 0.0f}, {feederColor}}); // bl
		buffer->SafeAppend({{xf + 10 * globalRendering->pixelX, drawArea[3], 0.0f}, {feederColor}}); // br

		buffer->SafeAppend({{xf + 10 * globalRendering->pixelX, drawArea[3], 0.0f}, {feederColor}}); // br
		buffer->SafeAppend({{xf + 10 * globalRendering->pixelX, drawArea[1], 0.0f}, {feederColor}}); // tr
		buffer->SafeAppend({{xf                               , drawArea[1], 0.0f}, {feederColor}}); // tl
	}
	{
		// draw scale (horizontal bar that indicates 30FPS timing length)
		const float xs1 = drawArea[2] - 1.0f / (30.0f * MAX_FRAMES_HIST_TIME) * (drawArea[2] - drawArea[0]);
		const float xs2 = drawArea[2] + 0.0f                                  * (drawArea[2] - drawArea[0]);

		buffer->SafeAppend({{xs1, drawArea[3] +  2.0f * globalRendering->pixelY, 0.0f}, {feederColor}}); // tl
		buffer->SafeAppend({{xs1, drawArea[3] + 10.0f * globalRendering->pixelY, 0.0f}, {feederColor}}); // bl
		buffer->SafeAppend({{xs2, drawArea[3] + 10.0f * globalRendering->pixelY, 0.0f}, {feederColor}}); // br

		buffer->SafeAppend({{xs2, drawArea[3] + 10.0f * globalRendering->pixelY, 0.0f}, {feederColor}}); // br
		buffer->SafeAppend({{xs2, drawArea[3] +  2.0f * globalRendering->pixelY, 0.0f}, {feederColor}}); // tr
		buffer->SafeAppend({{xs1, drawArea[3] +  2.0f * globalRendering->pixelY, 0.0f}, {feederColor}}); // tl
	}
}


static void DrawProfiler(GL::RenderDataBufferC* buffer)
{
	font->SetTextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// this locks a mutex, so don't call it every frame
	if ((globalRendering->drawFrame % 10) == 0)
		profiler.RefreshProfiles();

	constexpr float winColor[4] = {0.0f, 0.0f, 0.5f, 0.5f};
	constexpr float textSize = 0.5f;

	const auto& sortedProfiles = profiler.GetSortedProfiles();

	// draw the window background
	{
		buffer->SafeAppend({{MIN_X_COOR, MIN_Y_COOR - sortedProfiles.size() * LINE_HEIGHT - 0.010f, 0.0f}, {winColor}}); // tl
		buffer->SafeAppend({{MIN_X_COOR, MIN_Y_COOR +                         LINE_HEIGHT + 0.005f, 0.0f}, {winColor}}); // bl
		buffer->SafeAppend({{MAX_X_COOR, MIN_Y_COOR +                         LINE_HEIGHT + 0.005f, 0.0f}, {winColor}}); // br

		buffer->SafeAppend({{MAX_X_COOR, MIN_Y_COOR +                         LINE_HEIGHT + 0.005f, 0.0f}, {winColor}}); // br
		buffer->SafeAppend({{MAX_X_COOR, MIN_Y_COOR - sortedProfiles.size() * LINE_HEIGHT - 0.010f, 0.0f}, {winColor}}); // tr
		buffer->SafeAppend({{MIN_X_COOR, MIN_Y_COOR - sortedProfiles.size() * LINE_HEIGHT - 0.010f, 0.0f}, {winColor}}); // tl
	}

	// table header
	{
		const float fStartY = MIN_Y_COOR + 0.005f;
		      float fStartX = MIN_X_COOR + 0.005f + 0.015f + 0.005f;

		// print total-time running since application start
		font->glPrint(fStartX += 0.04f, fStartY, textSize, FONT_SHADOW | FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT | FONT_BUFFERED, "sum-time");

		// print percent of CPU time used within the last 500ms
		font->glPrint(fStartX += 0.06f, fStartY, textSize, FONT_SHADOW | FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT | FONT_BUFFERED, "cur-%usage");
		font->glPrint(fStartX += 0.04f, fStartY, textSize, FONT_SHADOW | FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT | FONT_BUFFERED, "max-%usage");
		font->glPrint(fStartX += 0.04f, fStartY, textSize, FONT_SHADOW | FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT | FONT_BUFFERED, "lag");

		// print timer name
		font->glPrint(fStartX += 0.01f, fStartY, textSize, FONT_SHADOW | FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "title");
	}

	// draw the textual info (total-time, short-time percentual time, timer-name)
	int y = 1;

	for (const auto& p: sortedProfiles) {
		const auto& profileData = p.second;

		const float fStartY = MIN_Y_COOR - (y++) * LINE_HEIGHT;
		      float fStartX = MIN_X_COOR + 0.005f + 0.015f + 0.005f;

		// print total-time running since application start
		font->glFormat(fStartX += 0.04f, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT | FONT_BUFFERED, "%.2fs", profileData.total.toSecsf());

		// print percent of CPU time used within the last 500ms
		font->glFormat(fStartX += 0.06f, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT | FONT_BUFFERED, "%.2f%%", profileData.stats.y * 100.0f);
		font->glFormat(fStartX += 0.04f, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT | FONT_BUFFERED, "\xff\xff%c%c%.2f%%", profileData.newPeak? 1: 255, profileData.newPeak? 1: 255, profileData.stats.z * 100.0f);
		font->glFormat(fStartX += 0.04f, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_RIGHT | FONT_BUFFERED, "\xff\xff%c%c%.0fms", profileData.newLagPeak? 1: 255, profileData.newLagPeak? 1: 255, profileData.stats.x);

		// print timer name
		font->glPrint(fStartX += 0.01f, fStartY, textSize, FONT_DESCENDER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, p.first);
	}


	{
		// draw the Timer boxes
		const float boxSize = LINE_HEIGHT * 0.9f;
		const float selOffset = boxSize * 0.2f;

		// translate to upper-left corner of first box
		const CMatrix44f boxMat(float3{MIN_X_COOR + 0.005f, MIN_Y_COOR + boxSize, 0.0f});

		int i = 1;

		for (const auto& p: sortedProfiles) {
			const CTimeProfiler::TimeRecord& tr = p.second;

			const SColor selColor(tr.color.x, tr.color.y, tr.color.z, 1.0f);
			const SColor actColor(1.0f, 0.0f, 0.0f, 1.0f);

			// selection box
			buffer->SafeAppend({boxMat * float3(   0.0f, -i * LINE_HEIGHT          , 0.0f), selColor}); // tl
			buffer->SafeAppend({boxMat * float3(   0.0f, -i * LINE_HEIGHT - boxSize, 0.0f), selColor}); // bl
			buffer->SafeAppend({boxMat * float3(boxSize, -i * LINE_HEIGHT - boxSize, 0.0f), selColor}); // br

			buffer->SafeAppend({boxMat * float3(boxSize, -i * LINE_HEIGHT - boxSize, 0.0f), selColor}); // br
			buffer->SafeAppend({boxMat * float3(boxSize, -i * LINE_HEIGHT          , 0.0f), selColor}); // tr
			buffer->SafeAppend({boxMat * float3(   0.0f, -i * LINE_HEIGHT          , 0.0f), selColor}); // tl

			// activated box
			if (tr.showGraph) {
				buffer->SafeAppend({boxMat * float3(LINE_HEIGHT +           selOffset, -i * LINE_HEIGHT -           selOffset, 0.0f), actColor}); // tl
				buffer->SafeAppend({boxMat * float3(LINE_HEIGHT +           selOffset, -i * LINE_HEIGHT - boxSize + selOffset, 0.0f), actColor}); // bl
				buffer->SafeAppend({boxMat * float3(LINE_HEIGHT + boxSize - selOffset, -i * LINE_HEIGHT - boxSize + selOffset, 0.0f), actColor}); // br

				buffer->SafeAppend({boxMat * float3(LINE_HEIGHT + boxSize - selOffset, -i * LINE_HEIGHT - boxSize + selOffset, 0.0f), actColor}); // br
				buffer->SafeAppend({boxMat * float3(LINE_HEIGHT + boxSize - selOffset, -i * LINE_HEIGHT -           selOffset, 0.0f), actColor}); // tr
				buffer->SafeAppend({boxMat * float3(LINE_HEIGHT +           selOffset, -i * LINE_HEIGHT -           selOffset, 0.0f), actColor}); // tl
			}

			i++;
		}
	}

	// flush all quad elements
	buffer->Submit(GL_TRIANGLES);

	// draw the graph lines
	GL::WideLineAdapterC* wla = GL::GetWideLineAdapterC();
	wla->Setup(buffer, globalRendering->viewSizeX, globalRendering->viewSizeY, 3.0f, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

	for (const auto& p: sortedProfiles) {
		const CTimeProfiler::TimeRecord& tr = p.second;

		const float3& fc = tr.color;
		const SColor c(fc[0], fc[1], fc[2]);

		if (!tr.showGraph)
			continue;

		const float range_x = MAX_X_COOR - MIN_X_COOR;
		const float steps_x = range_x / CTimeProfiler::TimeRecord::numFrames;

		for (size_t a = 0; a < CTimeProfiler::TimeRecord::numFrames; ++a) {
			// profile runtime; 0.5f means 50% of a CPU was used during that frame
			// this may exceed 1.0f if an operation which ran over multiple frames
			// ended in this one
			const float p = tr.frames[a].toSecsf() * GAME_SPEED;
			const float x = MIN_X_COOR + (a * steps_x);
			const float y = 0.02f + (p * 0.96f);

			wla->SafeAppend({{x, y, 0.0f}, c});
		}

		wla->Submit(GL_LINE_STRIP);
	}
}


static void DrawInfoText(GL::RenderDataBufferC* buffer)
{
	constexpr float bgColor[4] = {0.0f, 0.0f, 0.0f, 0.5f};

	// background
	buffer->SafeAppend({{             0.01f - 10.0f * globalRendering->pixelX, 0.02f - 10.0f * globalRendering->pixelY, 0.0f}, {bgColor}}); // tl
	buffer->SafeAppend({{             0.01f - 10.0f * globalRendering->pixelX, 0.17f + 20.0f * globalRendering->pixelY, 0.0f}, {bgColor}}); // bl
	buffer->SafeAppend({{MIN_X_COOR - 0.05f + 10.0f * globalRendering->pixelX, 0.17f + 20.0f * globalRendering->pixelY, 0.0f}, {bgColor}}); // br

	buffer->SafeAppend({{MIN_X_COOR - 0.05f + 10.0f * globalRendering->pixelX, 0.17f + 20.0f * globalRendering->pixelY, 0.0f}, {bgColor}}); // br
	buffer->SafeAppend({{MIN_X_COOR - 0.05f + 10.0f * globalRendering->pixelX, 0.02f - 10.0f * globalRendering->pixelY, 0.0f}, {bgColor}}); // tr
	buffer->SafeAppend({{             0.01f - 10.0f * globalRendering->pixelX, 0.02f - 10.0f * globalRendering->pixelY, 0.0f}, {bgColor}}); // tl

	// print performance-related information (timings, particle-counts, etc)
	font->SetTextColor(1.0f, 1.0f, 0.5f, 0.8f);

	const char* fpsFmtStr = "[1] {Draw,Sim}FrameRate={%0.1f, %0.1f}Hz";
	const char* ctrFmtStr = "[2] {Draw,Sim}FrameTick={%u, %d}";
	const char* avgFmtStr = "[3] {Sim,Update,Draw}FrameTime={%s%2.1f, %s%2.1f, %s%2.1f (GL=%2.1f)}ms";
	const char* spdFmtStr = "[4] {Current,Wanted}SimSpeedMul={%2.2f, %2.2f}x";
	const char* sfxFmtStr = "[5] {Synced,Unsynced}Projectiles={%u,%u} Particles=%u Saturation=%.1f";
	const char* pfsFmtStr = "[6] (%s)PFS-updates queued: {%i, %i}";
	const char* luaFmtStr = "[7] Lua-allocated memory: %.1fMB (%.1fK allocs : %.5u usecs : %.1u states)";
	const char* gpuFmtStr = "[8] GPU-allocated memory: %.1fMB / %.1fMB";
	const char* sopFmtStr = "[9] SOP-allocated memory: {U,F,P,W}={%.1f/%.1f, %.1f/%.1f, %.1f/%.1f, %.1f/%.1f}KB";

	const CProjectileHandler* ph = &projectileHandler;
	const IPathManager* pm = pathManager;

	font->glFormat(0.01f, 0.02f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, fpsFmtStr, globalRendering->FPS, gu->simFPS);
	font->glFormat(0.01f, 0.04f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, ctrFmtStr, globalRendering->drawFrame, gs->frameNum);

	// 16ms := 60fps := 30simFPS + 30drawFPS
	font->glFormat(0.01f, 0.06f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, avgFmtStr,
		(gu->avgSimFrameTime  > 16) ? "\xff\xff\x01\x01" : "", gu->avgSimFrameTime,
		(gu->avgFrameTime     > 30) ? "\xff\xff\x01\x01" : "", gu->avgFrameTime,
		(gu->avgDrawFrameTime > 16) ? "\xff\xff\x01\x01" : "", gu->avgDrawFrameTime,
		(globalRendering->CalcGLDeltaTime(CGlobalRendering::FRAME_REF_TIME_QUERY_IDX, CGlobalRendering::FRAME_END_TIME_QUERY_IDX) * 0.001f) * 0.001f
	);

	font->glFormat(0.01f, 0.08f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, spdFmtStr, gs->speedFactor, gs->wantedSpeedFactor);
	font->glFormat(0.01f, 0.10f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, sfxFmtStr, ph->projectileContainers[true].size(), ph->projectileContainers[false].size(), ph->GetCurrentParticles(), ph->GetParticleSaturation(true));

	{
		const int2 pfsUpdates = pm->GetNumQueuedUpdates();

		switch (pm->GetPathFinderType()) {
			case NOPFS_TYPE: {
				font->glFormat(0.01f, 0.12f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, pfsFmtStr, "NO", pfsUpdates.x, pfsUpdates.y);
			} break;
			case HAPFS_TYPE: {
				font->glFormat(0.01f, 0.12f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, pfsFmtStr, "HA", pfsUpdates.x, pfsUpdates.y);
			} break;
			case QTPFS_TYPE: {
				font->glFormat(0.01f, 0.12f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, pfsFmtStr, "QT", pfsUpdates.x, pfsUpdates.y);
			} break;
			default: {
			} break;
		}
	}

	{
		SLuaAllocState state = {{0}, {0}, {0}, {0}};
		spring_lua_alloc_get_stats(&state);

		const    float allocMegs = state.allocedBytes.load() / 1024.0f / 1024.0f;
		const    float kiloAlloc = state.numLuaAllocs.load() / 1000.0f;
		const uint32_t allocTime = state.luaAllocTime.load();
		const uint32_t numStates = state.numLuaStates.load();

		font->glFormat(0.01f, 0.14f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, luaFmtStr, allocMegs, kiloAlloc, allocTime, numStates);
	}

	{
		int2 vidMemInfo;

		GetAvailableVideoRAM(&vidMemInfo.x, globalRenderingInfo.glVendor);

		font->glFormat(0.01f, 0.16f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, gpuFmtStr, (vidMemInfo.x - vidMemInfo.y) / 1024.0f, vidMemInfo.x / 1024.0f);
	}

	font->glFormat(0.01f, 0.18f, 0.5f, DBG_FONT_FLAGS | FONT_BUFFERED, sopFmtStr,
		unitMemPool.alloc_size() / 1024.0f,
		unitMemPool.freed_size() / 1024.0f,
		featureMemPool.alloc_size() / 1024.0f,
		featureMemPool.freed_size() / 1024.0f,
		projMemPool.alloc_size() / 1024.0f,
		projMemPool.freed_size() / 1024.0f,
		weaponMemPool.alloc_size() / 1024.0f,
		weaponMemPool.freed_size() / 1024.0f
	);
}



void ProfileDrawer::DrawScreen()
{
	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

	font->SetTextColor(1.0f, 1.0f, 0.5f, 0.8f);

	DrawThreadBarcode(buffer);
	DrawFrameBarcode(buffer);

	// text before profiler to batch the background
	DrawInfoText(buffer);
	DrawProfiler(buffer);
	DrawBufferStats({0.01f, 0.605f});

	shader->Disable();
	font->DrawBufferedGL4();
}

bool ProfileDrawer::MousePress(int x, int y, int button)
{
	if (!IsAbove(x, y))
		return false;

	const float my = CInputReceiver::MouseY(y);
	const int selIndex = int((MIN_Y_COOR - my) / LINE_HEIGHT);

	if (selIndex < 0)
		return false;
	if (selIndex >= profiler.GetNumSortedProfiles())
		return false;

	auto& sortedProfiles = profiler.GetSortedProfiles();
	auto& timeRecord = sortedProfiles[selIndex].second;

	// switch the selected Timers showGraph value
	// this reverts when the profile is re-sorted
	timeRecord.showGraph = !timeRecord.showGraph;
	return true;
}

bool ProfileDrawer::IsAbove(int x, int y)
{
	const float mx = CInputReceiver::MouseX(x);
	const float my = CInputReceiver::MouseY(y);

	// check if a Timer selection box was hit
	return (mx >= MIN_X_COOR && mx <= MAX_X_COOR && my >= (MIN_Y_COOR - profiler.GetNumSortedProfiles() * LINE_HEIGHT) && my <= MIN_Y_COOR);
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
		default: {
		} break;
	}
}

