/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CURSORICONS_H
#define CURSORICONS_H

#include <set>
#include <string>

#include "System/float3.h"
#include "System/UnorderedMap.hpp"

class CMouseCursor;

class CCursorIcons
{
	public:
		void Enable(bool b) { enabled = b; }

		inline void AddIcon(int cmd, const float3& pos);
		inline void AddIconText(const std::string& text, const float3& pos);
		inline void AddBuildIcon(int cmd, const float3& pos, int team, int facing);

		void SetCustomType(int cmdID, const std::string& cursor);

		void Draw();

	protected:
		void Clear();
		void DrawCursors();
		void DrawTexts();
		void DrawBuilds();

		const CMouseCursor* GetCursor(int cmd) const;

	protected:
	
		bool enabled = true;

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

		struct IconText {
			IconText(const std::string& t, const float3& p)
			: text(t), pos(p) {}

			bool operator<(const IconText& i) const
			{
				if (pos.x < i.pos.x) return true;
				if (pos.x > i.pos.x) return false;
				if (pos.y < i.pos.y) return true;
				if (pos.y > i.pos.y) return false;
				if (pos.z < i.pos.z) return true;
				if (pos.z > i.pos.z) return false;
				return (text < i.text);
			}
			std::string text;
			float3 pos;
		};

		struct BuildIcon {
			BuildIcon(int c, const float3& p, int t, int f)
			: pos(p), cmd(c), team(t), facing(f) {}

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
		std::set<IconText> texts;
		std::set<BuildIcon> buildIcons;

		spring::unsynced_map<int, std::string> customTypes;
};


inline void CCursorIcons::AddIcon(int cmd, const float3& pos)
{
	if (!enabled)
		return;

	icons.emplace(cmd, pos);
}

inline void CCursorIcons::AddIconText(const std::string& text, const float3& pos)
{
	if (!enabled)
		return;

	texts.emplace(text, pos);
}

inline void CCursorIcons::AddBuildIcon(int cmd, const float3& pos, int team, int facing)
{
	if (!enabled)
		return;

	buildIcons.emplace(cmd, pos, team, facing);
}


extern CCursorIcons cursorIcons;


#endif /* CURSORICONS_H */
