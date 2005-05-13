#include "StdAfx.h"
// Ground.cpp: implementation of the CGround class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "StdAfx.h"
#include "Ground.h"
#include "ReadMap.h"
#include "Camera.h"
#include "ProjectileHandler.h"
#include "ReadMap.h"
#include "Projectile.h"
#include "InfoConsole.h"
#include "HeatCloudProjectile.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CGround* ground;

CGround::CGround()
{
	CReadMap::Instance();
}

CGround::~CGround()
{
	delete readmap;
}

void CGround::CheckCol(CProjectileHandler* ph)
{
	Projectile_List::iterator psi;
//	int x,y;
	for(psi=ph->ps.begin();psi != ph->ps.end();++psi){
		CProjectile* p=*psi;
		if(p->checkCol){
			if(GetHeight(p->pos.x,p->pos.z) > p->pos.y-p->radius)
				p->Collision();
/*			x=(int)floor((*psi)->pos.x*1.0f/SQUARE_SIZE);
			y=(int)floor((*psi)->pos.z*1.0f/SQUARE_SIZE);
			CheckColSquare(*psi,x,y);
*/		}
	}
}

void CGround::CheckColSquare(CProjectile* p,int x,int y)
{
	if(!(x>=0 && y>=0 && x<gs->mapx && y<gs->mapy))
		return;
	float xp=p->pos.x;
	float yp=p->pos.y;
	float zp=p->pos.z;

	float xt=x*SQUARE_SIZE;
	float yt=y*SQUARE_SIZE;
	if (((xp-xt)*readmap->facenormals[(y*gs->mapx+x)*2].x+(yp-readmap->heightmap[y*(gs->mapx+1)+x])*readmap->facenormals[(y*gs->mapx+x)*2].y+(zp-yt)*readmap->facenormals[(y*gs->mapx+x)*2].z<=p->radius) &&
		/*(xp-xt+p->radius>0) && (zp-yt+p->radius>0) &&*/ (xp+zp-xt-yt-p->radius<SQUARE_SIZE)){
		p->Collision();
	}
	if (((xp-(xt+2))*readmap->facenormals[(y*gs->mapx+x)*2+1].x+(yp-readmap->heightmap[(y+1)*(gs->mapx+1)+x+1])*readmap->facenormals[(y*gs->mapx+x)*2+1].y+(zp-(yt+2))*readmap->facenormals[(y*gs->mapx+x)*2+1].z<=p->radius) &&
		/*(xp-(xt+2)-p->radius<0) && (zp-(yt+2)-p->radius<0) &&*/ (xp+zp-xt-yt-SQUARE_SIZE*2+p->radius>-SQUARE_SIZE)){
		p->Collision();
	}
	return;
}
float CGround::LineGroundCol(float3 from, float3 to)
{
	from.CheckInBounds();

	float3 dir=to-from;
	float maxLength=dir.Length();
	dir/=maxLength;

	if(from.x+dir.x*maxLength<1)
		maxLength=(1-from.x)/dir.x;
	else if(from.x+dir.x*maxLength>float3::maxxpos)
		maxLength=(float3::maxxpos-from.x)/dir.x;

	if(from.z+dir.z*maxLength<1)
		maxLength=(1-from.z)/dir.z;
	else if(from.z+dir.z*maxLength>float3::maxzpos)
		maxLength=(float3::maxzpos-from.z)/dir.z;

	to=from+dir*maxLength;

	double dx=to.x-from.x;
	double dz=to.z-from.z;
	double xp=from.x;
	double zp=from.z;
	double ret;
	double xn,zn;

	bool keepgoing=true;

	if((floor(from.x/SQUARE_SIZE)==floor(to.x/SQUARE_SIZE)) && (floor(from.z/SQUARE_SIZE)==floor(to.z/SQUARE_SIZE))){
		ret = LineGroundSquareCol(from,to,(int)floor(from.x/SQUARE_SIZE),(int)floor(from.z/SQUARE_SIZE));
		if(ret>=0){
			return ret;
		}
	} else if(floor(from.x/SQUARE_SIZE)==floor(to.x/SQUARE_SIZE)){
		while(keepgoing){
			ret = LineGroundSquareCol(from,to,(int)floor(xp/SQUARE_SIZE),(int)floor(zp/SQUARE_SIZE));	
			if(ret>=0){
				return ret;
			}
			keepgoing=fabs(zp-from.z)<fabs(to.z-from.z);
			if(dz>0)
				zp+=SQUARE_SIZE;
			else 
				zp-=SQUARE_SIZE;
		}
	} else if(floor(from.z/SQUARE_SIZE)==floor(to.z/SQUARE_SIZE)){
		while(keepgoing){
			ret = LineGroundSquareCol(from,to,(int)floor(xp/SQUARE_SIZE),(int)floor(zp/SQUARE_SIZE));	
			if(ret>=0){
				return ret;
			}
			keepgoing=fabs(xp-from.x)<fabs(to.x-from.x);
			if(dx>0)
				xp+=SQUARE_SIZE;
			else 
				xp-=SQUARE_SIZE;
		}
	} else {
		while(keepgoing){
			int xs=(int)floor(xp/SQUARE_SIZE);
			int zs=(int)floor(zp/SQUARE_SIZE);
			ret = LineGroundSquareCol(from,to,xs,zs);	
			if(ret>=0){
				return ret;
			}
			keepgoing=fabs(xp-from.x)<fabs(to.x-from.x) && fabs(zp-from.z)<fabs(to.z-from.z);

			if(dx>0){
				xn=(floor(xp/SQUARE_SIZE)*SQUARE_SIZE+SQUARE_SIZE-xp)/dx;
			} else {
				xn=(floor(xp/SQUARE_SIZE)*SQUARE_SIZE-xp)/dx;
			}
			if(dz>0){
				zn=(floor(zp/SQUARE_SIZE)*SQUARE_SIZE+SQUARE_SIZE-zp)/dz;
			} else {
				zn=(floor(zp/SQUARE_SIZE)*SQUARE_SIZE-zp)/dz;
			}
			
			if(xn<zn){
				xp+=(xn+0.00001)*dx;
				zp+=(xn+0.00001)*dz;
			} else {
				xp+=(zn+0.00001)*dx;
				zp+=(zn+0.00001)*dz;
			}
		}
	}
	return -1;
}	

