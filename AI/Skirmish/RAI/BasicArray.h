// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_BASIC_ARRAY_H
#define RAI_BASIC_ARRAY_H

template <typename basicArrayType>

struct basicArray
{
	basicArray()
	{
		elementList = 0;
		elementSize = 0;
		elementIndex = 0;
	};
	void setSize(const int &size) { elementList = new basicArrayType[size]; };
	basicArray(const int &size)
	{
		elementList = new basicArrayType[size];
		elementSize = 0;
		elementIndex = 0;
	};
	~basicArray()
	{
		delete [] elementList;
	};
	basicArrayType* operator[] (int index) { return &elementList[index]; };
	void begin() { elementIndex = -1; };
	bool nextE(basicArrayType *&nextElement)
	{	elementIndex++;
		if( elementIndex >= elementSize )
			return false;
		nextElement = &elementList[elementIndex];
		return true;
	};
	bool nextE(basicArrayType &nextElement)
	{	elementIndex++;
		if( elementIndex >= elementSize )
			return false;
		nextElement = elementList[elementIndex];
		return true;
	};
	basicArrayType* push_back() { return &elementList[elementSize++]; };
	void push_back(const basicArrayType &element) { elementList[elementSize++] = element; };
	void removeE() { elementList[elementIndex--] = elementList[--elementSize]; };
	void removeE(const int &index) { elementList[index] = elementList[--elementSize]; }
	// use deleteE() instead of removeE() if using push_back(new *)
	void deleteE() { delete elementList[elementIndex]; removeE(); }
	void swap(const int &index, const int &index2)
	{	basicArrayType temp = elementList[index];
		elementList[index] = elementList[index2];
		elementList[index2] = temp;
	}

	basicArrayType *elementList;
	int elementIndex;	// essentially a built-in iterator
	int size() const { return elementSize; };
	int elementSize;	// the amount of elements in use
};

#endif
