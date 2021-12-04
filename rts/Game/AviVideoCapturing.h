/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AVI_VIDEO_CAPTURING_H
#define _AVI_VIDEO_CAPTURING_H

#if       defined AVI_CAPTURING

#include "IVideoCapturing.h"

class CAVIGenerator;


class AviVideoCapturing : public IVideoCapturing {

	friend class IVideoCapturing;

	AviVideoCapturing();
	virtual ~AviVideoCapturing();

public:
	virtual bool IsCapturingSupported() const;

	virtual bool IsCapturing() const;
	virtual void StartCapturing();
	virtual void StopCapturing();

	virtual void RenderFrame();

private:

	bool capturing;
	CAVIGenerator* aviGenerator;
};

#endif // defined AVI_CAPTURING

#endif // _AVI_VIDEO_CAPTURING_H
