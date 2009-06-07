#ifndef CR_MAP_TYPE_IMPL_H
#define CR_MAP_TYPE_IMPL_H

#ifdef _MSC_VER
	#define SPRING_HASH_MAP stdext::hash_map
	#include <hash_map>
#elif __GNUG__
/* Test for GCC >= 4.3.2 */
	#if __GNUC__ > 4 || \
		(__GNUC__ == 4 && (__GNUC_MINOR__ > 3 || \
						(__GNUC_MINOR__ == 3 && \
							__GNUC_PATCHLEVEL__ >= 2)))
		#include <tr1/unordered_map>
		#define SPRING_HASH_MAP std::tr1::unordered_map
	#else
		#define SPRING_HASH_MAP __gnu_cxx::hash_map
		#include <ext/hash_map>
	#endif
#else
	#error Unsupported compiler
#endif

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include "creg_cond.h"

namespace creg
{
	// implement map insert() function with an interface common to all map types
	template<typename T>
	struct MapInserter
	{
		typedef typename T::iterator iterator;
		typedef typename T::value_type value_type;
		iterator insert(T& m, value_type& p) {
			// std::map<>::insert returns std::pair<iterator, bool>.
			return m.insert(p).first;
		}
	};
	template<typename TKey, typename TValue>
	struct MapInserter < std::multimap <TKey, TValue> >
	{
		typedef typename std::multimap <TKey, TValue>::iterator iterator;
		typedef typename std::multimap <TKey, TValue>::value_type value_type;
		iterator insert(std::multimap<TKey, TValue>& m, value_type& p) {
			// std::multimap<>::insert returns iterator.
			return m.insert(p);
		}
	};

	// map/multimap, this template assumes that the map key type is copyable
	template<typename T>
	struct MapType : public IType
	{
		boost::shared_ptr<IType> keyType, mappedType;
		typedef typename T::iterator iterator;

		MapType (boost::shared_ptr<IType> keyType, boost::shared_ptr<IType> mappedType) :
			keyType (keyType), mappedType(mappedType) {}
		~MapType () {
		}

		void Serialize (ISerializer *s, void *instance)
		{
			T& ct = *(T*)instance;
			if (s->IsWriting ()) {
				int size=ct.size();
				s->SerializeInt (&size, sizeof(int));
				for (iterator i=ct.begin();i!=ct.end();++i)  {
					keyType->Serialize (s, (void*) &i->first);
					mappedType->Serialize (s, &i->second);
				}
			} else {
				int size;
				s->SerializeInt (&size, sizeof(int));
				for (int a=0;a<size;a++) {
					typename T::value_type pt;
					// only allow copying of the key type
					keyType->Serialize (s, (void*) &pt.first);
					iterator i = MapInserter<T>().insert(ct, pt);
					mappedType->Serialize (s, &i->second);
				}
			}
		}
		std::string GetName() { return "map<" + keyType->GetName() + ", " + mappedType->GetName(); }
	};

	// Map type
	template<typename TKey, typename TValue>
	struct DeduceType < std::map<TKey, TValue> > {
		boost::shared_ptr<IType> Get () {
			DeduceType<TValue> valuetype;
			DeduceType<TKey> keytype;
			return boost::shared_ptr<IType>(new MapType < std::map <TKey, TValue> > (keytype.Get(), valuetype.Get()));
		}
	};
	// Multimap
	template<typename TKey, typename TValue>
	struct DeduceType < std::multimap<TKey, TValue> > {
		boost::shared_ptr<IType> Get () {
			DeduceType<TValue> valuetype;
			DeduceType<TKey> keytype;
			return boost::shared_ptr<IType>(new MapType < std::multimap<TKey, TValue> > (keytype.Get(), valuetype.Get()));
		}
	};
	// Hash map
	template<typename TKey, typename TValue>
	struct DeduceType < SPRING_HASH_MAP<TKey, TValue> > {
		boost::shared_ptr<IType> Get () {
			DeduceType<TValue> valuetype;
			DeduceType<TKey> keytype;
			return boost::shared_ptr<IType>(new MapType < SPRING_HASH_MAP<TKey, TValue> > (keytype.Get(), valuetype.Get()));
		}
	};

	template<typename T>
	struct PairType : public IType
	{
		PairType (boost::shared_ptr<IType> first, boost::shared_ptr<IType> second):
				firstType(first), secondType(second) {}
		~PairType () {
		}
		boost::shared_ptr<IType> firstType, secondType;

		void Serialize (ISerializer *s, void *instance)
		{
			T& p = *(T*)instance;
			firstType->Serialize (s, &p.first);
			secondType->Serialize (s, &p.second);
		}
		std::string GetName() { return "pair<" + firstType->GetName() + "," + secondType->GetName() + ">"; }
	};

	// std::pair
	template<typename TFirst, typename TSecond>
	struct DeduceType < std::pair<TFirst, TSecond> >
	{
		boost::shared_ptr<IType> Get () {
			DeduceType<TFirst> first;
			DeduceType<TSecond> second;
			return boost::shared_ptr<IType>(new PairType <std::pair<TFirst, TSecond> > (first.Get(), second.Get()));
		}
	};
};

#endif
