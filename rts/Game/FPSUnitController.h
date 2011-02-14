/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FPS_UNIT_CONTROLLER_H
#define FPS_UNIT_CONTROLLER_H

#include "System/float3.h"

class CUnit;
class CPlayer;

struct FPSUnitController {
	FPSUnitController();

	void SetControlleeUnit(CUnit* u) { controllee = u; }
	void SetControllerPlayer(CPlayer* c) { controller = c; }

	void Update();
	void RecvStateUpdate(const unsigned char*);
	void SendStateUpdate(const bool*);

	const CUnit* GetControllee() const { return controllee; }
	      CUnit* GetControllee()       { return controllee; }
	const CPlayer* GetController() const { return controller; }
	      CPlayer* GetController()       { return controller; }

	CUnit* targetUnit;           //! synced
	CUnit* controllee;           //! synced
	CPlayer* controller;         //! synced, always points to player that owns us (unused)

	float3 viewDir;              //! synced
	float3 targetPos;            //! synced
	float targetDist;            //! synced

	bool forward;                //! synced
	bool back;                   //! synced
	bool left;                   //! synced
	bool right;                  //! synced
	bool mouse1;                 //! synced
	bool mouse2;                 //! synced

	short oldHeading, oldPitch;  //! unsynced
	unsigned char oldState;      //! unsynced
	float3 oldDCpos;             //! unsynced
};

#endif
