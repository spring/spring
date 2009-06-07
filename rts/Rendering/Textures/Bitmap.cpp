#include "StdAfx.h"
// Bitmap.cpp: implementation of the CBitmap class.
//
//////////////////////////////////////////////////////////////////////

#ifdef __APPLE__
/* The QuickTime loader is outdated and disabled for the moment. 
   Its only purpose seems to be to get rid of some dependencies.
   Maybe it should be removed. */
//#define USE_QUICKTIME
#endif

//#include <omp.h>
#include <ostream>
#include <fstream>
#include <string.h>
#if defined(USE_QUICKTIME)
#include <QuickTime/ImageCompression.h>
#include <QuickTime/QuickTimeComponents.h>
#else
#include <IL/il.h>
#include <IL/ilu.h>
#endif
#include "mmgr.h"

#include "Rendering/GL/myGL.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"
#include "GlobalUnsynced.h"
#include "Bitmap.h"
#include "bitops.h"



static const float blurkernel[9] = {
	1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f,
	2.0f/16.0f, 4.0f/16.0f, 2.0f/16.0f,
	1.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#if !defined(USE_QUICKTIME)
struct InitializeOpenIL {
	InitializeOpenIL() {
		ilInit();
		iluInit();
	}
	~InitializeOpenIL() {
		ilShutDown();
	}
} static initOpenIL;
#endif


CBitmap::CBitmap()
  : xsize(0), ysize(0), channels(4)
{
	mem = 0;
	ddsimage = 0;
	type = BitmapTypeStandardRGBA;
}


CBitmap::~CBitmap()
{
	delete[] mem;
	delete ddsimage;
}


CBitmap::CBitmap(const CBitmap& old)
{
	assert(old.type != BitmapTypeDDS);
	ddsimage = 0;
	type  = old.type;
	xsize = old.xsize;
	ysize = old.ysize;
	channels = old.channels;
	int size = xsize*ysize * channels;

	mem=new unsigned char[size];
	memcpy(mem,old.mem,size);
}


CBitmap::CBitmap(unsigned char *data, int xsize, int ysize)
  : xsize(xsize), ysize(ysize), channels(4)
{
	type = BitmapTypeStandardRGBA;
	ddsimage = 0;
	mem=new unsigned char[xsize*ysize*channels];
	memcpy(mem,data,xsize*ysize*channels);
}


CBitmap& CBitmap::operator=(const CBitmap& bm)
{
	if( this != &bm ){
		delete[] mem;
		type  = bm.type;
		xsize = bm.xsize;
		ysize = bm.ysize;
		channels = bm.channels;
		int size = xsize*ysize * channels;

		mem=new unsigned char[size];
		memcpy(mem,bm.mem,size);
	}
	return *this;
}


void CBitmap::Alloc(int w,int h)
{
	delete[] mem;
	xsize=w;
	ysize=h;
	mem=new unsigned char[w*h*channels];
	memset(mem, 0, w*h*channels);
}


bool CBitmap::Load(string const& filename, unsigned char defaultAlpha)
{
	bool noAlpha = true;

	delete[] mem;
	mem = NULL;

	if(filename.find(".dds")!=string::npos){
		ddsimage = new nv_dds::CDDSImage();
		type = BitmapTypeDDS;
		return ddsimage->load(filename);
	}
	type = BitmapTypeStandardRGBA;
	channels = 4;

	CFileHandler file(filename);
	if(file.FileExists() == false)
	{
		Alloc(1,1);
		return false;
	}

	unsigned char *buffer = new unsigned char[file.FileSize()+2];
	file.Read(buffer, file.FileSize());

#if defined(USE_QUICKTIME) // Use QuickTime to load images on Mac

	mem = LoadTextureData(filename, buffer, file.FileSize(),
		xsize, ysize, noAlpha);

	// Chagne the *hasAlpha* used in LoadTextureData to *noAlpha*
	noAlpha = !noAlpha;

	delete[] buffer;

	if (!mem) {
		xsize = 1;
		ysize = 1;
		mem=new unsigned char[4];
		mem[0] = 255; // Red allows us to easily see textures that failed to load
		mem[1] = 0;
		mem[2] = 0;
		mem[3] = 255; // Non Transparent
		return true;
	}

	// Because most of the algorithms that work on our texture are not generalized yet
	// We have to convert the image into the same format (RGBA instead of ARGB[ppc] or BGRA[x86]).
	// We then use the same OpenGL calls since they don't chagne between (windows or x86/ppc)
	int max_pixels = xsize * ysize;
	for (int i = 0; i < max_pixels; ++i) {
		int base = i * 4;
		// ARGB; temp = A; _RGB >> RGB_; A = temp; RGBA
		char temp = mem[base + 0]; // A
		mem[base + 0] = mem[base + 1]; // R
		mem[base + 1] = mem[base + 2]; // G
		mem[base + 2] = mem[base + 3]; // B
		mem[base + 3] = temp; // A
	}


#else // Use DevIL to load images on windows/linux/...
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer, file.FileSize());
	ilDisable(IL_ORIGIN_SET);
	delete [] buffer;

	if(success == false)
	{
		xsize = 1;
		ysize = 1;
		mem=new unsigned char[4];
		mem[0] = 255; // Red allows us to easily see textures that failed to load
		mem[1] = 0;
		mem[2] = 0;
		mem[3] = 255; // Non Transparent
		return false;
	}



	noAlpha=ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL)!=4;
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	xsize = ilGetInteger(IL_IMAGE_WIDTH);
	ysize = ilGetInteger(IL_IMAGE_HEIGHT);

	mem = new unsigned char[xsize * ysize * 4];
	//	ilCopyPixels(0,0,0,xsize,ysize,0,IL_RGBA,IL_UNSIGNED_BYTE,mem);
	memcpy(mem, ilGetData(), xsize * ysize * 4);

	ilDeleteImages(1, &ImageName);
