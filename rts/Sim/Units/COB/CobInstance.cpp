#include "StdAfx.h"
#include "CobInstance.h"
#include "CobThread.h"
#include "CobFile.h"
#include "CobEngine.h"

#ifndef _CONSOLE

#include "Sim/Units/Unit.h"
#include "Game/UI/InfoConsole.h"
#include <math.h>
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Sim/Projectiles/PieceProjectile.h"
#include "Sim/Projectiles/SmokeProjectile.h"
#include "Sim/Projectiles/WreckProjectile.h"
#include "Sim/Projectiles/HeatCloudProjectile.h"
#include "Game/GameHelper.h"
#include "Sim/Projectiles/MuzzleFlame.h"
#include "SyncTracer.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/BubbleProjectile.h"
#include "Sim/Projectiles/WakeProjectile.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sound.h"
#include "SDL_types.h"
#include "mmgr.h"

#endif

#ifdef _CONSOLE
/*
class CUnit {
public:
};
*/
class CUnit;

#endif

// Indices for set/get value
#define ACTIVATION			1	// set or get
#define STANDINGMOVEORDERS	2	// set or get
#define STANDINGFIREORDERS	3	// set or get
#define HEALTH				4	// get (0-100%)
#define INBUILDSTANCE		5	// set or get
#define BUSY				6	// set or get (used by misc. special case missions like transport ships)
#define PIECE_XZ			7	// get
#define PIECE_Y				8	// get
#define UNIT_XZ				9	// get
#define	UNIT_Y				10	// get
#define UNIT_HEIGHT			11	// get
#define XZ_ATAN				12	// get atan of packed x,z coords
#define XZ_HYPOT			13	// get hypot of packed x,z coords
#define ATAN				14	// get ordinary two-parameter atan
#define HYPOT				15	// get ordinary two-parameter hypot
#define GROUND_HEIGHT		16	// get
#define BUILD_PERCENT_LEFT	17	// get 0 = unit is built and ready, 1-100 = How much is left to build
#define YARD_OPEN			18	// set or get (change which plots we occupy when building opens and closes)
#define BUGGER_OFF			19	// set or get (ask other units to clear the area)
#define ARMORED				20	// set or get

/*#define WEAPON_AIM_ABORTED	21
#define WEAPON_READY		22
#define WEAPON_LAUNCH_NOW	23
#define FINISHED_DYING		26
#define ORIENTATION			27*/
#define IN_WATER   28
#define CURRENT_SPEED  29
//#define MAGIC_DEATH   31
#define VETERAN_LEVEL  32
#define ON_ROAD    34

#define MAX_ID					70
#define MY_ID					71
#define UNIT_TEAM				72
#define UNIT_BUILD_PERCENT_LEFT	73
#define UNIT_ALLIED				74

CCobInstance::CCobInstance(CCobFile &script, CUnit *unit)
: script(script)
{
	for (int i = 0; i < script.numStaticVars; ++i) {
		staticVars.push_back(0);
	}
	for (unsigned int i = 0; i < script.pieceNames.size(); ++i) {
		struct PieceInfo pi;

		//init from model?
		pi.coords[0] = 0; pi.coords[1] = 0; pi.coords[2] = 0;
		pi.rot[0] = 0; pi.rot[1] = 0; pi.rot[2] = 0;
		pi.name = script.pieceNames[i];
		pi.updated = false;
		pi.visible = true;
		
		pieces.push_back(pi);
	}

	this->unit = unit;
//	int mo = unit->pos.x;

	yardOpen = false;
	busy = false;
	smoothAnim = unit->unitDef->smoothAnim;
}

