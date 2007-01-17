/*
Made for Krogothe by Tournesol

Use with near guarantee of failure ;)
All pointer stuff was made by random (no kidding) until it compiled...

Dont share without permission
May freely be used by AI's for TA spring
A GPL licence can be obtained if needed.

*/

#ifndef _MyNode_h_included_
#define _MyNode_h_included_

using namespace std;

class MyNode
{
public:
	MyNode();
	~MyNode();
	MyNode(int unitID, int score);

	MyNode *pointerToLowerValue;
	MyNode *pointerToHigherValue;
	MyNode *linkedListPointer;
	int score;
	int unitID;

	
	void addUnit(MyNode *unit);

	int getBestUnitAndRemoveIt();

	bool removeUnit(int unitID);
};

#endif