#endif

	if (noAlpha){
		for (int y=0; y<ysize; ++y) {
			for (int x=0; x<xsize; ++x) {
				mem[((y*xsize+x) * 4) + 3] = defaultAlpha;
			}
		}
	}

	return true;
}


bool CBitmap::LoadGrayscale (const string& filename)
{
#if !defined(USE_QUICKTIME) // Temporary fix to allow testing of everything
						// else until i get a quicktime image loader written
	type = BitmapTypeStandardAlpha;
	channels = 1;

	CFileHandler file(filename);
	if(!file.FileExists())
		return false;

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	unsigned char *buffer = new unsigned char[file.FileSize()+1];
	file.Read(buffer, file.FileSize());

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer, file.FileSize());
	ilDisable(IL_ORIGIN_SET);
	delete [] buffer;

	if(success == false)
		return false;

	ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	xsize = ilGetInteger(IL_IMAGE_WIDTH);
	ysize = ilGetInteger(IL_IMAGE_HEIGHT);

	mem = new unsigned char[xsize * ysize];
	memcpy(mem, ilGetData(), xsize * ysize);
	
	ilDeleteImages(1, &ImageName);
#else
	xsize = 4;
	ysize = 4;

	mem = new unsigned char[xsize * ysize];
#endif
	
	return true;
}