CCobInstance::~CCobInstance(void)
{
	//Can't delete the thread here because that would confuse the scheduler to no end
	//Instead, mark it as dead. It is the function calling Tick that is responsible for delete.
	//Also unregister all callbacks
	for (list<CCobThread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
		(*i)->state = CCobThread::Dead;
		(*i)->SetCallback(NULL, NULL, NULL);
	}

	// Remove us from possible animation ticking (should only be needed when anims.size() > 0
	GCobEngine.RemoveInstance(this);

	for (list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {

		//All threads blocking on animations can be killed safely from here since the scheduler does not
		//know about them
		for (list<CCobThread *>::iterator j = (*i)->listeners.begin(); j != (*i)->listeners.end(); ++j) {
			delete *j;
		}
		delete *i;
	}
}
int CCobInstance::Call(const string &fname)
{
	vector<int> x;
	return Call(fname, x, NULL, NULL, NULL);
}

int CCobInstance::Call(const string &fname, vector<int> &args)
{
	return Call(fname, args, NULL, NULL, NULL);
}

int CCobInstance::Call(const string &fname, int p1)
{
	vector<int> x;
	x.push_back(p1);
	return Call(fname, x, NULL, NULL, NULL);
}

int CCobInstance::Call(const string &fname, CBCobThreadFinish cb, void *p1, void *p2)
{
	vector<int> x;
	return Call(fname, x, cb, p1, p2);
}

int CCobInstance::Call(const string &fname, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	int fn = script.getFunctionId(fname);
	if (fn == -1) {
		//info->AddLine("CobError: unknown function %s called by user", fname.c_str());
		return -1;
	}

	return RealCall(fn, args, cb, p1, p2);
}

int CCobInstance::Call(int id)
{
	vector<int> x;
	return Call(id, x, NULL, NULL, NULL);
}

int CCobInstance::Call(int id, int p1)
{
	vector<int> x;
	x.push_back(p1);
	return Call(id, x, NULL, NULL, NULL);
}

int CCobInstance::Call(int id, vector<int> &args)
{
	return Call(id, args, NULL, NULL, NULL);
}

int CCobInstance::Call(int id, CBCobThreadFinish cb, void *p1, void *p2)
{
	vector<int> x;
	return Call(id, x, cb, p1, p2);
}

int CCobInstance::Call(int id, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	int fn = script.scriptIndex[id];
	if (fn == -1) {
		//info->AddLine("CobError: unknown function index %d called by user", id);
		if(cb){
			(*cb)(0, p1, p2);	//in case the function doesnt exist the callback should still be called
		}
		return -1;
	}

	return RealCall(fn, args, cb, p1, p2);
}

//Returns 0 if the call terminated. If the caller provides a callback and the thread does not terminate,
//it will continue to run. Otherwise it will be killed. Returns 1 in this case.
int CCobInstance::RealCall(int functionId, vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2)
{
	CCobThread *t = new CCobThread(script, this);
	t->Start(functionId, args, false);

#if COB_DEBUG > 0
	if (COB_DEBUG_FILTER)
		info->AddLine("Calling %s:%s", script.name.c_str(), script.scriptNames[functionId].c_str());
#endif

	int res = t->Tick(30);
	t->CommitAnims(30);

	//Make sure this is run even if the call terminates instantly
	if (cb) 
		t->SetCallback(cb, p1, p2);

	if (res == -1) {
		//Retrieve parameter values from stack		
		for (unsigned int i = 0; i < args.size(); ++i) {
			args[i] = t->GetStackVal(i);
		}
		delete t;
		return 0;
	}
	else {
		//It has already added itself to the correct scheduler (global for sleep, or local for anim)
		return 1;
	}
}

//Updates cur, returns 1 if destination was reached, 0 otherwise
int CCobInstance::MoveToward(int &cur, int dest, int speed)
{
	if (dest > cur) {
		cur += speed;
		if (cur > dest) {
			cur = dest;
			return 1;
		}
	}
	else {
		cur -= speed;
		if (cur < dest) {
			cur = dest;
			return 1;
		}
	}

	return 0;
}

//Updates cur, returns 1 if destination was reached, 0 otherwise
int CCobInstance::TurnToward(int &cur, int dest, int speed)
{
	short int delta = dest - cur;
	if (delta > 0) {
		if (delta <= speed) {
			cur = dest;
			return 1;
		}
		cur += speed;
	}
	else {
		if (delta >= -speed) {
			cur = dest;
			return 1;
		}
		cur -= speed;
	}
	cur %= 65536;

	//info->AddLine("turning %d %d %d", cur, dest, speed);

	return 0;
}

//Dest is not the final angle (obviously) but the final desired speed
//Returns 1 if the desired speed is 0 and it is reached, otherwise 0
//Speed is updated if it is not equal to dest
//Divisor is the deltatime, it is not added before the call because speed may have to be updated
int CCobInstance::DoSpin(int &cur, int dest, int &speed, int accel, int divisor)
{
	//Check if we are not at the final speed
	if (speed != dest) {
		speed += accel * 30 / divisor;		//TA obviously defines accelerations in speed/frame (at 30 fps)
		if (accel > 0) {
			if (speed > dest)		
				speed = dest;		//We are accelerating, make sure we dont go past desired speed
		}
		else {
			if (speed < dest)
				speed = dest;		//Decelerating
			if (speed == 0)
				return 1;
		}
	}

	//info->AddLine("Spinning with %d %d %d %d", dest, speed, accel, divisor);

	cur += speed / divisor;
	cur %= 65536;

	return 0;
}

// Unblocks all threads waiting on this animation
void CCobInstance::UnblockAll(struct AnimInfo * anim)
{
	list<CCobThread *>::iterator li;

	for (li = anim->listeners.begin(); li != anim->listeners.end(); ++li) {
		//Not sure how to do this more cleanly.. Will probably rewrite it
		if (((*li)->state == CCobThread::WaitMove) ||((*li)->state == CCobThread::WaitTurn)) {
			(*li)->state = CCobThread::Run;
			GCobEngine.AddThread(*li);
		}
		else if ((*li)->state == CCobThread::Dead) {
			delete *li;
		}
		else {
			info->AddLine("CobError: Turn/move listenener in strange state %d", (*li)->state);
		}
	}
}

//Called by the engine when we are registered as animating. If we return -1 it means that
//there is no longer anything animating
int CCobInstance::Tick(int deltaTime) 
{
	int done;
	list<struct AnimInfo *>::iterator it = anims.begin();
	list<struct AnimInfo *>::iterator cur;

	while (it != anims.end()) {
		//Advance it, so we can erase cur safely
		cur = it++;		

		done = false;
		pieces[(*cur)->piece].updated = true;

		switch ((*cur)->type) {
			case AMove:
				done = MoveToward(pieces[(*cur)->piece].coords[(*cur)->axis], (*cur)->dest, (*cur)->speed / (1000 / deltaTime));
				break;
			case ATurn:
				done = TurnToward(pieces[(*cur)->piece].rot[(*cur)->axis], (*cur)->dest, (*cur)->speed / (1000 / deltaTime));
				break;
			case ASpin:
				done = DoSpin(pieces[(*cur)->piece].rot[(*cur)->axis], (*cur)->dest, (*cur)->speed, (*cur)->accel, 1000 / deltaTime);
				break;
		}

		//Tell listeners to unblock?
		if (done) {
			UnblockAll(*cur);
			delete *cur;
			anims.erase(cur);
		}

	}

	if (anims.size() == 0) 
		return -1;
	else
		return 0;
}

//Optimize this? 
//Returns anims list
struct CCobInstance::AnimInfo *CCobInstance::FindAnim(AnimType type, int piece, int axis)
{
	for (list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		if (((*i)->type == type) && ((*i)->piece == piece) && ((*i)->axis == axis))
			return *i;
	}
	return NULL;
}

// Returns true if an animation was found and deleted
void CCobInstance::RemoveAnim(AnimType type, int piece, int axis)
{
	for (list<struct AnimInfo *>::iterator i = anims.begin(); i != anims.end(); ++i) {
		if (((*i)->type == type) && ((*i)->piece == piece) && ((*i)->axis == axis)) {

			// We need to unblock threads waiting on this animation, otherwise they will be lost in the void
			UnblockAll(*i);

			delete *i;
			anims.erase(i);

			// If this was the last animation, remove from currently animating list
			if (anims.size() == 0) {
				GCobEngine.RemoveInstance(this);
			}
			return;
		}
	}
}

//Overwrites old information. This means that threads blocking on turn completion
//will now wait for this new turn instead. Not sure if this is the expected behaviour
//Other option would be to kill them. Or perhaps unblock them.
void CCobInstance::AddAnim(AnimType type, int piece, int axis, int speed, int dest, int accel, bool interpolated)
{
	struct AnimInfo *ai;

	//Turns override spins.. Not sure about the other way around? If so the system should probably be redesigned
	//to only have two types of anims.. turns and moves, with spin as a bool
	if (type == ATurn)
		RemoveAnim(ASpin, piece, axis);
	if (type == ASpin)
		RemoveAnim(ATurn, piece, axis);

	ai = FindAnim(type, piece, axis);
	if (!ai) {
		ai = new struct AnimInfo;
		ai->type = type;
		ai->piece = piece;
		ai->axis = axis;
		anims.push_back(ai);

		//If we were not animating before, inform the engine of this so it can schedule us
		if (anims.size() == 1) {
			GCobEngine.AddInstance(this);
		}

		// Check to make sure the piece exists
		if (piece >= pieces.size()) {
			info->AddLine("Invalid piece in anim %d (%d)", piece, pieces.size());
		}
	}
	ai->speed = speed;
	ai->dest = dest;
	ai->accel = accel;
	ai->interpolated = interpolated;
}

void CCobInstance::Spin(int piece, int axis, int speed, int accel)
{
	struct AnimInfo *ai;
	ai = FindAnim(ASpin, piece, axis);

	//info->AddLine("Spin called %d %d %d %d", piece, axis, speed, accel);

	//Test of default acceleration
	if (accel == 0)
		accel = 1000;

	//If we are already spinning, we may have to decelerate to the new speed
	if (ai) {
		ai->dest = speed;		
		if (accel > 0) {
			if (ai->speed > ai->dest)
				ai->accel = -accel;
			else
				ai->accel = accel;
		}
		else {
			//Go there instantly. Or have a defaul accel?
			ai->speed = speed;
			ai->accel = 0;
		}
	}
	else {
		//No accel means we start at desired speed instantly
		if (accel == 0)
			AddAnim(ASpin, piece, axis, speed, speed, accel);			
		else
			AddAnim(ASpin, piece, axis, 0, speed, accel);
	}
}

void CCobInstance::StopSpin(int piece, int axis, int decel)
{
	struct AnimInfo *ai;
	ai = FindAnim(ASpin, piece, axis);
	if (!ai)
		return;

	if (decel == 0) {
		RemoveAnim(ASpin, piece, axis);
	}
	else
		AddAnim(ASpin, piece, axis, ai->speed, 0, -decel);
}

void CCobInstance::Turn(int piece, int axis, int speed, int destination, bool interpolated)
{
	AddAnim(ATurn, piece, axis, speed, destination % 65536, 0, interpolated);
}

void CCobInstance::Move(int piece, int axis, int speed, int destination, bool interpolated)
{
	AddAnim(AMove, piece, axis, speed, destination, 0, interpolated);
}

void CCobInstance::MoveNow(int piece, int axis, int destination)
{
	pieces[piece].coords[axis] = destination;
	pieces[piece].updated = true;
}

void CCobInstance::TurnNow(int piece, int axis, int destination)
{
	pieces[piece].rot[axis] = destination;
	pieces[piece].updated = true;	
	//info->AddLine("moving %s on axis %d to %d", script.pieceNames[piece].c_str(), axis, destination);
}

void CCobInstance::SetVisibility(int piece, bool visible)
{
	if (pieces[piece].visible != visible) {
		pieces[piece].visible = visible;
		pieces[piece].updated = true;
	}
}

void CCobInstance::EmitSfx(int type, int piece)
{
	if (!unit->localmodel->PieceExists(piece)) {
		GCobEngine.ShowScriptError("Invalid piecenumber for emit-sfx");
		return;
	}

#ifndef _CONSOLE
	ENTER_MIXED;
	if(ph->particleSaturation>1){		//skip adding particles when we have to many (make sure below can be unsynced)
		ENTER_SYNCED;
		return;
	}

	float3 relPos = unit->localmodel->GetPiecePos(piece);
	float3 pos = unit->pos + unit->frontdir * relPos.z + unit->updir * relPos.y + unit->rightdir * relPos.x;

	float alpha = 0.3+gu->usRandFloat()*0.2;
	float alphaFalloff = 0.004;
	float fadeupTime=4;

	//Hovers need special care
	if (unit->unitDef->canhover) {		
		fadeupTime=8;
		alpha = 0.15+gu->usRandFloat()*0.2;
		alphaFalloff = 0.008;
	}
	
	//Make sure wakes are only emitted on water
	if ((type >= 2) && (type <= 5)) {
		if (ground->GetApproximateHeight(unit->pos.x, unit->pos.z) > 0){
			ENTER_SYNCED;
			return;
		}
	}

	switch (type) {
		case 4:
		case 5:		{	//reverse wake
			float3 relDir = -unit->localmodel->GetPieceDirection(piece) * 0.2;
			float3 dir = unit->frontdir * relDir.z + unit->updir * relDir.y + unit->rightdir * relDir.x;
			new CWakeProjectile(pos+gu->usRandVector()*2,dir*0.4,6+gu->usRandFloat()*4,0.15+gu->usRandFloat()*0.3,unit, alpha, alphaFalloff,fadeupTime);
			break;}
		case 3:			//wake 2, in TA it lives longer..
		case 2:		{	//regular ship wake
			float3 relDir = unit->localmodel->GetPieceDirection(piece) * 0.2;
			float3 dir = unit->frontdir * relDir.z + unit->updir * relDir.y + unit->rightdir * relDir.x;
			new CWakeProjectile(pos+gu->usRandVector()*2,dir*0.4,6+gu->usRandFloat()*4,0.15+gu->usRandFloat()*0.3,unit, alpha, alphaFalloff,fadeupTime);
			break;}
		case 259:	{	//submarine bubble. does not provide direction through piece vertices..
			float3 pspeed=gu->usRandVector()*0.1;
			pspeed.y+=0.2;
			new CBubbleProjectile(pos+gu->usRandVector()*2,pspeed,40+gu->usRandFloat()*30,1+gu->usRandFloat()*2,0.01,unit,0.3+gu->usRandFloat()*0.3);
			break;}
		case 257:	//damaged unit smoke
			new CSmokeProjectile(pos,gu->usRandVector()*0.5+UpVector*1.1,60,4,0.5,unit,0.5);
		case 258:		//damaged unit smoke
			new CSmokeProjectile(pos,gu->usRandVector()*0.5+UpVector*1.1,60,4,0.5,unit,0.6);
			break;
		case 0:{		//vtol
			float3 relDir = unit->localmodel->GetPieceDirection(piece) * 0.2;
			float3 dir = unit->frontdir * relDir.z + unit->updir * -fabs(relDir.y) + unit->rightdir * relDir.x;
			CHeatCloudProjectile* hc=new CHeatCloudProjectile(pos, unit->speed*0.7+dir * 0.5, 10 + gu->usRandFloat() * 5, 3 + gu->usRandFloat() * 2, unit);
			hc->size=3;
			break;}
		default:
			info->AddLine("Unknown sfx: %d", type);
			break;
	}
	ENTER_SYNCED;
#endif
}

void CCobInstance::AttachUnit(int piece, int u)
{
	// -1 is valid, indicates that the unit should be hidden
	if ((piece >= 0) && (!unit->localmodel->PieceExists(piece))) {
		GCobEngine.ShowScriptError("Invalid piecenumber for attach");
		return;
	}

#ifndef _CONSOLE
	CTransportUnit* tu=dynamic_cast<CTransportUnit*>(unit);

	if(tu && uh->units[u]){
		//info->AddLine("attach");
		tu->AttachUnit(uh->units[u],piece);
	}
#endif
}

void CCobInstance::DropUnit(int u)
{
#ifndef _CONSOLE
	CTransportUnit* tu=dynamic_cast<CTransportUnit*>(unit);

	if(tu && uh->units[u]){
		tu->DetachUnit(uh->units[u]);
	}
#endif
}

//Returns 1 if there was a turn to listen to
int CCobInstance::AddTurnListener(int piece, int axis, CCobThread *listener)
{
	struct AnimInfo *ai;
	ai = FindAnim(ATurn, piece, axis);
	if (ai) {
		ai->listeners.push_back(listener);
		return 1;
	}
	else 
		return 0;
}

int CCobInstance::AddMoveListener(int piece, int axis, CCobThread *listener)
{
	struct AnimInfo *ai;
	ai = FindAnim(AMove, piece, axis);
	if (ai) {
		ai->listeners.push_back(listener);
		return 1;
	}
	else 
		return 0;	
}

void CCobInstance::Signal(int signal)
{
	for (list<CCobThread *>::iterator i = threads.begin(); i != threads.end(); ++i) {
		if ((signal & (*i)->signalMask) != 0) {
			(*i)->state = CCobThread::Dead;
			//info->AddLine("Killing a thread %d %d", signal, (*i)->signalMask);
		}
	}
}
//Flags as defined by the cob standard
void CCobInstance::Explode(int piece, int flags)
{
	if (!unit->localmodel->PieceExists(piece)) {
		GCobEngine.ShowScriptError("Invalid piecenumber for explode");
		return;
	}


#ifndef _CONSOLE
	float3 pos = unit->localmodel->GetPiecePos(piece) + unit->pos;

#ifdef TRACE_SYNC
	tracefile << "Cob explosion: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << piece << " " << flags << "\n";
#endif

	//Do an explosion at the location first
	CHeatCloudProjectile* p=new CHeatCloudProjectile(pos, float3(0, 0, 0), 30, 30, NULL);

	//If this is true, no stuff should fly off
	if (flags & 32) 
		return;

	//This means that we are going to do a full fledged piece exlosion!
	int newflags = 0;
	if (flags & 2)
		newflags |= PP_Explode;
//	if (flags & 4)
		newflags |= PP_Fall;	//if they dont fall they could live forever
	if ((flags & 8) && ph->particleSaturation<1)
		newflags |= PP_Smoke;
	if ((flags & 16) && ph->particleSaturation<0.95)
		newflags |= PP_Fire;

	float3 baseSpeed=unit->speed+unit->residualImpulse*0.5;
	float l=baseSpeed.Length();
	if(l>3){
		float l2=3+sqrt(l-3);
		baseSpeed*=(l2/l);
	}
	float3 speed((0.5f-gs->randFloat())*6.0,1.2f+gs->randFloat()*5.0,(0.5f-gs->randFloat())*6.0);
	if(unit->pos.y - ground->GetApproximateHeight(unit->pos.x,unit->pos.z) > 15){
		speed.y=(0.5f-gs->randFloat())*6.0;
	}
	speed+=baseSpeed;
	if(speed.Length()>12)
		speed=speed.Normalize()*12;
	
	/* TODO Push this back. Don't forget to pass the team (color).  */

	LocalS3DO * pieceData = &( unit->localmodel->pieces[unit->localmodel->scritoa[piece]] );
	if (flags & 1) {		//Shatter
		ENTER_MIXED;
		
		float pieceChance=1-(ph->currentParticles-(ph->maxParticles-2000))/2000;
//		info->AddLine("Shattering %i %f",dl->prims.size(),pieceChance);

		S3DO* dl = pieceData->original3do;
		if(dl){
			/* 3DO */

			for(std::vector<S3DOPrimitive>::iterator pi=dl->prims.begin();pi!=dl->prims.end();++pi){
				if(gu->usRandFloat()>pieceChance || pi->numVertex!=4)
					continue;

				ph->AddFlyingPiece(pos,speed+gu->usRandVector()*2,dl,&*pi);
			}
		}
		SS3O* cookedPiece = pieceData->originals3o;
		if (cookedPiece){
			/* S3O */
		
			if (cookedPiece->primitiveType == 0){
				/* GL_TRIANGLES */
				
				for (int i = 0; i < cookedPiece->vertexDrawOrder.size(); i += 3){
					if(gu->usRandFloat()>pieceChance)
						continue;
					
					SS3OVertex * verts = new SS3OVertex[4];
					
					verts[0] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 0]];
					verts[1] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 1]];
					verts[2] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 1]];
					verts[3] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 2]];
	
					ph->AddFlyingPiece(unit->model->textureType,
						unit->team,
						pos, speed+gu->usRandVector()*2, verts);
				}
			} else if (cookedPiece->primitiveType == 1){
				/* GL_TRIANGLE_STRIP */
				for (int i = 2; i < cookedPiece->vertexDrawOrder.size(); i++){
					if(gu->usRandFloat()>pieceChance)
						continue;
					
					SS3OVertex * verts = new SS3OVertex[4];
					
					verts[0] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i - 2]];
					verts[1] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i - 1]];
					verts[2] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i - 1]];
					verts[3] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i - 0]];
	
					ph->AddFlyingPiece(unit->model->textureType,
						unit->team,
						pos, speed+gu->usRandVector()*2, verts);
				}
			} else if (cookedPiece->primitiveType == 2){
				/* GL_QUADS */
				
				for (int i = 0; i < cookedPiece->vertexDrawOrder.size(); i += 4){
					if(gu->usRandFloat()>pieceChance)
						continue;
					
					SS3OVertex * verts = new SS3OVertex[4];
					
					verts[0] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 0]];
					verts[1] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 1]];
					verts[2] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 2]];
					verts[3] = cookedPiece->vertices[cookedPiece->vertexDrawOrder[i + 3]];
	
					ph->AddFlyingPiece(unit->model->textureType,
						unit->team,
						pos, speed+gu->usRandVector()*2, verts);
				}
			}
		}
		ENTER_SYNCED;
	}
	else {
		if (pieceData->original3do != NULL || pieceData->originals3o != NULL) {
			//info->AddLine("Exploding %s as %d", script.pieceNames[piece].c_str(), dl);
			new CPieceProjectile(pos, speed, pieceData, newflags,unit,0.5);
		}
	}
