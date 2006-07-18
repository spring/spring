#include "StdAfx.h"
#include "MuzzleFlame.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "SyncTracer.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CMuzzleFlame::CMuzzleFlame(const float3& pos,const float3& speed,const float3& dir,float size)
: CProjectile(pos,speed,0),
	size(size),
	dir(dir),
	age(0)
{
	this->pos-=dir*size*0.2;
	checkCol=false;
	castShadow=true;
	numFlame=1+(int)(size*3);
	numSmoke=1+(int)(size*5);
	randSmokeDir=new float3[numSmoke];

	PUSH_CODE_MODE;
	ENTER_MIXED;
	for(int a=0;a<numSmoke;++a){
		randSmokeDir[a]=dir+gu->usRandFloat()*0.4;
	}
	POP_CODE_MODE;
#ifdef TRACE_SYNC
	tracefile << "New Muzzle: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << size <<  "\n";
#endif

}

CMuzzleFlame::~CMuzzleFlame(void)
{
	delete[] randSmokeDir;
}

void CMuzzleFlame::Update(void)
{
	age++;
	if(age>4+size*30){
		deleteMe=true;
	}
	pos+=speed;
}

void CMuzzleFlame::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	float alpha=max(0.f,1-age/(4+size*30));
	col[0]=(unsigned char) (200*alpha);
	col[1]=(unsigned char) (200*alpha);
	col[2]=(unsigned char) (200*alpha);
	col[3]=(unsigned char) (alpha*255);

	for(int a=0;a<numSmoke;++a){
		int tex=a%12;
		//float xmod=0.125+(float(int(tex%6)))/16;
		//float ymod=(int(tex/6))/16.0;

		float drawsize=(age+8)/3.0;
		float3 interPos(pos+randSmokeDir[a]*(a+2)*age/12.0);
		va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,ph->smoketex[a].xstart,ph->smoketex[a].ystart,col);
		va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,ph->smoketex[a].xend,ph->smoketex[a].ystart,col);
		va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,ph->smoketex[a].xend,ph->smoketex[a].yend,col);
		va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,ph->smoketex[a].xstart,ph->smoketex[a].yend,col);

	}


	if(age<6+size*2){
		inArray=true;
		unsigned char col[4];
		float alpha=(1-age/(6+size*2))*(1-age/(6+size*2));
		col[0]=(unsigned char) (255*alpha);
		col[1]=(unsigned char) (255*alpha);
		col[2]=(unsigned char) (255*alpha);
		col[3]=1;

		float curAge=age/(6.0+size*2.0);
		for(int a=0;a<numFlame;++a){
			float drawsize=(age+10)/4.0;
			drawsize+=max(0.,(0.25-fabs(curAge-(a/float(numFlame))))*0.5);
			float3 interPos(pos+dir*(a+2)*age/4);
			va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,ph->explotex.xstart,ph->explotex.ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,ph->explotex.xend,ph->explotex.ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,ph->explotex.xend,ph->explotex.yend,col);
			va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,ph->explotex.xstart,ph->explotex.yend,col);
		}
	}
}

