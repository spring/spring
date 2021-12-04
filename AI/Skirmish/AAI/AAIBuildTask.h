// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#ifndef AAI_BUILDTASK_H
#define AAI_BUILDTASK_H

#include "System/float3.h"

class AAI;

class AAIBuildTask
{
public:
	AAIBuildTask(AAI *ai, int unit_id, int def_id, float3 *pos, int tick);
	~AAIBuildTask(void);

	void BuilderDestroyed();

	void BuildtaskFailed();


	int def_id;
	int unit_id;

	float3 build_pos;

	int builder_id;

	int order_tick;
private:
	AAI* ai;
};

#endif