#endif
}

void CCobInstance::PlayUnitSound(int snr, int attr)
{
	int sid = script.sounds[snr];
	//info->AddLine("Playing %d %d %d", snr, attr, sid);
	sound->PlaySound(sid, unit->pos, attr);
}

void CCobInstance::ShowFlare(int piece)
{
	if (!unit->localmodel->PieceExists(piece)) {
		GCobEngine.ShowScriptError("Invalid piecenumber for show(flare)");
		return;
	}
#ifndef _CONSOLE
	float3 relpos = unit->localmodel->GetPiecePos(piece);
	float3 pos=unit->pos + unit->frontdir*relpos.z + unit->updir*relpos.y + unit->rightdir*relpos.x;
	float3 dir=unit->lastMuzzleFlameDir;

	float size=unit->lastMuzzleFlameSize;

	new CMuzzleFlame(pos, unit->speed,dir, size);
#endif
}

#define UNPACKX(xz) ((signed short)((Uint32)(xz) >> 16))
#define UNPACKZ(xz) ((signed short)((Uint32)(xz) & 0xffff))
#define PACKXZ(x,z) (((int)(x) << 16)+((int)(z) & 0xffff))
#define SCALE 65536

#ifdef _WIN32
#define hypot(x,y) _hypot(x,y)
#endif

