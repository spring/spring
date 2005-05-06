#ifndef MOVE_TEST_SCRIPT_H
#define MOVE_TEST_SCRIPT_H

#include "Script.h"

class CMoveTestScript :
	public CScript
{
public:
	CMoveTestScript(void);
	virtual ~CMoveTestScript(void);
	void Update(void);
};

#endif
