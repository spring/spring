#include "StdAfx.h"
// Bitmap.cpp: implementation of the CBitmap class.
//
//////////////////////////////////////////////////////////////////////

#include "Rendering/GL/myGL.h"
#include <ostream>
#include <fstream>
#include "FileSystem/FileHandler.h"
#if defined(__APPLE__)
#include <QuickTime/ImageCompression.h>
#include <QuickTime/QuickTimeComponents.h>
#else
#include <IL/il.h>
#endif
#include "Platform/FileSystem.h"
#include "Rendering/Textures/Bitmap.h"
#include "bitops.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#if !defined(__APPLE__)
struct InitializeOpenIL {
	InitializeOpenIL() {
		ilInit();
	}
	~InitializeOpenIL() {
		ilShutDown();
	}
} static initOpenIL;
#endif

CBitmap::CBitmap()
  : xsize(0), ysize(0)
{
	mem=0;
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
	type = old.type;
	xsize=old.xsize;
	ysize=old.ysize;
	int size;
	if(type == BitmapTypeStandardRGBA) 	size = xsize*ysize*4;
	else size = xsize*ysize; // Alpha

	mem=new unsigned char[size];
	memcpy(mem,old.mem,size);
}

CBitmap::CBitmap(unsigned char *data, int xsize, int ysize)
  : xsize(xsize), ysize(ysize)
{
	type = BitmapTypeStandardRGBA;
	ddsimage = 0;
	mem=new unsigned char[xsize*ysize*4];
	memcpy(mem,data,xsize*ysize*4);
}

CBitmap::CBitmap(string const& filename)
: mem(0),
  xsize(0),
  ysize(0)
{
	type = BitmapTypeStandardRGBA;
	ddsimage = 0;
	Load(filename);
}

CBitmap& CBitmap::operator=(const CBitmap& bm)
{
	if( this != &bm ){
		delete[] mem;
		xsize=bm.xsize;
		ysize=bm.ysize;
		int size;
		if(type == BitmapTypeStandardRGBA) 	size = xsize*ysize*4;
		else size = xsize*ysize; // Alpha

		mem=new unsigned char[size];
		memcpy(mem,bm.mem,size);
	}
	return *this;
}

void CBitmap::Alloc (int w,int h)
{
	bool noAlpha = true;
	
	delete[] mem;
	xsize=w;
	ysize=h;
	type=BitmapTypeStandardRGBA;
	mem=new unsigned char[w*h*4];
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

	CFileHandler file(filename);
	if(file.FileExists() == false)
	{
		xsize = 1;
		ysize = 1;
		mem=new unsigned char[4];
		memset(mem, 0, 4);
		return false;
	}

	unsigned char *buffer = new unsigned char[file.FileSize()+2];
	file.Read(buffer, file.FileSize());

#if defined(__APPLE__) // Use QuickTime to load images on Mac

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
		return;
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

	if(noAlpha){
		for(int y=0;y<ysize;++y){
			for(int x=0;x<xsize;++x){
			mem[(y*xsize+x)*4+3]=defaultAlpha;
			}
		}
	}

	return true;
}

bool CBitmap::LoadGrayscale (const string& filename)
{
	type = BitmapTypeStandardAlpha;

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	CFileHandler file(filename);
	if(!file.FileExists())
		return false;

	unsigned char *buffer = new unsigned char[file.FileSize()+1];
	file.Read(buffer, file.FileSize());

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	const bool success = ilLoadL(IL_TYPE_UNKNOWN, buffer, file.FileSize());
	delete [] buffer;

	if(success == false)
		return false;

#if !defined(__APPLE__) // Temporary fix to allow testing of everything
						// else until i get a quicktime image loader written
	ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	xsize = ilGetInteger(IL_IMAGE_WIDTH);
	ysize = ilGetInteger(IL_IMAGE_HEIGHT);

	mem = new unsigned char[xsize * ysize];
	memcpy(mem, ilGetData(), xsize * ysize);
#else
	xsize = 4;
	ysize = 4;

	mem = new unsigned char[xsize * ysize];
#endif

	ilDeleteImages(1, &ImageName); 
	return true;
}


