/*
 * Made for Krogothe by Tournesol
 *
 * Use with near guarantee of failure ;)
 * All pointer stuff was made by random (no kidding) until it compiled...
 *
 * Don't share without permission
 * May freely be used by AI's for TA spring
 * A GPL licence can be obtained if needed.
 */


#ifndef _KrogotheList_h_included_
#define _KrogotheList_h_included_


class MyNode;

using namespace std;

class CFastScoreList {
	public:
		CFastScoreList();
		~CFastScoreList();

		static const int BIT_INDEX_MASK = 0x0000001F;
		static const int numberOfPossibleScores = 256;

		MyNode list[numberOfPossibleScores];
		int countOfUnitsAtListPos[numberOfPossibleScores];

		MyNode* dummy;
		MyNode* theFirstOne;
		MyNode* theLastOne;

		static const int useTableSize = numberOfPossibleScores / 32;
		int useTable[useTableSize];

		int getIndexOfSetBit(int intBool);
		int getMaskWithFirstTrue(int x);
		bool getBoolFromIndex(int intBool, int index);
		int setBoolFromIndex(int intBool, int index, bool b);

		int getBestUnitAndRemoveIt();
		void addUnit(int unitID, int score);
		void removeUnit(int unitID, int score);
};


#endif
