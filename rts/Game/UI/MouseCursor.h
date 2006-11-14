#ifndef MOUSECURSOR_H
#define MOUSECURSOR_H

#include <string>
#include <vector>

using namespace std;

class CBitmap;


class CMouseCursor {
	public:
		enum HotSpot {TopLeft, Center};

		static CMouseCursor* New(const string &name, HotSpot hs);

		~CMouseCursor(void);

		void Update();
		void Draw(int x, int y, float scale);
		void DrawQuad(int x, int y);
		void BindTexture();

		int GetMaxSizeX() const { return xmaxsize; }
		int GetMaxSizeY() const { return ymaxsize; }

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
			: image(_image), length(time) {}
			ImageData image;
			float length;
			float startTime;
			float endTime;
		};

	protected:	
		CMouseCursor(const string &name, HotSpot hs);
		bool LoadImage(const string& name, struct ImageData& image);
		bool BuildFromSpecFile(const string& name);
		bool BuildFromFileNames(const string& name, int lastFrame);
		CBitmap* getAlignedBitmap(const CBitmap &orig);
		void setBitmapTransparency(CBitmap &bm, int r, int g, int b);

	protected:	
		vector<ImageData> images;
		vector<FrameData> frames;

		float animTime;
		float animPeriod;
		int currentFrame;

		int xmaxsize;
		int ymaxsize;

		int xofs; // describes where the center of the cursor is,
		int yofs; // based on xmaxsize, ymaxsize, and the hotspot
};


#endif /* MOUSECURSOR_H */
