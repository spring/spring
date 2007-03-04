#include "StdAfx.h"
#include "PieceProjectile.h"
#include "GlobalStuff.h"
#include "SmokeProjectile.h"
#include "Map/Ground.h"
#include "ProjectileHandler.h"
#include "LogOutput.h"
#include "Game/GameHelper.h"
#include "Sim/Units/Unit.h"
#include "myMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "SmokeTrailProjectile.h"
#include "Sync/SyncTracer.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Matrix44f.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

static const float Smoke_Time=40;

CR_BIND_DERIVED(CPieceProjectile, CProjectile, (float3(0,0,0),float3(0,0,0),NULL,0,NULL,0));

CR_REG_METADATA(CPieceProjectile,(
	CR_SERIALIZER(creg_Serialize), // numCallback, oldInfos
	CR_MEMBER(flags),
	CR_MEMBER(dispList),
	// TODO what to do with the next two fields
	//CR_MEMBER(piece3do),
	//CR_MEMBER(pieces3o),
	CR_MEMBER(spinVec),
	CR_MEMBER(spinSpeed),
	CR_MEMBER(spinPos),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldSmokeDir),
	CR_MEMBER(target),
	CR_MEMBER(drawTrail),
	CR_MEMBER(curCallback),
	CR_MEMBER(age),
	CR_MEMBER(team)
	));

void CPieceProjectile::creg_Serialize(creg::ISerializer& s)
{	
	s.Serialize(numCallback, sizeof(int));
	for(int i=0;i<8;i++) {
		s.Serialize(oldInfos[i],sizeof(CPieceProjectile::OldInfo));
	}
}

CPieceProjectile::CPieceProjectile(const float3& pos,const float3& speed, LocalS3DO * piece, int flags,CUnit* owner,float radius)
: CProjectile(pos,speed,owner, true),
  flags(flags),
  dispList(piece->displist),
	drawTrail(true),
	oldSmoke(pos),
	curCallback(0),
	spinPos(0),
	age(0)
{
	checkCol=false;
	if(owner){
		/* If we're an S3O unit, this is where ProjectileHandler
		   fetches our texture from. */
		s3domodel=owner->model;
		/* If we're part of an S3O unit, save this so we can 
		   draw with the right teamcolour. */
		team=owner->team;
	}

	/* Don't store piece; owner may be a dying unit, so piece could be freed. */

	/* //if (piece->texturetype == 0) {
	   Great. LocalPiece doesn't carry the texture name.
	   
	   HACK TODO PieceProjectile shouldn't need to know about
	   different model formats; push it into Rendering/UnitModels. 
	   
	   If this needs to change, also modify Sim/Units/COB/CobInstance.cpp::Explosion.
	   
	   Nothing else wants to draw just one part without PieceInfo, so this 
	   polymorphism can stay put for the moment.
	   */
	if (piece->original3do != NULL){
		piece3do = piece->original3do;
		pieces3o = NULL;
	} else if (piece->originals3o != NULL){
		piece3do = NULL;
		pieces3o = piece->originals3o;
	}

	castShadow=true;

	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;

	ENTER_MIXED;
	numCallback=SAFE_NEW int;
	*numCallback=0;
	oldSmokeDir=speed;
	oldSmokeDir.Normalize();
	float3 camDir=(pos-camera->pos).Normalize();
	if(camera->pos.distance(pos)+(1-fabs(camDir.dot(oldSmokeDir)))*3000 < 200)
		drawTrail=false;

	spinVec=gu->usRandVector();
	spinVec.Normalize();
	spinSpeed=gu->usRandFloat()*20;
	//logOutput.Print("New pp with %d", dispList);

	for(int a=0;a<8;++a){
		oldInfos[a]=SAFE_NEW OldInfo;
		oldInfos[a]->pos=pos;
		oldInfos[a]->size=gu->usRandFloat()*2+2;
	}
	ENTER_SYNCED;

	SetRadius(radius);
	drawRadius=32;
#ifdef TRACE_SYNC
	tracefile << "New pieceexplosive: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

CPieceProjectile::~CPieceProjectile(void)
{
	delete numCallback;
	if(curCallback)
		curCallback->drawCallbacker=0;

	for(int a=0;a<8;++a){
		delete oldInfos[a];
	}
}

void CPieceProjectile::Collision()
{
	if(speed.Length()>gs->randFloat()*5+1 && pos.y>radius+2){
		float3 norm=ground->GetNormal(pos.x,pos.z);
		float ns=speed.dot(norm);
		speed-=norm*ns*1.6f;
		pos+=norm*0.1f;
	} else {
		if (flags & PP_Explode) {
			helper->Explosion(pos,DamageArray()*50,5,0,10,owner,false,1.0f,false,0,0,ZeroVector, -1);
		}
		if(flags & PP_Smoke){
			float3 dir=speed;
			dir.Normalize();
			CSmokeTrailProjectile* tp=SAFE_NEW CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,false,true,7,Smoke_Time,0.5f,drawTrail);
			tp->creationTime+=8-((age)&7);
		}
		CProjectile::Collision();
		oldSmoke=pos;
	}
}

