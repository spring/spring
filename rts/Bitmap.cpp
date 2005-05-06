#include "StdAfx.h"
// Bitmap.cpp: implementation of the CBitmap class.
//
//////////////////////////////////////////////////////////////////////

#include "Bitmap.h"
#include "myGL.h"
#include <ostream>
#include <fstream>
#include "jpeglib.h"
#include "FileHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

struct pcx_header
{
	char manufacturer;
	char version;
	char encoding;
	char bits_per_pixel;
	short x, y;
	short width, height;
	short horz_res;
	short vert_res;
	char ega_palette[48];
	char reserved;
	char num_color_planes;
	short bytes_per_line;
	short palette_type;
	char padding[58];
};

CBitmap::CBitmap()
: xsize(1),
	ysize(1)
{
	mem=new unsigned char[4];
}

CBitmap::~CBitmap()
{
	if(mem!=0)
		delete[] mem;
}

CBitmap::CBitmap(const CBitmap& old)
{
	xsize=old.xsize;
	ysize=old.ysize;
	mem=new unsigned char[xsize*ysize*4];
	memcpy(mem,old.mem,xsize*ysize*4);
}

CBitmap::CBitmap(unsigned char *data, int xsize, int ysize)
: xsize(xsize),
	ysize(ysize)
{
	mem=new unsigned char[xsize*ysize*4];	
	memcpy(mem,data,xsize*ysize*4);
}

CBitmap::CBitmap(string filename)
: mem(0),
	xsize(0),
	ysize(0)
{
	Load(filename);
}

void CBitmap::operator=(const CBitmap& bm)
{
	delete[] mem;
	xsize=bm.xsize;
	ysize=bm.ysize;
	mem=new unsigned char[xsize*ysize*4];

	memcpy(mem,bm.mem,xsize*ysize*4);
}

void CBitmap::Load(string filename)
{
	if(mem!=0)
		delete[] mem;

	if(filename.find(".jpg")!=string::npos)
		LoadJPG(filename);
	else if(filename.find(".pcx")!=string::npos)
		LoadPCX(filename);
	else
		LoadBMP(filename);
}


void CBitmap::Save(string filename)
{
	if(filename.find(".jpg")!=string::npos)
		SaveJPG(filename);
	else
		SaveBMP(filename);
}

void CBitmap::LoadBMP(string filename)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	// Open file.
	CFileHandler bmpfile(filename);
	if (! bmpfile.FileExists()){
		MessageBox(0, "Unable to load BMP File", filename.c_str(), MB_OK);
		xsize=1;
		ysize=1;
		mem=new unsigned char[xsize*ysize*4];
		return;
	}

	// Load bitmap fileheader & infoheader
	bmpfile.Read(&bmfh,sizeof (BITMAPFILEHEADER));
	bmpfile.Read(&bmih,sizeof (BITMAPINFOHEADER));

	// Check filetype signature
	if (bmfh.bfType!=19778/*'MB'*/){
		MessageBox(0, "Unable to load BMP File", filename.c_str(), MB_OK);
		xsize=1;
		ysize=1;
		mem=new unsigned char[xsize*ysize*4];
		return;		
	}

	xsize=bmih.biWidth;
	ysize= (bmih.biHeight>0) ? bmih.biHeight : -bmih.biHeight; // absoulte value
	mem=new unsigned char[xsize*ysize*4];

	int BytesPerRow = xsize * bmih.biBitCount / 8;
	BytesPerRow += (4-BytesPerRow%4) % 4;	// int alignment

	int rowStart=0;
	unsigned char* row=new unsigned char[BytesPerRow];

	if(bmih.biBitCount==24){
		for(int y=0;y<ysize;++y){
			rowStart=(ysize-1-y)*BytesPerRow;
			bmpfile.Seek(bmfh.bfOffBits+rowStart);
			bmpfile.Read(row,BytesPerRow);
			for(int x=0;x<xsize;++x){
				mem[(y*xsize+x)*4+0]=row[x*3+2];
				mem[(y*xsize+x)*4+1]=row[x*3+1];
				mem[(y*xsize+x)*4+2]=row[x*3+0];
				mem[(y*xsize+x)*4+3]=255;
			}
		}
	} else {
		unsigned char* pal=new unsigned char[(1 << bmih.biBitCount)*4];
		bmpfile.Read(pal,(1 << bmih.biBitCount)*4);

		for(int y=0;y<ysize;++y){
			rowStart=(ysize-1-y)*BytesPerRow;
			bmpfile.Seek(bmfh.bfOffBits+rowStart);
			bmpfile.Read(row,BytesPerRow);
			for(int x=0;x<xsize;++x){
				int col=row[x];	//assume 8bit
				mem[(y*xsize+x)*4+0]=pal[col*4+2];
				mem[(y*xsize+x)*4+1]=pal[col*4+1];
				mem[(y*xsize+x)*4+2]=pal[col*4+0];
				mem[(y*xsize+x)*4+3]=255;
			}
		}

		delete[] pal;
	}

	delete[] row;
}

