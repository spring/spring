/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _VIDEO_CAPTURING_H
#define _VIDEO_CAPTURING_H

#include "System/Misc/NonCopyable.h"

class IVideoCapturing : public spring::noncopyable {

protected:
	IVideoCapturing();
	virtual ~IVideoCapturing();

public:
	static IVideoCapturing* GetInstance();
	static void FreeInstance();

	virtual void RenderFrame() = 0;

	static void SetCapturing(bool enabled);

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
	virtual bool IsCapturing() const { return false; }

protected:
	virtual void StartCapturing() {}
	virtual void StopCapturing() {}

};

#define videoCapturing IVideoCapturing::GetInstance()

#endif // _VIDEO_CAPTURING_H