int CCobInstance::GetUnitVal(int val, int p1, int p2, int p3, int p4)
{
#ifndef _CONSOLE
	switch(val)
	{
	case ACTIVATION:
		if (unit->activated)
			return 1;
		else
			return 0;
		break;
	case STANDINGMOVEORDERS:
		break;
	case STANDINGFIREORDERS:
		break;
	case HEALTH:
		return (int) ((unit->health/unit->maxHealth)*100.0f);
	case INBUILDSTANCE:
		if (unit->inBuildStance)
			return 1;
		else
			return 0;
	case BUSY:
		if (busy)
			return 1;
		else
			return 0;
		break;
	case PIECE_XZ:{
		if (!unit->localmodel->PieceExists(p1))
			GCobEngine.ShowScriptError("Invalid piecenumber for get piece_xz");
		float3 relPos = unit->localmodel->GetPiecePos(p1);
		float3 pos = unit->pos + unit->frontdir * relPos.z + unit->updir * relPos.y + unit->rightdir * relPos.x;
		return PACKXZ(pos.x, pos.z);}
	case PIECE_Y:{
		if (!unit->localmodel->PieceExists(p1))
			GCobEngine.ShowScriptError("Invalid piecenumber for get piece_y");
		float3 relPos = unit->localmodel->GetPiecePos(p1);
		float3 pos = unit->pos + unit->frontdir * relPos.z + unit->updir * relPos.y + unit->rightdir * relPos.x;
		return (int)(pos.y * SCALE);}
	case UNIT_XZ: {
		if (p1 == 0)	
			return PACKXZ(unit->pos.x, unit->pos.z);
		CUnit *u = (p1 < MAX_UNITS) ? uh->units[p1] : NULL;
		if (u == NULL)
			return PACKXZ(0,0);
		else
			return PACKXZ(u->pos.x, u->pos.z);}
	case UNIT_Y: {
		//info->AddLine("Unit-y %d", p1);
		if (p1 == 0)
			return (int)(unit->pos.y * SCALE);
		CUnit *u = (p1 < MAX_UNITS) ? uh->units[p1] : NULL;
		if (u == NULL)
			return 0;
		else
			return (int)(u->pos.y * SCALE);}
	case UNIT_HEIGHT:{
		if (p1 == 0)
			return (int)(unit->radius * SCALE);
		CUnit *u = (p1 < MAX_UNITS) ? uh->units[p1] : NULL;
		if (u == NULL)
			return 0;
		else
			return (int)(u->radius * SCALE);}
	case XZ_ATAN:
		return (int)(TAANG2RAD*atan2f(UNPACKX(p1), UNPACKZ(p1)) + 32768 - unit->heading);
	case XZ_HYPOT:
		return (int)(hypot(UNPACKX(p1), UNPACKZ(p1)) * SCALE);
	case ATAN:
		return (int)(TAANG2RAD*atan2f(p1, p2));
	case HYPOT:
		return (int)hypot(p1, p2);
	case GROUND_HEIGHT:
		return (int)(ground->GetHeight(UNPACKX(p1), UNPACKZ(p1)) * SCALE);
	case BUILD_PERCENT_LEFT:
		return (int)((1 - unit->buildProgress) * 100);
	case YARD_OPEN:
		if (yardOpen)
			return 1;
		else
			return 0;
	case BUGGER_OFF:
		break;
	case ARMORED:
		if (unit->armoredState)
			return 1;
		else
			return 0;
	default:
		info->AddLine("CobError: Unknown get constant %d", val);
	case VETERAN_LEVEL: 
		return (int)(100*unit->experience);
	case CURRENT_SPEED:
		if (unit->moveType)
			return (int)(unit->speed.Length()*SCALE);
		return 0;
	case ON_ROAD:
		return 0;
	case IN_WATER:
		return (unit->pos.y < 0.0f) ? 1 : 0;
	case MAX_ID:
		return MAX_UNITS-1;
	case MY_ID:
		return unit->id;
	case UNIT_TEAM:{
		CUnit *u = (p1 >= 0 && p1 < MAX_UNITS) ? uh->units[p1] : NULL;
		return u ? unit->team : 0; }
	case UNIT_ALLIED:{
		CUnit *u = (p1 >= 0 && p1 < MAX_UNITS) ? uh->units[p1] : NULL;
		if (u) return gs->Ally (unit->allyteam, u->allyteam) ? 1 : 0;
		return 0;}
	case UNIT_BUILD_PERCENT_LEFT:{
		CUnit *u = (p1 >= 0 && p1 < MAX_UNITS) ? uh->units[p1] : NULL;
		if (u) return (int)((1 - u->buildProgress) * 100);
		return 0;}
	}
#endif

	return 0;
}

