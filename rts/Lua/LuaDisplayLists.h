/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DISPLAY_LISTS_H
#define LUA_DISPLAY_LISTS_H

#include <vector>
using std::vector;

#include "Rendering/GL/myGL.h"


class CLuaDisplayLists {
	typedef std::map<unsigned int, int> MatrixData;
	public:
		CLuaDisplayLists()
		{
			active.push_back(DLdata(0));
		}
		
		~CLuaDisplayLists()
		{
			// free the display lists
			for (int i = 0; i < (int)active.size(); i++) {
				glDeleteLists(active[i].id, 1);
			}
		}

		void Clear()
		{
			unused.clear();
			active.clear();
			active.push_back(DLdata(0));
		}

		unsigned int GetCount() const { return active.size(); }
		
		GLuint GetDList(unsigned int index) const
		{
			if (index < active.size()) {
				return active[index].id;
			} else {
				return 0;
			}
		}

		MatrixData GetMatrixData(unsigned int index) const
		{
			if (index < active.size()) {
				return active[index].matData;
			} else {
				return MatrixData();
			}
		}

		unsigned int NewDList(GLuint dlist, MatrixData& m)
		{
			if (dlist == 0) {
				return 0;
			}
			if (!unused.empty()) {
				const unsigned int index = unused[unused.size() - 1];
				active[index] = DLdata(dlist, m);
				unused.pop_back();
				return index;
			}
			active.push_back(DLdata(dlist, m));
			return active.size() - 1;
		}
				
		void FreeDList(unsigned int index)
		{
			if ((index < active.size()) && (active[index].id != 0)) {
				active[index] = DLdata(0);
				unused.push_back(index);
			}
		}

	private:
		struct DLdata {
			DLdata(int i) { id = i; }
			DLdata(int i, MatrixData &m) { id = i; matData = m; }
			GLuint id;
			MatrixData matData;
		};
		vector<DLdata> active;
		vector<unsigned int> unused; // references slots in active
};


#endif /* LUA_DISPLAY_LISTS_H */
