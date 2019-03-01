/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Win/win32.h"
#include "Rendering/GlobalRendering.h"

#if defined(__APPLE__) || defined(HEADLESS)
	// FIXME: no hardware cursor support for macs
#elif defined(WIN32)
	#include <windows.h>
	#include "System/Input/MouseInput.h"
	typedef unsigned char byte;
#else
	#include <X11/Xcursor/Xcursor.h>
#endif

#include "HwMouseCursor.h"

#if !defined(__APPLE__) && !defined(HEADLESS)

#include "System/bitops.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include <SDL_config.h>
#include <SDL_syswm.h>
#include <SDL_mouse.h>
#include <SDL_events.h>
#endif

#include <cstring> // memset




//////////////////////////////////////////////////////////////////////
// Platform dependent classes
//////////////////////////////////////////////////////////////////////

#if defined(__APPLE__) || defined(HEADLESS)
class HardwareCursor: public IHardwareCursor {
public:
	void PushImage(int xsize, int ysize, const void* mem) override {}
	void PushFrame(int index, float delay) override {}
	void SetDelay(float delay) override {}
	void SetHotSpot(CMouseCursor::HotSpot hs) override {}
	void Finish() override {}

	bool NeedsYFlip() const override { return false; }
	bool IsValid() const override { return false; }

	void Init(CMouseCursor::HotSpot hs) override {}
	void Kill() override {}
	void Bind() override {}
};


#elif defined(WIN32)


class HardwareCursor: public IHardwareCursor {
public:
	void PushImage(int xsize, int ysize, const void* mem) override;
	void PushFrame(int index, float delay) override;
	void SetDelay(float delay) override;
	void SetHotSpot(CMouseCursor::HotSpot hs) override { hotSpot = hs; }
	void Finish() override;

	bool NeedsYFlip() const override { return true; }
	bool IsValid() const override { return (cursor != nullptr); }

	void Init(CMouseCursor::HotSpot hs) override;
	void Kill() override;
	void Bind() override;

private:
	#pragma pack(push, 1)
	struct CursorDirectoryHeader {
		byte xsize;
		byte ysize;
		byte ncolors;
		byte reserved1;
		short hotx;
		short hoty;
		DWORD size;
		DWORD offset;
	};

	struct CursorInfoHeader {
		DWORD size;
		DWORD width;
		DWORD height;
		WORD  planes;
		WORD  bpp;
		DWORD res1;
		DWORD res2;
		DWORD res3;
		DWORD res4;
		DWORD res5;
		DWORD res6;
	};

	struct AnihStructure {
		DWORD size;
		DWORD images;
		DWORD frames;
		DWORD width;
		DWORD height;
		DWORD bpp;
		DWORD planes;
		DWORD rate;
		DWORD flags;
	};
	#pragma pack(pop)

	struct ImageData {
		unsigned char* data;
		int width;
		int height;
	};

	void buildIco(unsigned char* dst, ImageData& image);
	void resizeImage(ImageData* image, int new_x, int new_y);

private:
	HCURSOR cursor = nullptr;
	CMouseCursor::HotSpot hotSpot = CMouseCursor::Center;

	int xmaxsize = 0;
	int ymaxsize = 0;
	short hotx = 0;
	short hoty = 0;

	byte imageCount = 0;

	std::vector<ImageData> icons;
	std::vector<byte>  frames;
	std::vector<int>   framerates;
};


#else


// X11
class HardwareCursor: public IHardwareCursor {
public:
	void PushImage(int xsize, int ysize, const void* mem) override;
	void PushFrame(int index, float delay) override;
	void SetDelay(float delay) override;
	void SetHotSpot(CMouseCursor::HotSpot hs) override { hotSpot = hs; }
	void Finish() override;

	bool NeedsYFlip() const override { return false; }
	bool IsValid() const override { return (cursor != 0); }

