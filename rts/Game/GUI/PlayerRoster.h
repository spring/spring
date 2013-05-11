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

		PlayerRoster();

		const std::vector<int>& GetIndices(int* count, bool includePathingFlag = false) const;

		bool SetSortTypeByName(const std::string& type);
		bool SetSortTypeByCode(SortType type);

		SortType GetSortType() const;
		const char* GetSortName();

	private:
		void SetCompareFunc();

	private:
		SortType compareType;
		int (*compareFunc)(const void* a, const void* b);
};


inline PlayerRoster::SortType PlayerRoster::GetSortType() const
{
	return compareType;
}

extern PlayerRoster playerRoster;

#endif /* PLAYER_ROSTER */
