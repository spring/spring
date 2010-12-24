/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MOVE_TYPE_FACTORY_H_
#define _MOVE_TYPE_FACTORY_H_

class AMoveType;
class CUnit;
struct UnitDef;

class MoveTypeFactory {
public:
	static AMoveType* GetMoveType(CUnit*, const UnitDef*);
};

#endif
