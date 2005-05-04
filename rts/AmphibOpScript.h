// AmphiOpScript.h
///////////////////////////////////////////////////////////////////////////

#ifndef __AMPHI_OP_SCRIPT_H__
#define __AMPHI_OP_SCRIPT_H__

#include "archdef.h"

#include "script.h"

class CAmphibOpScript :
	public CScript
{
public:
	CAmphibOpScript(void);
	~CAmphibOpScript(void);
	void Update(void);
	std::string GetMapName(void);
};

#endif __AMPHI_OP_SCRIPT_H__
