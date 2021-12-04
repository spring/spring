/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLAYER_ROSTER
#define PLAYER_ROSTER

#include <string>
#include <vector>

class PlayerRoster {
public:
	enum SortType {
		Disabled   = 0,
		Allies     = 1,
		TeamID     = 2,
		PlayerName = 3,
		PlayerCPU  = 4,
		PlayerPing = 5
	};
	typedef int (*SortFunc)(const int a, const int b);

	PlayerRoster();

	const std::vector<int>& GetIndices(bool includePathingFlag = false, bool callerBlockResort = false);

	bool SetSortTypeByName(const std::string& type);
	bool SetSortTypeByCode(SortType type);

	SortType GetSortType() const { return compareType; }
	const char* GetSortName() const;

private:
	void SetCompareFunc(SortType cmpType);

private:
	SortType compareType;
	SortFunc compareFunc;

	std::vector<int> playerIndices;
};


extern PlayerRoster playerRoster;

#endif /* PLAYER_ROSTER */
