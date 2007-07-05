/** @file FeatureSet.cpp
 *  @brief Defines STL like container wrapper for storing CFeature pointers.
 *  @author Tobi Vollebregt
 */

#include "StdAfx.h"
#include "creg/STL_Map.h"
#include "FeatureSet.h"

CR_BIND(CFeatureSet, );

CR_REG_METADATA(CFeatureSet, CR_MEMBER(features));
