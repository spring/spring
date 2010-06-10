/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAMHIGHLIGHT_H
#define TEAMHIGHLIGHT_H

#include <map>

class CTeamHighlight
{
public:
	static void Enable(unsigned currentTime);
	static void Disable();
	static void Update(int team);
	static bool highlight;
	static std::map<int, int> oldColors;
};


#endif // TEAMHIGHLIGHT_H