bool CBitmap::Save(string const& filename, bool opaque)
{
	if (type == BitmapTypeDDS) {
		return ddsimage->save(filename);
	}

#if defined(USE_QUICKTIME)

	return false; // I'll add a quicktime exporter for mac soonish...Krysole

#else

	// Use DevIL on Windows/Linux/...

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	unsigned char* buf = new unsigned char[xsize * ysize * 4];
	const int ymax = (ysize - 1);
	/* HACK Flip the image so it saves the right way up.
		(Fiddling with ilOriginFunc didn't do anything?)
		Duplicated with ReverseYAxis. */
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int bi = 4 * (x + (xsize * (ymax - y)));
			const int mi = 4 * (x + (xsize * (y)));
			buf[bi + 0] = mem[mi + 0];
			buf[bi + 1] = mem[mi + 1];
			buf[bi + 2] = mem[mi + 2];
			buf[bi + 3] = opaque ? 0xff : mem[mi + 3];
		}
	}

	ilHint(IL_COMPRESSION_HINT, IL_USE_COMPRESSION);
	ilSetInteger(IL_JPG_QUALITY, 80);

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	ilTexImage(xsize, ysize, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
	ilSetData(buf);

	const string fullpath = filesystem.LocateFile(filename, FileSystem::WRITE);
	const bool success = ilSaveImage((char*)fullpath.c_str());

	ilDeleteImages(1,&ImageName);
	ilDisable(IL_ORIGIN_SET);
	delete[] buf;

	return success;

#endif // USE_QUICKTIME
}


#ifndef BITMAP_NO_OPENGL

unsigned int CBitmap::CreateTexture(bool mipmaps)
{
	if(type == BitmapTypeDDS)
	{
		return CreateDDSTexture();
	}

	if(mem==NULL)
		return 0;

	// jcnossen: Some drivers return "2.0" as a version string,
	// but switch to software rendering for non-power-of-two textures.
	// GL_ARB_texture_non_power_of_two indicates that the hardware will actually support it.
	if (!gu->supportNPOTs && (xsize != next_power_of_2(xsize) || ysize != next_power_of_2(ysize)))
	{
		CBitmap bm = CreateRescaled(next_power_of_2(xsize), next_power_of_2(ysize));
		return bm.CreateTexture(mipmaps);
	}

	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//FIXME add glPixelStorei(GL_UNPACK_ALIGNMENT, 1); for NPOTs

	if(mipmaps)
	{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);

		glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		//glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,xsize, ysize, 0,GL_RGBA, GL_UNSIGNED_BYTE, mem);
		//gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, xsize, ysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	}
	return texture;
}


unsigned int CBitmap::CreateDDSTexture()
{
	GLuint texobj;
	glPushAttrib(GL_TEXTURE_BIT);

	glGenTextures(1, &texobj);

	switch(ddsimage->get_type())
	{
	case nv_dds::TextureNone:
		glDeleteTextures(1, &texobj);
		texobj = 0;
		break;
	case nv_dds::TextureFlat:    // 1D, 2D, and rectangle textures
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texobj);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
		if(!ddsimage->upload_texture2D()) {
			glDeleteTextures(1, &texobj);
			texobj = 0;
		}
		break;
	case nv_dds::Texture3D:
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, texobj);
		if(!ddsimage->upload_texture3D()) {
			glDeleteTextures(1, &texobj);
			texobj = 0;
		}
		break;
	case nv_dds::TextureCubemap:
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, texobj);
		if(!ddsimage->upload_textureCubemap()) {
			glDeleteTextures(1, &texobj);
			texobj = 0;
		}
		break;
	default:
		assert(false);
		break;
	}

	glPopAttrib();
	return texobj;
}

#endif // !BITMAP_NO_OPENGL


void CBitmap::CreateAlpha(unsigned char red,unsigned char green,unsigned char blue)
{
	float3 aCol;
	for(int a=0;a<3;++a)
	{
		int cCol=0;
		int numCounted=0;
		for(int y=0;y<ysize;++y){
			for(int x=0;x<xsize;++x){
				if(mem[(y*xsize+x)*4+3]!=0 && !(mem[(y*xsize+x)*4+0]==red && mem[(y*xsize+x)*4+1]==green && mem[(y*xsize+x)*4+2]==blue)){
					cCol+=mem[(y*xsize+x)*4+a];
					++numCounted;
				}
			}
		}
		if ( numCounted != 0 )
		{
			aCol[a]=cCol/255.0f/numCounted;
		}
	}
	for(int y=0;y<ysize;++y)
	{
		for(int x=0;x<xsize;++x)
		{
			if(mem[(y*xsize+x)*4+0]==red && mem[(y*xsize+x)*4+1]==green && mem[(y*xsize+x)*4+2]==blue) {
				mem[(y*xsize+x)*4+0]= (unsigned char) (aCol.x*255);
				mem[(y*xsize+x)*4+1]= (unsigned char) (aCol.y*255);
				mem[(y*xsize+x)*4+2]= (unsigned char) (aCol.z*255);
				mem[(y*xsize+x)*4+3]=0;
			}
		}
	}
}


