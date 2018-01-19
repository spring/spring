/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOUSECURSOR_H
#define MOUSECURSOR_H

#include <string>
#include <vector>

#include "System/float4.h"

class CBitmap;
class IHwCursor;

class CMouseCursor {
public:
	enum HotSpot {TopLeft, Center};

public:
	static CMouseCursor New(const std::string& name, HotSpot hs) { return (CMouseCursor(name, hs)); }

	CMouseCursor(); // null-cursor
	CMouseCursor(const CMouseCursor& mc) = delete;
	CMouseCursor(CMouseCursor&& mc) { *this = std::move(mc); }
	~CMouseCursor();

	CMouseCursor& operator = (const CMouseCursor& mc) = delete;
	CMouseCursor& operator = (CMouseCursor&& mc) {
		images = std::move(mc.images);
		frames = std::move(mc.frames);

		hwCursor = mc.hwCursor;
		mc.hwCursor = nullptr;

		hotSpot = mc.hotSpot;

		animTime = mc.animTime;
		animPeriod = mc.animPeriod;
		currentFrame = mc.currentFrame;

		xmaxsize = mc.xmaxsize;
		ymaxsize = mc.ymaxsize;

		xofs = mc.xofs;
		yofs = mc.yofs;

		hwValid = mc.hwValid;
		return *this;
	}

	void Update();
	void Draw(int x, int y, float scale) const;   // software cursor draw

	void BindTexture() const;                     // software mouse cursor
	void BindHwCursor() const;                    // hardware mouse cursor

	float4 CalcFrameMatrixParams(const float3& winCoors, const float2& winScale) const;

	int GetMaxSizeX() const { return xmaxsize; }
	int GetMaxSizeY() const { return ymaxsize; }

	bool IsHWValid() const { return hwValid; }
	bool IsValid() const { return (!frames.empty()); }

protected:
	struct ImageData {
		unsigned int texture;
		int xOrigSize;
		int yOrigSize;
		int xAlignedSize;
		int yAlignedSize;
	};
	struct FrameData {
		FrameData(const ImageData& _image, float time)
			: image(_image), length(time), startTime(0.0f), endTime(0.0f) {}
		ImageData image;
		float length;
		float startTime;
		float endTime;
	};

protected:
	CMouseCursor(const std::string& name, HotSpot hs);
	bool LoadCursorImage(const std::string& name, struct ImageData& image);
	bool BuildFromSpecFile(const std::string& name);
	bool BuildFromFileNames(const std::string& name, int lastFrame);

protected:
	std::vector<ImageData> images;
	std::vector<FrameData> frames;

	IHwCursor* hwCursor; // hardware cursor

	HotSpot hotSpot;

	float animTime;
	float animPeriod;
	int currentFrame;

	int xmaxsize;
	int ymaxsize;

	int xofs; // describes where the center of the cursor is,
	int yofs; // based on xmaxsize, ymaxsize, and the hotspot

	bool hwValid; // if hardware cursor is valid
};


static const float minFrameLength = 0.010f;  // seconds
static const float defFrameLength = 0.100f;  // seconds


#endif /* MOUSECURSOR_H */
