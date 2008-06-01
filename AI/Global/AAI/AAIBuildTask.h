#pragma once

#include "aidef.h"
class AAI;

class AAIBuildTask
{
public:
	AAIBuildTask(AAI *ai, int unit_id, int def_id, float3 *pos, int tick);
	~AAIBuildTask(void);

	void BuilderDestroyed();

	void BuildtaskFailed();

	AAI* ai;

	int def_id;
	int unit_id;

	float3 build_pos;

	int builder_id;

	int order_tick;
};