// Depreciated (Only used by GUI which will be replaced by CEGUI anyway)
void CBitmap::SetTransparent( unsigned char red, unsigned char green, unsigned char blue )
{
	for ( unsigned int y = 0; y < xsize; y++ )
	{
		for ( unsigned int x = 0; x < xsize; x++ )
		{
			unsigned int index = (y*xsize + x)*4;
			if ( mem[index+0] == red &&
				mem[index+1] == green &&
				mem[index+2] == blue )
			{
				// set transparent
				mem[index+3] = 0;
			}
		}
	}
}


void CBitmap::Renormalize(float3 newCol)
{
	float3 aCol;
	//	float3 aSpread;

	float3 colorDif;
	//	float3 spreadMul;
	for(int a=0;a<3;++a){
		int cCol=0;
		int numCounted=0;
		for(int y=0;y<ysize;++y){
			for(int x=0;x<xsize;++x){
				if(mem[(y*xsize+x)*4+3]!=0){
					cCol+=mem[(y*xsize+x)*4+a];
					++numCounted;
				}
			}
		}
		aCol[a]=cCol/255.0f/numCounted;
		cCol/=xsize*ysize;
		colorDif[a]=newCol[a]-aCol[a];

/*		int spread=0;
        for(int y=0;y<ysize;++y){
        for(int x=0;x<xsize;++x){
        if(mem[(y*xsize+x)*4+3]!=0){
        int dif=mem[(y*xsize+x)*4+a]-cCol;
        spread+=abs(dif);
        }
        }
        }
        aSpread.xyz[a]=spread/255.0f/numCounted;
        spreadMul.xyz[a]=(float)(newSpread[a]/aSpread[a]);*/
	}
	for(int a=0;a<3;++a){
		for(int y=0;y<ysize;++y){
			for(int x=0;x<xsize;++x){
				float nc=float(mem[(y*xsize+x)*4+a])/255.0f+colorDif[a];
				/*				float r=newCol.xyz[a]+(nc-newCol.xyz[a])*spreadMul.xyz[a];*/
				mem[(y*xsize+x)*4+a]=(unsigned char)(std::min(255.f,std::max(0.f,nc*255)));
			}
		}
	}
}


inline void kernelBlur(CBitmap* dst, const unsigned char* src, int x, int y, int channel, float weight) {
	float fragment = 0.0f;

	const int pos = (x + y * dst->xsize) * dst->channels + channel;

	for (int i=0; i<9; ++i) {
		int yoffset = (i / 3) - 1;
		int xoffset = (i - (yoffset + 1) * 3) - 1;

		const int tx = x + xoffset;
		const int ty = y + yoffset;
		if (tx<0 || tx>=dst->xsize) {
			xoffset = 0;
		}
		if (ty<0 || ty>=dst->ysize) {
			yoffset = 0;
		}
		int offset = (xoffset + yoffset * dst->xsize) * dst->channels;
		if (i == 4) {
			fragment += weight * blurkernel[i] * src[pos + offset];
		} else {
			fragment += blurkernel[i] * src[pos + offset];
		}
	}

	dst->mem[pos] = (unsigned char)std::min(255.0f,std::max(0.0f, fragment ));
}


