#include "StdAfx.h"
// TreeDrawer.cpp: implementation of the CBasicTreeDrawer class.
//
//////////////////////////////////////////////////////////////////////

#include "BasicTreeDrawer.h"
#include "BaseGroundDrawer.h"
#include "Ground.h"
#include "Camera.h"
#include "VertexArray.h"
#include "ReadMap.h"
#include <windows.h>		// Header File For Windows
#include "myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include "Bitmap.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBasicTreeDrawer::CBasicTreeDrawer()
{
	PrintLoadMsg("Loading tree texture");

	lastListClean=0;

	CBitmap TexImage("bitmaps\\gran.bmp");
	TexImage.ReverseYAxis();
	unsigned char gran[1024][512][4];//=new unsigned char[1024][512][4]; 
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<256;x++){
				if(TexImage.mem[(y*256+x)*4]==72 && TexImage.mem[(y*256+x)*4+1]==72){
					gran[y][x][0]=33;
					gran[y][x][1]=54;
					gran[y][x][2]=29;
					gran[y][x][3]=0;
				} else {
					gran[y][x][0]=TexImage.mem[(y*256+x)*4];
					gran[y][x][1]=TexImage.mem[(y*256+x)*4+1];
					gran[y][x][2]=TexImage.mem[(y*256+x)*4+2];
					gran[y][x][3]=255;
				}
			}
		}
	}

	TexImage.Load("bitmaps\\gran2.bmp");
	TexImage.ReverseYAxis();
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<256;x++){
				if(TexImage.mem[(y*256+x)*4]==72 && TexImage.mem[(y*256+x)*4+1]==72){
					gran[y][x+256][0]=33;
					gran[y][x+256][1]=54;
					gran[y][x+256][2]=29;
					gran[y][x+256][3]=0;
				} else {
					gran[y][x+256][0]=TexImage.mem[(y*256+x)*4];
					gran[y][x+256][1]=TexImage.mem[(y*256+x)*4+1];
					gran[y][x+256][2]=TexImage.mem[(y*256+x)*4+2];
					gran[y][x+256][3]=255;
				}
			}
		}
	}

	TexImage.Load("bitmaps\\birch1.bmp");
	TexImage.ReverseYAxis();
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<128;x++){
				if(TexImage.mem[(y*128+x)*4]==72 && TexImage.mem[(y*128+x)*4+1]==72){
					gran[y+256][x][0]=125*0.6f;;
					gran[y+256][x][1]=146*0.7f;;
					gran[y+256][x][2]=82*0.6f;;
					gran[y+256][x][3]=0;
				} else {
					gran[y+256][x][0]=TexImage.mem[(y*128+x)*4]*0.6f;;
					gran[y+256][x][1]=TexImage.mem[(y*128+x)*4+1]*0.7f;;
					gran[y+256][x][2]=TexImage.mem[(y*128+x)*4+2]*0.6f;;
					gran[y+256][x][3]=255;
				}
			}
		}
	}

	TexImage.Load("bitmaps\\birch2.bmp");
	TexImage.ReverseYAxis();
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<128;x++){
				if(TexImage.mem[(y*128+x)*4]==72 && TexImage.mem[(y*128+x)*4+1]==72){
					gran[y+256][x+128][0]=125*0.6f;;
					gran[y+256][x+128][1]=146*0.7f;;
					gran[y+256][x+128][2]=82*0.6f;;
					gran[y+256][x+128][3]=0;
				} else {
					gran[y+256][x+128][0]=TexImage.mem[(y*128+x)*4]*0.6f;;
					gran[y+256][x+128][1]=TexImage.mem[(y*128+x)*4+1]*0.7f;;
					gran[y+256][x+128][2]=TexImage.mem[(y*128+x)*4+2]*0.6f;;
					gran[y+256][x+128][3]=255;
				}
			}
		}
	}

	TexImage.Load("bitmaps\\birch3.bmp");
	TexImage.ReverseYAxis();
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<256;x++){
				if(TexImage.mem[(y*256+x)*4]==72 && TexImage.mem[(y*256+x)*4+1]==72){
					gran[y+256][x+256][0]=125*0.6f;;
					gran[y+256][x+256][1]=146*0.7f;
					gran[y+256][x+256][2]=82*0.6f;;
					gran[y+256][x+256][3]=0;
				} else {
					gran[y+256][x+256][0]=TexImage.mem[(y*256+x)*4]*0.6f;;
					gran[y+256][x+256][1]=TexImage.mem[(y*256+x)*4+1]*0.7f;;
					gran[y+256][x+256][2]=TexImage.mem[(y*256+x)*4+2]*0.6f;;
					gran[y+256][x+256][3]=255;
				}
			}
		}
	}

	// create mipmapped texture
	CreateTreeTex(treetex,gran[0][0],512,1024);