	void Init(CMouseCursor::HotSpot hs) override;
	void Kill() override;
	void Bind() override;

private:
	void resizeImage(XcursorImage*& image, const int new_x, const int new_y);

private:
	Cursor cursor = 0;
	CMouseCursor::HotSpot hotSpot = CMouseCursor::Center;

	int xmaxsize = 0;
	int ymaxsize = 0;

	std::vector<XcursorImage*> cimages;
};
#endif




IHardwareCursor* IHardwareCursor::Alloc(void* mem) {
	static_assert(sizeof(HardwareCursor) <= CMouseCursor::HWC_MEM_SIZE, "");
	return (new (mem) HardwareCursor());
}

void IHardwareCursor::Free(IHardwareCursor* hwc) {
	hwc->~IHardwareCursor();
	memset(reinterpret_cast<char*>(hwc), 0, sizeof(HardwareCursor));
}




//////////////////////////////////////////////////////////////////////
// Implementations
//////////////////////////////////////////////////////////////////////


#if defined(__APPLE__) || defined(HEADLESS)


#elif defined(WIN32)


void HardwareCursor::Init(CMouseCursor::HotSpot hs)
{
	cursor = nullptr;
	hotSpot = hs;

	imageCount = 0;

	xmaxsize = 0;
	ymaxsize = 0;

	hotx = 0;
	hoty = 0;
}

void HardwareCursor::Kill()
{
	if (cursor != nullptr)
		DestroyCursor(cursor);

	for (ImageData& image: icons)
		delete[] image.data;

	icons.clear();
}


void HardwareCursor::PushImage(int xsize, int ysize, const void* mem)
{
	xmaxsize = std::max(xmaxsize, xsize);
	ymaxsize = std::max(ymaxsize, ysize);

	icons.emplace_back();
	ImageData& icon = icons.back();

	icon.data = new unsigned char[xsize * ysize * 4];
	icon.width = xsize;
	icon.height = ysize;
	memcpy(icon.data, mem, xsize * ysize * 4);

	frames.push_back(imageCount++);
	framerates.push_back(std::max(int(CMouseCursor::DEF_FRAME_LENGTH * 60.0f), 1));
}

void HardwareCursor::SetDelay(float delay)
{
	framerates.back() = std::max(int(delay * 60.0f), 1);
}

void HardwareCursor::PushFrame(int index, float delay)
{
	if (index >= imageCount)
		return;

	frames.push_back((byte)index);
	framerates.push_back(std::max((int)(delay * 60.0f), 1));
}

void HardwareCursor::resizeImage(ImageData* image, int new_x, int new_y)
{
	if (image->width == new_x && image->height == new_y)
		return;

	unsigned char* newdata = new unsigned char[new_x * new_y * 4];
	memset(newdata, 0, new_x * new_y * 4);

	for (int y = 0; y < image->height; ++y)
		for (int x = 0; x < image->width; ++x)
			for (int v = 0; v < 4; ++v)
				newdata[((y + (new_y - image->height)) * new_x + x) * 4 + v] = image->data[(y * image->width + x) * 4 + v];

	delete[] image->data;
	image->data = newdata;

	image->width  = new_x;
	image->height = new_y;
}

