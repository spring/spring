/*
Made for Krogothe by Tournesol

Use with near guarantee of failure ;)
All pointer stuff was made by random (no kidding) until it compiled...

Dont share without permission
May freely be used by AI's for TA spring
A GPL licence can be obtained if needed.

*/


#include "globalstuff.h"
#include "MyNode.h"


MyNode::MyNode()
{
	pointerToLowerValue = NULL;
	pointerToHigherValue = NULL;
	linkedListPointer = NULL;
}

MyNode::~MyNode()
{
	//delete linkedListPointer;
}


MyNode::MyNode(int unitID_, int score_)
{
	pointerToLowerValue = NULL;
	pointerToHigherValue = NULL;
	linkedListPointer = NULL;
	unitID = unitID_;
	score = score_;

}


void MyNode::addUnit(MyNode *unit)
{
	if(linkedListPointer == NULL)
		linkedListPointer = unit;
	else
	{
		//System.out.println("adding unit: " + unit.unitID);
		MyNode *temp = linkedListPointer;
		linkedListPointer = unit;
		unit->linkedListPointer = temp;
	}
}

int MyNode::getBestUnitAndRemoveIt()
{
	if(linkedListPointer == NULL)
		return -1;
	else
	{
		int unitID_temp = linkedListPointer->unitID;
		MyNode * oldNode = linkedListPointer;
		linkedListPointer = linkedListPointer->linkedListPointer;
		delete oldNode;
		return unitID_temp;
	}
}

bool MyNode::removeUnit(int unitID)
{
		if(linkedListPointer->unitID == unitID)
		{
			getBestUnitAndRemoveIt();
			return true;
		}
		
		MyNode *current = linkedListPointer;
		MyNode *last = current;
		while(current->unitID != unitID) 
		{
			last = current;
			current = current->linkedListPointer;
			// Test for null here just in case
			if(current == NULL)
				return false;
		}
		last->linkedListPointer = current->linkedListPointer;
		delete current;
		return true;
}

