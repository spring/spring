#ifndef PLAYER_ROSTER
#define PLAYER_ROSTER
// PlayerRoster.h: interface for PlayerRoster class
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>

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

		const int* GetIndices(int* count);

		bool SetSortTypeByName(const std::string& type);
		bool SetSortTypeByCode(SortType type);

		SortType GetSortType();
		const char* GetSortName();

	private:
		void SetCompareFunc();

	private:
		SortType compareType;
		int (*compareFunc)(const void* a, const void* b);
};


inline PlayerRoster::SortType PlayerRoster::GetSortType()
{
	return compareType;
}


extern PlayerRoster playerRoster;


#endif /* PLAYER_ROSTER */
