/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DUMMY_VIDEO_CAPTURING_H
#define _DUMMY_VIDEO_CAPTURING_H

#include "IVideoCapturing.h"

class DummyVideoCapturing : public IVideoCapturing {
	friend class IVideoCapturing;

public:
	bool IsCapturingSupported() const override { return false; }

	void StartCapturing() override;
	void StopCapturing() override {}

	void RenderFrame() override {}
};

#endif // _DUMMY_VIDEO_CAPTURING_H
