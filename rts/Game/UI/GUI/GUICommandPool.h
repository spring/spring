#ifndef GUICOMMANDPOOL_H
#define GUICOMMANDPOOL_H

#include <string>
using namespace std;

void InitCommands();
int FindCommand( const string& command );

#define ENTRIES \
	ENTRY( decreaseviewradius ) \
	ENTRY( pausegame ) \
	ENTRY( trajectory ) \
	ENTRY( drag_command_add ) \
	ENTRY( drag_command_replace ) \
	ENTRY( drag_default_command ) \
	ENTRY( replace_selection ) \
	ENTRY( add_selection ) \
	ENTRY( stop_command ) \
	ENTRY( deselect_all ) \
	ENTRY( cancel_or_deselect ) \
	ENTRY( default_command ) \
	ENTRY( dispatch_command ) \
	ENTRY( maybe_add_selection ) \
	ENTRY( maybe_replace_selection ) \
	ENTRY( showhealthbars ) \
	ENTRY( group0 ) \
	ENTRY( group1 ) \
	ENTRY( group2 ) \
	ENTRY( group3 ) \
	ENTRY( group4 ) \
	ENTRY( group5 ) \
	ENTRY( group6 ) \
	ENTRY( group7 ) \
	ENTRY( group8 ) \
	ENTRY( group9 ) \
	ENTRY( quit ) \
	ENTRY( togglelos ) \
	ENTRY( mousestate ) \
	ENTRY( updatefov ) \
	ENTRY( drawtrees ) \
	ENTRY( dynamicsky ) \
	ENTRY( increaseviewradius ) \
	ENTRY( moretrees ) \
	ENTRY( lesstrees ) \
	ENTRY( moreclouds ) \
	ENTRY( lessclouds ) \
	ENTRY( screenshot ) \
	ENTRY( speedup ) \
	ENTRY( slowdown ) \
	ENTRY( controlunit ) \
	ENTRY( showshadowmap ) \
	ENTRY( showstandard ) \
	ENTRY( showelevation ) \
	ENTRY( lastmsgpos ) \
	ENTRY( showmetalmap ) \
	ENTRY( showpathmap ) \
	ENTRY( drawinmapon ) \
	ENTRY( drawinmapoff ) \
	ENTRY( singlestep ) \
	ENTRY( chat ) \
	ENTRY( debug ) \
	ENTRY( track ) \
	ENTRY( nosound ) \
	ENTRY( savegame ) \
	ENTRY( createvideo ) \
	ENTRY( start_moveforward ) \
	ENTRY( start_moveback ) \
	ENTRY( start_moveleft ) \
	ENTRY( start_moveright ) \
	ENTRY( start_moveup ) \
	ENTRY( start_movedown ) \
	ENTRY( start_movefast ) \
	ENTRY( start_moveslow ) \
	ENTRY( end_moveforward ) \
	ENTRY( end_moveback ) \
	ENTRY( end_moveleft ) \
	ENTRY( end_moveright ) \
	ENTRY( end_moveup ) \
	ENTRY( end_movedown ) \
	ENTRY( end_movefast ) \
	ENTRY( end_moveslow )

enum COMMANDS
{
	COMMAND_INVALID = -1
#define ENTRY(e) , COMMAND_##e
	ENTRIES
#undef ENTRY
};

#endif
