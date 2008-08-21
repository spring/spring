#include "StdAfx.h"
#include <SDL_syswm.h>
#include "Rendering/Textures/Bitmap.h"

#ifdef WIN32
#  include "windows.h"
#  include "MouseInput.h"
typedef unsigned char byte;
#elif defined(__APPLE__)
	/*do nothing (duno how to create cursors on runtime on macs)*/
#else
#  include <X11/Xcursor/Xcursor.h>
#endif

#include "mmgr.h"

#include "bitops.h"
#include "CommandColors.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/SimpleParser.h"
#include "LogOutput.h"
#include "MouseCursor.h"
#include "HwMouseCursor.h"
#include "myMath.h"
#include "Rendering/GL/myGL.h"



//int savedcount=0;

//////////////////////////////////////////////////////////////////////
// Platform dependent classes
//////////////////////////////////////////////////////////////////////

#ifdef __APPLE__
// no hardware cursor support for mac's
class CHwDummyCursor : public IHwCursor {
	public:
		void PushImage(int xsize, int ysize, void* mem){};
		void SetDelay(float delay){};
		void PushFrame(int index, float delay){};
		void Finish(){};

		bool needsYFlip() {return false;};

		bool IsValid(){return false;};
		void Bind(){};
};
#elif defined(WIN32)
class CHwWinCursor : public IHwCursor {
	public:
		CHwWinCursor(void);
		~CHwWinCursor(void);

		void PushImage(int xsize, int ysize, void* mem);
		void SetDelay(float delay);
		void PushFrame(int index, float delay);
		void Finish();

		bool needsYFlip() {return true;};

		void Bind();

		bool IsValid() {return (cursor!=NULL);};
	protected:
		HCURSOR cursor;

		struct CursorDirectoryHeader {
			byte  xsize,ysize,ncolors,reserved1;
			short hotx,hoty;
			DWORD size,offset;
		};

		struct CursorInfoHeader {
			DWORD size,width,height;
			WORD  planes, bpp;
			DWORD res1,res2,res3,res4,res5,res6;
		};

		struct AnihStructure {
			DWORD size,images,frames,width,height,bpp,planes,rate,flags;
		};

	protected:
		struct ImageData {
			unsigned char* data;
			int width,height;
		};

		void buildIco(unsigned char* dst, ImageData &image);
		void resizeImage(ImageData *image, int new_x, int new_y);

		int xmaxsize, ymaxsize;
		short hotx, hoty;

		byte image_count;

		std::vector<ImageData> icons;
		std::vector<byte>  frames;
		std::vector<int>   framerates;
};
#else
class CHwX11Cursor : public IHwCursor {
	public:
		CHwX11Cursor(void);
		~CHwX11Cursor(void);

		void PushImage(int xsize, int ysize, void* mem);
		void SetDelay(float delay);
		void PushFrame(int index, float delay);
		void Finish();

		bool needsYFlip() {return false;};

		bool IsValid() {return (cursor!=0);};
		void Bind();
	protected:
		int xmaxsize, ymaxsize;

		void resizeImage(XcursorImage*& image, const int new_x, const int new_y);

		Cursor cursor;
		std::vector<XcursorImage*> cimages;
};
#endif

//////////////////////////////////////////////////////////////////////
// GetHwCursor()
//////////////////////////////////////////////////////////////////////

IHwCursor* GetNewHwCursor()
{
#ifdef WIN32
	return SAFE_NEW CHwWinCursor();
#elif defined(__APPLE__)
	return SAFE_NEW CHwDummyCursor();
#else
	return SAFE_NEW CHwX11Cursor();
#endif
}


//////////////////////////////////////////////////////////////////////
// Implementation 
//////////////////////////////////////////////////////////////////////


#ifdef __APPLE__
	// no hardware cursor support for mac's
#elif defined(WIN32)

void CHwWinCursor::PushImage(int xsize, int ysize, void* mem)
{
	xmaxsize=std::max(xmaxsize,xsize);
	ymaxsize=std::max(ymaxsize,ysize);

	ImageData icon;
	icon.data = new unsigned char[xsize*ysize*4];
	icon.width = xsize;
	icon.height = ysize;
	memcpy(icon.data,mem,xsize*ysize*4);

	icons.push_back(icon);

	frames.push_back(image_count);
	framerates.push_back(std::max((int)(defFrameLength*60.0f),1));
	image_count++;
}

