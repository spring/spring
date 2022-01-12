//
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "System/Misc/SpringTime.h"

#define GAME_SPEED_HZ 30
#define FRAME_TIME_MS (1000.0f / GAME_SPEED_HZ)

static unsigned int lastSimFrame                = 0;
static unsigned int lastSimFrameRateUpdateFrame = 0;

static spring_time lastDrawFrameTime          = spring_notime;
static spring_time lastSimFrameTime           = spring_notime;
static spring_time lastSimFrameRateUpdateTime = spring_notime;

static float simFrameRate       = 0.0f; // globalUnsynced->simFPS (undefined during first 30 sim-frames)
static float simFrameTimeOffset = 0.0f; // globalRendering->timeOffset
static float simSpeedFactor     = 0.0f; // globalRendering->weightedSpeedFactor



static void NewSimFrame(unsigned int currSimFrame) {
	if (currSimFrame > GAME_SPEED_HZ) {
		printf("[%s] sf=%u fps=%f to=%f\n\n", __func__, currSimFrame, simFrameRate, simFrameTimeOffset);
	}

	lastSimFrame = currSimFrame;
	lastSimFrameTime = spring_gettime();
}



static void GameUpdateUnsynced(unsigned int currSimFrame, unsigned int currDrawFrame) {
	const spring_time currentTime = spring_gettime();
	const float dt = (currentTime - lastSimFrameRateUpdateTime).toSecsf();

	// update simFPS counter
	if (dt >= 1.0f) {
		simFrameRate = (currSimFrame - lastSimFrameRateUpdateFrame) / dt;

		lastSimFrameRateUpdateTime = currentTime;
		lastSimFrameRateUpdateFrame = currSimFrame;
	}

	simSpeedFactor = 0.001f * simFrameRate;
	simFrameTimeOffset = (currentTime - lastSimFrameTime).toMilliSecsf() * simSpeedFactor;
}

static void GameDraw(unsigned int currSimFrame, unsigned int currDrawFrame) {
	// do something useful that takes non-trivial time, eg. sleeping
	// at ~5 milliseconds per draw-frame, rFPS would be at most ~200
	// wake-up can vary and affects FPS too much, busy-loop instead
	{
		const spring_time currentTime = spring_gettime();
		const spring_time wakeUpTime = currentTime + spring_time(1 + (random() % 5)); // MILLIseconds

		while (spring_gettime() < wakeUpTime) {
		}
	}

	const float currTimeOffset = simFrameTimeOffset;
	static float lastTimeOffset = simFrameTimeOffset;

	// do nothing first 30 frames
	if (currSimFrame < GAME_SPEED_HZ)
		return;

	if (currTimeOffset < 0.0f)
		printf("[%s] assert(timeOffset >= 0.0f) failed (SF=%u : DF=%u : TO=%f)\n", __func__, currSimFrame, currDrawFrame, currTimeOffset);
	if (currTimeOffset > 1.0f)
		printf("[%s] assert(timeOffset <= 1.0f) failed (SF=%u : DF=%u : TO=%f)\n", __func__, currSimFrame, currDrawFrame, currTimeOffset);

	// test for monotonicity
	if (currTimeOffset < lastTimeOffset)
		printf("[%s] assert(timeOffset < lastTimeOffset) failed (SF=%u : DF=%u : CTO=%f LTO=%f)\n", __func__, currSimFrame, currDrawFrame, currTimeOffset, lastTimeOffset);

	lastTimeOffset = currTimeOffset;
}

static void NewDrawFrame(unsigned int currSimFrame, unsigned int currDrawFrame) {
	lastDrawFrameTime = spring_gettime();

	GameUpdateUnsynced(currSimFrame, currDrawFrame);
	GameDraw(currSimFrame, currDrawFrame);
}

static void TimerFunc(unsigned int N) {
	printf("[%s][N=%u]\n\n", __func__, N);

	lastDrawFrameTime = spring_gettime();
	lastSimFrameTime = spring_gettime();
	lastSimFrameRateUpdateTime = spring_gettime();

	for (unsigned int sf = 0, df = 0; sf < N; /*no-op*/) {
		NewDrawFrame(sf, df++);

		if ((spring_gettime() - lastSimFrameTime).toMilliSecsf() > FRAME_TIME_MS) {
			NewSimFrame(sf++);
		}
	}
}

int main(int argc, char** argv) {
	srandom(time(nullptr));

	std::thread timer = std::thread(std::bind(&TimerFunc, (argc >= 2)? atoi(argv[1]): (GAME_SPEED_HZ * 60 * 60)));
	timer.join();
	return 0;
}

