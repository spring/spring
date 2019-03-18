#ifndef CR_STL_PAIR_H
#define CR_STL_PAIR_H

#include "creg_cond.h"
#include <utility>

#ifdef USING_CREG

namespace creg
{
	template<typename T>
	class PairType : public IType
	{
	public:
		PairType() : IType(sizeof(T)) { }
		~PairType() { }

		void Serialize(ISerializer* s, void* instance)
		{
			T& p = *(T*)instance;
			DeduceType<typename T::first_type>::Get()->Serialize(s,(void*) &p.first);
			DeduceType<typename T::second_type>::Get()->Serialize(s,(void*) &p.second);
		}
		std::string GetName() const { return "pair<" + DeduceType<typename T::first_type>::Get()->GetName() + "," + DeduceType<typename T::second_type>::Get()->GetName() + ">"; }
	};


	// Set type
	template<typename T1, typename T2>
	struct DeduceType<std::pair<T1, T2> > {
		static std::unique_ptr<IType> Get() {
			return std::unique_ptr<IType>(new PairType<std::pair<T1, T2> >());
		}
	};
}

#endif // USING_CREG

#endif // CR_STL_PAIR_H
