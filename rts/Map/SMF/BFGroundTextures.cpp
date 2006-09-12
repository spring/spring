#include "StdAfx.h"
#include "BFGroundTextures.h"
#include "FileSystem/FileHandler.h"
#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include <GL/glu.h>
#include "mapfile.h"
#include "Platform/errorhandler.h"
#include "SmfReadMap.h"
#include "mmgr.h"

using namespace std;

CBFGroundTextures::CBFGroundTextures(CSmfReadMap *rm)
{
	CFileHandler *ifs = rm->ifs;
	map = rm;

	numBigTexX=gs->mapx/128;
	numBigTexY=gs->mapy/128;

	MapHeader* header=&map->header;
	ifs->Seek(header->tilesPtr);

	tileSize = header->tilesize;
	
	MapTileHeader tileHeader;
	READPTR_MAPTILEHEADER(tileHeader,ifs);

	tileMap = new int[(header->mapx*header->mapy)/16];
	tiles = new char[tileHeader.numTiles*SMALL_TILE_SIZE];
	int curTile=0;

	for(int a=0;a<tileHeader.numTileFiles;++a){
		int size;
		ifs->Read(&size,4);
		size = swabdword(size);
		string name;

		while(true){
			char ch;
			ifs->Read(&ch,1);
			/* char, no swab */
			if(ch==0)
				break;
			name+=ch;
		}
		name=string("maps/")+name;

		CFileHandler tileFile(name);
		if(!tileFile.FileExists()){
			logOutput.Print("Couldnt find tile file %s",name.c_str());
			memset(&tiles[curTile*SMALL_TILE_SIZE],0xaa,size*SMALL_TILE_SIZE);
			curTile+=size;
			continue;
		}
		TileFileHeader tfh;
		READ_TILEFILEHEADER(tfh,tileFile);

		if(strcmp(tfh.magic,"spring tilefile")!=0 || tfh.version!=1 || tfh.tileSize!=32 || tfh.compressionType!=1){
			char t[500];
			sprintf(t,"Error couldnt open tile file %s",name.c_str());
			handleerror(0,t,"Error when reading tile file",0);
			exit(0);
		}

		for(int b=0;b<size;++b){
			tileFile.Read(&tiles[curTile*SMALL_TILE_SIZE],SMALL_TILE_SIZE);
			curTile++;
		}
	}
	
	int count = (header->mapx*header->mapy)/16;

	ifs->Read(tileMap, count*sizeof(int));
	
	for (int i = 0; i < count; i++) {
		tileMap[i] = swabdword(tileMap[i]);
	}

	tileMapXSize = header->mapx/4;
	tileMapYSize = header->mapy/4;

	squares=new GroundSquare[numBigTexX*numBigTexY];

	for(int y=0;y<numBigTexY;++y){
		for(int x=0;x<numBigTexX;++x){
			GroundSquare* square=&squares[y*numBigTexX+x];
			square->texLevel=1;
			square->lastUsed=-100;

			LoadSquare(x,y,2);
		}
	}
	inRead=false;
}

CBFGroundTextures::~CBFGroundTextures(void)
{
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
		dy=max(0.0f,float(fabs(dy)-64.f*SQUARE_SIZE));
		for(int x=0;x<numBigTexX;++x){
			GroundSquare* square=&squares[y*numBigTexX+x];

			float dx=cam2->pos.x - x*128*SQUARE_SIZE-64*SQUARE_SIZE;
			dx=max(0.0f,float(fabs(dx)-64.f*SQUARE_SIZE));
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
					wantedNew=(int) wantedLevel;
				}
			}
			if(inRead && x==readX && y==readY){
					currentReadWantedLevel=(int) wantedLevel;
			}
			if(square->texLevel!=(int)wantedLevel){
				glDeleteTextures(1,&square->texture);
				LoadSquare(x,y,(int)wantedLevel);
			}
		}
	}
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
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, size, size, 0, size*size/2, buf);

	delete[] buf;
}