void CCobInstance::SetUnitVal(int val, int param)
{
#ifndef _CONSOLE
	switch(val)
	{
	case ACTIVATION:
		if (param == 0)
			unit->Deactivate();
		else
			unit->Activate();
		break;
	case STANDINGMOVEORDERS:
		break;
	case STANDINGFIREORDERS:
		break;
	case HEALTH:
		break;
	case INBUILDSTANCE:
		//info->AddLine("buildstance %d", param);
		unit->inBuildStance = (param != 0);
		break;
	case BUSY:
		busy = (param != 0);
		break;
	case PIECE_XZ:
		break;
	case PIECE_Y:
		break;
	case UNIT_XZ:
		break;
	case UNIT_Y:
		break;
	case UNIT_HEIGHT:
		break;
	case XZ_ATAN:
		break;
	case XZ_HYPOT:
		break;
	case ATAN:
		break;
	case HYPOT:
		break;
	case GROUND_HEIGHT:
		break;
	case BUILD_PERCENT_LEFT:
		break;
	case YARD_OPEN:
		if (param == 0) {
			if (uh->CanCloseYard(unit))
				yardOpen = false;
		}
		else {
			yardOpen = true;
		}
		break;
	case BUGGER_OFF:
		if (param != 0) {
			helper->BuggerOff(unit->pos+unit->frontdir*unit->radius,unit->radius*1.5);
		}
		//yardOpen = (param != 0);
		break;
	case ARMORED:
		if(param){
			unit->curArmorMultiple=unit->armoredMultiple;
		} else {
			unit->curArmorMultiple=1;
		}
		unit->armoredState = (param != 0);
		break;
	default:
		info->AddLine("CobError: Unknown set constant %d", val);
	case VETERAN_LEVEL: 
		unit->experience=param*0.01f;
		break;
	}
#endif
}

