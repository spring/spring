#include "StdAfx.h"
#include "SolidObject.h"
#include "Map/ReadMap.h"
//#include "PhysicsEngine.h"
#include "Rendering/GL/myGL.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "myMath.h"
#include "Rendering/GL/glExtra.h"

CR_BIND_DERIVED(CSolidObject, CWorldObject, );

CR_REG_METADATA(CSolidObject, 
(
	CR_MEMBER(mass),
	CR_MEMBER(blocking),
	CR_MEMBER(floatOnWater),
	CR_MEMBER(isUnderWater),
	CR_MEMBER(immobile),
	CR_MEMBER(blockHeightChanges),
	CR_MEMBER(xsize),
	CR_MEMBER(ysize),
	CR_MEMBER(height),
	CR_MEMBER(heading),
	CR_MEMBER(midPos),
	CR_MEMBER(isMoving),
	CR_MEMBER(residualImpulse),
	CR_MEMBER(mobility),
	CR_MEMBER(mapPos),
	CR_MEMBER(isMarkedOnBlockingMap),
	CR_ENUM_MEMBER(physicalState),
	CR_MEMBER(speed))
);

CSolidObject::CSolidObject(EObjectType synced):
	CWorldObject(synced),
	mass(100000),
	blocking(false),
	blockHeightChanges(false),
	floatOnWater(false),
	xsize(1),
	ysize(1),
	height(1),
	//relMidPos(0,0,0),

	physicalState(Ghost),
	midPos(pos),

	immobile(false),
	//velocity(0,0,0),
	//momentum(0,0,0),
	isMoving(false),
	heading(0),
	buildFacing(0),
	//newVelocity(0,0,0),
	//newMomentum(0,0,0),
	residualImpulse(0,0,0),

	mobility(0),
	yardMap(0),
	isMarkedOnBlockingMap(false),

	speed(0,0,0),
	isUnderWater(false)
{
	mapPos=GetMapPos();
//	mapPos.x=0;
//	mapPos.y=0;
}


/*
Destructor
*/
CSolidObject::~CSolidObject() {
	UnBlock();
//	readmap->CleanBlockingMap(this);	//Debug

	if(mobility)
		delete mobility;
}


/*
Draws some additional information about this object.
*/
void CSolidObject::Draw() {
/*	if(gu->drawdebug) {
		glDisable(GL_TEXTURE_2D);

		//Draw heading-directions.
		glBegin(GL_LINE_STRIP);
		glColor4f(0,0,1,1);
		glVertexf3(midPos);
		glVertexf3(midPos + frontdir*50);
		glEnd();

		glBegin(GL_LINE_STRIP);
		glColor4f(1,1,0,1);
		glVertexf3(midPos);
		glVertexf3(midPos + rightdir*50);
		glEnd();

		glBegin(GL_LINE_STRIP);
		glColor4f(0,1,0,1);
		glVertexf3(midPos);
		glVertexf3(midPos + updir*50);
		glEnd();

		//Draw velocity-vector.
		glBegin(GL_LINE_STRIP);
		glColor4f(1,0,0,1);
		glVertexf3(midPos);
		glVertexf3(midPos + velocity*50);
		glEnd();

		//Draw blocked area on GroundBlockingMap.
		if(isMarkedOnBlockingMap) {
			glBegin(GL_LINE_STRIP);
			glColor4f(1,0,0,1);
			glVertexf3((mapPos + float3(xsize*SQUARE_SIZE/2, 5, ysize*SQUARE_SIZE/2)));
			glVertexf3((mapPos + float3(-xsize*SQUARE_SIZE/2, 5, ysize*SQUARE_SIZE/2)));
			glVertexf3((mapPos + float3(-xsize*SQUARE_SIZE/2, 5, -ysize*SQUARE_SIZE/2)));
			glVertexf3((mapPos + float3(xsize*SQUARE_SIZE/2, 5, -ysize*SQUARE_SIZE/2)));
			glVertexf3((mapPos + float3(xsize*SQUARE_SIZE/2, 5, ysize*SQUARE_SIZE/2)));
			glEnd();
		}

		glEnable(GL_TEXTURE_2D);
	}*/
}



/////////////////////
// Useful fuctions //
/////////////////////

/*
Removes this object from GroundBlockingMap.
*/
void CSolidObject::UnBlock() {
	if(isMarkedOnBlockingMap) {
		readmap->RemoveGroundBlockingObject(this);
	}
}


/*
Adds this object to the GroundBlockingMap.
*/
void CSolidObject::Block() {
	if(isMarkedOnBlockingMap)
		UnBlock();

	if(blocking && (physicalState == OnGround || physicalState == Floating || physicalState == Hovering || physicalState == Submarine)){
		//Using yardmap if available.
		if(yardMap) {
			readmap->AddGroundBlockingObject(this, yardMap);
		} else {
			readmap->AddGroundBlockingObject(this);
		}
	}
}



bool CSolidObject::AddBuildPower(float amount, CUnit* builder) {
	return false;
}

int2 CSolidObject::GetMapPos()
{
	int2 p;
	p.x = (int(pos.x+SQUARE_SIZE/2) / SQUARE_SIZE) - xsize/2;
	p.y = (int(pos.z+SQUARE_SIZE/2) / SQUARE_SIZE) - ysize/2;

	if(p.x<0)
		p.x=0;
	if(p.x>gs->mapx-xsize)
		p.x=gs->mapx-xsize;

	if(p.y<0)
		p.y=0;
	if(p.y>gs->mapy-ysize)
		p.y=gs->mapy-ysize;

	return p;
}