void CBitmap::LoadJPG(string filename)
{
	struct jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	FILE *pFile;

	if((pFile = fopen(filename.c_str(), "rb")) == NULL) 
	{
		// Display an error message saying the file was not found, then return NULL
		MessageBox(0, "Unable to load JPG File!", "Error", MB_OK);
		xsize=1;
		ysize=1;
		mem=new unsigned char[xsize*ysize*4];
		return;
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, pFile);

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	int rowSpan = cinfo.image_width * cinfo.num_components;
	xsize = cinfo.image_width;
	ysize = cinfo.image_height;

	unsigned char* tempLine=new unsigned char[rowSpan];
	mem=new unsigned char[xsize*ysize*4];
	
	for(int y=0;y<ysize;++y){
		jpeg_read_scanlines(&cinfo, &tempLine, 1);
		for(int x=0;x<xsize;++x){
			mem[(y*xsize+x)*4+0]=tempLine[x*3];
			mem[(y*xsize+x)*4+1]=tempLine[x*3+1];
			mem[(y*xsize+x)*4+2]=tempLine[x*3+2];
			mem[(y*xsize+x)*4+3]=255;
		}
	}

	delete[] tempLine;
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(pFile);
}

void CBitmap::LoadPCX(string filename)
{
	short num_bytes, i;
	long count;
	BYTE data;
	DWORD bw = 0;

	pcx_header header;
	long lImageSize;
	BYTE *lpBuffer;

	//HANDLE file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CFileHandler file(filename);
	if(!file.FileExists())
	{
		//MessageBox(0, ("Unable to load PCX File " + filename).c_str(), "Error", MB_OK);
		mem = NULL;
		return;
	}

	file.Read(&header, sizeof(header));

	if(header.num_color_planes != 1)
	{
		MessageBox(0, ("Unsuported PCX format: " + filename).c_str(), "Error", MB_OK);
		return;
	}

	long lFileSize = file.FileSize() - sizeof(header);

	long nWidth = header.width + 1;
	long nHeight = header.height + 1;
	long lSize = (long)nWidth * nHeight;
	lImageSize = lSize;

	BYTE *tmpbuf = new BYTE[lFileSize];
	BYTE *svtmpbuf = tmpbuf;

	lpBuffer = new BYTE[lSize];
	if(lpBuffer == NULL || tmpbuf == NULL)
	{
		MessageBox(0, ("Read PCX error: " + filename).c_str(), "Error", MB_OK);
		return;
	}
	file.Read(tmpbuf, lFileSize);
	count = 0;
	while(count < lSize)
	{
		data = *tmpbuf++;
		if((data >= 192) && (data <= 255))
		{
			num_bytes = data - 192;
			data  = *tmpbuf++;
			while(num_bytes-- > 0)
			{
				lpBuffer[count++] = data;
			}
		}
		else
		{
			lpBuffer[count++] = data;
		}
	}
	if(header.num_color_planes == 1)
	{
		while(*tmpbuf++ != 0xc);
		PALETTEENTRY palette[256];
		for(i = 0; i < 256; i++)
		{

			palette[i].peRed   = *tmpbuf++;
			palette[i].peGreen = *tmpbuf++;
			palette[i].peBlue  = *tmpbuf++;
			palette[i].peFlags = 0;
		}

		xsize = nWidth;
		ysize = nHeight;

		mem=new unsigned char[xsize*ysize*4];
		for(int a=0;a<xsize*ysize;++a){
			mem[a*4] = palette[lpBuffer[a]].peRed;
			mem[a*4+1] = palette[lpBuffer[a]].peGreen;
			mem[a*4+2] = palette[lpBuffer[a]].peBlue;
			mem[a*4+3]=255;
		}
	}
	delete[] svtmpbuf;
	delete[] lpBuffer;
}

unsigned int CBitmap::CreateTexture(bool mipmaps)
{
	if(mem==NULL)
		return 0;

	unsigned int texture;

	glGenTextures(1, &texture);			

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	if(mipmaps)
	{
		// create mipmapped texture
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		//glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,xsize, ysize, 0,GL_RGBA, GL_UNSIGNED_BYTE, mem);
		gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	}

	return texture;
}

