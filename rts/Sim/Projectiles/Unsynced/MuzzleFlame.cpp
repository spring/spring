#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "MuzzleFlame.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"


CR_BIND_DERIVED(CMuzzleFlame, CProjectile, (float3(0,0,0),float3(0,0,0),float3(0,0,0),0));

CR_REG_METADATA(CMuzzleFlame,(
	CR_SERIALIZER(creg_Serialize), // randSmokeDir
	CR_MEMBER(dir),
	CR_MEMBER(size),
	CR_MEMBER(age),
	CR_MEMBER(numFlame),
	CR_MEMBER(numSmoke),
	CR_MEMBER(randSmokeDir),
	CR_RESERVED(8)
	));

void CMuzzleFlame::creg_Serialize(creg::ISerializer& s)
{
//	s.Serialize(randSmokeDir, numSmoke*sizeof(float3));
}

CMuzzleFlame::CMuzzleFlame(const float3& pos, const float3& speed, const float3& dir, float size GML_PARG_C):
	CProjectile(pos, speed, 0, false, false, false GML_PARG_P),
	dir(dir),
	size(size),
	age(0)
{
	this->pos -= dir * size * 0.2f;
	checkCol = false;
	castShadow = true;
	numFlame = 1 + (int)(size * 3);
	numSmoke = 1 + (int)(size * 5);
//	randSmokeDir=new float3[numSmoke];
	randSmokeDir.resize(numSmoke);

	for (int a = 0; a < numSmoke; ++a) {
		randSmokeDir[a] = dir + gu->usRandFloat() * 0.4f;
	}
}

CMuzzleFlame::~CMuzzleFlame(void)
{
//	delete[] randSmokeDir;
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
	float modAge=fastmath::sqrt(static_cast<float>(age+2));

	va->EnlargeArrays(numSmoke*8,0,VA_SIZE_TC);

	for (int a = 0; a < numSmoke; ++a) { //! CAUTION: loop count must match EnlargeArrays above
		int tex = a % ph->smoketex.size();
		//float xmod=0.125f+(float(int(tex%6)))/16;
		//float ymod=(int(tex/6))/16.0f;

		float drawsize=modAge*3;
		float3 interPos(pos+randSmokeDir[a]*(a+2)*modAge*0.4f);
		float fade=std::max(0.f, std::min(1.f, (1-alpha)*(20+a)*0.1f));

		col[0]=(unsigned char) (180*alpha*fade);
		col[1]=(unsigned char) (180*alpha*fade);
		col[2]=(unsigned char) (180*alpha*fade);
		col[3]=(unsigned char) (alpha*255*fade);

		va->AddVertexQTC(interPos-camera->right*drawsize-camera->up*drawsize,ph->smoketex[tex].xstart,ph->smoketex[tex].ystart,col);
		va->AddVertexQTC(interPos+camera->right*drawsize-camera->up*drawsize,ph->smoketex[tex].xend,ph->smoketex[tex].ystart,col);
		va->AddVertexQTC(interPos+camera->right*drawsize+camera->up*drawsize,ph->smoketex[tex].xend,ph->smoketex[tex].yend,col);
		va->AddVertexQTC(interPos-camera->right*drawsize+camera->up*drawsize,ph->smoketex[tex].xstart,ph->smoketex[tex].yend,col);

		if(fade<1){
			float ifade= 1-fade;
			col[0]=(unsigned char)(ifade*255);
			col[1]=(unsigned char)(ifade*255);
			col[2]=(unsigned char)(ifade*255);
			col[3]=(unsigned char)(1);

			va->AddVertexQTC(interPos-camera->right*drawsize-camera->up*drawsize,ph->muzzleflametex.xstart,ph->muzzleflametex.ystart,col);
			va->AddVertexQTC(interPos+camera->right*drawsize-camera->up*drawsize,ph->muzzleflametex.xend ,ph->muzzleflametex.ystart,col);
			va->AddVertexQTC(interPos+camera->right*drawsize+camera->up*drawsize,ph->muzzleflametex.xend ,ph->muzzleflametex.yend ,col);
			va->AddVertexQTC(interPos-camera->right*drawsize+camera->up*drawsize,ph->muzzleflametex.xstart,ph->muzzleflametex.yend ,col);
		}
	}
}


