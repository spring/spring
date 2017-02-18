/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_DISPLAY_LISTS_H
#define LUA_DISPLAY_LISTS_H

#include <vector>

#include "Rendering/GL/myGL.h"

struct SMatrixStateData {
	SMatrixStateData(): mode(GL_MODELVIEW), 
						modelView(0), 
						projection(0),
						texture(0) {}
	int mode;
	int modelView;
	int projection;
	int texture;
};

class CLuaDisplayLists {
	public:
		CLuaDisplayLists() { Clear(); }
		~CLuaDisplayLists()
		{
			// free the display lists
			// NOTE:
			//   it is not an error to delete a list with id=0, but we might
			//   be called from ~LuaParser which can run in multiple threads
			//   and the null-list is always present (even after Clear())
			for (size_t i = 1; i < active.size(); i++) {
				glDeleteLists(active[i].id, 1);
			}
		}

		void Clear() {
			unused.clear();
			active.clear();
			active.push_back(DLdata(0));
		}

		unsigned int GetCount() const { return active.size(); }
		
		GLuint GetDList(unsigned int index) const
		{
			if (index < active.size())
				return active[index].id;

			return 0;
		}

		SMatrixStateData GetMatrixState(unsigned int index) const
		{
			if (index < active.size())
				return active[index].matData;

			return SMatrixStateData();
		}

		unsigned int NewDList(GLuint dlist, SMatrixStateData& m)
		{
			if (dlist == 0)
				return 0;

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
			DLdata(int i): id(i) {}
			DLdata(int i, SMatrixStateData &m): id(i), matData(m) {}
			GLuint id;
			SMatrixStateData matData;
		};
		std::vector<DLdata> active;
		std::vector<unsigned int> unused; // references slots in active
};


#endif /* LUA_DISPLAY_LISTS_H */
