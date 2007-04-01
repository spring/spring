#ifndef LUA_DISPLAY_LISTS_H
#define LUA_DISPLAY_LISTS_H
// LuaDisplayLists.h: interface for the CLuaDisplayLists class.
//
//////////////////////////////////////////////////////////////////////


#include <vector>
using std::vector;


class CLuaDisplayLists {
	public:
		CLuaDisplayLists()
		{
			active.push_back(0);
		}

		void Clear()
		{
			unused.clear();
			active.clear();
			active.push_back(0);
		}

		unsigned int GetCount() const { return active.size(); }
		
		unsigned int GetDList(unsigned int index)
		{
			if ((index <= 0) || (index >= active.size())) {
				return 0;
			}
			return active[index];
		}

		unsigned int NewDList(unsigned int dlist)
		{
			if (dlist == 0) {
				return 0;
			}
			if (!unused.empty()) {
				const unsigned int index = unused[unused.size() - 1];
				active[index] = dlist;
				unused.pop_back();
				return index;
			}
			active.push_back(dlist);
			return active.size() - 1;
		}
				
		void FreeDList(unsigned int index)
		{
			if ((index <= 0) || (index >= active.size())) {
				return;
			}
			if (active[index] != 0) {
				active[index] = 0;
				unused.push_back(index);
			}
		}

	private:
		vector<unsigned int> active;
		vector<unsigned int> unused; // references slots in active
};


#endif /* LUA_DISPLAY_LISTS_H */
