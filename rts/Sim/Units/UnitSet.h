/** @file UnitSet.h
 *  @brief Defines STL like container wrapper for storing CUnit pointers.
 *  @author Tobi Vollebregt
 *
 *  This file has a strong resemblence to Sim/Features/FeatureSet.h, if you find a
 *  bug in this one don't forget to update the other too. Or refactor them both
 *  using one set of template code.
 */

#ifndef UNITSET_H
#define UNITSET_H

#include "Unit.h"

class CUnitSetIterator
{
	private:

		typedef std::map<int, CUnit*> container;

		container::iterator iterator;
		friend class CUnitSet;
		friend class CUnitSetConstIterator;

	public:

		CUnitSetIterator() {}
		explicit CUnitSetIterator(container::iterator i): iterator(i) {}

		const CUnitSetIterator& operator++() { ++iterator; return *this; }

		CUnit* operator*()   const { return iterator->second; }
		CUnit** operator->() const { return &iterator->second; }

		bool operator==(const CUnitSetIterator& other) const { return iterator == other.iterator; }
		bool operator!=(const CUnitSetIterator& other) const { return !(*this == other); }
};

class CUnitSetConstIterator
{
	private:

		typedef std::map<int, CUnit*> container;

		container::const_iterator iterator;
		friend class CUnitSet;

	public:

		CUnitSetConstIterator() {}
		CUnitSetConstIterator(CUnitSetIterator other): iterator(other.iterator) {}
		explicit CUnitSetConstIterator(container::const_iterator i): iterator(i) {}

		const CUnitSetConstIterator& operator++() { ++iterator; return *this; }

		CUnit* operator*()         const { return iterator->second; }
		CUnit* const* operator->() const { return &iterator->second; }

		bool operator==(const CUnitSetConstIterator& other) const { return iterator == other.iterator; }
		bool operator!=(const CUnitSetConstIterator& other) const { return !(*this == other); }
};


/** @brief Like a std::set<CUnit*>.
 *  But this class guarantees the order of the units by actually putting them
 *  in a std::map<int, CUnit*> where the int is the unit ID.
 */
class CUnitSet
{
	CR_DECLARE_STRUCT(CUnitSet);

	private:

		typedef std::map<int, CUnit*> container;

		container units;

	public:

		typedef container::size_type size_type;
		typedef CUnitSetIterator iterator;
		typedef CUnitSetConstIterator const_iterator;

		size_type size() const { return units.size(); }
		size_type max_size() const { return units.max_size(); }
		bool empty() const { return units.empty(); }

		iterator begin() { return iterator(units.begin()); }
		iterator end()   { return iterator(units.end()); }

		const_iterator begin() const { return const_iterator(units.begin()); }
		const_iterator end()   const { return const_iterator(units.end()); }

		std::pair<iterator, bool> insert(CUnit* unit) {
			std::pair<container::iterator, bool> ret =
					units.insert(container::value_type(unit->id, unit));
			return std::pair<iterator, bool>(iterator(ret.first), ret.second);
		}

		void erase(iterator i) { units.erase((*i)->id); }
		void erase(const CUnit* unit) { units.erase(unit->id); }

		void clear() { units.clear(); }

		iterator find(const CUnit* unit) { return iterator(units.find(unit->id)); }
		const_iterator find(const CUnit* unit) const { return const_iterator(units.find(unit->id)); }

		iterator find(int id) { return iterator(units.find(id)); }
		const_iterator find(int id) const { return const_iterator(units.find(id)); }

		bool operator==(const CUnitSet& other) const { return units == other.units; }
		bool operator!=(const CUnitSet& other) const { return !(*this == other); }
};

#endif // !defined(UNITSET_H)
