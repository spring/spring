#include "StdAfx.h"
#include "BFGroundTextures.h"
#include "FileHandler.h"
#include "myGL.h"
#include "jpeglib.h"
#include "Camera.h"
#include "InfoConsole.h"
#include <GL/glu.h>
#include "sm2header.h"
#include <algorithm>
using namespace std;
//#include "mmgr.h"

CBFGroundTextures* groundTextures=0;

CBFGroundTextures::CBFGroundTextures(CFileHandler* ifs)
{
	numBigTexX=gs->mapx/128;
	numBigTexY=gs->mapy/128;

	//SetupJpeg();

	//textureOffsets=new int[numBigTexX*numBigTexY*4*3+1];
	//ifs->Read(textureOffsets,numBigTexX*numBigTexY*4*3*4+4);

	//jpegBuffer=new unsigned char[textureOffsets[numBigTexX*numBigTexY*4*3]];
	//ifs->Read(jpegBuffer,textureOffsets[numBigTexX*numBigTexY*4*3]);
	ifs->Seek(0);
	SM2Header header;

	ifs->Read(&header, sizeof(SM2Header));

	tileSize = header.tilesize;
	tileMap = new int[(header.xsize*header.ysize)/16];
	ifs->Seek(header.tilemapPtr);
	ifs->Read(tileMap, ((header.xsize*header.ysize)/16)*sizeof(int));

	tiles = new char[header.numtiles*SMALL_TILE_SIZE];
	ifs->Seek(header.tilesPtr);
	ifs->Read(tiles, header.numtiles*SMALL_TILE_SIZE);
	
	tileMapXSize = header.xsize/4;
	tileMapYSize = header.ysize/4;

	squares=new GroundSquare[numBigTexX*numBigTexY];

	for(int y=0;y<numBigTexY;++y){
		for(int x=0;x<numBigTexX;++x){
			GroundSquare* square=&squares[y*numBigTexX+x];
			square->texLevel=1;
			square->lastUsed=-100;

			LoadSquare(x,y,2);
		}
	}
	//readBuffer=new unsigned char[1024*1024*4];
	//readTempLine=new unsigned char[1024*4];
	inRead=false;
}

CBFGroundTextures::~CBFGroundTextures(void)
{
	//AbortRead();
	//jpeg_destroy_decompress(&cinfo);

	//delete[] readBuffer;
	//delete[] readTempLine;
	//delete[] textureOffsets;
	//delete[] jpegBuffer;
	delete[] squares;

	delete[] tileMap;
	delete[] tiles;
}


void CBFGroundTextures::SetTexture(int x, int y)
{
	GroundSquare* square=&squares[y*numBigTexX+x];
	glBindTexture(GL_TEXTURE_2D,square->texture);
	square->lastUsed=gs->frameNum;
}

