#include "StdAfx.h"
#include "ExplosionGraphics.h"
#include "myGL.h"
#include "HeatCloudProjectile.h"
#include "SmokeProjectile2.h"
#include "WreckProjectile.h"
#include "GroundFlash.h"
#include "SpherePartProjectile.h"
#include "BubbleProjectile.h"
#include "WakeProjectile.h"
#include "Camera.h"
#include "ProjectileHandler.h"
#include "Ground.h"
#include "DirtProjectile.h"
#include "ExploSpikeProjectile.h"

CExplosionGraphics::CExplosionGraphics(void)
{
}

CExplosionGraphics::~CExplosionGraphics(void)
{
}

CStdExplosionGraphics::CStdExplosionGraphics(void)
{
}

CStdExplosionGraphics::~CStdExplosionGraphics(void)
{
}

void CStdExplosionGraphics::Explosion(const float3 &pos, const DamageArray& damages, float radius, CUnit *owner,float gfxMod)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	float h2=ground->GetHeight2(pos.x,pos.z);

	float height=pos.y-h2;
	if(height<0)
		height=0;

	bool waterExplosion=h2<-3;
	bool uwExplosion=pos.y<-15;
	bool airExplosion=pos.y-max((float)0,h2)>20;

	float damage=damages[0]/20;
	if(damage>radius*1.5) //limit the visual effects based on the radius
		damage=radius*1.5;
	damage*=gfxMod;
	for(int a=0;a<1;++a){
//		float3 speed((gs->randFloat()-0.5f)*(radius*0.04),0.05+(gs->randFloat())*(radius*0.007),(gs->randFloat()-0.5)*(radius*0.04));
		float3 speed(0,0.3f,0);
		float3 camVect=camera->pos-pos;
		float camLength=camVect.Length();
		camVect/=camLength;
		float moveLength=radius*0.03;
		if(camLength<moveLength+2)
			moveLength=camLength-2;
		float3 npos=pos+camVect*moveLength;
		float heatcloudsize = 7+damage*2.8;
		float heatcloudtemperature = 8+sqrt(damage)*0.5;
		CHeatCloudProjectile* p=new CHeatCloudProjectile(npos,speed,heatcloudtemperature,heatcloudsize,(-heatcloudsize/heatcloudtemperature)*0.5,owner);
		//p->Update();
		//p->maxheat=p->heat;
	}
	if(ph->particleSaturation<1){		//turn off lots of graphic only particles when we have more particles than we want
		float smokeDamage=damage;
		if(uwExplosion)
			smokeDamage*=0.3;
		if(airExplosion || waterExplosion)
			smokeDamage*=0.6;
		float invSqrtsmokeDamage=1/(sqrt(smokeDamage)*0.35);
		for(int a=0;a<smokeDamage*0.6;++a){
			float3 speed(-0.1f+gu->usRandFloat()*0.2,(0.1f+gu->usRandFloat()*0.3)*invSqrtsmokeDamage,-0.1f+gu->usRandFloat()*0.2);
			float3 npos(pos+gu->usRandVector()*(smokeDamage*1.0));
			float h=ground->GetApproximateHeight(npos.x,npos.z);
			if(npos.y<h)
				npos.y=h;
			float time=(40+sqrt(smokeDamage)*15)*(0.8+gu->usRandFloat()*0.7);
			new CSmokeProjectile2(pos,npos,speed,time,sqrt(smokeDamage)*4,0.4,owner,0.6);
		}
		if(!airExplosion && !uwExplosion){
			int numDirt=(int)min(20.,damage*0.8);
			float3 color(0.15,0.1,0.05);
			if(waterExplosion)
				color=float3(1,1,1);
			for(int a=0;a<numDirt;++a){
				float3 speed((0.5-gu->usRandFloat())*1.5,1.7f+gu->usRandFloat()*1.6,(0.5-gu->usRandFloat())*1.5);
				speed*=0.7+min((float)30,damage)/30;
				float3 npos(pos.x-(0.5-gu->usRandFloat())*(radius*0.6),pos.y-2.0-damage*0.2,pos.z-(0.5-gu->usRandFloat())*(radius*0.6));
				if(waterExplosion)
					npos=float3(pos.x-(0.5-gu->usRandFloat())*(radius*0.3),pos.y-2.0-damage*0.2,pos.z-(0.5-gu->usRandFloat())*(radius*0.3));
				CDirtProjectile* dp=new CDirtProjectile(npos,speed,90+damage*2,2.0+sqrt(damage)*1.5,0.4,0.999f,owner,color);
			}
		}
		if(damage>=20 && !uwExplosion && !airExplosion){
			int numDebris=gu->usRandInt()%6;
			if(numDebris>0)
				numDebris+=3+(int)(damage*0.04);
			for(int a=0;a<numDebris;++a){
				float3 speed;
				if(height<4)
					speed=float3((0.5f-gu->usRandFloat())*2.0,1.8f+gu->usRandFloat()*1.8,(0.5f-gu->usRandFloat())*2.0);
				else
					speed=float3(gu->usRandVector()*2);
				speed*=0.7+min((float)30,damage)/23;
				float3 npos(pos.x-(0.5-gu->usRandFloat())*(radius*1),pos.y,pos.z-(0.5-gu->usRandFloat())*(radius*1));
				new CWreckProjectile(npos,speed,90+damage*2,owner);
			}
		}
		if(uwExplosion){
			int numBubbles=(int)(damage*0.7);
			for(int a=0;a<numBubbles;++a){
				new CBubbleProjectile(pos+gu->usRandVector()*radius*0.5,gu->usRandVector()*0.2+float3(0,0.2,0),damage*2+gu->usRandFloat()*damage,1+gu->usRandFloat()*2,0.02,owner,0.5+gu->usRandFloat()*0.3);
			}
		}
		if(waterExplosion && !uwExplosion && !airExplosion){
			int numWake=(int)(damage*0.5);
			for(int a=0;a<numWake;++a){
				new CWakeProjectile(pos+gu->usRandVector()*radius*0.2,gu->usRandVector()*radius*0.003,sqrt(damage)*4,damage*0.03,owner,0.3+gu->usRandFloat()*0.2,0.8/(sqrt(damage)*3+50+gu->usRandFloat()*90),1);
			}
		}
		if(radius>10 && damage>4){
			int numSpike=(int)sqrt(damage)+8;
			for(int a=0;a<numSpike;++a){
				float3 speed=gu->usRandVector();
				speed.Normalize();
				speed*=(8+damage*3.0)/(9+sqrt(damage)*0.7)*0.35;
				if(!airExplosion && !waterExplosion && speed.y<0)
					speed.y=-speed.y;
				new CExploSpikeProjectile(pos+speed,speed*(0.9f+gu->usRandFloat()*0.4f),radius*0.1,radius*0.1,0.6f,0.8/(8+sqrt(damage)),owner);
			}
		}
	}
	if(radius>20 && damage>6 && height<radius*0.7){
		float modSize=max(radius,damage*2);
		float circleAlpha=0;
		float circleGrowth=0;
		float ttl=8+sqrt(damage)*0.8;
		if(radius>40 && damage>12){
			circleAlpha=min(0.5,damage*0.01);
			circleGrowth=(8+damage*2.5)/(9+sqrt(damage)*0.7)*0.55;
		}
		float flashSize=modSize;
		float flashAlpha=min(0.8,damage*0.01);
		new CGroundFlash(pos,circleAlpha,flashAlpha,flashSize,circleGrowth,ttl);
	}

	if(radius>40 && damage>12){
		CSpherePartProjectile::CreateSphere(pos,min(0.7,damage*0.02),5+(int)(sqrt(damage)*0.7),(8+damage*2.5)/(9+sqrt(damage)*0.7)*0.5,owner);	
	}
	POP_CODE_MODE;
}
