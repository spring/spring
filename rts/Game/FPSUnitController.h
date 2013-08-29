/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FPS_UNIT_CONTROLLER_H
#define FPS_UNIT_CONTROLLER_H

#include "System/float3.h"

class CUnit;
class CPlayer;


struct FPSUnitController {
	FPSUnitController();

	void SetControlleeUnit(CUnit* controlleeUnit) { controllee = controlleeUnit; }
	void SetControllerPlayer(CPlayer* controllerPlayer) { controller = controllerPlayer; }

	void Update();
	/**
	 * Extract and store direction info from byte array,
	 * used for network transfer.
	 * TODO move this to network message handling code when it is modularized
	 */
	void RecvStateUpdate(const unsigned char* buf);
	/**
	 * Compress direction info into byte array, and send it to the host.
	 * TODO move this to network message handling code when it is modularized
	 */
	void SendStateUpdate();

	const CUnit* GetControllee() const { return controllee; }
	      CUnit* GetControllee()       { return controllee; }
	const CPlayer* GetController() const { return controller; }
	      CPlayer* GetController()       { return controller; }

	CUnit* targetUnit;           ///< synced
	CUnit* controllee;           ///< synced
	/// synced, always points to the player that owns us (unused)
	CPlayer* controller;

	float3 viewDir;              ///< synced
	float3 targetPos;            ///< synced
	float targetDist;            ///< synced

	bool forward;                ///< synced
	bool back;                   ///< synced
	bool left;                   ///< synced
	bool right;                  ///< synced
	bool mouse1;                 ///< synced
	bool mouse2;                 ///< synced

	short oldHeading;            ///< unsynced
	short oldPitch;              ///< unsynced
	unsigned char oldState;      ///< unsynced
	float3 oldDCpos;             ///< unsynced
};

#endif // FPS_UNIT_CONTROLLER_H