//	delete[] gran;

	treesX=gs->mapx/TREE_SQUARE_SIZE;
	treesY=gs->mapy/TREE_SQUARE_SIZE;
	trees=new TreeSquareStruct[treesX*treesY];

	for(int y=0;y<treesY;y++){
		for(int x=0;x<treesX;x++){
			trees[y*treesX+x].displist=0;
			trees[y*treesX+x].farDisplist=0;
		}
	}
}

CBasicTreeDrawer::~CBasicTreeDrawer()
{
	glDeleteTextures (1, &treetex);

	for(int y=0;y<treesY;y++){
		for(int x=0;x<treesX;x++){
			if(trees[y*treesX+x].displist)
				glDeleteLists(trees[y*treesX+x].displist,1);
			if(trees[y*treesX+x].farDisplist)
				glDeleteLists(trees[y*treesX+x].farDisplist,1);
		}
	}
	delete[] trees;
}

static CVertexArray* va;

static void inline SetArray(float t1,float t2,float3 v)
{
	va->AddVertexT(v,t1,t2);
}

void CBasicTreeDrawer::Draw(float treeDistance,bool drawReflection)
{
	int cx=(int)(camera->pos.x/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	int cy=(int)(camera->pos.z/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	
	int treeSquare=int(treeDistance*2)+1;

	glBindTexture(GL_TEXTURE_2D, treetex);
	int sy=cy-treeSquare;
	if(sy<0)
		sy=0;
	int ey=cy+treeSquare;
	if(ey>=treesY)
		ey=treesY-1;

	for(int y=sy;y<=ey;y++){
		int sx=cx-treeSquare;
		if(sx<0)
			sx=0;
		int ex=cx+treeSquare;
		if(ex>=treesX)
			ex=treesX-1;
		float xtest,xtest2;
		std::vector<CBaseGroundDrawer::fline>::iterator fli;
		for(fli=groundDrawer->left.begin();fli!=groundDrawer->left.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*TREE_SQUARE_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*TREE_SQUARE_SIZE)+TREE_SQUARE_SIZE)));
			if(xtest>xtest2)
				xtest=xtest2;
			xtest=xtest/TREE_SQUARE_SIZE;
			if(xtest>sx)
				sx=xtest;
		}
		for(fli=groundDrawer->right.begin();fli!=groundDrawer->right.end();fli++){
			xtest=((fli->base/SQUARE_SIZE+fli->dir*(y*TREE_SQUARE_SIZE)));
			xtest2=((fli->base/SQUARE_SIZE+fli->dir*((y*TREE_SQUARE_SIZE)+TREE_SQUARE_SIZE)));
			if(xtest<xtest2)
				xtest=xtest2;
			xtest=xtest/TREE_SQUARE_SIZE;
			if(xtest<ex)
				ex=xtest;
		}
		for(int x=sx;x<=ex;x++){/**/
			TreeSquareStruct* tss=&trees[y*treesX+x];

			float3 dif;
			dif.x=camera->pos.x-(x*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
			dif.y=0;
			dif.z=camera->pos.z-(y*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
			float dist=dif.Length();
			dif/=dist;
			
			if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2 && dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance)){//far trees
				tss->lastSeenFar=gs->frameNum;
				if(!tss->farDisplist || dif.dot(tss->viewVector)<0.97){
					va=GetVertexArray();
					va->Initialize();
					tss->viewVector=dif;
					if(!tss->farDisplist)
						tss->farDisplist=glGenLists(1);
					float3 up(0,1,0);
					float3 side=up.cross(dif);

					for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
						TreeStruct* ts=&ti->second;
						if(ts->type<8){
							float3 base(ts->pos);
							float height=MAX_TREE_HEIGHT;
							float width=MAX_TREE_HEIGHT*0.3;

							SetArray(0,0,base+side*width);
							SetArray(0,0.25,base+side*width+float3(0,height,0));
							SetArray(0.5f,0.25,base-side*width+float3(0,height,0));
							SetArray(0.5f,0,base-side*width);
						} else {
							float3 base(ts->pos);
							float height=MAX_TREE_HEIGHT;
							float width=MAX_TREE_HEIGHT*0.3;

							SetArray(0,0.25,base+side*width);
							SetArray(0,0.5,base+side*width+float3(0,height,0));
							SetArray(0.25f,0.5,base-side*width+float3(0,height,0));
							SetArray(0.25f,0.25,base-side*width);
						}
					}
					glNewList(trees[y*treesX+x].farDisplist,GL_COMPILE);
					va->DrawArrayT(GL_QUADS);
					glEndList();
				}
				if(dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance*2-1)){
					float trans=(SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2-dist)/(SQUARE_SIZE*TREE_SQUARE_SIZE);
					glEnable(GL_BLEND);
					glColor4f(1,1,1,trans);
					glAlphaFunc(GL_GREATER,(SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2-dist)/(SQUARE_SIZE*TREE_SQUARE_SIZE*2));
				} else {
					glColor4f(1,1,1,1);			
					glDisable(GL_BLEND);
					glAlphaFunc(GL_GREATER,0.5f);
				}
				glCallList(tss->farDisplist);
			}
			
			if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance){	//midle distance trees
				tss->lastSeen=gs->frameNum;
				if(!tss->displist){
					va=GetVertexArray();
					va->Initialize();
					tss->displist=glGenLists(1);

					for(std::map<int,TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
						TreeStruct* ts=&ti->second;
						if(ts->type<8){
							float3 base(ts->pos);
							float height=MAX_TREE_HEIGHT;
							float width=MAX_TREE_HEIGHT*0.3;

							SetArray(0,0,base+float3(width,0,0));
							SetArray(0,0.25,base+float3(width,height,0));
							SetArray(0.5f,0.25,base+float3(-width,height,0));
							SetArray(0.5f,0,base+float3(-width,0,0));

							SetArray(0,0,base+float3(0,0,width));
							SetArray(0,0.25,base+float3(0,height,width));
							SetArray(0.5f,0.25,base+float3(0,height,-width));
							SetArray(0.5f,0,base+float3(0,0,-width));

							width*=1.2f;
							SetArray(0.5,0,base+float3(width,height*0.25,0));
							SetArray(0.5,0.25,base+float3(0,height*0.25,-width));
							SetArray(1,0.25,base+float3(-width,height*0.25,0));
							SetArray(1,0,base+float3(0,height*0.25,width));
						} else {
							float3 base(ts->pos);
							float height=MAX_TREE_HEIGHT;
							float width=MAX_TREE_HEIGHT*0.3;

							SetArray(0,0.25,base+float3(width,0,0));
							SetArray(0,0.5,base+float3(width,height,0));
							SetArray(0.25f,0.5,base+float3(-width,height,0));
							SetArray(0.25f,0.25,base+float3(-width,0,0));

							SetArray(0.25f,0.25,base+float3(0,0,width));
							SetArray(0.25f,0.5,base+float3(0,height,width));
							SetArray(0.5f,0.5,base+float3(0,height,-width));
							SetArray(0.5f,0.25,base+float3(0,0,-width));

							width*=1.2f;
							SetArray(0.5,0.25,base+float3(width,height*0.3,0));
							SetArray(0.5,0.5,base+float3(0,height*0.3,-width));
							SetArray(1,0.5,base+float3(-width,height*0.3,0));
							SetArray(1,0.25,base+float3(0,height*0.3,width));
						}
					}
					glNewList(tss->displist,GL_COMPILE);
					va->DrawArrayT(GL_QUADS);
					glEndList();
				}
				glColor4f(1,1,1,1);			
				glDisable(GL_BLEND);
				glAlphaFunc(GL_GREATER,0.5f);
				glCallList(tss->displist);
			}
		}
	}

	int startClean=lastListClean*20%(treesX*treesY);
	lastListClean=gs->frameNum;
	int endClean=gs->frameNum*20%(treesX*treesY);

	if(startClean>endClean){
		for(int a=startClean;a<treesX*treesY;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
		for(int a=0;a<endClean;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
	} else {
		for(int a=startClean;a<endClean;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
	}
	glDisable(GL_BLEND);
}

void CBasicTreeDrawer::Update()
{

}

void CBasicTreeDrawer::CreateTreeTex(unsigned int& texnum, unsigned char *data, int xsize, int ysize)
{
	glGenTextures(1, &texnum);
	glBindTexture(GL_TEXTURE_2D, texnum);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	int mipnum=0;
	glTexImage2D(GL_TEXTURE_2D,mipnum,GL_RGBA8 ,xsize, ysize,0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	while(xsize!=1 || ysize!=1){		
		mipnum++;
		if(xsize!=1)
			xsize/=2;
		if(ysize!=1)
			ysize/=2;
		for(int y=0;y<ysize;++y){
			for(int x=0;x<xsize;++x){
/*				for(int a=0;a<3;a++){
					int temp=0;
					int num=0;
					if(data[(y*2*xsize*2+x*2)*4+3]){
						temp+=data[(y*2*xsize*2+x*2)*4+a];
						num++;
					}
					if(data[((y*2+1)*xsize*2+x*2)*4+3]){
						temp+=data[((y*2+1)*xsize*2+x*2)*4+a];
						num++;
					}
					if(data[((y*2)*xsize*2+x*2+1)*4+3]){
						temp+=data[((y*2)*xsize*2+x*2+1)*4+a];
						num++;
					}
					if(data[((y*2+1)*xsize*2+x*2+1)*4+3]){
						temp+=data[((y*2+1)*xsize*2+x*2+1)*4+a];
						num++;
					}
					if(num>1)
						temp/=num;
					data[(y*xsize+x)*4+a]=temp;
				}
*/			data[(y*xsize+x)*4+0]=(data[(y*2*xsize*2+x*2)*4+0]+data[((y*2+1)*xsize*2+x*2)*4+0]+data[(y*2*xsize*2+x*2+1)*4+0]+data[((y*2+1)*xsize*2+x*2+1)*4+0])/4;
				data[(y*xsize+x)*4+1]=(data[(y*2*xsize*2+x*2)*4+1]+data[((y*2+1)*xsize*2+x*2)*4+1]+data[(y*2*xsize*2+x*2+1)*4+1]+data[((y*2+1)*xsize*2+x*2+1)*4+1])/4;
				data[(y*xsize+x)*4+2]=(data[(y*2*xsize*2+x*2)*4+2]+data[((y*2+1)*xsize*2+x*2)*4+2]+data[(y*2*xsize*2+x*2+1)*4+2]+data[((y*2+1)*xsize*2+x*2+1)*4+2])/4;
				data[(y*xsize+x)*4+3]=(data[(y*2*xsize*2+x*2)*4+3]+data[((y*2+1)*xsize*2+x*2)*4+3]+data[(y*2*xsize*2+x*2+1)*4+3]+data[((y*2+1)*xsize*2+x*2+1)*4+3])/4;
				if(data[(y*xsize+x)*4+3]>=127){
					data[(y*xsize+x)*4+3]=255;
				} else {
					data[(y*xsize+x)*4+3]=0;
				}
			}
		}
		glTexImage2D(GL_TEXTURE_2D,mipnum,GL_RGBA8 ,xsize, ysize,0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
}

void CBasicTreeDrawer::ResetPos(const float3& pos)
{
	int x=pos.x/TREE_SQUARE_SIZE/SQUARE_SIZE;
	int y=pos.z/TREE_SQUARE_SIZE/SQUARE_SIZE;
	int a=y*treesX+x;
	if(trees[a].displist){
		glDeleteLists(trees[a].displist,1);
		trees[a].displist=0;
	}
	if(trees[a].farDisplist){
		glDeleteLists(trees[a].farDisplist,1);
		trees[a].farDisplist=0;
	}
}

void CBasicTreeDrawer::AddTree(int type, float3 pos, float size)
{
	TreeStruct ts;
	ts.pos=pos;
	ts.type=type;
	int hash=(int)pos.x+((int)(pos.z))*20000;
	int square=((int)pos.x)/(SQUARE_SIZE*TREE_SQUARE_SIZE)+((int)pos.z)/(SQUARE_SIZE*TREE_SQUARE_SIZE)*treesX;
	trees[square].trees[hash]=ts;
	ResetPos(pos);
}

void CBasicTreeDrawer::DeleteTree(float3 pos)
{
	int hash=(int)pos.x+((int)(pos.z))*20000;
	int square=((int)pos.x)/(SQUARE_SIZE*TREE_SQUARE_SIZE)+((int)pos.z)/(SQUARE_SIZE*TREE_SQUARE_SIZE)*treesX;

	trees[square].trees.erase(hash);

	ResetPos(pos);
}
