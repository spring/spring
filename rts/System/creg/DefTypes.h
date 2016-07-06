/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CR_DEF_TYPES_H
#define CR_DEF_TYPES_H

#include "creg_cond.h"

#ifdef USING_CREG

#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"

namespace creg
{

#define DECTYPE(T, cb) \
	class T ## Type : public IType                                         \
	{                                                                      \
	public:                                                                \
	void Serialize(ISerializer* s, void* instance)                         \
		{                                                                  \
			const T** defPtr = (const T**) instance;                       \
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
		size_t GetSize() const { return sizeof(T*); }                      \
	};                                                                     \
	template<>                                                             \
	struct DeduceType<const T*> {                                          \
		static boost::shared_ptr<IType> Get() {                            \
			return boost::shared_ptr<IType>(new T ## Type());              \
		}                                                                  \
	};

	DECTYPE(UnitDef, unitDefHandler->GetUnitDefByID)
	DECTYPE(FeatureDef, featureDefHandler->GetFeatureDefByID)
	DECTYPE(WeaponDef, weaponDefHandler->GetWeaponDefByID)

}

#endif // USING_CREG

#endif // CR_SET_TYPE_IMPL_H