void CHwWinCursor::SetDelay(float delay)
{
	int &last_item = framerates.back();
	last_item = std::max((int)(delay*60.0f),1);
}

void CHwWinCursor::PushFrame(int index, float delay)
{
	if (index>=image_count)
		return;

	frames.push_back((byte)index);
	framerates.push_back(std::max((int)(delay*60.0f),1));
}

void CHwWinCursor::resizeImage(ImageData *image, int new_x, int new_y)
{
	if (image->width==new_x && image->height==new_y)
		return;

	unsigned char* newdata = new unsigned char[new_x*new_y*4];
	memset(newdata, 0, new_x*new_y*4);

	for (int y = 0; y < image->height; ++y)
		for (int x = 0; x < image->width; ++x)
			for (int v = 0; v < 4; ++v)
				newdata[((y + (new_y-image->height))*new_x+x)*4+v] = image->data[(y*image->width+x)*4+v];

	delete[] image->data;

	image->data = newdata;

	image->width  = new_x;
	image->height = new_y;
}

void CHwWinCursor::buildIco(unsigned char* dst, ImageData &image)
{
	const int xsize = image.width;
	const int ysize = image.height;

	//small header needed in .ani
	strcpy((char*)dst,"icon");
	dst += 4;
	DWORD* cursize = (DWORD*)&dst[0];
	cursize[0] = 3*sizeof(WORD)+sizeof(CursorDirectoryHeader)+sizeof(CursorInfoHeader)+xsize*ysize*4+xsize*ysize/8;
	dst += 4;

	////////////////////////////////////////////////////////////////////////////////////////////////
	// the following code writes a full working .cur file in the memory (in a .ani container)

	//file header
	WORD* header = (WORD*)&dst[0]; int i=0;
	header[i++] = 0;		//reserved
	header[i++] = 2;		//is cursor
	header[i++] = 1;		//number of cursor in this file
	dst += 3*sizeof(WORD);

	CursorDirectoryHeader curHeader;
	memset(&curHeader,0,sizeof(CursorDirectoryHeader));
	curHeader.xsize  = (byte)xsize;		//width
	curHeader.ysize  = (byte)ysize;		//height
	curHeader.hotx   = hotx;
	curHeader.hoty   = hoty;
	curHeader.size   = 40+xsize*ysize*4+xsize*ysize/8;	//size of the bmp data (infoheader 40byte, 32bit color, 1bit mask)
	curHeader.offset = 22;			//offset, where the infoHeader starts
	memcpy(dst, &curHeader, sizeof(CursorDirectoryHeader));
	dst += sizeof(CursorDirectoryHeader);

	CursorInfoHeader infoHeader;
	memset(&infoHeader,0,sizeof(CursorInfoHeader));
	infoHeader.size   = 40;		//size of the infoHeader
	infoHeader.width  = xsize;		//cursor width
	infoHeader.height = ysize*2;		//cursor height + mask height
	infoHeader.planes = 1;		//number of color planes
	infoHeader.bpp    = 32;		//bits per pixel
	memcpy(dst, &infoHeader, sizeof(CursorInfoHeader));
	dst += sizeof(CursorInfoHeader);

	//copy colormap
	unsigned char* src = image.data;
	unsigned char* end = src+xsize*ysize*4;
	do{
		if (src[3]==0) {
			dst[0] = dst[1] = dst[2] = dst[3] = 0;
		}else{
			dst[0]=src[2];      // B
			dst[1]=src[1];      // G
			dst[2]=src[0];      // R
			dst[3]=src[3];      // A
		}
		dst+=4;
		src+=4;
	}while(src<end);

	//create mask
	src -= xsize*ysize*4;
	int b = 0;
	do{
		dst[0] = 0x00;
		for (b=0; b<8; b++) {
			if (src[3]==0) //alpha greater zero?
				dst[0] |= 128>>b;
			src+=4;
		}
		dst++;
	}while(src<end);

/*	char fname[256];
	SNPRINTF(fname, sizeof(fname), "mycursor%d.cur", ++savedcount);
	FILE * pFile = fopen( fname , "wb" );
	fwrite(curs.back(), 1 , curmem-curs.back(), pFile );
	fclose(pFile); */
}