void CBFGroundTextures::DrawUpdate(void)
{
	float maxDif=0;
	float totalDif=0;
	int maxX;
	int maxY;
	int wantedNew;
	int currentReadWantedLevel;

	for(int y=0;y<numBigTexY;++y){
		float dy=cam2->pos.z - y*128*SQUARE_SIZE-64*SQUARE_SIZE;
		dy=max(0.,fabs(dy)-64*SQUARE_SIZE);
		for(int x=0;x<numBigTexX;++x){
			GroundSquare* square=&squares[y*numBigTexX+x];

			float dx=cam2->pos.x - x*128*SQUARE_SIZE-64*SQUARE_SIZE;
			dx=max(0.,fabs(dx)-64*SQUARE_SIZE);
			float dist=sqrt(dx*dx+dy*dy);

			if(square->lastUsed<gs->frameNum-60)
				dist=8000;

			float wantedLevel=dist/1000;
			if(wantedLevel>2.5)
				wantedLevel=2.5;
			if(wantedLevel<square->texLevel-1)
				wantedLevel=square->texLevel-1;

			float dif=square->texLevel+0.5-wantedLevel;
			if(dif<0)
				dif*=-0.5;

			if((int)wantedLevel!=square->texLevel){
				if(dist<8)
					dif+=5;
				totalDif+=dif;
				if(dif>maxDif){
					maxDif=dif;
					maxX=x;
					maxY=y;
					wantedNew=wantedLevel;
				}
			}
			if(inRead && x==readX && y==readY){
					currentReadWantedLevel=wantedLevel;
			}
			if(square->texLevel!=(int)wantedLevel){
				glDeleteTextures(1,&square->texture);
				LoadSquare(x,y,wantedLevel);
			}
		}
	}
	//if(inRead && fabsf(currentReadWantedLevel-readLevel)>=fabsf(readSquare->texLevel-readLevel)){
	//	inRead=false;
	//	AbortRead();
	//}
	//
	//if(!inRead && maxDif>0){
	//	readX=maxX;
	//	readY=maxY;
	//	readSquare=&squares[readY*numBigTexX+readX];
	//	readLevel=wantedNew;
	//	inRead=true;
	//	readProgress=0;
	//}

	//if(inRead){
	//	ReadSlow(sqrt(totalDif)+1);
	//}
}

int tileoffset[] = {0, 512, 640, 672};

void CBFGroundTextures::LoadSquare(int x, int y, int level)
{
	GroundSquare* square=&squares[y*numBigTexX+x];
	square->texLevel=level;

	int size=1024>>level;
	//int hsize=size/2;

	char* buf=new char[size*size/2];

	int tilepitch = 64/(1<<level);
	int bufpitch = tilepitch*32;

	int numblocks = 8/(1<<level);

	for(int y1=0; y1<32; y1++)
	{
		for(int x1=0; x1<32; x1++)
		{
			char *tile = &tiles[tileMap[(x1+x*32)+(y1+y*32)*tileMapXSize]*SMALL_TILE_SIZE + tileoffset[level]];
			for(int yt=0; yt<numblocks; yt++)
			{
				for(int xt=0; xt<numblocks; xt++)
				{
					char *sbuf = &tile[(xt+yt*numblocks)*8];
					char *dbuf = &buf[(x1*numblocks+xt + (y1*numblocks+yt)*(numblocks*32))*8];
					for(int i=0; i<8; i++)
					{
						dbuf[i] = sbuf[i];
					}
				}
			}
		}
	}

	glGenTextures(1, &square->texture);
	glBindTexture(GL_TEXTURE_2D, square->texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	//glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	//gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,size, size, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, size, size, 0, size*size/2, buf);


	//for(int y1=0; y1<32; y1++)
	//{
	//	for(int x1=0; x1<32; x1++)
	//	{

	//		char *tile = &tiles[tileMap[(x1+x*32)+(y1+y*32)*tileMapYSize]*TILE_SIZE + 672];
	//		glCompressedTexSubImage2DARB(GL_TEXTURE_2D,0,x1*4,y1*4, 4,4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8, tile);
	//	}
	//}


	delete[] buf;
}

void CBFGroundTextures::ReadSlow(int speed)
{
//	info->AddLine("Read speed %i",speed);

	int size=1024>>readLevel;
	int hsize=size/2;

	speed=speed<<readLevel;			//read faster on smaller texture sizes
	while(speed){
		if(readProgress<80){
			int tx=(readProgress/20)&1;
			int ty=(readProgress/40);
			int num=readProgress-tx*20-ty*40;
			if(num==0){
				int bufnum=numBigTexX*numBigTexY*4*readLevel + (readY*2+ty)*numBigTexX*2 + readX*2+tx;
				SetJpegMemSource(&jpegBuffer[textureOffsets[bufnum]],textureOffsets[bufnum+1]-textureOffsets[bufnum]);

				jpeg_read_header(&cinfo, TRUE);
				jpeg_start_decompress(&cinfo);
			} else {
				unsigned char* buffer=&readBuffer[(ty*hsize*size+tx*hsize)*4];
				for(int y=(num-1)*hsize/19;y<(num)*hsize/19;++y){
					jpeg_read_scanlines(&cinfo, &readTempLine, 1);
					for(int x=0;x<cinfo.image_width;++x){
						buffer[(y*size+x)*4+0]=readTempLine[x*3];
						buffer[(y*size+x)*4+1]=readTempLine[x*3+1];
						buffer[(y*size+x)*4+2]=readTempLine[x*3+2];
						buffer[(y*size+x)*4+3]=255;
					}
				}
				if(num==19){
					jpeg_finish_decompress(&cinfo);
				}
			}
			readProgress++;
		} else {
			glDeleteTextures(1,&readSquare->texture);
			glGenTextures(1, &readSquare->texture);
			glBindTexture(GL_TEXTURE_2D, readSquare->texture);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, readBuffer);
			readSquare->texLevel=readLevel;
			inRead=false;
			return;
		}
		--speed;
	}
}

