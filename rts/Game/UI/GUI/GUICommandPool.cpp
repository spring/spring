#include <map>
#include <string>

using namespace std;

map < string, int > mCommands;

#include "GUICommandPool.h"

int smCommands = 0;

void InitCommands()
{
	if ( smCommands != 0 )
	{
		return;
	}

#define ENTRY(e) mCommands[#e] = smCommands++;
	ENTRIES
#undef ENTRY
}

int FindCommand ( const string& command )
{
	map < string, int >::const_iterator iter = mCommands.find(command);
	if ( iter == mCommands.end() )
	{
		return -1;
	}
	else
	{
		return iter->second;
	}
}
