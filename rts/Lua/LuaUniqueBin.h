/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UNIQUE_BIN_H
#define LUA_UNIQUE_BIN_H

//
//  A container that holds unique copies of a data type (with smart references).
//
//  Accessing the data for any reference is always valid.
//
//  Data Types that use this template must have the following properties:
//
//    1. the  'bool operator<(const T&) const'  member function
//
//    2. the  'static T def'  static variable that holds default values
//

#include <set>


/******************************************************************************/
/******************************************************************************/
//
//  UniqueBin
//

template<typename T>
class UniqueBin
{
	private:
		class Data;
		struct SortObj;

	public:

		class Ref;

		typedef std::set<Data*, SortObj> DataSet;
		typedef typename DataSet::const_iterator const_iterator;

		const_iterator begin() const {
			return dataSet.begin();
		}

		const_iterator end() const {
			return dataSet.end();
		}

		unsigned int size() const {
			return dataSet.size();
		}

		bool empty() const {
			return dataSet.empty();
		}

		Ref GetRef(const T& t) {
			Data* cmpPtr = (Data*) &t; // note the cast
			const_iterator it = dataSet.find(cmpPtr);
			if (it != dataSet.end()) {
				return Ref(*it); // already in the set
			}
			Data* data = new Data(t);
			dataSet.insert(data);
			return Ref(data);
		}

		bool Exists(const T& t) const {
			Data* cmpPtr = (Data*) &t; // note the cast
			const_iterator it = dataSet.find(cmpPtr);
			if (it != dataSet.end()) {
				return true;
			}
			return false;
		}

		static UniqueBin handler;

	private:
		struct SortObj
		{
			bool operator()(const Data* a, const Data* b) {
				return (*a < *b);
			}
		};

	private:
		typedef typename DataSet::iterator iterator;

	private:
		UniqueBin() {
			defData = new Data(T::def);
			defData->Ref(); // make it permanent
			dataSet.insert(defData);
		}

		~UniqueBin() {
			defData->UnRef();
			for (iterator it = dataSet.begin(); it != dataSet.end(); ++it) {
				delete *it;
			}
		}

		void EraseData(Data* data) {
			dataSet.erase(data);
			delete data;
		}

	private:
		Data*   defData;
		DataSet dataSet;

/******************************************************************************/
//
//  Ref
//

	public:
		class Ref {

			friend Ref UniqueBin::GetRef(const T& t);  // for Ref(Data*) access

			public:
				Ref() {
					dataPtr = UniqueBin::handler.defData;
				}

				const T& operator*()	const {
					return (**dataPtr);
				}

				const T* operator->() const {
					return &(**dataPtr);
				}

				bool operator==(const Ref& r) const {
					return dataPtr == r.dataPtr;
				}

				bool operator!=(const Ref& r) const {
					return dataPtr != r.dataPtr;
				}

				void Reset() {
					dataPtr->UnRef();
					dataPtr = UniqueBin::handler->defData;
					dataPtr->Ref();
				}

				~Ref() {
					dataPtr->UnRef();
				};

				Ref(const Ref& r) {
					dataPtr = r.dataPtr;
					dataPtr->Ref();
				}

				Ref& operator=(const Ref& r) {
					if (dataPtr != r.dataPtr) {
						dataPtr->UnRef();
						dataPtr = r.dataPtr;
						dataPtr->Ref();
					}
					return *this;
				}

			private:
				Ref(Data* dp) {
					dataPtr = dp;
					dataPtr->Ref();
				}

			private:
				Data* dataPtr;
		};

/******************************************************************************/
//
//	Data
//

	private:
		class Data {
			public:
				bool operator<(const Data& d) const {
					return (data < d.data);
				}

				const T& operator*() const {
					return data;
				}

				void Ref() {
					refCount++;
				}

				void UnRef() {
					refCount--;
					if (refCount <= 0) {
						UniqueBin::handler.EraseData(this);
					}
				}

				Data(const T& t)
				: data(t), refCount(0) {}

			private:
				T data; // NOTE: must come first
				unsigned int refCount;
		};
};

/******************************************************************************/
/******************************************************************************/


template<typename T> UniqueBin<T> UniqueBin<T>::handler;


/******************************************************************************/
/******************************************************************************/

#endif // LUA_UNIQUE_BIN_H
