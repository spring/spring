#ifndef NTAI_EDATA_H
#define NTAI_EDATA_H

#include "CEvent.h"

class EData{
public:
	virtual void Init(CEvent e);
	virtual CEvent GetEvent();
	virtual ~EData();
};

#endif
