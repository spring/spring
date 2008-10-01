#ifndef AREAMANAGER_H
#define AREAMANAGER_H

#include "GlobalAI.h"

class CAreaManager
{
public:
	CAreaManager(AIClasses* ai);
	virtual ~CAreaManager();



private:

	AIClasses* ai;


};

#endif /* AREAMANAGER_H */