void HardwareCursor::buildIco(unsigned char* dst, ImageData& image)
{
	const int xsize = image.width;
	const int ysize = image.height;

	{
		//small header needed in .ani
		strcpy((char*) dst, "icon");
		dst += 4;
		DWORD* cursize = (DWORD*) &dst[0];
		cursize[0] = 3 * sizeof(WORD) + sizeof(CursorDirectoryHeader) + sizeof(CursorInfoHeader) + (xsize * ysize * 4) + (xsize * ysize / 8);
		dst += 4;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// the following code writes a fully working .cur file in the memory (in a .ani container)

	{
		// file header
		int i = 0;
		WORD* header = (WORD*) &dst[0];
		header[i++] = 0;		// reserved
		header[i++] = 2;		// is cursor
		header[i++] = 1;		// number of cursors in this file
		dst += (3 * sizeof(WORD));
	}
	{
		CursorDirectoryHeader curHeader;
		memset(&curHeader, 0, sizeof(CursorDirectoryHeader));
		curHeader.xsize  = (byte) xsize;
		curHeader.ysize  = (byte) ysize;
		curHeader.hotx   = hotx;
		curHeader.hoty   = hoty;
		curHeader.size   = 40 + xsize * ysize * 4 + xsize * ysize / 8; // size of the bmp data (infoheader 40byte, 32bit color, 1bit mask)
		curHeader.offset = 22;                                         // offset where the infoHeader starts
		memcpy(dst, &curHeader, sizeof(CursorDirectoryHeader));
		dst += sizeof(CursorDirectoryHeader);
	}
	{
		CursorInfoHeader infoHeader;
		memset(&infoHeader, 0, sizeof(CursorInfoHeader));
		infoHeader.size   = 40;		//size of the infoHeader
		infoHeader.width  = xsize;		//cursor width
		infoHeader.height = ysize*2;		//cursor height + mask height
		infoHeader.planes = 1;		//number of color planes
		infoHeader.bpp    = 32;		//bits per pixel
		memcpy(dst, &infoHeader, sizeof(CursorInfoHeader));
		dst += sizeof(CursorInfoHeader);
	}

	//copy colormap
	unsigned char* src = image.data;
	unsigned char* end = src + xsize * ysize * 4;

	do {
		if (src[3] == 0) {
			dst[0] = dst[1] = dst[2] = dst[3] = 0;
		} else {
			dst[0] = src[2];      // B
			dst[1] = src[1];      // G
			dst[2] = src[0];      // R
			dst[3] = src[3];      // A
		}
		dst += 4;
		src += 4;
	} while (src < end);

	// create mask
	src -= xsize*ysize*4;
	int b = 0;
	do {
		dst[0] = 0x00;
		for (b = 0; b < 8; b++) {
			if (src[3] == 0) //alpha greater zero?
				dst[0] |= (128 >> b);
			src += 4;
		}
		dst++;
	} while (src < end);

	#if 0
	char fname[256];
	SNPRINTF(fname, sizeof(fname), "mycursor%d.cur", ++savedcount);
	FILE* pFile = fopen(fname, "wb" );
	fwrite(curs.back(), 1, curmem - curs.back(), pFile);
	fclose(pFile);
	#endif
}


static inline int GetBestCursorSize(const int minSize)
{
	constexpr int stdSizes[] = {/*16,*/ 32, 48, 64, 96, 128};
	for (const int s: stdSizes)
		if (s >= minSize)
			return s;

	return next_power_of_2(minSize);
}


void HardwareCursor::Finish()
{
	if (frames.empty())
		return;

	hotx = (hotSpot == CMouseCursor::TopLeft) ? 0 : (short)xmaxsize / 2;
	hoty = (hotSpot == CMouseCursor::TopLeft) ? 0 : (short)ymaxsize / 2;

	const int squaresize = GetBestCursorSize(std::max(xmaxsize, ymaxsize));

	for (ImageData& id: icons)
		resizeImage(&id, squaresize, squaresize);

	const int riffsize  = 32 + sizeof(AnihStructure) + (frames.size() + 2) * 2 * sizeof(DWORD);
	const int iconssize = icons.size() * (2 * sizeof(DWORD) + 3 * sizeof(WORD) +
					       sizeof(CursorDirectoryHeader) +
					       sizeof(CursorInfoHeader) +
					       squaresize * squaresize * 4 +
					       squaresize * squaresize / 8);
	const int totalsize = riffsize + iconssize;


	static std::vector<unsigned char> mem;

	mem.clear();
	mem.resize(totalsize, 0);


	unsigned char* curmem = mem.data();
	DWORD* dwmem = nullptr;

	{
		//write RIFF header
		strcpy((char*) curmem, "RIFF");   curmem += 4;
		dwmem = reinterpret_cast<DWORD*>(&curmem[0]);
		dwmem[0] = totalsize - 8;         curmem += 4; // filesize
		strcpy((char*) curmem, "ACON");   curmem += 4;
	}
	{
		//Anih header
		strcpy((char*) curmem, "anih");
		curmem += 4;
		curmem[0] = 36;
		curmem[1] = curmem[2] = curmem[3] = 0;
		curmem += 4;

		AnihStructure anih;
		memset(&anih, 0, sizeof(AnihStructure));
		anih.size   = 36;                   // anih structure size
		anih.images = imageCount;           // number of images
		anih.frames = framerates.size();    // number of frames
		anih.flags  = 0x3L;                 // using seq structure and .cur format for saving bmp data
		memcpy(curmem, &anih, sizeof(AnihStructure));
		curmem += sizeof(AnihStructure);
	}
	{
		//LIST + icons
		strcpy((char*) curmem, "LIST");   curmem += 4;
		dwmem = reinterpret_cast<DWORD*>(&curmem[0]);
		dwmem[0] = iconssize + 4;         curmem += 4;
		strcpy((char*) curmem, "fram");   curmem += 4;

		for (ImageData& id: icons) {
			buildIco(curmem, id);
			curmem += (2 * sizeof(DWORD) + 3 * sizeof(WORD));
			curmem += (sizeof(CursorDirectoryHeader) + sizeof(CursorInfoHeader));
			curmem += (squaresize * squaresize * 4 + squaresize * squaresize / 8);
		}
	}
	{
		//SEQ header
		strcpy((char*) curmem, "seq ");
		curmem += 4;
		DWORD* seq = (DWORD*) &curmem[0];
		seq[0] = frames.size() * sizeof(DWORD);
		seq++;
		for (size_t i = 0; i < frames.size(); i++)
			seq[i] = frames.at(i);
		curmem += (frames.size() + 1) * sizeof(DWORD);
	}
	{
		//RATE header
		strcpy((char*) curmem, "rate");
		curmem += 4;
		DWORD* rate = (DWORD*) &curmem[0];
		rate[0] = framerates.size() * sizeof(DWORD);
		rate++;
		for (size_t i = 0; i < framerates.size(); i++)
			rate[i] = framerates.at(i);
		curmem += (framerates.size() + 1) * sizeof(DWORD);
	}

	#if 0
	char fname[256];
	SNPRINTF(fname, sizeof(fname), "cursors/mycursor%d.ani", ++savedcount);
	FILE* pFile = fopen( fname , "wb");
	fwrite(mem, 1, curmem - mem, pFile);
	fclose(pFile);
	#endif

	cursor = (HCURSOR)CreateIconFromResourceEx((PBYTE) mem.data(), totalsize, FALSE, 0x00030000, squaresize, squaresize, 0);

	for (ImageData& id: icons)
		delete[] id.data;

	icons.clear();
}

void HardwareCursor::Bind()
{
	#if 0
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (!SDL_GetWMInfo(&info)) {
		LOG_L(L_ERROR, "SDL error: can't get window handle");
		return;
	}

	// SDL doesn't let us use it
	SetClassLong(info.window, GCL_HCURSOR, (LONG) cursor);
	#endif

	SDL_ShowCursor(SDL_ENABLE);
	SetCursor(cursor);
	mouseInput->SetWMMouseCursor(cursor);
}


#else


void HardwareCursor::Init(CMouseCursor::HotSpot hs)
{
	cursor   = 0;
	hotSpot  = hs;
	xmaxsize = 0;
	ymaxsize = 0;
}

void HardwareCursor::Kill()
{
	for (XcursorImage* img: cimages)
		XcursorImageDestroy(img);

	cimages.clear();

	if (cursor != 0) {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);

		if (!SDL_GetWindowWMInfo(globalRendering->GetWindow(0), &info)) {
			LOG_L(L_ERROR, "SDL error: can't get X11 window info");
			return;
		}

		XFreeCursor(info.info.x11.display, cursor);
	}
}


