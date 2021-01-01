/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _VIDEO_CAPTURING_H
#define _VIDEO_CAPTURING_H

#include "System/Misc/NonCopyable.h"

class IVideoCapturing : public spring::noncopyable {

protected:
	IVideoCapturing() {}
	virtual ~IVideoCapturing() { FreeInstance(); }

public:
	static IVideoCapturing* GetInstance();
	static void FreeInstance();
	static bool SetCapturing(bool enable);

	virtual void RenderFrame() = 0;

	void SetAllowRecord(bool enable) { allowRecord = enable; }

	void SetLastFrameTime(float time) { lastFrameTime = time; }
	void SetTimeOffset(float offset) { timeOffset = offset; }

	float GetLastFrameTime() const { return lastFrameTime; }
	float GetTimeOffset() const { return timeOffset; }

	/**
	 * Indicates whether it is possible/supported to capture a video.
	 *
	 * @return	true if it is possible/supported to capture a video
	 */
	virtual bool IsCapturingSupported() const = 0;

	/**
	 * Indicates whether a video is currently being captured.
	 *
	 * @return	true if a video is currently being captured
	 */
	bool IsCapturing() const { return capturing; }
	bool AllowRecord() const { return allowRecord; }

protected:
	virtual void StartCapturing() {}
	virtual void StopCapturing() {}

protected:
	// (can also be) set by Lua; regulates framerate and timing interpolation
	bool allowRecord = false;
	// internal state
	bool capturing = false;

	float lastFrameTime = 0.0f;
	float timeOffset = 0.0f;
};

#define videoCapturing IVideoCapturing::GetInstance()

#endif // _VIDEO_CAPTURING_H
