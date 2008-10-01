_____________________________________________________

RAI - Skirmish AI for the Spring Game Engine
Author: Reth / Michael Vadovszki
_____________________________________________________

Forum Link:
  http://spring.clan-sy.com/phpbb/viewtopic.php?t=7924

Compiling:
 - as of v0.76b1, the mingwlibs dependency is needed to compile an AI.
    - http://spring.clan-sy.com/wiki/Engine_Development#SCONS.2FMinGW
    - http://spring.clan-sy.com/dl/spring-mingwlibs-v10.exe
 - while compiling,
    - the RAI source folder was located in {spring source}/AI/Global/
    - the mingwlibs folder was in the same directory as the
      {spring source} directory

 - CodeBlocks - I barely used this one, it should be able to compile a
   working dll but I have not been keeping it up-to-date, so who knows.
    - website: http://www.codeblocks.org/
    - (OS - Windows Vista):  unless you know what you are doing then
      you might want to just give up right now, getting it to work
      probably won't be easy.
 - Visual Studios 2005 - The dlls that it compiles will not work with
   spring at this point, I now only use it as a writing aid.
    - The project configuration starts on debug, switch it to release.
 - DevC - the last few RAIs were compiled using this project
   (DevC v4.9.9.2 with WinPorts GCC for Windows v4.1.1 incorporated
   into it)
    - website: http://www.bloodshed.net/devcpp.html
    - updating GCC to 4.1.1 isn't a necessary step
    - I had DevC installed on the default C directory, a dll in the
      DevC directory was included in the project file.
    - The .o files in the DevC project directory were copied directly
      from the DevC directories to resolve a Vista/DevC bug.
    - (OS - Windows Vista): Compiler Options > Directories > Libraries:
      Add "{DevC directory}\lib\gcc\mingw32\3.4.2"


Copyright: GPL
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License or
    any later version.