void HardwareCursor::resizeImage(XcursorImage*& image, const int new_x, const int new_y)
{
	if (int(image->width) == new_x && int(image->height) == new_y)
		return;

	const int old_x = image->width;
	const int old_y = image->height;

	XcursorImage* new_image = XcursorImageCreate(new_x, new_y);
	new_image->delay = image->delay;

	const unsigned char* src = (unsigned char*)image->pixels;
	      unsigned char* dst = (unsigned char*)new_image->pixels;

	memset(dst, 0, new_x * new_y * 4);

	for (int y = 0; y < old_y; ++y)
		for (int x = 0; x < old_x; ++x)
			for (int v = 0; v < 4; ++v)
				dst[(y * new_x + x) * 4 + v] = src[(y * old_x + x) * 4 + v];

	XcursorImageDestroy(image);
	image = new_image;
}

void HardwareCursor::PushImage(int xsize, int ysize, const void* mem)
{
	xmaxsize = std::max(xmaxsize, xsize);
	ymaxsize = std::max(ymaxsize, ysize);

	XcursorImage* image = XcursorImageCreate(xsize, ysize);
	image->delay = (XcursorUInt) (CMouseCursor::DEF_FRAME_LENGTH * 1000.0f);

	char* dst = (char*)image->pixels;
	char* src = (char*)mem;
	char* end = src + xsize * ysize * 4;

	do {
		dst[0] = src[2];      // B
		dst[1] = src[1];      // G
		dst[2] = src[0];      // R
		dst[3] = src[3];      // A
		dst += 4;
		src += 4;
	} while (src < end);

	cimages.push_back(image);
}