void CBitmap::Blur(int iterations, float weight)
{
	if (type == BitmapTypeDDS) {
		return;
	}

	CBitmap* src = this;
	CBitmap* dst = new CBitmap();
	dst->channels = src->channels;
	dst->Alloc(xsize,ysize);

	for (int i=0;i<iterations;++i){
		#pragma omp parallel private(y,x,i)
		{
			#pragma omp for
			for (int y=0;y<ysize;++y){
				for (int x=0;x<xsize;++x){
					for (int i=0;i<channels;++i){
						kernelBlur(dst,src->mem,x,y,i,weight);
					}
				}
			}
		}
		CBitmap* buf = dst;
		dst = src;
		src = buf;
	}

	if (dst == this) {
		CBitmap* buf = dst;
		dst = src;
		src = buf;
	}

	delete dst;
}


// Unused
CBitmap CBitmap::GetRegion(int startx, int starty, int width, int height)
{
	CBitmap bm;

	delete[] bm.mem;
	bm.mem=new unsigned char[width*height*channels];
	bm.channels = channels;
	bm.type = type;
	bm.xsize = width;
	bm.ysize = height;

	for(int y=0;y<height;++y){
		for(int x=0;x<width;++x){
			for (int i=0; i<channels;++i) {
				bm.mem[(y*width+x)*channels+i]=mem[((starty+y)*xsize+startx+x)*channels+i];
			}
		}
	}

	return bm;
}


CBitmap CBitmap::CreateMipmapLevel(void)
{
	CBitmap bm;

	delete[] bm.mem;
	bm.xsize=xsize/2;
	bm.ysize=ysize/2;
	bm.mem=new unsigned char[bm.xsize*bm.ysize*4];

	for(int y=0;y<ysize/2;++y){
		for(int x=0;x<xsize/2;++x){
			float r=0,g=0,b=0,a=0;
			for(int y2=0;y2<2;++y2){
				for(int x2=0;x2<2;++x2){
					r+=mem[((y*2+y2)*xsize+x*2+x2)*4+0];
					g+=mem[((y*2+y2)*xsize+x*2+x2)*4+1];
					b+=mem[((y*2+y2)*xsize+x*2+x2)*4+2];
					a+=mem[((y*2+y2)*xsize+x*2+x2)*4+3];
				}
			}
			bm.mem[(y*bm.xsize+x)*4]=(unsigned char)(r/4);
			bm.mem[(y*bm.xsize+x)*4+1]=(unsigned char)(g/4);
			bm.mem[(y*bm.xsize+x)*4+2]=(unsigned char)(b/4);
			bm.mem[(y*bm.xsize+x)*4+3]=(unsigned char)(a/4);
		}
	}

	return bm;

}


CBitmap CBitmap::CreateRescaled(int newx, int newy)
{
	CBitmap bm;

	delete[] bm.mem;
	bm.xsize=newx;
	bm.ysize=newy;
	bm.mem=new unsigned char[bm.xsize*bm.ysize*4];

	float dx=float(xsize)/newx;
	float dy=float(ysize)/newy;

	float cy=0;
	for(int y=0;y<newy;++y){
		int sy=(int)cy;
		cy+=dy;
		int ey=(int)cy;
		if(ey==sy)
			ey=sy+1;

		float cx=0;
		for(int x=0;x<newx;++x){
			int sx=(int)cx;
			cx+=dx;
			int ex=(int)cx;
			if(ex==sx)
				ex=sx+1;

			int r=0,g=0,b=0,a=0;
			for(int y2=sy;y2<ey;++y2){
				for(int x2=sx;x2<ex;++x2){
					r+=mem[(y2*xsize+x2)*4+0];
					g+=mem[(y2*xsize+x2)*4+1];
					b+=mem[(y2*xsize+x2)*4+2];
					a+=mem[(y2*xsize+x2)*4+3];
				}
			}
			bm.mem[(y*bm.xsize+x)*4+0]=r/((ex-sx)*(ey-sy));
			bm.mem[(y*bm.xsize+x)*4+1]=g/((ex-sx)*(ey-sy));
			bm.mem[(y*bm.xsize+x)*4+2]=b/((ex-sx)*(ey-sy));
			bm.mem[(y*bm.xsize+x)*4+3]=a/((ex-sx)*(ey-sy));
		}
	}
	return bm;
}


