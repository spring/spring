#include "StdAfx.h"
#include "MuzzleFlame.h"
#include "Camera.h"
#include "myGL.h"
#include "VertexArray.h"
#include "ProjectileHandler.h"
#include "GlobalStuff.h"
#include "SyncTracer.h"
//#include "mmgr.h"

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
		float xmod=0.125+(float(int(tex%6)))/16;
		float ymod=(int(tex/6))/16.0;

		float drawsize=(age+8)/3.0;
		float3 interPos(pos+randSmokeDir[a]*(a+2)*age/12.0);
		va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,xmod,ymod,col);
		va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,xmod+1.0/16,ymod,col);
		va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,xmod+1.0/16,ymod+1.0/16,col);
		va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,xmod,ymod+1.0/16,col);

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
			va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,0.25,0.25,col);
			va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,0.5,0.25,col);
			va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,0.5,0.5,col);
			va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,0.25,0.5,col);
		}
	}
}

