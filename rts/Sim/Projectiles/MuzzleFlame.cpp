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
	this->pos-=dir*size*0.2f;
	checkCol=false;
	castShadow=true;
	numFlame=1+(int)(size*3);
	numSmoke=1+(int)(size*5);
	randSmokeDir=SAFE_NEW float3[numSmoke];

	PUSH_CODE_MODE;
	ENTER_MIXED;
	for(int a=0;a<numSmoke;++a){
		randSmokeDir[a]=dir+gu->usRandFloat()*0.4f;
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
	float alpha=std::max(0.f,1-age/(4+size*30));
	float modAge=sqrtf(age+2);

	for(int a=0;a<numSmoke;++a){
		int tex=a%12;
		//float xmod=0.125f+(float(int(tex%6)))/16;
		//float ymod=(int(tex/6))/16.0f;

		float drawsize=modAge*3;
		float3 interPos(pos+randSmokeDir[a]*(a+2)*modAge*0.4f);
		float fade=std::max(0.f, std::min(1.f, (1-alpha)*(20+a)*0.1f));

		col[0]=(unsigned char) (180*alpha*fade);
		col[1]=(unsigned char) (180*alpha*fade);
		col[2]=(unsigned char) (180*alpha*fade);
		col[3]=(unsigned char) (alpha*255*fade);

		va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,ph->smoketex[tex].xstart,ph->smoketex[tex].ystart,col);
		va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,ph->smoketex[tex].xend,ph->smoketex[tex].ystart,col);
		va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,ph->smoketex[tex].xend,ph->smoketex[tex].yend,col);
		va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,ph->smoketex[tex].xstart,ph->smoketex[tex].yend,col);

		if(fade<1){
			float ifade= 1-fade;
			col[0]=(unsigned char)(ifade*255);
			col[1]=(unsigned char)(ifade*255);
			col[2]=(unsigned char)(ifade*255);
			col[3]=(unsigned char)(1);

			va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,ph->explotex.xstart,ph->explotex.ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,ph->explotex.xend ,ph->explotex.ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,ph->explotex.xend ,ph->explotex.yend ,col);
			va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,ph->explotex.xstart,ph->explotex.yend ,col);
		}
	}
}