void CBitmap::InvertColors()
{
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for(int y = 0; y < ysize; ++y) {
		for(int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 0] = 0xFF - mem[base + 0];
			mem[base + 1] = 0xFF - mem[base + 1];
			mem[base + 2] = 0xFF - mem[base + 2];
			// do not invert alpha
		}
	}
}


void CBitmap::GrayScale()
{
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for(int y = 0; y < ysize; ++y) {
		for(int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			const float illum =
				(mem[base + 0] * 0.299f) +
				(mem[base + 1] * 0.587f) +
				(mem[base + 2] * 0.114f);
			const unsigned int  ival = (unsigned int)(illum * (256.0f / 255.0f));
			const unsigned char cval = (ival <= 0xFF) ? ival : 0xFF;
			mem[base + 0] = cval;
			mem[base + 1] = cval;
			mem[base + 2] = cval;
		}
	}
}

#if !defined(USE_QUICKTIME) 
static ILubyte TintByte(ILubyte value, float tint)
#else
static unsigned char TintByte(unsigned char value, float tint)
#endif
{
	float f = (float)value;
	f = std::max(0.0f, std::min(255.0f, f * tint));
	return (unsigned char)f;
}


void CBitmap::Tint(const float tint[3])
{
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for (int y = 0; y < ysize; y++) {
		for (int x = 0; x < xsize; x++) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 0] = TintByte(mem[base + 0], tint[0]);
			mem[base + 1] = TintByte(mem[base + 1], tint[1]);
			mem[base + 2] = TintByte(mem[base + 2], tint[2]);
			// don't touch the alpha channel
		}
	}
}


void CBitmap::ReverseYAxis()
{
	//FIXME: optimize, so it doesn't alloc a new array

	unsigned char* buf=new unsigned char[xsize*ysize*channels];

	for(int y=0;y<ysize;++y){
		for(int x=0;x<xsize;++x){
			for (int i=0;i<channels;++i){
				buf[((ysize-1-y)*xsize+x)*channels+i]=mem[((y)*xsize+x)*channels+i];
			}
		}
	}
	delete[] mem;
	mem=buf;
}


#if defined(USE_QUICKTIME)

Handle CBitmap::GetPtrDataRef(unsigned char *data, unsigned int size,
	const std::string &filename)
{
	// Load Data Reference
	Handle dataRef;
	Handle fileNameHandle;
	PointerDataRefRecord ptrDataRefRec;
	ComponentInstance dataRefHandler;
	unsigned char pstr[255];

	ptrDataRefRec.data = data;
	ptrDataRefRec.dataLength = size;

	/*err = */PtrToHand(&ptrDataRefRec, &dataRef, sizeof(PointerDataRefRecord));

	// Open a Data Handler for the Data Reference
	/*err = */OpenADataHandler(dataRef, PointerDataHandlerSubType, NULL,
		(OSType)0, NULL, kDataHCanRead, &dataRefHandler);

	// Convert From CString in filename to a PascalString in pstr
	if (filename.length() > 255) {
		CopyCStringToPascal(filename.c_str(), pstr);
		//hmm...not good, pascal string limit is 255!
		//do some error handling maybe?!
	}

	// Add filename extension
	/*err = */PtrToHand(pstr, &fileNameHandle, filename.length() + 1);
	/*err = */DataHSetDataRefExtension(dataRefHandler, fileNameHandle,
		kDataRefExtensionFileName);
	DisposeHandle(fileNameHandle);

	// Release old handler which does not have the extensions
	DisposeHandle(dataRef);

	// Grab the new version of the data ref from the data handler
	/*err = */ DataHGetDataRef(dataRefHandler, &dataRef);

	return dataRef;
}


