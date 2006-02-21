#ifndef CR_MAP_TYPE_IMPL_H
#define CR_MAP_TYPE_IMPL_H

#include <map>
// hash_map, defined in stdafx.h
#include SPRING_HASH_MAP_H

namespace creg 
{
	// map/multimap, this template assumes that the map key type is copyable
	template<typename T>
	struct MapType : public IType
	{
		IType *mappedType, *keyType;
		typedef typename T::iterator iterator;

		MapType (IType *keyType,IType *mappedType) :
			keyType (keyType), mappedType(mappedType) {}
		~MapType () { 
			delete keyType;
			delete mappedType;
		}

		void Serialize (ISerializer *s, void *instance)
		{
			T& ct = *(T*)instance;
			if (s->IsWriting ()) {
				int size=ct.size();
				s->Serialize (&size, sizeof(int));
				for (iterator i=ct.begin();i!=ct.end();++i)  {
					keyType->Serialize (s, &i->first);
					mappedType->Serialize (s, &i->second);
				}
			} else {
				int size;
				s->Serialize (&size, sizeof(int));
				for (int a=0;a<size;a++) {
					typename T::value_type pt;
					// only allow copying of the key type
					keyType->Serialize (s, &pt.first);
					iterator i = ct.insert (pt);
					mappedType->Serialize (s, &i->second);
				}
			}
		}
	};

	// Map type
	template<typename TKey, typename TValue>
	struct DeduceType < std::map<TKey, TValue> > {
		IType* Get () {
			DeduceType<TValue> valuetype;
			DeduceType<TKey> keytype;
			return new MapType < std::map <TKey, TValue> > (elemtype.Get());
		}
	};
	// Multimap
	template<typename TKey, typename TValue>
	struct DeduceType < std::multimap<TKey, TValue> > {
		IType* Get () {
			DeduceType elemtype;
			return new MapType < std::multimap<T> > (elemtype.Get());
		}
	};
	// Hash map
	template<typename TKey, typename TValue>
	struct DeduceType < SPRING_HASH_MAP<TKey, TValue> > {
		IType* Get () {
			DeduceType elemtype;
			return new MapType < SPRING_HASH_SET<TKey, TValue> > (elemtype.Get());
		}
	};

	template<typename T>
	struct PairType : public IType
	{
		PairType (IType *first, IType *second) { firstType=first; secondType=second; }
		~PairType () { 
			delete firstType;
			delete secondType;
		}
		IType *firstType, *secondType;

		void Serialize (ISerializer *s, void *instance)
		{
			T& p = *(T*)instance;
			firstType->Serialize (s, &p.first);
			secondType->Serialize (s, &p.second);
		}
	};

	// std::pair
	template<typename TFirst, typename TSecond>
	struct DeduceType < std::pair<TFirst, TSecond> >
	{
		IType* Get () {
			DeduceType<TFirst> first;
			DeduceType<TSecond> second;
			PairType *pt = new PairType <std::pair<TFirst, TSecond> > (first.Get(), second.Get());
			return pt;
		}
	};
};

#endif
