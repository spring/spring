/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AVI_VIDEO_CAPTURING_H
#define _AVI_VIDEO_CAPTURING_H

#if       defined AVI_CAPTURING

#include "IVideoCapturing.h"

class CAVIGenerator;


class AviVideoCapturing : public IVideoCapturing {
	friend class IVideoCapturing;

public:
	bool IsCapturingSupported() const override { return true; }

	void StartCapturing() override;
	void StopCapturing() override;

	void RenderFrame() override;

private:
	CAVIGenerator* aviGenerator = nullptr;
};

#endif // defined AVI_CAPTURING

#endif // _AVI_VIDEO_CAPTURING_H