bool CCobInstance::HasScriptFunction(int id) 
{
	return (script.scriptIndex[id] >= 0);
}

void CCobInstance::MoveSmooth(int piece, int axis, int destination, int delta, int deltaTime)
{
	//Make sure we do not overwrite animations of non-interpolated origin
	AnimInfo *ai = FindAnim(AMove, piece, axis);
	if (ai) {
		if (!ai->interpolated) {
			//info->AddLine("Anim move overwrite");
			MoveNow(piece, axis, destination);
			return;
		}
	}

	int cur = pieces[piece].coords[axis];
	int dist = abs(destination - cur);
	int timeFactor = (1000 * 1000) / (deltaTime * deltaTime);
	int speed = (dist * timeFactor) / delta;

	//info->AddLine("Move %d got %d %d", cur, destination, speed);

	Move(piece, axis, speed, destination, true);
}

void CCobInstance::TurnSmooth(int piece, int axis, int destination, int delta, int deltaTime)
{
	AnimInfo *ai = FindAnim(ATurn, piece, axis);
	if (ai) {
		if (!ai->interpolated) {
			//info->AddLine("Anim turn overwrite");
			TurnNow(piece, axis, destination);
			return;
		}
	}

	int cur = pieces[piece].rot[axis];
	short int dist = destination - cur;
	int timeFactor = (1000 * 1000) / (deltaTime * deltaTime);
	dist = abs(dist);
	int speed = (dist * timeFactor) / delta;

	//info->AddLine("Turnx %d:%d cur %d got %d %d dist %d", piece, axis, cur, destination, speed, dist);

	Turn(piece, axis, speed, destination, true);
}
