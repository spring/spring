#ifndef KROGOTHSCRIPT_H
#define KROGOTHSCRIPT_H
/*pragma once removed*/
#include "Script.h"

class CKrogothScript :
	public CScript
{
public:
	CKrogothScript(void);
	~CKrogothScript(void);
	void Update(void);
	std::string GetMapName(void);
};


#endif /* KROGOTHSCRIPT_H */