unsigned char *CBitmap::LoadTextureData(const std::string &filename,
	unsigned char *data, unsigned int sizeData, int &xsize,
	int &ysize, bool &hasAlpha)
{
	unsigned char *imageData = 0;
	GWorldPtr gworld = 0;
	OSType pixelFormat;
	int rowStride;
	GraphicsImportComponent gicomp;
	Rect rectImage;
	GDHandle origDevice;
	CGrafPtr origPort;
	ImageDescriptionHandle desc;
	int depth;

	// Data Handle for file data ( & load data from file )
	Handle dataRef = GetPtrDataRef(data, sizeData, filename);

	// GraphicsImporter - Get Importer for our filetype
	GetGraphicsImporterForDataRef(dataRef, 'ptr ', &gicomp);

	// GWorld - Get Texture Info
	if (noErr != GraphicsImportGetNaturalBounds(gicomp, &rectImage)) {
		return 0;
	}
	xsize = (unsigned int)(rectImage.right - rectImage.left);
	ysize = (unsigned int)(rectImage.bottom - rectImage.top);

	// ImageDescription - Get Image Description
	if (noErr != GraphicsImportGetImageDescription(gicomp, &desc)) {
		CloseComponent(gicomp);
		return 0;
	}

	// ImageDescription - Get Bit Depth
	HLock(reinterpret_cast<char **>(desc));
	if (depth == 32) { // Only if it returns 32 does the image have an alpha chanel!
		hasAlpha = true;
	} else {
		hasAlpha = false;
	}

	// GWorld - Pixel Format stuff
	pixelFormat = k32ARGBPixelFormat; // Make sure its forced...NOTE: i'm pretty sure this cannot be RGBA!

	// GWorld - Row stride
	rowStride = xsize * 4; // (width * depth_bpp / 8)

	// GWorld - Allocate output buffer
	imageData = new unsigned char[rowStride * ysize];

	// GWorld - Actually Create IT!
	QTNewGWorldFromPtr(&gworld, pixelFormat, &rectImage, 0, 0, 0, imageData, rowStride);
	if (!gworld) {
		delete imageData;
		CloseComponent(gicomp);
		DisposeHandle(reinterpret_cast<char **>(desc));
		return 0;
	}

	// Save old Graphics Device and Graphics Port to reset to later
	GetGWorld (&origPort, &origDevice);

	// GraphicsImporter - Set Destination GWorld (our buffer)
	if (noErr != GraphicsImportSetGWorld(gicomp, gworld, 0)) {
		DisposeGWorld(gworld);
		delete imageData;
		CloseComponent(gicomp);
		DisposeHandle(reinterpret_cast<char **>(desc));
		return 0;
	}

	// GraphicsImporter - Set Quality Level
	if (noErr != GraphicsImportSetQuality(gicomp, codecLosslessQuality)) {
		DisposeGWorld(gworld);
		delete imageData;
		CloseComponent(gicomp);
		DisposeHandle(reinterpret_cast<char **>(desc));
		return 0;
	}

	// Lock pixels so that we can draw to our memory texture
	if (!GetGWorldPixMap(gworld) || !LockPixels(GetGWorldPixMap(gworld))) {
		DisposeGWorld(gworld);
		delete imageData;
		CloseComponent(gicomp);
		DisposeHandle(reinterpret_cast<char **>(desc));
		return 0;
	}

	//*** Draw GWorld into our Memory Texture!
	GraphicsImportDraw(gicomp);

	// Clean up
	UnlockPixels(GetGWorldPixMap(gworld));
	SetGWorld(origPort, origDevice); // set graphics port to offscreen (we don't need it now)
	DisposeGWorld(gworld);
	CloseComponent(gicomp);
	DisposeHandle(reinterpret_cast<char **>(desc));

	return imageData;

}

#endif /* USE_QUICKTIME */
