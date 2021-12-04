/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DUMMY_VIDEO_CAPTURING_H
#define _DUMMY_VIDEO_CAPTURING_H

#include "IVideoCapturing.h"

class DummyVideoCapturing : public IVideoCapturing {

	friend class IVideoCapturing;

	DummyVideoCapturing();
	virtual ~DummyVideoCapturing();

public:
	virtual bool IsCapturingSupported() const;

	virtual bool IsCapturing() const;
	virtual void StartCapturing();
	virtual void StopCapturing();

	virtual void RenderFrame();
};

#endif // _DUMMY_VIDEO_CAPTURING_H