float CGround::LineGroundSquareCol(const float3 &from,const float3 &to,int xs,int ys)
{
	if((xs<0) || (ys<0) || (xs>=gs->mapx-1) || (ys>=gs->mapy-1))
		return -1;

	float3 dir=to-from;
	float3 tri;
	//triangel 1
	tri.x=xs*SQUARE_SIZE;
	tri.z=ys*SQUARE_SIZE;
	tri.y=readmap->heightmap[ys*(gs->mapx+1)+xs];

	float3 norm=readmap->facenormals[(ys*gs->mapx+xs)*2];
	double side1=(from-tri).dot(norm);
	double side2=(to-tri).dot(norm);

	if(side2<=0){				//linjen passerar triangelns plan?
		double dif=side1-side2;
		if(dif!=0){
			double frontpart=side1/dif;
			float3 col=from+(dir*frontpart);

			if((col.x>=tri.x) && (col.z>=tri.z) && (col.x+col.z<=tri.x+tri.z+SQUARE_SIZE)){	//kollision inuti triangeln (utnyttja trianglarnas "2d aktighet")
				return col.distance(from);
			}
		}
	}
	//triangel 2
	tri.x=(xs+1)*SQUARE_SIZE;
	tri.z=(ys+1)*SQUARE_SIZE;
	tri.y=readmap->heightmap[(ys+1)*(gs->mapx+1)+xs+1];

	norm=readmap->facenormals[(ys*gs->mapx+xs)*2+1];
	side1=(from-tri).dot(norm);
	side2=(to-tri).dot(norm);

	if(side2<=0){				//linjen passerar triangelns plan?
		double dif=side1-side2;
		if(dif!=0){
			double frontpart=side1/dif;
			float3 col=from+(dir*frontpart);

			if((col.x<=tri.x) && (col.z<=tri.z) && (col.x+col.z>=tri.x+tri.z-SQUARE_SIZE)){	//kollision inuti triangeln (utntri.ytja trianglarnas "2d aktighet")
				return col.distance(from);
			}
		}
	}
	return -2;
}

float CGround::GetApproximateHeight(float x,float y)
{
	int xsquare=int(x)/SQUARE_SIZE;
	int ysquare=int(y)/SQUARE_SIZE;
	if(xsquare<0)
		xsquare=0;
	else if(xsquare>gs->mapx-1)
		xsquare=gs->mapx-1;
	if(ysquare<0)
		ysquare=0;
	else if(ysquare>gs->mapy-1)
		ysquare=gs->mapy-1;
	return readmap->centerheightmap[xsquare+ysquare*gs->mapx];
}

float CGround::GetHeight(float x, float y)
{
	if(x<1)
		x=1;
	else if(x>float3::maxxpos)
		x=float3::maxxpos;

	if(y<1)
		y=1;
	else if(y>float3::maxzpos)
		y=float3::maxzpos;

	float r;
	int sx=x/SQUARE_SIZE;
	int sy=y/SQUARE_SIZE;
	float dx=(x-sx*SQUARE_SIZE)*(1.0/SQUARE_SIZE);
	float dy=(y-sy*SQUARE_SIZE)*(1.0/SQUARE_SIZE);
	int hs=sx+sy*(gs->mapx+1);
	if(dx+dy<1){
		float xdif=(dx)*(readmap->heightmap[hs+1]-readmap->heightmap[hs]);
		float ydif=(dy)*(readmap->heightmap[hs+gs->mapx+1]-readmap->heightmap[hs]);
		r=readmap->heightmap[hs]+xdif+ydif;
	} else {
		float xdif=(1-dx)*(readmap->heightmap[hs+gs->mapx+1]-readmap->heightmap[hs+1+1+gs->mapx]);
		float ydif=(1-dy)*(readmap->heightmap[hs+1]-readmap->heightmap[hs+1+1+gs->mapx]);
		r=readmap->heightmap[hs+1+1+gs->mapx]+xdif+ydif;
	}
	if(r<0)
		r=0;
	return r;
}