void CPieceProjectile::Collision(CUnit* unit)
{
	if(unit==owner)
		return;
	if (flags & PP_Explode) {
		helper->Explosion(pos,DamageArray()*50,5,0,10,owner,false,1.0f, false, 0, unit, ZeroVector, -1);
	}
	if(flags & PP_Smoke){
		float3 dir=speed;
		dir.Normalize();
		CSmokeTrailProjectile* tp=SAFE_NEW CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,false,true,7,Smoke_Time,0.5f,drawTrail);
		tp->creationTime+=8-((age)&7);
	}
	CProjectile::Collision(unit);
	oldSmoke=pos;
}

bool CPieceProjectile::HasVertices(void)
{	
	if (piece3do != NULL) {
		/* 3DO */
		return !piece3do->vertices.empty();
	}
	else {
		/* S3O */
		return !pieces3o->vertexDrawOrder.empty();
	}
}

float3 CPieceProjectile::RandomVertexPos(void)
{
	float3 pos;

	if (piece3do != NULL) { 
		/* 3DO */
		int vertexNum=(int) (gu->usRandFloat()*0.99f*piece3do->vertices.size());
		pos=piece3do->vertices[vertexNum].pos;
	}
	else {
		/* S3O */
		int vertexNum=(int) (gu->usRandFloat()*0.99f*pieces3o->vertexDrawOrder.size());
		pos=pieces3o->vertices[pieces3o->vertexDrawOrder[vertexNum]].pos;
	}
	
	return pos;
}

void CPieceProjectile::Update()
{
	if (flags & PP_Fall)
		speed.y += gs->gravity;
/*	speed.x *= 0.994f;
	speed.z *= 0.994f;
	if(speed.y > 0)
		speed.y *= 0.998f;*/
	speed*=0.997f;
	pos += speed;
	spinPos+=spinSpeed;

	*numCallback=0;
	if ((flags & PP_Fire && HasVertices() )/* && (gs->frameNum & 1)*/) {
		ENTER_MIXED;
		OldInfo* tempOldInfo=oldInfos[7];
		for(int a=6;a>=0;--a){
			oldInfos[a+1]=oldInfos[a];
		}
		CMatrix44f m;
		m.Translate(pos.x,pos.y,pos.z);
		m.Rotate(spinPos*PI/180,spinVec);
		float3 pos=RandomVertexPos();
		m.Translate(pos.x,pos.y,pos.z);

		oldInfos[0]=tempOldInfo;
		oldInfos[0]->pos=m.GetPos();
		oldInfos[0]->size=gu->usRandFloat()*1+1;
		ENTER_SYNCED;
	}

	age++;
	if(!(age & 7) && (flags & PP_Smoke)){
		float3 dir=speed;
		dir.Normalize();
		if(curCallback)
			curCallback->drawCallbacker=0;
		curCallback=SAFE_NEW CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner,age==8,false,14,Smoke_Time,0.5f,drawTrail,this);
		useAirLos=curCallback->useAirLos;
		oldSmoke=pos;
		oldSmokeDir=dir;
		if(!drawTrail){
			float3 camDir=(pos-camera->pos).Normalize();
			if(camera->pos.distance(pos)+(1-fabs(camDir.dot(dir)))*3000 > 300)
				drawTrail=true;
		}
	}
	if(age>10)
		checkCol=true;
}