void HardwareCursor::SetDelay(float delay)
{
	cimages.back()->delay = (XcursorUInt)(delay*1000.0f); //in millseconds
}

void HardwareCursor::PushFrame(int index, float delay)
{
	if (index >= int(cimages.size()))
		return;

	if (cimages[index]->delay != delay) {
		// make a copy of the existing one
		const XcursorImage* ci = cimages[index];

		PushImage(ci->width, ci->height, ci->pixels);
		SetDelay(delay);
	} else {
		cimages.push_back( cimages[index] );
	}
}

void HardwareCursor::Finish()
{
	if (cimages.empty())
		return;

	const int squaresize = next_power_of_2(std::max(xmaxsize, ymaxsize));

	// resize images
	for (XcursorImage*& ci: cimages)
		resizeImage(ci, squaresize, squaresize);

	XcursorImages* cis = XcursorImagesCreate(cimages.size());
	cis->nimage = cimages.size();

	for (size_t i = 0; i < cimages.size(); ++i) {
		XcursorImage* ci = cimages[i];
		ci->xhot = (hotSpot == CMouseCursor::TopLeft) ? 0 : xmaxsize / 2;
		ci->yhot = (hotSpot == CMouseCursor::TopLeft) ? 0 : ymaxsize / 2;
		cis->images[i] = ci;
	}

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(globalRendering->GetWindow(0), &info)) {
		LOG_L(L_ERROR, "SDL error: can't get X11 window info");
		XcursorImagesDestroy(cis);
		cimages.clear();
		return;
	}

	cursor = XcursorImagesLoadCursor(info.info.x11.display, cis);
	XcursorImagesDestroy(cis);
	cimages.clear();
}

void HardwareCursor::Bind()
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (!SDL_GetWindowWMInfo(globalRendering->GetWindow(0), &info)) {
		LOG_L(L_ERROR, "SDL error: can't get X11 window info");
		return;
	}

	// do between lock/unlock so SDL's default cursor doesn't flicker in
	SDL_ShowCursor(SDL_ENABLE);
	XDefineCursor(info.info.x11.display, info.info.x11.window, cursor);
}


#endif

