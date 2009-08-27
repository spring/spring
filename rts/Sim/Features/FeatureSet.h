/** @file FeatureSet.h
 *  @brief Defines STL like container wrapper for storing CFeature pointers.
 *  @author Tobi Vollebregt
 *
 *  This file has a strong resemblence to Sim/Units/UnitSet.h, if you find a
 *  bug in this one don't forget to update the other too. Or refactor them both
 *  using one set of template code.
 */

#ifndef FEATURESET_H
#define FEATURESET_H

#include <iterator>

#include "Feature.h"

class CFeatureSetIterator : public std::iterator<std::input_iterator_tag, CFeature*>
{
	private:

		typedef std::map<int, CFeature*> container;

		container::iterator iterator;
		friend class CFeatureSet;
		friend class CFeatureSetConstIterator;

	public:

		CFeatureSetIterator() {}
		explicit CFeatureSetIterator(container::iterator i): iterator(i) {}

		const CFeatureSetIterator& operator++() { ++iterator; return *this; }

		CFeature* operator*()   const { return iterator->second; }
		CFeature** operator->() const { return &iterator->second; }

		bool operator==(const CFeatureSetIterator& other) const { return iterator == other.iterator; }
		bool operator!=(const CFeatureSetIterator& other) const { return !(*this == other); }
};

class CFeatureSetConstIterator : public std::iterator<std::input_iterator_tag, const CFeature*>
{
	private:

		typedef std::map<int, CFeature*> container;

		container::const_iterator iterator;
		friend class CFeatureSet;

	public:

		CFeatureSetConstIterator() {}
		CFeatureSetConstIterator(CFeatureSetIterator other): iterator(other.iterator) {}
		explicit CFeatureSetConstIterator(container::const_iterator i): iterator(i) {}

		const CFeatureSetConstIterator& operator++() { ++iterator; return *this; }

		CFeature* operator*()         const { return iterator->second; }
		CFeature* const* operator->() const { return &iterator->second; }

		bool operator==(const CFeatureSetConstIterator& other) const { return iterator == other.iterator; }
		bool operator!=(const CFeatureSetConstIterator& other) const { return !(*this == other); }
};


/** @brief Like a std::set<CFeature*>.
 *  But this class guarantees the order of the features by actually putting them
 *  in a std::map<int, CFeature*> where the int is the feature ID.
 */
class CFeatureSet
{
	CR_DECLARE_STRUCT(CFeatureSet);

	private:

		typedef std::map<int, CFeature*> container;

		container features;

	public:

		typedef container::size_type size_type;
		typedef CFeatureSetIterator iterator;
		typedef CFeatureSetConstIterator const_iterator;

		size_type size() const { return features.size(); }
		size_type max_size() const { return features.max_size(); }
		bool empty() const { return features.empty(); }

		iterator begin() { return iterator(features.begin()); }
		iterator end()   { return iterator(features.end()); }

		const_iterator begin() const { return const_iterator(features.begin()); }
		const_iterator end()   const { return const_iterator(features.end()); }

		std::pair<iterator, bool> insert(CFeature* feature) {
			std::pair<container::iterator, bool> ret =
					features.insert(container::value_type(feature->id, feature));
			return std::pair<iterator, bool>(iterator(ret.first), ret.second);
		}

		void erase(iterator i) { features.erase((*i)->id); }
		void erase(const CFeature* feature) { features.erase(feature->id); }

		void clear() { features.clear(); }

		iterator find(const CFeature* feature) { return iterator(features.find(feature->id)); }
		const_iterator find(const CFeature* feature) const { return const_iterator(features.find(feature->id)); }

		iterator find(int id) { return iterator(features.find(id)); }
		const_iterator find(int id) const { return const_iterator(features.find(id)); }

		bool operator==(const CFeatureSet& other) const { return features == other.features; }
		bool operator!=(const CFeatureSet& other) const { return !(*this == other); }
};

#endif // !defined(FEATURESET_H)
