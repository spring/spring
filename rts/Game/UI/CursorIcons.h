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
		inline void AddBuildIcon(int cmd, const float3& pos, int team, int facing);

		void Draw(); // also clears the list
		void DrawCursors();
		void DrawBuilds();

	protected:
		CMouseCursor* GetCursor(int cmd);

	protected:

		struct Icon {
			Icon(int c, const float3& p)
			: cmd(c), pos(p) {}

			bool operator<(const Icon& i) const
			{
				// render the WAIT type commands last
				if (cmd > i.cmd) return true;
				if (cmd < i.cmd) return false;
				if (pos.x < i.pos.x) return true;
				if (pos.x > i.pos.x) return false;
				if (pos.y < i.pos.y) return true;
				if (pos.y > i.pos.y) return false;
				if (pos.z < i.pos.z) return true;
				if (pos.z > i.pos.z) return false;
				return false;
			}
			int cmd;
			float3 pos;
		};

		struct BuildIcon {
			BuildIcon(int c, const float3& p, int t, int f)
			: cmd(c), pos(p), team(t), facing(f) {}

			bool operator<(const BuildIcon& i) const
			{
				if (cmd > i.cmd) return true;
				if (cmd < i.cmd) return false;
				if (pos.x < i.pos.x) return true;
				if (pos.x > i.pos.x) return false;
				if (pos.y < i.pos.y) return true;
				if (pos.y > i.pos.y) return false;
				if (pos.z < i.pos.z) return true;
				if (pos.z > i.pos.z) return false;
				if (team > i.team) return true;
				if (team < i.team) return false;
				if (facing > i.facing) return true;
				if (facing < i.facing) return false;
				return false;
			}
			float3 pos;
			int cmd;
			int team;
			int facing;
		};
		// use a set to minimize the number of texture bindings,
		// and to avoid overdraw from multiple units with the
		// same command

		std::set<Icon> icons;
		std::set<BuildIcon> buildIcons;
};


inline void CCursorIcons::AddIcon(int cmd, const float3& pos)
{
	Icon icon(cmd, pos);
	icons.insert(icon);
}


inline void CCursorIcons::AddBuildIcon(int cmd, const float3& pos,
                                       int team, int facing)
{
	BuildIcon icon(cmd, pos, team, facing);
	buildIcons.insert(icon);
}


extern CCursorIcons* cursorIcons;


#endif /* CURSORICONS_H */