void CHwWinCursor::Finish()
{
	if (frames.size()<1)
		return;

	hotx = (hotSpot==CMouseCursor::TopLeft) ? 0 : (short)xmaxsize/2;
	hoty = (hotSpot==CMouseCursor::TopLeft) ? 0 : (short)ymaxsize/2;

	//note: windows only except 16x16,32x32,64x64,etc. (and some more not 2^n ones)
	int squaresize =  next_power_of_2( std::max(xmaxsize,ymaxsize) );

	//resize images
	for (std::vector<ImageData>::iterator it=icons.begin(); it<icons.end(); it++)
		resizeImage(&*it,squaresize,squaresize);

	const int riffsize  = 32 + sizeof(AnihStructure) + (frames.size()+2) * 2 * sizeof(DWORD);
	const int iconssize = icons.size() * (2*sizeof(DWORD) + 3*sizeof(WORD) +
					       sizeof(CursorDirectoryHeader) +
					       sizeof(CursorInfoHeader) +
					       squaresize*squaresize*4 +
					       squaresize*squaresize/8);
	const int totalsize = riffsize + iconssize;
	unsigned char* mem = SAFE_NEW unsigned char[ totalsize ];

	unsigned char* curmem = mem;
	DWORD* dwmem;

	//write RIFF header
		strcpy((char*)curmem,"RIFF");	curmem+=4;
		dwmem = (DWORD*)&curmem[0];
		dwmem[0] = totalsize-8;		curmem+=4; //filesize
		strcpy((char*)curmem,"ACON");	curmem+=4;

	//Anih header
		strcpy((char*)curmem,"anih");
		curmem += 4;
		curmem[0] = 36;
		curmem[1] = curmem[2] = curmem[3] = 0;
		curmem += 4;

		AnihStructure anih;
		memset(&anih,0,sizeof(AnihStructure));
		anih.size   = 36;		//anih structure size
		anih.images = image_count;		//number of images
		anih.frames = framerates.size();	//number of frames
		anih.flags  = 0x3L;		//using seq structure and .cur format for saving bmp data
		memcpy(curmem, &anih, sizeof(AnihStructure));
		curmem += sizeof(AnihStructure);

	//LIST + icons
		strcpy((char*)curmem,"LIST");	curmem+=4;
		dwmem = (DWORD*)&curmem[0];
		dwmem[0] = iconssize+4;		curmem+=4;
		strcpy((char*)curmem,"fram");	curmem+=4;

		for (std::vector<ImageData>::iterator it=icons.begin(); it<icons.end(); it++) {
			buildIco(curmem,*it);
			curmem += 2*sizeof(DWORD) + 3*sizeof(WORD)+sizeof(CursorDirectoryHeader)+sizeof(CursorInfoHeader)+squaresize*squaresize*4+squaresize*squaresize/8;
		}

	//SEQ header
		strcpy((char*)curmem,"seq ");
		curmem += 4;
		DWORD* seq = (DWORD*)&curmem[0];
		seq[0] = frames.size()*sizeof(DWORD);
		seq++;
		for (int i=0; i<frames.size(); i++)
			seq[i] = frames.at(i);
		curmem += (frames.size()+1)*sizeof(DWORD);

	//RATE header
		strcpy((char*)curmem,"rate");
		curmem += 4;
		DWORD* rate = (DWORD*)&curmem[0];
		rate[0] = framerates.size()*sizeof(DWORD);
		rate++;
		for (int i=0; i<framerates.size(); i++)
			rate[i] = framerates.at(i);
		curmem += (framerates.size()+1)*sizeof(DWORD);

	/*char fname[256];
	SNPRINTF(fname, sizeof(fname), "cursors/mycursor%d.ani", ++savedcount);
	FILE * pFile = fopen( fname , "wb" );
	fwrite(mem , 1 , curmem-mem, pFile );
	fclose(pFile);*/

	cursor = (HCURSOR)CreateIconFromResourceEx((PBYTE)mem,totalsize,FALSE,0x00030000,squaresize,squaresize,0);

	delete[] mem;
	for (std::vector<ImageData>::iterator it=icons.begin(); it<icons.end(); it++)
		delete[] (*it).data;
	icons.clear();

	//if (cursor==NULL) logOutput.Print("hw cursor failed: x%d y%d",squaresize,squaresize);
}

void CHwWinCursor::Bind()
{
	/*SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWMInfo(&info)) {
		logOutput.Print("SDL error: can't get window handle");
		return;
	}
	SetClassLong(info.window,GCL_HCURSOR,(LONG)cursor);*/ //SDL doesn't let us use it :<

	SetCursor(cursor);
	mouseInput->SetWMMouseCursor(cursor);
}

CHwWinCursor::CHwWinCursor(void)
{
	cursor = NULL;
	hotSpot= CMouseCursor::Center;
	image_count = 0;
	xmaxsize = ymaxsize = 0;
}

