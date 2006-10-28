/*
Made for Krogothe by Tournesol

Use with near guarantee of failure ;)
All pointer stuff was made by random (no kidding) until it compiled...

Dont share without permission
May freely be used by AI's for TA spring
A GPL licence can be obtained if needed.

*/

/*
	// Add this to a GroupAI to test it + debug it:
	CFastScoreList *k = new CFastScoreList();
	k->addUnit(3200, 32);
	k->addUnit(6300, 63);
	
	k->addUnit(4700, 47);
	
	//k->addUnit(4400, 44);
	//k->addUnit(4500, 45);
	//k->addUnit(100, 100);
	//aicb->SendTextMsg("added 100,100", 0);
	//k->addUnit(200, 200);
	//aicb->SendTextMsg("added 200,200", 0);
	

	
	
	char blank[] = "       ";
	for (int i = 0; i < k->numberOfPossibleScores; i++) {
		char textToPrint[60];
		sprintf(textToPrint, "%d , %d :", i , k->countOfUnitsAtListPos[i]);
		if(k->list[i].pointerToLowerValue == k->dummy )
			strcat(textToPrint, blank);// string += "       ";
		else
		{
			char str[20];
			sprintf(str, "num: %d", k->list[i].pointerToLowerValue->score);
			strcat(textToPrint, str);
		}
		strcat(textToPrint, " | ");
		if(k->list[i].pointerToHigherValue == k->dummy )
			strcat(textToPrint, blank);// string += "       ";
		else
		{
			char str[20];
			sprintf(str, "num: %d", k->list[i].pointerToHigherValue->score);
			strcat(textToPrint, str);
		}
		char str[20];
		sprintf(str, " | at %p", &(k->list[i]));
		strcat(textToPrint, str);
		aicb->SendTextMsg(textToPrint, 0);
	}

	char textToPrintFirstOne[50];
	sprintf(textToPrintFirstOne, "theFirstOne is ");
	if(k->theFirstOne == k->dummy )
		strcat(textToPrintFirstOne, "dummy");// string += "       ";
	else
	{
		char str[30];
		sprintf(str, "num: %d at %p", k->theFirstOne->score, (k->theFirstOne));
		strcat(textToPrintFirstOne, str);
	}
	aicb->SendTextMsg(textToPrintFirstOne, 0);

	char textToPrintLastOne[50];
	sprintf(textToPrintLastOne, "theLastOne is ");
	if(k->theLastOne == k->dummy )
		strcat(textToPrintLastOne, "dummy");// string += "       ";
	else
	{
		char str[30];
		sprintf(str, "num: %d at %p", k->theLastOne->score, (k->theLastOne));
		strcat(textToPrintLastOne, str);

	}
	aicb->SendTextMsg(textToPrintLastOne, 0);

	char textToPrintDummy[50];
	sprintf(textToPrintDummy, "theDummy is at %x , %p", (int) &(k->dummy), &k->dummy);

	aicb->SendTextMsg(textToPrintDummy, 0);

	char text2[50];
	int data2 = k->getBestUnitAndRemoveIt();
	sprintf(text2, "Got %d", data2);
	aicb->SendTextMsg(text2, 0);

	char text3[50];
	int data3 = k->getBestUnitAndRemoveIt();
	sprintf(text3, "Got %d", data3);
	aicb->SendTextMsg(text3, 0);


	char text4[50];
	int data4 = k->getBestUnitAndRemoveIt();
	sprintf(text4, "Got %d", data4);
	aicb->SendTextMsg(text4, 0);


*/


#include "globalstuff.h"
#include "MyNode.h"
#include "FastScoreList.h"

