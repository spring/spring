#include "StdAfx.h"
#include "ShieldPartProjectile.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"

using namespace std;

CShieldPartProjectile::CShieldPartProjectile(const float3& centerPos,int xpart,int ypart,float sphereSize,float3 color,float alpha,CUnit* owner)
: CProjectile(centerPos,ZeroVector,owner),
	centerPos(centerPos),
	sphereSize(sphereSize),
	baseAlpha(alpha),
	color(color)
{
	checkCol=false;

	for(int y=0;y<5;++y){
		float yp=(y+ypart)/16.0*PI-PI/2;
		for(int x=0;x<5;++x){
			float xp=(x+xpart)/32.0*2*PI;
			vectors[y*5+x]=float3(sin(xp)*cos(yp),sin(yp),cos(xp)*cos(yp));
		}
	}
	pos=centerPos+vectors[12]*sphereSize;

	alwaysVisible=false;
	useAirLos=true;
	drawRadius=sphereSize*0.4;
}

CShieldPartProjectile::~CShieldPartProjectile(void)
{
}

void CShieldPartProjectile::Update(void)
{
	pos=centerPos+vectors[12]*sphereSize;
}

void CShieldPartProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];

	float interSize=sphereSize;
	for(int y=0;y<4;++y){
		for(int x=0;x<4;++x){
			float alpha=baseAlpha*255;

			col[0]=(unsigned char) (color.x*alpha);
			col[1]=(unsigned char) (color.y*alpha);
			col[2]=(unsigned char) (color.z*alpha);
			col[3]=(unsigned char) (alpha);
			va->AddVertexTC(centerPos+vectors[y*5+x]*interSize,1.0/16,1.0/16,col);
			va->AddVertexTC(centerPos+vectors[y*5+x+1]*interSize,1.0/16,1.0/16,col);
			va->AddVertexTC(centerPos+vectors[(y+1)*5+x+1]*interSize,1.0/16,1.0/16,col);
			va->AddVertexTC(centerPos+vectors[(y+1)*5+x]*interSize,1.0/16,1.0/16,col);
		}
	}
}
