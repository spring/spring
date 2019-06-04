/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_DEF_TYPES_H
#define CR_DEF_TYPES_H

#include "creg_cond.h"

#ifdef USING_CREG

#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Rendering/Textures/ColorMap.h"

namespace creg
{

#define DECTYPE(T, cb, c) \
	class T ## Type : public IType                                         \
	{                                                                      \
	public:                                                                \
	T ## Type() : IType(sizeof(T*)) { }                                    \
	void Serialize(ISerializer* s, void* instance)                         \
		{                                                                  \
			c T** defPtr = (c T**) instance;                       \
			if (s->IsWriting()) {                                          \
				int id = (*defPtr) != nullptr ? (*defPtr)->id : -1;        \
				s->SerializeInt(&id, sizeof(id));                          \
			} else {                                                       \
				int id;                                                    \
				s->SerializeInt(&id, sizeof(id));                          \
				*defPtr = cb(id);                                          \
			}                                                              \
		}                                                                  \
		std::string GetName() const { return #T "*"; }                     \
	};                                                                     \
	template<>                                                             \
	struct DeduceType<c T*> {                                          \
		static std::unique_ptr<IType> Get() {                            \
			return std::unique_ptr<IType>(new T ## Type());              \
		}                                                                  \
	};

	DECTYPE(UnitDef, unitDefHandler->GetUnitDefByID, const)
	DECTYPE(FeatureDef, featureDefHandler->GetFeatureDefByID, const)
	DECTYPE(WeaponDef, weaponDefHandler->GetWeaponDefByID, const)

}

#endif // USING_CREG

#endif // CR_SET_TYPE_IMPL_H