void CPieceProjectile::Draw()
{
	if(flags & PP_Smoke){
		float3 interPos=pos+speed*gu->timeOffset;
		inArray=true;
		float age2=(age&7)+gu->timeOffset;

		float color=0.5f;
		unsigned char col[4];

		float3 dir=speed;
		dir.Normalize();

		if(drawTrail){		//draw the trail as a single quad
			float3 dif(interPos-camera->pos);
			dif.Normalize();
			float3 dir1(dif.cross(dir));
			dir1.Normalize();
			float3 dif2(oldSmoke-camera->pos);
			dif2.Normalize();
			float3 dir2(dif2.cross(oldSmokeDir));
			dir2.Normalize();


			float a1=(1-float(0)/(Smoke_Time))*255;
			a1*=0.7f+fabs(dif.dot(dir));
			float alpha=min(255.f,max(0.f,a1));
			col[0]=(unsigned char) (color*alpha);
			col[1]=(unsigned char) (color*alpha);
			col[2]=(unsigned char) (color*alpha);
			col[3]=(unsigned char) (alpha);

			unsigned char col2[4];
			float a2=(1-float(age2)/(Smoke_Time))*255;
			a2*=0.7f+fabs(dif2.dot(oldSmokeDir));
			if(age<8)
				a2=0;
			alpha=min(255.f,max(0.f,a2));
			col2[0]=(unsigned char) (color*alpha);
			col2[1]=(unsigned char) (color*alpha);
			col2[2]=(unsigned char) (color*alpha);
			col2[3]=(unsigned char) (alpha);

			float xmod=0;
			float ymod=0.25f;
			float size=(1);
			float size2=1+(age2*(1/Smoke_Time))*14;

			float txs=ph->smoketrailtex.xstart - (ph->smoketrailtex.xend-ph->smoketrailtex.xstart)*(age2/8.0f);//(1-age2/8.0f);
			va->AddVertexTC(interPos-dir1*size, txs, ph->smoketrailtex.ystart, col);
			va->AddVertexTC(interPos+dir1*size, txs, ph->smoketrailtex.yend, col);
			va->AddVertexTC(oldSmoke+dir2*size2, ph->smoketrailtex.xend, ph->smoketrailtex.yend, col2);
			va->AddVertexTC(oldSmoke-dir2*size2, ph->smoketrailtex.xend, ph->smoketrailtex.ystart, col2);
		} else {	//draw the trail as particles
			float dist=pos.distance(oldSmoke);
			float3 dirpos1=pos-dir*dist*0.33f;
			float3 dirpos2=oldSmoke+oldSmokeDir*dist*0.33f;

			int numParts=age&7;
			for(int a=0;a<numParts;++a){
				float a1=1-float(a)/Smoke_Time;
				float alpha=255;
				col[0]=(unsigned char) (color*alpha);
				col[1]=(unsigned char) (color*alpha);
				col[2]=(unsigned char) (color*alpha);
				col[3]=(unsigned char) (alpha);//min(255,max(0,a1*255));
				float size=1+((a)*(1/Smoke_Time))*14;

				float3 pos1=CalcBeizer(float(a)/(numParts),pos,dirpos1,dirpos2,oldSmoke);
				va->AddVertexTC(pos1+( camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
				va->AddVertexTC(pos1+( camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
				va->AddVertexTC(pos1+(-camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
				va->AddVertexTC(pos1+(-camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
			}
		}
	}
	DrawCallback();
	if(curCallback==0)
		DrawCallback();
}

void CPieceProjectile::DrawUnitPart(void)
{
	glPushMatrix();
	glTranslatef(pos.x,pos.y,pos.z);
	glRotatef(spinPos,spinVec.x,spinVec.y,spinVec.z);
	//glRotatef(180, 0, 1, 0);
	glCallList(dispList);
	glPopMatrix();
	*numCallback=0;
}

void CPieceProjectile::DrawS3O(void)
{
	/* TODO Hmm, S3O nuclear missile... might want to copy this into WeaponProjectile? */
	unitDrawer->SetS3OTeamColour(team);
	DrawUnitPart();
}

void CPieceProjectile::DrawCallback(void)
{
	(*numCallback)++;
	if((*numCallback)<2){
		return;
	}
	(*numCallback)=0;

	inArray=true;
	unsigned char col[4];

	if (flags & PP_Fire) {
		for(int age=0;age<8;++age){
			float modage=age;
			float3 interPos=oldInfos[age]->pos;
			float size=oldInfos[age]->size;

			float alpha=(7.5f-modage)*(1.0f/8);
//			alpha*=alpha;
			col[0]=(unsigned char) (255*alpha);
			col[1]=(unsigned char) (200*alpha);
			col[2]=(unsigned char) (150*alpha);
			col[3]=(unsigned char) (alpha*50);
			float drawsize=(0.5f+modage)*size;
			va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,ph->explofadetex.xstart,ph->explofadetex.ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,ph->explofadetex.xend,ph->explofadetex.ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,ph->explofadetex.xend,ph->explofadetex.yend,col);
			va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,ph->explofadetex.xstart,ph->explofadetex.yend,col);
		}
	}
}
