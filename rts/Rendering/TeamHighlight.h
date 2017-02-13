/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAMHIGHLIGHT_H
#define TEAMHIGHLIGHT_H

class CTeamHighlight
{
public:
	enum { HIGHLIGHT_FIRST = 0, HIGHLIGHT_NONE = 0, HIGHLIGHT_PLAYERS, HIGHLIGHT_ALL, HIGHLIGHT_SIZE, HIGHLIGHT_LAST = HIGHLIGHT_SIZE - 1 };
	static void Enable(unsigned currentTime);
	static void Disable();
	static void Update(int frameNum);
	static bool highlight;
};


#endif // TEAMHIGHLIGHT_H
