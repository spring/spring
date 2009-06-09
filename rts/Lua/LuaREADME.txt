
Class Tree
----------
  LuaHandle
  |
  |- LuaHandleSynced
  |  |
  |  |- LuaGaia
  |  '- LuaRules
  |
  `- LuaUI


Synced / Unsynced Protection
----------------------------

The LuaHandleSynced derivatives all use the same mechanism for sync protection.
1. The global namespace is synced
2. The synced code can not read from the unsynced code, but may send
   messages to it.
3. The unsynced code can read from the synced code using the SYNCED proxy table.
   That table does not allow access to functions. There snext(), spairs(),
   and sipairs() functions can be used on the SYNCED tables and its sub-tables.


Access Modes
------------
             userMode  readFull  readAllyTeam  ctrlFull  ctrlTeam    selectTeam
             --------  --------  ------------  --------  --------    ----------
LuaRules     false     true      ALL           true      ALL         ALL
LuaGaia      false     true      ALL           true      ALL         Gaia
LuaUI        true      false     PlayerTeam    false     PlayerTeam  PlayerTeam
LuaUI(spec)  true      true      ALL           false     NONE        *depends*


The files
---------

Lua Handles

  LuaCallInHandler.cpp -- manages most call-ins for LuaHandles

  LuaHandle.cpp        -- Base for LuaHandleSynced's and LuaUI
	LuaHandleSynced.cpp  -- Base for LuaGaia and LuaRules
  LuaGaia.cpp          -- Gaia controller
  LuaRules.cpp         -- Mod game rules controller


Constants

  LuaConstGL.cpp       -- constants for OpenGL
  LuaConstCMD.cpp      -- constants for Commands
  LuaConstCMDTYPE.cpp  -- constants for CommandDescriptions


Call-out libraries

  LuaOpenGL.cpp        -- OpenGL interface
  LuaSyncedCtrl.cpp    -- Synchronized control
  LuaSyncedRead.cpp    -- Synchronized   information
  LuaUnsyncedCall.cpp  -- Unsynchronized inter-script calls
  LuaUnsyncedCtrl.cpp  -- Unsynchronized control
  LuaUnsyncedRead.cpp  -- Unsynchronized information
  LuaUnitDefs.cpp      -- UnitDefs table
  LuaFeatureDefs.cpp   -- FeatureDefs table
  LuaWeaponDefs.cpp    -- WeaponDefs table
  ...  ;-)


Utilities

  LuaUtils.h           -- misc crap
  LuaDefs.h            -- utilities for LuaUnitdefs and LuaWeaponDefs
  LuaHashString.h      -- wrapper for pre-hashed lua strings