void CBFGroundTextures::ReadJpeg(int bufnum, unsigned char* buffer, int xstep)
{
	SetJpegMemSource(&jpegBuffer[textureOffsets[bufnum]],textureOffsets[bufnum+1]-textureOffsets[bufnum]);

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	int rowSpan = cinfo.image_width * cinfo.num_components;

	unsigned char* tempLine=new unsigned char[rowSpan];
	
	for(int y=0;y<cinfo.image_height;++y){
		jpeg_read_scanlines(&cinfo, &tempLine, 1);
		for(int x=0;x<cinfo.image_width;++x){
			buffer[(y*xstep+x)*4+0]=tempLine[x*3];
			buffer[(y*xstep+x)*4+1]=tempLine[x*3+1];
			buffer[(y*xstep+x)*4+2]=tempLine[x*3+2];
			buffer[(y*xstep+x)*4+3]=255;
		}
	}

	delete[] tempLine;
	jpeg_finish_decompress(&cinfo);
}

static void InitSource (j_decompress_ptr cinfo)
{
#ifndef linux
  cinfo->src->start_of_file = TRUE;
#else
#warning cinfo->src->start_of_file = TRUE disabled, should be OK without
#endif
}

static void* JpegBufferMem;
static int JpegBufferLength;
static boolean FillInputBuffer (j_decompress_ptr cinfo)
{
	cinfo->src->bytes_in_buffer=JpegBufferLength;
	cinfo->src->next_input_byte=(JOCTET*)JpegBufferMem;

	return true;
}

static void SkipInputData (j_decompress_ptr cinfo, long num_bytes)
{
	MessageBox(0,"error skipping in ground jpeg","",0);
	cinfo->src->next_input_byte += (size_t) num_bytes;
	cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
	//	exit(0);
}

static void TermSource (j_decompress_ptr cinfo)
{
}

static boolean ResyncToRestart(j_decompress_ptr cinfo, int desired)
{
	MessageBox(0,"resync to restart","",0);
	return true;
}

void CBFGroundTextures::SetJpegMemSource(void* inbuffer,int length)
{
	static struct jpeg_source_mgr* memsrc=cinfo.src;

	memsrc->bytes_in_buffer=0;

	JpegBufferMem=inbuffer;
	JpegBufferLength=length;
}


void CBFGroundTextures::SetupJpeg(void)
{
	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, (FILE*)1);

	static struct jpeg_source_mgr* memsrc=cinfo.src;
  memsrc->init_source = InitSource;
  memsrc->fill_input_buffer = FillInputBuffer;
  memsrc->skip_input_data = SkipInputData;
  memsrc->resync_to_restart = ResyncToRestart; 
  memsrc->term_source = TermSource;
}


void CBFGroundTextures::AbortRead(void)
{
	jpeg_abort_decompress(&cinfo);
}

