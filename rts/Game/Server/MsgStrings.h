#ifndef SERVERMSGSTRINGS_H_
#define SERVERMSGSTRINGS_H_

#include <string>

// All strings used by CGameServer, in a printf / boost::format compatible way

const std::string ServerStart = "Server started on port %d"; 
const std::string PlayingDemo = "Opening demofile %s";
const std::string ConnectAutohost = "Connecting to autohost on port %d";
const std::string DemoStart = "Beginning demo playback";
const std::string DemoEnd = "End of demo reached";
const std::string GameEnd = "Game has ended";
const std::string NoClientsExit = "No clients connected, shutting down server";

const std::string NoSyncResponse = "Error: Player %s did not send sync checksum for frame %d";
const std::string SyncError = "Sync error for %s in frame %d (%x)";
const std::string NoSyncCheck = "Warning: Sync checking disabled!";

const std::string ConnectionReject = "Connection attempt rejected (Message ID: %d Network version: %d Datalength: %d)";
const std::string WrongPlayer = "Got message %d from %d claiming to be from %d";
const std::string PlayerJoined = "%s %s finished loading and is now ingame";
const std::string PlayerLeft = "%s %s left the game: %s";

const std::string NoStartposChange = "%s tried to change his startposition illegally";
const std::string NoHelperAI = "%s (%d) is using a helper AI illegally";
const std::string NoTeamChange = "%s (%d) tried to change his team illegally";
const std::string NoAICreated = "%s (%d) tried to control team %i with an AI illegally";
const std::string NoAIChangeState = "%s (%d) tried to change the state of an AI (%i) controlling team %i to state %i illegally";

const std::string UnknownTeammsg = "Unknown action in NETMSG_TEAM (%d) from player %d";
const std::string UnknownNetmsg = "Unhandled net msg (%d) in server from %d";

const std::string CommandNotAllowed = "Player %d is not allowed to execute command %s";

#endif /*SERVERMSGSTRINGS_H_*/
