Update: this functionality is directly integrated into Spring.

Please see rts/ExternalAI/GlobalAICInterface

======================================

This is a prototype ABI compatibility layer for spring

It builds with msvc at the moment
It should be possible to build ABIC with mingw; you'll need a mingw-compiled spring to use it

Preliminaries

Modify constants at top of buildabic.bat, buildmyai.bat and buildmyaimingw.bat to match your environment

Instructions

Open a cmd and change into this directory
Run buildabic.bat check no error messages.  Type:
   buildabic myailoader.dll myai.dll
   buildabic myaimingwloader.dll myaimingw.dll
run buildmyai.bat  check no error messages
run buildmyaimingw.bat check no error messages

Create a multiplayer game in lobby
- add myailoader.dll as a bot   This is an msvc-compiled bot
- add myaimingwloader.dll as a bot  This is a mingw-compiled bot
- check box "spectator"
- start the game

The AIs should greet you, state the map name, the name of the commander, and the commander's maximum slope
