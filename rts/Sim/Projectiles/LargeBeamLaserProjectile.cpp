#include "StdAfx.h"
#include "LargeBeamLaserProjectile.h"
#include "Sim/Units/Unit.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CLargeBeamLaserProjectile::CLargeBeamLaserProjectile(const float3& startPos,const float3& endPos,const float3& color, const float3& color2,CUnit* owner,float thickness, float corethickness,
													 float flaresize, float tilelength, float scrollspeed, float pulseSpeed, AtlasedTexture *beamtex, AtlasedTexture *side)
: CProjectile((startPos+endPos)*0.5,ZeroVector,owner),
	startPos(startPos),
	endPos(endPos),
	thickness(thickness),
	corethickness(corethickness),
	flaresize(flaresize),
	tilelength(tilelength),
	scrollspeed(scrollspeed),
	pulseSpeed(pulseSpeed)
{
	checkCol=false;
	useAirLos=true;

	this->beamtex = *beamtex;
	this->side = *side;

	SetRadius(pos.distance(endPos));

	//midtexx = ph->laserendtex.xstart + (ph->laserendtex.xend-ph->laserendtex.xstart)*0.5;
	corecolstart[0]=(unsigned char)(color2.x*255);
	corecolstart[1]=(unsigned char)(color2.y*255);
	corecolstart[2]=(unsigned char)(color2.z*255);
	corecolstart[3]=1;
	/*corecolend[0]=(unsigned char)(color2.x*endAlpha);
	corecolend[1]=(unsigned char)(color2.y*endAlpha);
	corecolend[2]=(unsigned char)(color2.z*endAlpha);
	corecolend[3]=1;*/
	kocolstart[0]=(unsigned char)(color.x*255);
	kocolstart[1]=(unsigned char)(color.y*255);
	kocolstart[2]=(unsigned char)(color.z*255);
	kocolstart[3]=1;
	/*kocolend[0]=(unsigned char)(color.x*endAlpha);
	kocolend[1]=(unsigned char)(color.y*endAlpha);
	kocolend[2]=(unsigned char)(color.z*endAlpha);
	kocolend[3]=1;*/

	//tilelength = 200;
	//scrollspeed = 5;
	//pulseSpeed = 1;
}

CLargeBeamLaserProjectile::~CLargeBeamLaserProjectile(void)
{
}


void CLargeBeamLaserProjectile::Update(void)
{
	deleteMe=true;
}

