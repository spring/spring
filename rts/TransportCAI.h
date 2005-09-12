#ifndef TRANSPORTCAI_H
#define TRANSPORTCAI_H

#include "MobileCAI.h"

class CTransportCAI :
	public CMobileCAI
{
public:
	CTransportCAI(CUnit* owner);
	~CTransportCAI(void);
	void SlowUpdate(void);
	void ScriptReady(void);

	bool CanTransport(CUnit* unit);
	bool FindEmptySpot(float3 center, float radius,float emptyRadius, float3& found);
	CUnit* FindUnitToTransport(float3 center, float radius);
	int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	void DrawCommands(void);
	void FinishCommand(void);

	int toBeTransportedUnitId;
	bool scriptReady;
	int lastCall;
};


#endif /* TRANSPORTCAI_H */
