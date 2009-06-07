#include "StdAfx.h"
#include "mmgr.h"

#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "LargeBeamLaserProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CLargeBeamLaserProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),float3(0,0,0),float3(0,0,0),NULL,NULL));

CR_REG_METADATA(CLargeBeamLaserProjectile,(
	CR_MEMBER(startPos),
	CR_MEMBER(endPos),
	CR_MEMBER(corecolstart),
	CR_MEMBER(kocolstart),
	CR_MEMBER(thickness),
	CR_MEMBER(corethickness),
	CR_MEMBER(flaresize),
	CR_MEMBER(tilelength),
	CR_MEMBER(scrollspeed),
	CR_MEMBER(pulseSpeed),
	CR_MEMBER(beamtex),
	CR_MEMBER(side),
	CR_RESERVED(16)
	));

CLargeBeamLaserProjectile::CLargeBeamLaserProjectile(const float3& startPos, const float3& endPos,
		const float3& color, const float3& color2, CUnit* owner, const WeaponDef* weaponDef GML_PARG_C)
:	CWeaponProjectile(startPos + (endPos - startPos) * 0.5f, ZeroVector, owner, 0, ZeroVector, weaponDef, 0, false,  1 GML_PARG_P),
	startPos(startPos),
	endPos(endPos),
	decay(1.0f)
	//thickness(thickness),
	//corethickness(corethickness),
	//flaresize(flaresize),
	//tilelength(tilelength),
	//scrollspeed(scrollspeed),
	//pulseSpeed(pulseSpeed)
{
	checkCol=false;
	useAirLos=true;

	if (weaponDef) {
		this->beamtex = *weaponDef->visuals.texture1;
		this->side = *weaponDef->visuals.texture3;
	}

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

	if (weaponDef) {
		thickness = weaponDef->thickness;
		corethickness = weaponDef->corethickness;
		flaresize = weaponDef->laserflaresize;
		tilelength = weaponDef->visuals.tilelength;
		scrollspeed = weaponDef->visuals.scrollspeed;
		pulseSpeed = weaponDef->visuals.pulseSpeed;
		ttl = weaponDef->visuals.beamttl;
		decay = weaponDef->visuals.beamdecay;
	}

	// tilelength = 200;
	// scrollspeed = 5;
	// pulseSpeed = 1;
	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CLargeBeamLaserProjectile::~CLargeBeamLaserProjectile(void)
{
}


void CLargeBeamLaserProjectile::Update(void)
{
	if (ttl > 0) {
		ttl--;
		for (int i = 0; i < 3; i++) {
			corecolstart[i] = (unsigned char) (corecolstart[i] * decay);
			kocolstart[i] = (unsigned char) (kocolstart[i] * decay);
		}

		if (cegTag.size() > 0) {
			ceg.Explosion(startPos + ((endPos - startPos) / ttl), 0.0f, flaresize, 0x0, 0.0f, 0x0, endPos - startPos);
		}
	}
	else {
		deleteMe = true;
	}
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

	float starttex=(gu->modGameTime)*scrollspeed;
	starttex = 1-(starttex - (int)starttex);


	//beamtex = ph->textureAtlas->GetTexture("largebeam");
	float texxsize = beamtex.xend-beamtex.xstart;
	AtlasedTexture tex = beamtex;

	float polylength = (tex.xend-beamtex.xstart)*(1/texxsize)*tilelength;

	float istart=polylength*(1-starttex);
	float iend=beamlength-tilelength;
  va->EnlargeArrays(64+8*((int)((iend-istart)/tilelength)+2),0,VA_SIZE_TC);
	if(istart>beamlength)  //beam short enough to be drawn by one polygon
	{
		pos2=endPos;

		//draw laser start
		tex.xstart = beamtex.xstart + starttex*((beamtex.xend-beamtex.xstart));

		va->AddVertexQTC(pos1-dir1*size,tex.xstart,tex.ystart,		kocolstart);
		va->AddVertexQTC(pos1+dir1*size,tex.xstart,tex.yend,			kocolstart);
		va->AddVertexQTC(pos2+dir1*size,tex.xend,tex.yend,			kocolstart);
		va->AddVertexQTC(pos2-dir1*size,tex.xend,tex.ystart,			kocolstart);
		va->AddVertexQTC(pos1-dir1*coresize,tex.xstart,tex.ystart,	corecolstart);
		va->AddVertexQTC(pos1+dir1*coresize,tex.xstart,tex.yend,	corecolstart);
		va->AddVertexQTC(pos2+dir1*coresize,tex.xend,tex.yend,		corecolstart);
		va->AddVertexQTC(pos2-dir1*coresize,tex.xend,tex.ystart,		corecolstart);
	}
	else  //beam longer than one polygon
	{
		pos2=pos1+dir*istart;

		//draw laser start
		tex.xstart = beamtex.xstart + starttex*((beamtex.xend-beamtex.xstart));

		va->AddVertexQTC(pos1-dir1*size,tex.xstart,tex.ystart,		kocolstart);
		va->AddVertexQTC(pos1+dir1*size,tex.xstart,tex.yend,			kocolstart);
		va->AddVertexQTC(pos2+dir1*size,tex.xend,tex.yend,			kocolstart);
		va->AddVertexQTC(pos2-dir1*size,tex.xend,tex.ystart,			kocolstart);
		va->AddVertexQTC(pos1-dir1*coresize,tex.xstart,tex.ystart,	corecolstart);
		va->AddVertexQTC(pos1+dir1*coresize,tex.xstart,tex.yend,	corecolstart);
		va->AddVertexQTC(pos2+dir1*coresize,tex.xend,tex.yend,		corecolstart);
		va->AddVertexQTC(pos2-dir1*coresize,tex.xend,tex.ystart,		corecolstart);

		//draw continous beam
		tex.xstart = beamtex.xstart;
		float i;
		for(i=istart; i<iend; i+=tilelength) //! CAUTION: loop count must match EnlargeArrays above
		{
			pos1=startPos+dir*i;
			pos2=startPos+dir*(i+tilelength);

			va->AddVertexQTC(pos1-dir1*size,tex.xstart,tex.ystart,		kocolstart);
			va->AddVertexQTC(pos1+dir1*size,tex.xstart,tex.yend,			kocolstart);
			va->AddVertexQTC(pos2+dir1*size,tex.xend,tex.yend,			kocolstart);
			va->AddVertexQTC(pos2-dir1*size,tex.xend,tex.ystart,			kocolstart);
			va->AddVertexQTC(pos1-dir1*coresize,tex.xstart,tex.ystart,	corecolstart);
			va->AddVertexQTC(pos1+dir1*coresize,tex.xstart,tex.yend,	corecolstart);
			va->AddVertexQTC(pos2+dir1*coresize,tex.xend,tex.yend,		corecolstart);
			va->AddVertexQTC(pos2-dir1*coresize,tex.xend,tex.ystart,		corecolstart);

		}

		//draw laser end
		pos1=startPos+dir*i;
		pos2=endPos;
		tex.xend = tex.xstart + ((pos2-pos1).Length()/tilelength)*texxsize;
		va->AddVertexQTC(pos1-dir1*size,tex.xstart,tex.ystart,		kocolstart);
		va->AddVertexQTC(pos1+dir1*size,tex.xstart,tex.yend,			kocolstart);
		va->AddVertexQTC(pos2+dir1*size,tex.xend,tex.yend,			kocolstart);
		va->AddVertexQTC(pos2-dir1*size,tex.xend,tex.ystart,			kocolstart);
		va->AddVertexQTC(pos1-dir1*coresize,tex.xstart,tex.ystart,	corecolstart);
		va->AddVertexQTC(pos1+dir1*coresize,tex.xstart,tex.yend,	corecolstart);
		va->AddVertexQTC(pos2+dir1*coresize,tex.xend,tex.yend,		corecolstart);
		va->AddVertexQTC(pos2-dir1*coresize,tex.xend,tex.ystart,		corecolstart);
	}

	//float 	midtexx = weaponDef->visuals.texture2->xstart + (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*0.5f;
	va->AddVertexQTC(pos2-dir1*size,	weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->ystart,    kocolstart);
	va->AddVertexQTC(pos2+dir1*size,	weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->yend,kocolstart);
	va->AddVertexQTC(pos2+dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,kocolstart);
	va->AddVertexQTC(pos2-dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,kocolstart);
	va->AddVertexQTC(pos2-dir1*coresize,weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->ystart,    corecolstart);
	va->AddVertexQTC(pos2+dir1*coresize,weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->yend,corecolstart);
	va->AddVertexQTC(pos2+dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,corecolstart);
	va->AddVertexQTC(pos2-dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,corecolstart);

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

		va->AddVertexQTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,kocol);
		va->AddVertexQTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,kocol);
		va->AddVertexQTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,kocol);
		va->AddVertexQTC(pos1-dir1*muzzlesize,side.xstart,side.yend,kocol);
		muzzlesize = muzzlesize*0.6f;
		va->AddVertexQTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,corcol);
		va->AddVertexQTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,corcol);
		va->AddVertexQTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,corcol);
		va->AddVertexQTC(pos1-dir1*muzzlesize,side.xstart,side.yend,corcol);

		starttex+=0.5f;
		if(starttex>1)
			starttex=starttex-1;
		for(int i=0; i<3; i++)
		{
			corcol[i] = int(corecolstart[i]*(1-starttex));
			kocol[i] = int(kocolstart[i]*(1-starttex));
		}
		muzzlesize = size*flaresize*starttex;
		va->AddVertexQTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,kocol);
		va->AddVertexQTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,kocol);
		va->AddVertexQTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,kocol);
		va->AddVertexQTC(pos1-dir1*muzzlesize,side.xstart,side.yend,kocol);
		muzzlesize = muzzlesize*0.6f;
		va->AddVertexQTC(pos1+dir1*muzzlesize,side.xstart,side.ystart,corcol);
		va->AddVertexQTC(pos1+dir1*muzzlesize+dir*muzzlesize,side.xend,side.ystart,corcol);
		va->AddVertexQTC(pos1-dir1*muzzlesize+dir*muzzlesize,side.xend,side.yend,corcol);
		va->AddVertexQTC(pos1-dir1*muzzlesize,side.xstart,side.yend,corcol);


	//CTextureAtlas::Texture texture = ph->textureAtlas->GetTexture("largebeam");

	//draw flare
	float fsize = size*flaresize;
	pos1 = startPos - camera->forward*3;//move flare slightly in camera direction
	va->AddVertexQTC(pos1-camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture4->xstart,weaponDef->visuals.texture4->ystart,kocolstart);
	va->AddVertexQTC(pos1+camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture4->xend,weaponDef->visuals.texture4->ystart,kocolstart);
	va->AddVertexQTC(pos1+camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture4->xend,weaponDef->visuals.texture4->yend,kocolstart);
	va->AddVertexQTC(pos1-camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture4->xstart,weaponDef->visuals.texture4->yend,kocolstart);

	fsize = fsize*corethickness;
	va->AddVertexQTC(pos1-camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture4->xstart,weaponDef->visuals.texture4->ystart,corecolstart);
	va->AddVertexQTC(pos1+camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture4->xend,weaponDef->visuals.texture4->ystart,corecolstart);
	va->AddVertexQTC(pos1+camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture4->xend,weaponDef->visuals.texture4->yend,corecolstart);
	va->AddVertexQTC(pos1-camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture4->xstart,weaponDef->visuals.texture4->yend,corecolstart);

}

void CLargeBeamLaserProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	unsigned char color[4] = {kocolstart[0], kocolstart[1], kocolstart[2],255};
	lines.AddVertexQC(startPos,color);
	lines.AddVertexQC(endPos,color);
}
