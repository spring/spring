/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_DESCRIPTION_H
#define COMMAND_DESCRIPTION_H

#include "Command.h"

struct SCommandDescription {
private:
	CR_DECLARE_STRUCT(SCommandDescription)

public:
	SCommandDescription():
		id(0),
		type(CMDTYPE_ICON),
		queueing(true),
		hidden(false),
		disabled(false),
		showUnique(false),
		onlyTexture(false) {}

	/// CMD_xxx code (custom codes can also be used)
	int id;
	/// CMDTYPE_xxx code
	int type;

	/// command name
	std::string name;
	/// the associated command action binding name
	std::string action;
	/// button texture
	std::string iconname;
	/// mouse cursor
	std::string mouseicon;
	/// tooltip text
	std::string tooltip;

	bool queueing;
	/// if true dont show a button for the command
	bool hidden;
	/// for greying-out commands
	bool disabled;
	/// command only applies to single units
	bool showUnique;
	/// do not draw the name if the texture is available
	bool onlyTexture;

	std::vector<std::string> params;
};



#endif // COMMAND_DESCRIPTION_H

