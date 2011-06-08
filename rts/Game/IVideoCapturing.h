/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _VIDEO_CAPTURING_H
#define _VIDEO_CAPTURING_H

#include <boost/noncopyable.hpp>

class IVideoCapturing : public boost::noncopyable {

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
	virtual bool IsCapturing() const = 0;

protected:
	virtual void StartCapturing() = 0;
	virtual void StopCapturing() = 0;

};

#define videoCapturing IVideoCapturing::GetInstance()

#endif // _VIDEO_CAPTURING_H