void CLargeBeamLaserProjectile::Draw(void)
{
	inArray=true;
	float3 dif(pos-camera->pos);
	float camDist=dif.Length();
	dif/=camDist;
	float3 dir=endPos-startPos;
	float beamlength = dir.Length();
	dir.Normalize();
	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 dir2(dif.cross(dir1));

	float size=thickness;
	float coresize=size*corethickness;

	float3 pos1=startPos;
	float3 pos2=endPos;

	float length = (endPos-startPos).Length();
	float starttex=(gu->modGameTime)*scrollspeed;
	starttex = 1-(starttex - (int)starttex);


	//beamtex = ph->textureAtlas->GetTexture("largebeam");
	float texxsize = beamtex.xend-beamtex.xstart;
	AtlasedTexture tex = beamtex;

	float polylength = (tex.xend-beamtex.xstart)*(1/texxsize)*tilelength;


	if(polylength*(1-starttex)>beamlength)  //beam short enough to be drawn by one polygon
	{
		pos2=endPos;

		//draw laser start
		tex.xstart = beamtex.xstart + starttex*((beamtex.xend-beamtex.xstart));

		va->AddVertexTC(pos1-dir1*size,tex.xstart,tex.ystart,		kocolstart);
		va->AddVertexTC(pos1+dir1*size,tex.xstart,tex.yend,			kocolstart);
		va->AddVertexTC(pos2+dir1*size,tex.xend,tex.yend,			kocolstart);
		va->AddVertexTC(pos2-dir1*size,tex.xend,tex.ystart,			kocolstart);
		va->AddVertexTC(pos1-dir1*coresize,tex.xstart,tex.ystart,	corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,tex.xstart,tex.yend,	corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,tex.xend,tex.yend,		corecolstart);
		va->AddVertexTC(pos2-dir1*coresize,tex.xend,tex.ystart,		corecolstart);
	}
	else  //beam longer than one polygon
	{
		pos2=pos1+dir*polylength*(1-starttex);

		//draw laser start
		tex.xstart = beamtex.xstart + starttex*((beamtex.xend-beamtex.xstart));

		va->AddVertexTC(pos1-dir1*size,tex.xstart,tex.ystart,		kocolstart);
		va->AddVertexTC(pos1+dir1*size,tex.xstart,tex.yend,			kocolstart);
		va->AddVertexTC(pos2+dir1*size,tex.xend,tex.yend,			kocolstart);
		va->AddVertexTC(pos2-dir1*size,tex.xend,tex.ystart,			kocolstart);
		va->AddVertexTC(pos1-dir1*coresize,tex.xstart,tex.ystart,	corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,tex.xstart,tex.yend,	corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,tex.xend,tex.yend,		corecolstart);
		va->AddVertexTC(pos2-dir1*coresize,tex.xend,tex.ystart,		corecolstart);

		//draw continous beam
		tex.xstart = beamtex.xstart;
		float i;
		for(i=polylength*(1-starttex); i<beamlength-tilelength; i+=tilelength)
		{
			pos1=startPos+dir*i;
			pos2=startPos+dir*(i+tilelength);

			va->AddVertexTC(pos1-dir1*size,tex.xstart,tex.ystart,		kocolstart);
			va->AddVertexTC(pos1+dir1*size,tex.xstart,tex.yend,			kocolstart);
			va->AddVertexTC(pos2+dir1*size,tex.xend,tex.yend,			kocolstart);
			va->AddVertexTC(pos2-dir1*size,tex.xend,tex.ystart,			kocolstart);
			va->AddVertexTC(pos1-dir1*coresize,tex.xstart,tex.ystart,	corecolstart);
			va->AddVertexTC(pos1+dir1*coresize,tex.xstart,tex.yend,	corecolstart);
			va->AddVertexTC(pos2+dir1*coresize,tex.xend,tex.yend,		corecolstart);
			va->AddVertexTC(pos2-dir1*coresize,tex.xend,tex.ystart,		corecolstart);

		}

		//draw laser end
		pos1=startPos+dir*i;
		pos2=endPos;
		tex.xend = tex.xstart + ((pos2-pos1).Length()/tilelength)*texxsize;
		va->AddVertexTC(pos1-dir1*size,tex.xstart,tex.ystart,		kocolstart);
		va->AddVertexTC(pos1+dir1*size,tex.xstart,tex.yend,			kocolstart);
		va->AddVertexTC(pos2+dir1*size,tex.xend,tex.yend,			kocolstart);
		va->AddVertexTC(pos2-dir1*size,tex.xend,tex.ystart,			kocolstart);
		va->AddVertexTC(pos1-dir1*coresize,tex.xstart,tex.ystart,	corecolstart);
		va->AddVertexTC(pos1+dir1*coresize,tex.xstart,tex.yend,	corecolstart);
		va->AddVertexTC(pos2+dir1*coresize,tex.xend,tex.yend,		corecolstart);
		va->AddVertexTC(pos2-dir1*coresize,tex.xend,tex.ystart,		corecolstart);
	}

	//float 	midtexx = ph->laserendtex.xstart + (ph->laserendtex.xend-ph->laserendtex.xstart)*0.5;
	va->AddVertexTC(pos2-dir1*size,	ph->laserendtex.xstart,ph->laserendtex.ystart,    kocolstart);
	va->AddVertexTC(pos2+dir1*size,	ph->laserendtex.xstart,ph->laserendtex.yend,kocolstart);
	va->AddVertexTC(pos2+dir1*size+dir2*size, ph->laserendtex.xend,ph->laserendtex.yend,kocolstart);
	va->AddVertexTC(pos2-dir1*size+dir2*size, ph->laserendtex.xend,ph->laserendtex.ystart,kocolstart);
	va->AddVertexTC(pos2-dir1*coresize,ph->laserendtex.xstart,ph->laserendtex.ystart,    corecolstart);
	va->AddVertexTC(pos2+dir1*coresize,ph->laserendtex.xstart,ph->laserendtex.yend,corecolstart);
	va->AddVertexTC(pos2+dir1*coresize+dir2*coresize,ph->laserendtex.xend,ph->laserendtex.yend,corecolstart);
	va->AddVertexTC(pos2-dir1*coresize+dir2*coresize,ph->laserendtex.xend,ph->laserendtex.ystart,corecolstart);

	//for(float bpos=0; bpos
	//CTextureAtlas::Texture side = ph->textureAtlas->GetTexture("muzzleside");

	//draw muzzleflare
	starttex=(gu->modGameTime)*pulseSpeed;
	starttex = (starttex - (int)starttex);

		float muzzlesize = size*flaresize*starttex;
		unsigned char corcol[4];
		unsigned char kocol[4];
		corcol[3] = 1;
		kocol[3] = 1;
		for(int i=0; i<3; i++)
		{
			corcol[i] = int(corecolstart[i]*(1-starttex));
			kocol[i] = int(kocolstart[i]*(1-starttex));
		}

		pos1 = startPos-dir*(size*flaresize)*0.02;

		va->AddVertexTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,kocol);
		va->AddVertexTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,kocol);
		va->AddVertexTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,kocol);
		va->AddVertexTC(pos1-dir1*muzzlesize,side.xstart,side.yend,kocol);
		muzzlesize = muzzlesize*0.6;
		va->AddVertexTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,corcol);
		va->AddVertexTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,corcol);
		va->AddVertexTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,corcol);
		va->AddVertexTC(pos1-dir1*muzzlesize,side.xstart,side.yend,corcol);

		starttex+=0.5;
		if(starttex>1)
			starttex=starttex-1;
		for(int i=0; i<3; i++)
		{
			corcol[i] = int(corecolstart[i]*(1-starttex));
			kocol[i] = int(kocolstart[i]*(1-starttex));
		}
		muzzlesize = size*flaresize*starttex;
		va->AddVertexTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,kocol);
		va->AddVertexTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,kocol);
		va->AddVertexTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,kocol);
		va->AddVertexTC(pos1-dir1*muzzlesize,side.xstart,side.yend,kocol);
		muzzlesize = muzzlesize*0.6;
		va->AddVertexTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,corcol);
		va->AddVertexTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,corcol);
		va->AddVertexTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,corcol);
		va->AddVertexTC(pos1-dir1*muzzlesize,side.xstart,side.yend,corcol);


	//CTextureAtlas::Texture texture = ph->textureAtlas->GetTexture("largebeam");

	//draw flare
	float fsize = size*flaresize;
	pos1 = startPos - camera->forward*3;//move flare slightly in camera direction
	va->AddVertexTC(pos1-camera->right*fsize-camera->up*fsize,ph->flaretex.xstart,ph->flaretex.ystart,kocolstart);
	va->AddVertexTC(pos1+camera->right*fsize-camera->up*fsize,ph->flaretex.xend,ph->flaretex.ystart,kocolstart);
	va->AddVertexTC(pos1+camera->right*fsize+camera->up*fsize,ph->flaretex.xend,ph->flaretex.yend,kocolstart);
	va->AddVertexTC(pos1-camera->right*fsize+camera->up*fsize,ph->flaretex.xstart,ph->flaretex.yend,kocolstart);

	fsize = fsize*corethickness;
	va->AddVertexTC(pos1-camera->right*fsize-camera->up*fsize,ph->flaretex.xstart,ph->flaretex.ystart,corecolstart);
	va->AddVertexTC(pos1+camera->right*fsize-camera->up*fsize,ph->flaretex.xend,ph->flaretex.ystart,corecolstart);
	va->AddVertexTC(pos1+camera->right*fsize+camera->up*fsize,ph->flaretex.xend,ph->flaretex.yend,corecolstart);
	va->AddVertexTC(pos1-camera->right*fsize+camera->up*fsize,ph->flaretex.xstart,ph->flaretex.yend,corecolstart);

}
