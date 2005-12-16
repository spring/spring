#ifndef __COMMANDER_SCRIPT_H__
#define __COMMANDER_SCRIPT_H__

#include "Script.h"

class CCommanderScript :
	public CScript
{
public:
	CCommanderScript(void);
	virtual ~CCommanderScript(void);
	void Update(void);
};

#endif // __COMMANDER_SCRIPT_H__
