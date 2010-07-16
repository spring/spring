/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DISPLAY_LISTS_H
#define LUA_DISPLAY_LISTS_H

#include <vector>
using std::vector;

#include "Rendering/GL/myGL.h"


class CLuaDisplayLists {
	public:
		CLuaDisplayLists()
		{
			active.push_back(0);
		}
		
		~CLuaDisplayLists()
		{
			// free the display lists
			for (int i = 0; i < (int)active.size(); i++) {
				glDeleteLists(active[i], 1);
			}
		}

		void Clear()
		{
			unused.clear();
			active.clear();
			active.push_back(0);
		}

		unsigned int GetCount() const { return active.size(); }
		
		GLuint GetDList(unsigned int index) const
		{
			if ((index <= 0) || (index >= active.size())) {
				return 0;
			}
			return active[index];
		}

		unsigned int NewDList(GLuint dlist)
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
		vector<GLuint> active;
		vector<unsigned int> unused; // references slots in active
};


#endif /* LUA_DISPLAY_LISTS_H */
