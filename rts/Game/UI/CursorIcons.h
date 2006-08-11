#ifndef CURSORICONS_H
#define CURSORICONS_H

#include <set>
#include "float3.h"

class CMouseCursor;

class CCursorIcons
{
	public:
		CCursorIcons();
		~CCursorIcons();

		inline void AddIcon(int cmd, const float3& pos);
		void Draw(); // also clears the list

	protected:
		CMouseCursor* GetCursor(int cmd);

	protected:
		struct Icon {
			Icon(int c, const float3& p) : command(c), pos(p) {}
			bool operator<(const Icon& i) const
			{
				if (command < i.command) return true;
				if (command > i.command) return false;
				if (pos.x < i.pos.x) return true;
				if (pos.x > i.pos.x) return false;
				if (pos.y < i.pos.y) return true;
				if (pos.y > i.pos.y) return false;
				if (pos.z < i.pos.z) return true;
				if (pos.z > i.pos.z) return false;
				return false;
			}
			int command;
			float3 pos;
		};
		// use a set to minimize the number of texture bindings,
		// and to avoid overdraw from multiple units with the
		// same command
		std::set<Icon> icons;
};

inline void CCursorIcons::AddIcon(int cmd, const float3& pos)
{
	Icon icon(cmd, pos);
	icons.insert(icon);
}


extern CCursorIcons* cursorIcons;


#endif /* CURSORICONS_H */
