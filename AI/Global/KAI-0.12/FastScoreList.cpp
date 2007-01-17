/*
 * Made for Krogothe by Tournesol
 *
 * Use with near guarantee of failure ;)
 * All pointer stuff was made by random (no kidding) until it compiled...
 *
 * Dont share without permission
 * May freely be used by AI's for TA spring
 * A GPL licence can be obtained if needed.
 */


#include "GlobalStuff.h"
#include "MyNode.h"
#include "FastScoreList.h"

CFastScoreList::CFastScoreList() {
	dummy = new MyNode(-1, -1);

	theFirstOne = dummy;
	theLastOne = dummy;

	for (int i = 0; i < numberOfPossibleScores; i++) {
		list[i].score = i;
		list[i].pointerToHigherValue = dummy;
		list[i].pointerToLowerValue = dummy;
	}

	for (int i = 0; i < useTableSize; i++) {
		useTable[i] = 0;
	}
}

CFastScoreList::~CFastScoreList() {
}

int CFastScoreList::getBestUnitAndRemoveIt() {
	if (theFirstOne == dummy)
		return -1;
	else {
		int unitID = theFirstOne -> getBestUnitAndRemoveIt();
		int score = theFirstOne -> score;

		// Update the list
		countOfUnitsAtListPos[score]--;

		if(countOfUnitsAtListPos[score] == 0) {
			// That list is now empty, mark it.
			int useTableDataElement = useTable[score >> 5];
			int mask = 1 << (score & BIT_INDEX_MASK );
			useTable[score >> 5] = useTableDataElement & ~(mask);

			// Fix the linked list:
			MyNode* temp = theFirstOne;
			theFirstOne = theFirstOne -> pointerToLowerValue;
			theFirstOne -> pointerToHigherValue = dummy;
			temp -> pointerToLowerValue = dummy;

			if (theFirstOne == dummy)
				theLastOne = dummy;
		}

		return unitID;
	}
}



/*
 * Isolate rightmost 1-bit
 */
int CFastScoreList::getMaskWithFirstTrue(int x) {
	return (x & (-x));
}

bool CFastScoreList::getBoolFromIndex(int intBool, int index) {
	int maske = 1;
	maske <<= index;

	return ((intBool & maske) != 0);
}

int CFastScoreList::setBoolFromIndex(int intBool, int index, bool b) {
	int maske = 1;
	maske <<= index;

	if (b) {
		intBool = intBool | maske;
	}
	else {
		intBool = intBool & (~maske);
	}

	return intBool;
}

int CFastScoreList::getIndexOfSetBit(int intBool) {
	int index = -1;
	int maske = 1;

	for (int i = 0; i < 32 && index == -1; i++) {
		if ((intBool & maske) == 0) {
			maske <<= 1;
		}
		else {
			index = i;
		}
	}

	return index;
}


void CFastScoreList::addUnit(int unitID, int score) {
	MyNode* newUnit = new MyNode(unitID, score);

	// test if the insert place is used
	int useTableDataElement = useTable[score >> 5];
	int mask = 1 << (score & BIT_INDEX_MASK);

	if (getBoolFromIndex( useTableDataElement, score & BIT_INDEX_MASK)) {
		// it's in use, add the new node to the start
		MyNode oldFirstNodeWithThisScore = list[score];
		oldFirstNodeWithThisScore.addUnit(newUnit);

		// update the list:
		countOfUnitsAtListPos[score]++;
	}
	else {
		// find the first list[pos] after list[score] that has a node
		int temp = (useTableDataElement >> (score & BIT_INDEX_MASK ) << (score & BIT_INDEX_MASK));
		int temp2 = getMaskWithFirstTrue(temp);
		int index = score >> 5;

		if (temp2 == 0) {
			// continue searching
			index++;
			// this will run max ((numberOfPossibleScores / 32) - 1) times
			while(index < useTableSize && temp2 == 0) {
				temp2 = getMaskWithFirstTrue(useTable[index]);
				index++;
			}

			if(index == useTableSize && temp2 == 0)
				index = -1;
		}

		if (index == -1) {
			// didn't find any nodes, so this one will be the first one now

			MyNode* nodePosWithThisScore = &(list[score]);
			nodePosWithThisScore -> addUnit(newUnit);
			nodePosWithThisScore -> pointerToLowerValue = theFirstOne;
			nodePosWithThisScore -> pointerToHigherValue = dummy;

			if (theFirstOne != dummy)
				theFirstOne -> pointerToHigherValue = nodePosWithThisScore;

			theFirstOne = nodePosWithThisScore;

			// it may be the last node in the list too
			if (theLastOne == dummy) {
				theLastOne = theFirstOne;
			}
		}
		else {
			// get the node
			int postInsertIndex = index * 32 + getIndexOfSetBit(temp2);
			MyNode* nodeHigherThanNewNode = &(list[postInsertIndex]);
			MyNode* nodeLowerThanNewNode = nodeHigherThanNewNode->pointerToLowerValue;
			MyNode* nodeNewNodeInsertPlace = &(list[score]);

			nodeNewNodeInsertPlace->addUnit(newUnit);
			// insert the new node
			nodeNewNodeInsertPlace -> pointerToHigherValue = nodeHigherThanNewNode;
			nodeNewNodeInsertPlace -> pointerToLowerValue = nodeLowerThanNewNode;
			nodeLowerThanNewNode -> pointerToHigherValue = nodeNewNodeInsertPlace;
			nodeHigherThanNewNode -> pointerToLowerValue = nodeNewNodeInsertPlace;
		}

		// It was not in use, so set it now.
		useTable[score >> 5] = useTableDataElement | mask;
		countOfUnitsAtListPos[score]++;
	}
}


void CFastScoreList::removeUnit(int unitID, int score) {
	if (countOfUnitsAtListPos[score] <= 0) {
		return;
	}

	MyNode* nodeToRemoveFrom = &(list[score]);
	// remove the node
	if (!nodeToRemoveFrom->removeUnit(unitID)) {
		return;
	}
	
	// fix the list
	if (countOfUnitsAtListPos[score] == 1) {
		countOfUnitsAtListPos[score]--;

		// that list is now empty, mark it
		int useTableDataElement = useTable[score >> 5];
		int mask = 1 << (score & BIT_INDEX_MASK );
		useTable[score >> 5] = useTableDataElement & ~(mask);

		// it's the last unit with this score, update the pointers
		if (theFirstOne == nodeToRemoveFrom) {
			// it was the first node
			MyNode* temp = theFirstOne;
			theFirstOne = theFirstOne -> pointerToLowerValue;
			theFirstOne -> pointerToHigherValue = dummy;
			temp -> pointerToLowerValue = dummy;

			if (theFirstOne == dummy)
				theLastOne = dummy;

			return;
		}
		
		if (theLastOne == nodeToRemoveFrom) {
			// it was the last node
			theLastOne = theLastOne -> pointerToHigherValue;
			theLastOne -> pointerToLowerValue = dummy;
			nodeToRemoveFrom -> pointerToHigherValue = dummy;
		}
		else {
			// the node is somewhere in the middle
			MyNode* nodeHigher = nodeToRemoveFrom -> pointerToHigherValue;
			MyNode* nodeLower = nodeToRemoveFrom -> pointerToLowerValue;

			nodeToRemoveFrom -> pointerToHigherValue = dummy;
			nodeToRemoveFrom -> pointerToLowerValue = dummy;

			nodeHigher -> pointerToLowerValue = nodeLower;
			nodeLower -> pointerToHigherValue = nodeHigher;
		}
	}
}
