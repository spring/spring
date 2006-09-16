// Ground.cpp: implementation of the CGround class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "StdAfx.h"
#include "Ground.h"
#include "ReadMap.h"
#include "Game/Camera.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "LogOutput.h"
#include "Sim/Misc/GeometricObjects.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CGround* ground;

CGround::CGround()
{
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
			if(GetHeight(p->pos.x,p->pos.z) > p->pos.y/*-p->radius*/)		//too many projectiles seems to hit the ground before hitting so remove the radius till a better fix is done
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

	float* hm = readmap->GetHeightmap();
	float xt=x*SQUARE_SIZE;
	float yt=y*SQUARE_SIZE;
	if (((xp-xt)*readmap->facenormals[(y*gs->mapx+x)*2].x+(yp-hm[y*(gs->mapx+1)+x])*readmap->facenormals[(y*gs->mapx+x)*2].y+(zp-yt)*readmap->facenormals[(y*gs->mapx+x)*2].z<=p->radius) &&
		/*(xp-xt+p->radius>0) && (zp-yt+p->radius>0) &&*/ (xp+zp-xt-yt-p->radius<SQUARE_SIZE)){
		p->Collision();
	}
	if (((xp-(xt+2))*readmap->facenormals[(y*gs->mapx+x)*2+1].x+(yp-hm[(y+1)*(gs->mapx+1)+x+1])*readmap->facenormals[(y*gs->mapx+x)*2+1].y+(zp-(yt+2))*readmap->facenormals[(y*gs->mapx+x)*2+1].z<=p->radius) &&
		/*(xp-(xt+2)-p->radius<0) && (zp-(yt+2)-p->radius<0) &&*/ (xp+zp-xt-yt-SQUARE_SIZE*2+p->radius>-SQUARE_SIZE)){
		p->Collision();
	}
	return;
}
float CGround::LineGroundCol(float3 from, float3 to)
{
	float savedLength=0;
	if(from.z>float3::maxzpos && to.z<float3::maxzpos){		//a special case since the camera in overhead mode can often do this
		float3 dir=to-from;
		float maxLength=dir.Length();
		dir/=maxLength;

		savedLength=-(from.z-float3::maxzpos)/dir.z;

		from+=dir*savedLength;
	}
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

	const float dx=to.x-from.x;
	const float dz=to.z-from.z;
	float xp=from.x;
	float zp=from.z;
	float ret;
	float xn,zn;

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
				return ret+savedLength;
			}
			keepgoing=fabs(zp-from.z)<fabs(dz);
			if(dz>0)
				zp+=SQUARE_SIZE;
			else 
				zp-=SQUARE_SIZE;
		}
		// if you hit this the collision detection hit an infinite loop
		assert(!keepgoing);
	} else if(floor(from.z/SQUARE_SIZE)==floor(to.z/SQUARE_SIZE)){
		while(keepgoing){
			ret = LineGroundSquareCol(from,to,(int)floor(xp/SQUARE_SIZE),(int)floor(zp/SQUARE_SIZE));	
			if(ret>=0){
				return ret+savedLength;
			}
			keepgoing=fabs(xp-from.x)<fabs(dx);
			if(dx>0)
				xp+=SQUARE_SIZE;
			else 
				xp-=SQUARE_SIZE;
		}
		// if you hit this the collision detection hit an infinite loop
		assert(!keepgoing);
	} else {
		while(keepgoing){
			float xs, zs;

			// Push value just over the edge of the square
			// This is the best accuracy we can get with floats:
			// add one digit and (xp*constant) reduces to xp itself
			// This accuracy means that at (16384,16384) (lower right of 32x32 map)
			// 1 in every 1/(16384*1e-7f/8)=4883 clicks on the map will be ignored.
			if (dx>0) xs = floor(xp*1.0000001f/SQUARE_SIZE);
			else      xs = floor(xp*0.9999999f/SQUARE_SIZE);
			if (dz>0) zs = floor(zp*1.0000001f/SQUARE_SIZE);
			else      zs = floor(zp*0.9999999f/SQUARE_SIZE);

			ret = LineGroundSquareCol(from, to, (int)xs, (int)zs);
			if(ret>=0){
				return ret+savedLength;
			}
			keepgoing=fabs(xp-from.x)<fabs(dx) && fabs(zp-from.z)<fabs(dz);

			if(dx>0){
				// distance xp to right edge of square (xs,zs) divided by dx, xp += xn*dx puts xp on the right edge
				xn=(xs*SQUARE_SIZE+SQUARE_SIZE-xp)/dx;
			} else {
				// distance xp to left edge of square (xs,zs) divided by dx, xp += xn*dx puts xp on the left edge
				xn=(xs*SQUARE_SIZE-xp)/dx;
			}
			if(dz>0){
				// distance zp to bottom edge of square (xs,zs) divided by dz, zp += zn*dz puts zp on the bottom edge
				zn=(zs*SQUARE_SIZE+SQUARE_SIZE-zp)/dz;
			} else {
				// distance zp to top edge of square (xs,zs) divided by dz, zp += zn*dz puts zp on the top edge
				zn=(zs*SQUARE_SIZE-zp)/dz;
			}
			// xn and zn are always positive, minus signs are divided out above

			// this puts (xp,zp) exactly on the first edge you see if you look from (xp,zp) in the (dx,dz) direction
			if(xn<zn){
				xp+=xn*dx;
				zp+=xn*dz;
			} else {
				xp+=zn*dx;
				zp+=zn*dz;
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
	float* heightmap=readmap->GetHeightmap();
	tri.y=heightmap[ys*(gs->mapx+1)+xs];

	float3 norm=readmap->facenormals[(ys*gs->mapx+xs)*2];
	float side1=(from-tri).dot(norm);
	float side2=(to-tri).dot(norm);

	if(side2<=0){				//linjen passerar triangelns plan?
		float dif=side1-side2;
		if(dif!=0){
			float frontpart=side1/dif;
			float3 col=from+(dir*frontpart);

			if((col.x>=tri.x) && (col.z>=tri.z) && (col.x+col.z<=tri.x+tri.z+SQUARE_SIZE)){	//kollision inuti triangeln (utnyttja trianglarnas "2d aktighet")
				return col.distance(from);
			}
		}
	}
	//triangel 2
	tri.x=(xs+1)*SQUARE_SIZE;
	tri.z=(ys+1)*SQUARE_SIZE;
	tri.y=heightmap[(ys+1)*(gs->mapx+1)+xs+1];

	norm=readmap->facenormals[(ys*gs->mapx+xs)*2+1];
	side1=(from-tri).dot(norm);
	side2=(to-tri).dot(norm);

	if(side2<=0){				//linjen passerar triangelns plan?
		float dif=side1-side2;
		if(dif!=0){
			float frontpart=side1/dif;
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
	int sx=(int) (x/SQUARE_SIZE);
	int sy=(int) (y/SQUARE_SIZE);
	float dx=(x-sx*SQUARE_SIZE)*(1.0f/SQUARE_SIZE);
	float dy=(y-sy*SQUARE_SIZE)*(1.0f/SQUARE_SIZE);
	int hs=sx+sy*(gs->mapx+1);

	float* heightmap = readmap->GetHeightmap();

	if(dx+dy<1){
		float xdif=(dx)*(heightmap[hs+1]-heightmap[hs]);
		float ydif=(dy)*(heightmap[hs+gs->mapx+1]-heightmap[hs]);
		r=heightmap[hs]+xdif+ydif;
	} else {
		float xdif=(1-dx)*(heightmap[hs+gs->mapx+1]-heightmap[hs+1+1+gs->mapx]);
		float ydif=(1-dy)*(heightmap[hs+1]-heightmap[hs+1+1+gs->mapx]);
		r=heightmap[hs+1+1+gs->mapx]+xdif+ydif;
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
	int sx=(int) (x/SQUARE_SIZE);
	int sy=(int) (y/SQUARE_SIZE);
	float dx=(x-sx*SQUARE_SIZE)*(1.0f/SQUARE_SIZE);
	float dy=(y-sy*SQUARE_SIZE)*(1.0f/SQUARE_SIZE);
	int hs=sx+sy*(gs->mapx+1);
	float* heightmap = readmap->GetHeightmap();
	if(dx+dy<1){
		float xdif=(dx)*(heightmap[hs+1]-heightmap[hs]);
		float ydif=(dy)*(heightmap[hs+gs->mapx+1]-heightmap[hs]);
		r=heightmap[hs]+xdif+ydif;
	} else {
		float xdif=(1-dx)*(heightmap[hs+gs->mapx+1]-heightmap[hs+1+1+gs->mapx]);
		float ydif=(1-dy)*(heightmap[hs+1]-heightmap[hs+1+1+gs->mapx]);
		r=heightmap[hs+1+1+gs->mapx]+xdif+ydif;
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
	int sx=(int)floor(x/SQUARE_SIZE);
	int sy=(int)floor(y/SQUARE_SIZE);

	if(sy<1)
		sy=1;
	if(sx<1)
		sx=1;
	if(sy>=gs->mapy-1)
		sy=gs->mapy-2;
	if(sx>=gs->mapx-1)
		sx=gs->mapx-2;

	float dx=(x-sx*SQUARE_SIZE)/SQUARE_SIZE;
	float dy=(y-sy*SQUARE_SIZE)/SQUARE_SIZE;

	int sy2;
	float fy;
	if(dy>0.5f){
		sy2=sy+1;
		fy=dy-0.5f;
	} else {
		sy2=sy-1;
		fy=0.5f-dy;
	}
	int sx2;
	float fx;
	if(dx>0.5f){
		sx2=sx+1;
		fx=dx-0.5f;
	} else {
		sx2=sx-1;
		fx=0.5f-dx;
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

float CGround::TrajectoryGroundCol(float3 from, float3 flatdir, float length, float linear, float quadratic)
{
	from.CheckInBounds();

	float3 dir(flatdir.x,linear,flatdir.z);
//	float3 oldpos=from;
	for(float l=0;l<length;l+=8){
		float3 pos(from+dir*l);
		pos.y+=quadratic*l*l;
		if(GetApproximateHeight(pos.x,pos.z)>pos.y){
			return l;
		}
//		geometricObjects->AddLine(pos,oldpos,3,0,16);
//		oldpos=pos;
	}	
	return -1;
}