void CBitmap::CreateAlpha(unsigned char red,unsigned char green,unsigned char blue)
{
	float3 aCol;
	for(int a=0;a<3;++a){
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
		aCol.xyz[a]=cCol/255.0f/numCounted;
	}
	for(int y=0;y<ysize;++y){
		for(int x=0;x<xsize;++x){
			if(mem[(y*xsize+x)*4+0]==red && mem[(y*xsize+x)*4+1]==green && mem[(y*xsize+x)*4+2]==blue){
				mem[(y*xsize+x)*4+0]=unsigned char (aCol.x*255);
				mem[(y*xsize+x)*4+1]=unsigned char (aCol.y*255);
				mem[(y*xsize+x)*4+2]=unsigned char (aCol.z*255);
				mem[(y*xsize+x)*4+3]=0;
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
		aCol.xyz[a]=cCol/255.0f/numCounted;
		cCol/=xsize*ysize;
		colorDif.xyz[a]=newCol[a]-aCol[a];

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
				float nc=float(mem[(y*xsize+x)*4+a])/255.0f+colorDif.xyz[a];
/*				float r=newCol.xyz[a]+(nc-newCol.xyz[a])*spreadMul.xyz[a];*/
				mem[(y*xsize+x)*4+a]=unsigned char(min(255,max(0,nc*255)));
			}
		}
	}
}

void CBitmap::SaveBMP(string filename)
{
	char* buf=new char[xsize*ysize*3];
	for(int y=0;y<ysize;y++){
		for(int x=0;x<xsize;x++){
			buf[((ysize-1-y)*xsize+x)*3+2]=mem[(y*xsize+x)*4+0];
			buf[((ysize-1-y)*xsize+x)*3+1]=mem[(y*xsize+x)*4+1];
			buf[((ysize-1-y)*xsize+x)*3+0]=mem[(y*xsize+x)*4+2];
		}
	}
	BITMAPFILEHEADER bmfh;
	bmfh.bfType=('B')+('M'<<8);
	bmfh.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+ysize*xsize*3;
	bmfh.bfReserved1=0;
	bmfh.bfReserved2=0;
	bmfh.bfOffBits=sizeof(BITMAPINFOHEADER)+sizeof(BITMAPFILEHEADER);
	BITMAPINFOHEADER bmih;
	bmih.biSize=sizeof(BITMAPINFOHEADER);
	bmih.biWidth=xsize;
	bmih.biHeight=ysize;
	bmih.biPlanes=1;
	bmih.biBitCount=24;
	bmih.biCompression=BI_RGB;
	bmih.biSizeImage=0;
	bmih.biXPelsPerMeter=1000;
	bmih.biYPelsPerMeter=1000;
	bmih.biClrUsed=0;
	bmih.biClrImportant=0;
	std::ofstream ofs(filename.c_str(), std::ios::out|std::ios::binary);
	if(ofs.bad() || !ofs.is_open())
		MessageBox(0,"Couldnt save file",filename.c_str(),0);
	ofs.write((char*)&bmfh,sizeof(bmfh));
	ofs.write((char*)&bmih,sizeof(bmih));
	ofs.write((char*)buf,xsize*ysize*3);
	delete[] buf;
}

void CBitmap::SaveJPG(string filename,int quality)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	FILE * outfile;
	if ((outfile = fopen(filename.c_str(), "wb")) == NULL) {
		MessageBox(0,"Error reading jpeg","CBitmap error",0);
		exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	unsigned char* buf=new unsigned char[xsize*ysize*3];
	for(int a=0;a<xsize*ysize;a++){
		buf[a*3]=mem[a*4];
		buf[a*3+1]=mem[a*4+1];
		buf[a*3+2]=mem[a*4+2];
	}

	cinfo.image_width = xsize;
	cinfo.image_height = ysize;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo,quality,false);

	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer[1];	/* pointer to a single row */
	int row_stride;			/* physical row width in buffer */

	row_stride = xsize * 3;	/* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = & buf[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(outfile);
}

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
			bm.mem[(y*bm.xsize+x)*4]=unsigned char(r/4);
			bm.mem[(y*bm.xsize+x)*4+1]=unsigned char(g/4);
			bm.mem[(y*bm.xsize+x)*4+2]=unsigned char(b/4);
			bm.mem[(y*bm.xsize+x)*4+3]=unsigned char(a/4);
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