float CGround::GetHeight2(float x, float y)
{
	if(x<1)
		x=1;
	else if(x>float3::maxxpos)
		x=float3::maxxpos;

	if(y<1)
		y=1;
	else if(y>float3::maxzpos)
		y=float3::maxzpos;

	float r;
	int sx=x/SQUARE_SIZE;
	int sy=y/SQUARE_SIZE;
	float dx=(x-sx*SQUARE_SIZE)*(1.0/SQUARE_SIZE);
	float dy=(y-sy*SQUARE_SIZE)*(1.0/SQUARE_SIZE);
	int hs=sx+sy*(gs->mapx+1);
	if(dx+dy<1){
		float xdif=(dx)*(readmap->heightmap[hs+1]-readmap->heightmap[hs]);
		float ydif=(dy)*(readmap->heightmap[hs+gs->mapx+1]-readmap->heightmap[hs]);
		r=readmap->heightmap[hs]+xdif+ydif;
	} else {
		float xdif=(1-dx)*(readmap->heightmap[hs+gs->mapx+1]-readmap->heightmap[hs+1+1+gs->mapx]);
		float ydif=(1-dy)*(readmap->heightmap[hs+1]-readmap->heightmap[hs+1+1+gs->mapx]);
		r=readmap->heightmap[hs+1+1+gs->mapx]+xdif+ydif;
	}
	return r;
}

float3& CGround::GetNormal(float x, float y)
{
	if(x<1)
		x=1;
	else if(x>float3::maxxpos)
		x=float3::maxxpos;

	if(y<1)
		y=1;
	else if(y>float3::maxzpos)
		y=float3::maxzpos;

	return readmap->facenormals[(int(x)/SQUARE_SIZE+int(y)/SQUARE_SIZE*gs->mapx)*2];
}
/*
float CGround::GetApproximateHeight(float x, float y)
{
	if((x<0) || (y<0) || (x>gs->mapx*SQUARE_SIZE) || (y>gs->mapy*SQUARE_SIZE))
		return 0;
	return readmap->centerheightmap[int(x)/SQUARE_SIZE+int(y)/SQUARE_SIZE*(gs->mapx)];
}
*/
float CGround::GetSlope(float x, float y)
{
	if(x<1)
		x=1;
	else if(x>float3::maxxpos)
		x=float3::maxxpos;

	if(y<1)
		y=1;
	else if(y>float3::maxzpos)
		y=float3::maxzpos;

	return 1-readmap->facenormals[(int(x)/SQUARE_SIZE+int(y)/SQUARE_SIZE*gs->mapx)*2].y;
}


float3 CGround::GetSmoothNormal(float x, float y)
{
	int sx=floor(x/SQUARE_SIZE);
	int sy=floor(y/SQUARE_SIZE);

	if(sy<1)
		sy=1;
	if(sx<1)
		sx=1;
	if(sy>=gs->mapy-1)
		sy=gs->mapy-2;
	if(sx>=gs->mapx-1)
		sx=gs->mapy-2;

	float dx=(x-sx*SQUARE_SIZE)/SQUARE_SIZE;
	float dy=(y-sy*SQUARE_SIZE)/SQUARE_SIZE;

	int sy2;
	float fy;
	if(dy>0.5){
		sy2=sy+1;
		fy=dy-0.5;
	} else {
		sy2=sy-1;
		fy=0.5-dy;
	}
	int sx2;
	float fx;
	if(dx>0.5){
		sx2=sx+1;
		fx=dx-0.5;
	} else {
		sx2=sx-1;
		fx=0.5-dx;
	}

	float ify=1-fy;
	float ifx=1-fx;

	float3 n1=(readmap->facenormals[(sy*gs->mapx+sx)*2]+readmap->facenormals[(sy*gs->mapx+sx)*2+1])*ifx*ify;
	float3 n2=(readmap->facenormals[(sy*gs->mapx+sx2)*2]+readmap->facenormals[(sy*gs->mapx+sx2)*2+1])*fx*ify;
	float3 n3=(readmap->facenormals[((sy2)*gs->mapx+sx)*2]+readmap->facenormals[((sy2)*gs->mapx+sx)*2+1])*ifx*fy;
	float3 n4=(readmap->facenormals[((sy2)*gs->mapx+sx2)*2]+readmap->facenormals[((sy2)*gs->mapx+sx2)*2+1])*fx*fy;

	float3 norm1=n1+n2+n3+n4;
	norm1.Normalize();

	return norm1;
}