CFastScoreList::CFastScoreList()
{
	//dummy = new MyNode(-1, -1);
	//dummy.score = -1;
	//dummy.unitID = -1;


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

CFastScoreList::~CFastScoreList()
{
}

int CFastScoreList::getBestUnitAndRemoveIt()
{
	
	if(theFirstOne == dummy)
		return -1;
	else
	{
		int unitID = theFirstOne->getBestUnitAndRemoveIt();
		int score = theFirstOne->score;
		
		// Update the list
		
		countOfUnitsAtListPos[score]--;

		if(countOfUnitsAtListPos[score] == 0)
		{
			// That list is now empty, mark it.
			//System.out.println("That list is now empty, mark it.");
			int useTableDataElement = useTable[score >> 5];
			int mask = 1 << (score & BIT_INDEX_MASK );
			useTable[score >> 5] = useTableDataElement & ~(mask);
			
			// Fix the linked list:
			MyNode *temp = theFirstOne;
			theFirstOne = theFirstOne->pointerToLowerValue;
			theFirstOne->pointerToHigherValue = dummy;
			temp->pointerToLowerValue = dummy;
			if(theFirstOne == dummy)
				theLastOne = dummy;
			
		}
		
		return unitID;
	}
}



/**
 * Isoler rigthmost 1-bit
 * Gir 0 hvis ikke det finnes en
 */
int CFastScoreList::getMaskWithFirstTrue(int x)
{
	return x & (-x);
}

bool CFastScoreList::getBoolFromIndex(int intBool, int index)
{
	int maske = 1;
	maske <<= index;
	bool b = false;
	//System.out.println("Test bit: " + index);
	//System.out.println("intBool: " + Integer.toBinaryString(intBool));
	
	b = (intBool & maske) != 0;
	
	//System.out.println("bool is: " + b);
	return b;
}

int CFastScoreList::setBoolFromIndex(int intBool, int index, bool b)
{
	int maske = 1;
	maske <<= index;
	//System.out.println("Bit: " + index + " -> " + b);
	//System.out.println("intBool: " + Integer.toBinaryString(intBool));
	if(b)
	{
		intBool = intBool | maske;
	}
	else
	{
		intBool = intBool & (~maske);
	}
	//System.out.println("intBool: " + Integer.toBinaryString(intBool));
	return intBool;
}

int CFastScoreList::getIndexOfSetBit(int intBool)
{
	int index = -1;
	int maske = 1;
	//System.out.println("Bit: ");
	for (int i = 0; i < 32 && index == -1; i++)
	{
		//System.out.print("maske: " + Integer.toBinaryString(maske));
		if((intBool & maske) == 0)
		{
			maske <<= 1;
		}
		else
		{
			//maske <<= 1;
			index = i;
		}
	}
	return index;
}

/*
static void printInt(int i, int length)
{
	for (int j = length -1; j >= 0; j--)
		if(getBoolFromIndex(i, j))
			System.out.print("1");
		else
			System.out.print("0");
}
*/

void CFastScoreList::addUnit(int unitID, int score)
{
	MyNode *newUnit = new MyNode(unitID, score);
	
	// Test if the insert place is used:
	int useTableDataElement = useTable[score >> 5];
	int mask = 1 << (score & BIT_INDEX_MASK );
	if(getBoolFromIndex( useTableDataElement, score & BIT_INDEX_MASK))
	{
		// Its in use, add the new node to the start:
		//System.out.println("Its in use, add the new node to the start:" + score);
		MyNode oldFirstNodeWithThisScore = list[score];
		oldFirstNodeWithThisScore.addUnit(newUnit);
		
		// Update the list:
		countOfUnitsAtListPos[score]++;
		
		// Done
	}
	else
	{
		// Find the first list[pos] after list[score] that have have an node
		//printInt(useTableDataElement , 32);
		int temp = (useTableDataElement >> (score & BIT_INDEX_MASK ) << (score & BIT_INDEX_MASK ));
		//printInt(temp , 32);
		//System.out.println();
		int temp2 = getMaskWithFirstTrue(temp);
		//printInt(temp2 , 32);
		int index = score >> 5;
		
		if(temp2 == 0)
		{
			// Continue searching:
			index++;
			// This will run max numberOfPossibleScores/32 -1 times
			while(index < useTableSize && temp2 == 0)
			{
				temp2 = getMaskWithFirstTrue(useTable[index]);
				index++;
			}
			if(index == useTableSize && temp2 == 0)
				index = -1;
		}
		if(index == -1)
		{
			// Didnt find any nodes, so this one will be the first one now
			//System.out.println("Didnt find any nodes, so this one will be the first one now: " + unitID);
			
			MyNode *nodePosWithThisScore = &(list[score]);
			nodePosWithThisScore->addUnit(newUnit);
			nodePosWithThisScore->pointerToLowerValue = theFirstOne;
			nodePosWithThisScore->pointerToHigherValue = dummy;
			
			if(theFirstOne != dummy)
				theFirstOne->pointerToHigherValue = nodePosWithThisScore;
			theFirstOne = nodePosWithThisScore;
			
			// It may be the last node in the list too:
			if(theLastOne == dummy)
			{
				theLastOne = theFirstOne;
				//System.out.println("It may be the last node in the list too:");
			}
			//theFirstOne->getBestUnitAndRemoveIt();
		}
		else
		{
			// Get the node
			int postInsertIndex = index * 32 + getIndexOfSetBit(temp2);
			//System.out.println("index: " + index);
			//System.out.println("Get the node: " + postInsertIndex);
			//MyNode nodeLowerThanNewNode = list[postInsertIndex];
			//MyNode nodeHigherThanNewNode = nodeLowerThanNewNode.pointerToHigherValue;
			MyNode *nodeHigherThanNewNode = &(list[postInsertIndex]);
			MyNode *nodeLowerThanNewNode = nodeHigherThanNewNode->pointerToLowerValue;
			MyNode *nodeNewNodeInsertPlace = &(list[score]);
			
			nodeNewNodeInsertPlace->addUnit(newUnit);
			// Insert the new node 
			nodeNewNodeInsertPlace->pointerToHigherValue = nodeHigherThanNewNode;
			nodeNewNodeInsertPlace->pointerToLowerValue = nodeLowerThanNewNode;
			nodeLowerThanNewNode->pointerToHigherValue = nodeNewNodeInsertPlace;
			nodeHigherThanNewNode->pointerToLowerValue = nodeNewNodeInsertPlace;
			if(theLastOne == nodeLowerThanNewNode)
			{
				//theLastOne.pointerToLowerValue = nodeNewNodeInsertPlace;
				//theLastOne = nodeNewNodeInsertPlace;
			}
			
		}
		
		
		
	// It was not in use, so set it now.
	useTable[score >> 5] = useTableDataElement | mask;
	
	countOfUnitsAtListPos[score]++;
	}
}


void CFastScoreList::removeUnit(int unitID, int score)
{
	if(countOfUnitsAtListPos[score] <= 0)
	{
		//System.out.println("ERROR the unit is not in the list");
		return;
	}
	
	MyNode *nodeToRemoveFrom = &(list[score]);
	// Remove the node
	if(!nodeToRemoveFrom->removeUnit(unitID))
	{
		//System.out.println("ERROR the unit is not in the list");
		return;
	}
	
	// Fix the list:
	if(countOfUnitsAtListPos[score] == 1)
	{
		countOfUnitsAtListPos[score]--;
		
		// That list is now empty, mark it.
		//System.out.println("That list is now empty, mark it.");
		int useTableDataElement = useTable[score >> 5];
		int mask = 1 << (score & BIT_INDEX_MASK );
		useTable[score >> 5] = useTableDataElement & ~(mask);
		
		// Its the last unit with this score, update the pointers:
		if(theFirstOne == nodeToRemoveFrom)
		{
			// It was the first node 
			MyNode *temp = theFirstOne;
			theFirstOne = theFirstOne->pointerToLowerValue;
			theFirstOne->pointerToHigherValue = dummy;
			temp->pointerToLowerValue = dummy;
			if(theFirstOne == dummy)
				theLastOne = dummy;
			return;
		}
		
		if(theLastOne == nodeToRemoveFrom)
		{
			// It was the last node
			theLastOne = theLastOne->pointerToHigherValue;
			theLastOne->pointerToLowerValue = dummy;
			nodeToRemoveFrom->pointerToHigherValue = dummy;
		}
		else
		{
			// The node is somewhere in the middle
			MyNode *nodeHigher = nodeToRemoveFrom->pointerToHigherValue;
			MyNode *nodeLower = nodeToRemoveFrom->pointerToLowerValue;
			
			nodeToRemoveFrom->pointerToHigherValue = dummy;
			nodeToRemoveFrom->pointerToLowerValue = dummy;
			
			nodeHigher->pointerToLowerValue = nodeLower;
			nodeLower->pointerToHigherValue = nodeHigher;
		}
	}
}
