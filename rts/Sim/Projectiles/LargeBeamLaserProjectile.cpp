#include "StdAfx.h"
#include "LargeBeamLaserProjectile.h"
#include "Sim/Units/Unit.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"
#include "ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"

CLargeBeamLaserProjectile::CLargeBeamLaserProjectile(const float3& startPos,const float3& endPos,const float3& color, const float3& color2,CUnit* owner, WeaponDef *weaponDef)
:	CWeaponProjectile(startPos+(endPos-startPos)*0.5f,ZeroVector, owner, 0, ZeroVector, weaponDef,damages,0),//CProjectile((startPos+endPos)*0.5f,ZeroVector,owner),
	startPos(startPos),
	endPos(endPos)
	//thickness(thickness),
	//corethickness(corethickness),
	//flaresize(flaresize),
	//tilelength(tilelength),
	//scrollspeed(scrollspeed),
	//pulseSpeed(pulseSpeed)
{
	checkCol=false;
	useAirLos=true;

	this->beamtex = *weaponDef->visuals.texture1;
	this->side = *weaponDef->visuals.texture3;

	SetRadius(pos.distance(endPos));

	//midtexx = weaponDef->visuals.texture2->xstart + (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*0.5f;
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

	thickness = weaponDef->thickness;
	corethickness = weaponDef->corethickness;
	flaresize = weaponDef->laserflaresize;
	tilelength = weaponDef->visuals.tilelength;
	scrollspeed = weaponDef->visuals.scrollspeed;
	pulseSpeed = weaponDef->visuals.pulseSpeed;

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

	//float 	midtexx = weaponDef->visuals.texture2->xstart + (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*0.5f;
	va->AddVertexTC(pos2-dir1*size,	weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->ystart,    kocolstart);
	va->AddVertexTC(pos2+dir1*size,	weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->yend,kocolstart);
	va->AddVertexTC(pos2+dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,kocolstart);
	va->AddVertexTC(pos2-dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,kocolstart);
	va->AddVertexTC(pos2-dir1*coresize,weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->ystart,    corecolstart);
	va->AddVertexTC(pos2+dir1*coresize,weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->yend,corecolstart);
	va->AddVertexTC(pos2+dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,corecolstart);
	va->AddVertexTC(pos2-dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,corecolstart);

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

		pos1 = startPos-dir*(size*flaresize)*0.02f;

		va->AddVertexTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,kocol);
		va->AddVertexTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,kocol);
		va->AddVertexTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,kocol);
		va->AddVertexTC(pos1-dir1*muzzlesize,side.xstart,side.yend,kocol);
		muzzlesize = muzzlesize*0.6f;
		va->AddVertexTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,corcol);
		va->AddVertexTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,corcol);
		va->AddVertexTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,corcol);
		va->AddVertexTC(pos1-dir1*muzzlesize,side.xstart,side.yend,corcol);

		starttex+=0.5f;
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
		muzzlesize = muzzlesize*0.6f;
		va->AddVertexTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,corcol);
		va->AddVertexTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,corcol);
		va->AddVertexTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,corcol);
		va->AddVertexTC(pos1-dir1*muzzlesize,side.xstart,side.yend,corcol);


	//CTextureAtlas::Texture texture = ph->textureAtlas->GetTexture("largebeam");

	//draw flare
	float fsize = size*flaresize;
	pos1 = startPos - camera->forward*3;//move flare slightly in camera direction
	va->AddVertexTC(pos1-camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture4->xstart,weaponDef->visuals.texture4->ystart,kocolstart);
	va->AddVertexTC(pos1+camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture4->xend,weaponDef->visuals.texture4->ystart,kocolstart);
	va->AddVertexTC(pos1+camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture4->xend,weaponDef->visuals.texture4->yend,kocolstart);
	va->AddVertexTC(pos1-camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture4->xstart,weaponDef->visuals.texture4->yend,kocolstart);

	fsize = fsize*corethickness;
	va->AddVertexTC(pos1-camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture4->xstart,weaponDef->visuals.texture4->ystart,corecolstart);
	va->AddVertexTC(pos1+camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture4->xend,weaponDef->visuals.texture4->ystart,corecolstart);
	va->AddVertexTC(pos1+camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture4->xend,weaponDef->visuals.texture4->yend,corecolstart);
	va->AddVertexTC(pos1-camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture4->xstart,weaponDef->visuals.texture4->yend,corecolstart);

}
