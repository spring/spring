/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOUSECURSOR_H
#define MOUSECURSOR_H

#include <string>
#include <vector>

#include "System/float4.h"

class CBitmap;
class IHardwareCursor;

class CMouseCursor {
public:
	enum HotSpot {TopLeft, Center};

public:
	CMouseCursor() = default; // null-cursor
	CMouseCursor(const std::string& name, HotSpot hs);
	CMouseCursor(const CMouseCursor& mc) = delete;
	CMouseCursor(CMouseCursor&& mc) { *this = std::move(mc); }
	~CMouseCursor();

	CMouseCursor& operator = (const CMouseCursor& mc) = delete;
	CMouseCursor& operator = (CMouseCursor&& mc) noexcept;

	void Update();
	void Draw(int x, int y, float scale) const;   // software cursor draw

	void BindTexture() const;                     // software mouse cursor
	void BindHwCursor() const;                    // hardware mouse cursor

	float4 CalcFrameMatrixParams(const float3& winCoors, const float2& winScale) const;

	int GetMaxSizeX() const { return xmaxsize; }
	int GetMaxSizeY() const { return ymaxsize; }

	bool IsHWValid() const { return hwValid; }
	bool IsValid() const { return (!frames.empty()); }

private:
	struct ImageData;

	bool LoadDummyImage();
	bool LoadCursorImage(const std::string& name, ImageData& image);
	bool Build(const std::string& name);
	bool BuildFromSpecFile(const std::string& name, int& lastFrame);
	bool BuildFromFileNames(const std::string& name, int lastFrame);

public:
	static constexpr float MIN_FRAME_LENGTH = 0.010f;  // seconds
	static constexpr float DEF_FRAME_LENGTH = 0.100f;  // seconds

	static constexpr size_t HWC_MEM_SIZE = 128;


private:
	struct ImageData {
		unsigned int texture = 0;
		int xOrigSize = 0;
		int yOrigSize = 0;
		int xAlignedSize = 0;
		int yAlignedSize = 0;
	};
	struct FrameData {
		FrameData(unsigned int idx, float time): imageIdx(idx), length(time) {}

		unsigned int imageIdx = 0;

		float length = 0.0f;
		float startTime = 0.0f;
		float endTime = 0.0f;
	};

	std::vector<ImageData> images;
	std::vector<FrameData> frames;

	unsigned char hwCursorMem[HWC_MEM_SIZE] = {0};

	IHardwareCursor* hwCursor = nullptr;

	HotSpot hotSpot = Center;

	float animTime = 0.0f;
	float animPeriod = 0.0f;
	int currentFrame = 0;

	int xmaxsize = 0;
	int ymaxsize = 0;

	int xofs = 0; // describes where the center of the cursor is,
	int yofs = 0; // based on xmaxsize, ymaxsize, and the hotspot

	bool hwValid = false; // if hardware cursor is valid
};


#endif /* MOUSECURSOR_H */
