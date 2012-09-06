/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOUSECURSOR_H
#define MOUSECURSOR_H

#include <string>
#include <vector>

class CBitmap;
class IHwCursor;

class CMouseCursor {
	public:
		enum HotSpot {TopLeft, Center};

	public:
		static CMouseCursor* New(const std::string &name, HotSpot hs);
		static CMouseCursor* GetNullCursor();
		~CMouseCursor();

		void Update();
		void Draw(int x, int y, float scale) const;   // software cursor draw
		void DrawQuad(int x, int y) const;            // draw command queue icon
		void BindTexture() const;                     // software mouse cursor
		void BindHwCursor() const;                    // hardware mouse cursor

		int GetMaxSizeX() const { return xmaxsize; }
		int GetMaxSizeY() const { return ymaxsize; }

	public:
		bool animated;
		bool hwValid; //if hardware cursor is valid

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
		CMouseCursor(const std::string &name, HotSpot hs);
		bool LoadCursorImage(const std::string& name, struct ImageData& image);
		bool BuildFromSpecFile(const std::string& name);
		bool BuildFromFileNames(const std::string& name, int lastFrame);

	protected:
		HotSpot hotSpot;

		std::vector<ImageData> images;
		std::vector<FrameData> frames;

		IHwCursor* hwCursor; // hardware cursor

		float animTime;
		float animPeriod;
		int currentFrame;

		int xmaxsize;
		int ymaxsize;

		int xofs; // describes where the center of the cursor is,
		int yofs; // based on xmaxsize, ymaxsize, and the hotspot
};


static const float minFrameLength = 0.010f;  // seconds
static const float defFrameLength = 0.100f;  // seconds


#endif /* MOUSECURSOR_H */