void CBitmap::Save(string const& filename)
{
	if (type == BitmapTypeDDS) {
		ddsimage->save(filename);
		return;
	}

#if !defined(__APPLE__) // Use devil on Windows/Linux/...
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	unsigned char* buf=new unsigned char[xsize*ysize*4];
	/* HACK Flip the image so it saves the right way up.
		(Fiddling with ilOriginFunc didn't do anything?)
		Duplicated with ReverseYAxis. */
	for(int y=0;y<ysize;++y){
		for(int x=0;x<xsize;++x){
			buf[((ysize-1-y)*xsize+x)*4+0]=mem[((y)*xsize+x)*4+0];
			buf[((ysize-1-y)*xsize+x)*4+1]=mem[((y)*xsize+x)*4+1];
			buf[((ysize-1-y)*xsize+x)*4+2]=mem[((y)*xsize+x)*4+2];
			buf[((ysize-1-y)*xsize+x)*4+3]=mem[((y)*xsize+x)*4+3];
		}
	}

	ilHint(IL_COMPRESSION_HINT, IL_USE_COMPRESSION);
	ilSetInteger (IL_JPG_QUALITY, 80);

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	ilTexImage(xsize,ysize,1,4,IL_RGBA,IL_UNSIGNED_BYTE,NULL);
	ilSetData(buf);

	std::vector<std::string> filenames = filesystem.GetNativeFilenames(filename, true);
	for (std::vector<std::string>::iterator it = filenames.begin(); it != filenames.end(); ++it) {
		// TODO how to detect errors, so the image doesn't end up in every writable data directory?
		ilSaveImage((char*)(it->c_str()));
	}

	ilDeleteImages(1,&ImageName);
	delete[] buf;
#endif // I'll add a quicktime exporter for mac soonish...Krysole
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
	if ((xsize != next_power_of_2(xsize) || ysize != next_power_of_2(ysize)) && !GLEW_ARB_texture_non_power_of_two)
		 //&& strcmp(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "2.0") < 0 )
	{
		CBitmap bm = CreateRescaled(next_power_of_2(xsize), next_power_of_2(ysize));
		return bm.CreateTexture(mipmaps);
	}

	unsigned int texture;

	glGenTextures(1, &texture);			
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if(mipmaps)
	{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);

		// create mipmapped texture
		if (strcmp(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "1.4") >= 0) {
			// This required GL-1.4
			// instead of using glu, we rely on glTexImage2D to create the Mipmaps.
			glTexParameteri(GL_TEXTURE_2D,GL_GENERATE_MIPMAP,true);
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,xsize, ysize, 0,GL_RGBA, GL_UNSIGNED_BYTE, mem);
		} else
			gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,xsize, ysize,GL_RGBA, GL_UNSIGNED_BYTE, mem);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		//glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,xsize, ysize, 0,GL_RGBA, GL_UNSIGNED_BYTE, mem);
		//gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);

		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,xsize, ysize, 0,GL_RGBA, GL_UNSIGNED_BYTE, mem);
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
		if(!ddsimage->upload_texture2D())
		{
		glDeleteTextures(1, &texobj);
		texobj = 0;
		}
		break;
	case nv_dds::Texture3D:
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, texobj);
		if(!ddsimage->upload_texture3D())
		{
		glDeleteTextures(1, &texobj);
		texobj = 0;
		}
		break;
	case nv_dds::TextureCubemap:
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, texobj);
		if(!ddsimage->upload_textureCubemap())
		{
		glDeleteTextures(1, &texobj);
		texobj = 0;
		}
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

// Unused
CBitmap CBitmap::GetRegion(int startx, int starty, int width, int height)
{
	CBitmap bm;

	delete[] bm.mem;
	bm.mem=new unsigned char[width*height*4];
	bm.xsize=width;
	bm.ysize=height;

	for(int y=0;y<height;++y){
		for(int x=0;x<width;++x){
			bm.mem[(y*width+x)*4]=mem[((starty+y)*xsize+startx+x)*4];
			bm.mem[(y*width+x)*4+1]=mem[((starty+y)*xsize+startx+x)*4+1];
			bm.mem[(y*width+x)*4+2]=mem[((starty+y)*xsize+startx+x)*4+2];
			bm.mem[(y*width+x)*4+3]=mem[((starty+y)*xsize+startx+x)*4+3];
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

void CBitmap::ReverseYAxis(void)
{
	unsigned char* buf=new unsigned char[xsize*ysize*4];

	for(int y=0;y<ysize;++y){
		for(int x=0;x<xsize;++x){
			buf[((ysize-1-y)*xsize+x)*4+0]=mem[((y)*xsize+x)*4+0];
			buf[((ysize-1-y)*xsize+x)*4+1]=mem[((y)*xsize+x)*4+1];
			buf[((ysize-1-y)*xsize+x)*4+2]=mem[((y)*xsize+x)*4+2];
			buf[((ysize-1-y)*xsize+x)*4+3]=mem[((y)*xsize+x)*4+3];
		}
	}
	delete[] mem;
	mem=buf;
}

#if defined(__APPLE__)

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

#endif /* __APPLE__ */