CHwWinCursor::~CHwWinCursor(void)
{
	if (cursor!=NULL)
		DestroyCursor(cursor);

	for (std::vector<ImageData>::iterator it=icons.begin(); it<icons.end(); it++)
		delete[] (*it).data;
	icons.clear();
}

#else

void CHwX11Cursor::resizeImage(XcursorImage*& image, const int new_x, const int new_y)
{
	if (image->width==new_x && image->height==new_y)
		return;

	const int old_x = image->width;
	const int old_y = image->height;

	XcursorImage* new_image = XcursorImageCreate(new_x, new_y);
	new_image->delay = image->delay;

	unsigned char* src = (unsigned char*)image->pixels;
	unsigned char* dst = (unsigned char*)new_image->pixels;
	memset(dst, 0, new_x*new_y*4);

	for (int y = 0; y < old_y; ++y)
		for (int x = 0; x < old_x; ++x)
			for (int v = 0; v < 4; ++v)
				dst[(y*new_x+x)*4+v] = src[(y*old_x+x)*4+v];

	XcursorImageDestroy(image);
	image = new_image;
}

void CHwX11Cursor::PushImage(int xsize, int ysize, void* mem)
{
	xmaxsize=std::max(xmaxsize,xsize);
	ymaxsize=std::max(ymaxsize,ysize);

	XcursorImage* image = XcursorImageCreate(xsize,ysize);
	image->delay = (XcursorUInt)(defFrameLength*1000.0f);
	char* dst=(char*)image->pixels;
	char* src=(char*)mem;
	char* end=src+xsize*ysize*4;
	do{
		dst[0]=src[2];      // B
		dst[1]=src[1];      // G
		dst[2]=src[0];      // R
		dst[3]=src[3];      // A
		dst+=4;
		src+=4;
	}while(src<end);

	cimages.push_back(image);
}

void CHwX11Cursor::SetDelay(float delay)
{
	cimages.back()->delay = (XcursorUInt)(delay*1000.0f); //in millseconds
}

void CHwX11Cursor::PushFrame(int index, float delay)
{
	if (index>=cimages.size())
		return;

	if (cimages[index]->delay!=delay) {
		// make a copy of the existing one
		XcursorImage* ci = cimages[index];
		PushImage( ci->width, ci->height, ci->pixels );
		SetDelay(delay);
	}else{
		cimages.push_back( cimages[index] );
	}
}

void CHwX11Cursor::Finish()
{
	if (cimages.size()<1)
		return;

	//resize images
	for (std::vector<XcursorImage*>::iterator it=cimages.begin(); it<cimages.end(); it++)
		resizeImage(*it,xmaxsize,ymaxsize);

	XcursorImages *cis = XcursorImagesCreate(cimages.size());
	cis->nimage = cimages.size();
	for (int i=0; i < cimages.size(); i++ ) {
		XcursorImage* ci = cimages[i];
		ci->xhot = (hotSpot==CMouseCursor::TopLeft) ? 0 : ci->width/2;
		ci->yhot = (hotSpot==CMouseCursor::TopLeft) ? 0 : ci->height/2;
		cis->images[i] = ci;
	}

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWMInfo(&info)) {
		XcursorImagesDestroy(cis);
		cimages.clear();
		logOutput.Print("SDL error: can't get X11 window info");
		return;
	}

	cursor = XcursorImagesLoadCursor(info.info.x11.display,cis);
	XcursorImagesDestroy(cis);
	cimages.clear();
}

void CHwX11Cursor::Bind()
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWMInfo(&info)) {
		logOutput.Print("SDL error: can't get X11 window info");
		return;
	}
	XDefineCursor(info.info.x11.display,info.info.x11.window,cursor);
}

CHwX11Cursor::CHwX11Cursor(void)
{
	cursor = 0;
	hotSpot=CMouseCursor::Center;
	xmaxsize = ymaxsize = 0;
}

CHwX11Cursor::~CHwX11Cursor(void)
{
	for (std::vector<XcursorImage*>::iterator it=cimages.begin() ; it < cimages.end(); it++ )
		XcursorImageDestroy(*it);
	cimages.clear();

	if (cursor!=0) {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (!SDL_GetWMInfo(&info)) {
			logOutput.Print("SDL error: can't get X11 window info");
			return;
		}
		XFreeCursor(info.info.x11.display,cursor);
	}
}

#endif
