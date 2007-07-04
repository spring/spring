/** @file UnitSet.cpp
 *  @brief Defines STL like container wrapper for storing CUnit pointers.
 *  @author Tobi Vollebregt
 */

#include "StdAfx.h"
#include "creg/STL_Map.h"
#include "UnitSet.h"

CR_BIND(CUnitSet, );

CR_REG_METADATA(CUnitSet, CR_MEMBER(units));
