--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    spring.h.lua
--  brief:   Spring constants (Command IDs and CommandDescription types)
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

CMD_STOP                 =   0
CMD_WAIT                 =   5
CMD_TIMEWAIT             =   6
CMD_DEATHWAIT            =   7
CMD_SQUADWAIT            =   8
CMD_GATHERWAIT           =   9
CMD_MOVE                 =  10
CMD_PATROL               =  15
CMD_FIGHT                =  16
CMD_ATTACK               =  20
CMD_AREA_ATTACK          =  21
CMD_GUARD                =  25
CMD_AISELECT             =  30
CMD_GROUPSELECT          =  35
CMD_GROUPADD             =  36
CMD_GROUPCLEAR           =  37
CMD_REPAIR               =  40
CMD_FIRE_STATE           =  45
CMD_MOVE_STATE           =  50
CMD_SETBASE              =  55
CMD_INTERNAL             =  60
CMD_SELFD                =  65
CMD_SET_WANTED_MAX_SPEED =  70
CMD_LOAD_UNITS           =  75
CMD_UNLOAD_UNITS         =  80
CMD_UNLOAD_UNIT          =  81
CMD_ONOFF                =  85
CMD_RECLAIM              =  90
CMD_CLOAK                =  95
CMD_STOCKPILE            = 100
CMD_DGUN                 = 105
CMD_RESTORE              = 110
CMD_REPEAT               = 115
CMD_TRAJECTORY           = 120
CMD_RESURRECT            = 125
CMD_CAPTURE              = 130
CMD_AUTOREPAIRLEVEL      = 135
CMD_LOOPBACKATTACK       = 140

CommandNames = {
  [CMD_STOP]                 = 'STOP',
  [CMD_WAIT]                 = 'WAIT',
  [CMD_TIMEWAIT]             = 'TIMEWAIT',
  [CMD_DEATHWAIT]            = 'DEATHWAIT',
  [CMD_SQUADWAIT]            = 'SQUADWAIT',
  [CMD_GATHERWAIT]           = 'GATHERWAIT',
  [CMD_MOVE]                 = 'MOVE',
  [CMD_PATROL]               = 'PATROL',
  [CMD_FIGHT]                = 'FIGHT',
  [CMD_ATTACK]               = 'ATTACK',
  [CMD_AREA_ATTACK]          = 'AREA_ATTACK',
  [CMD_GUARD]                = 'GUARD',
  [CMD_AISELECT]             = 'AISELECT',
  [CMD_GROUPSELECT]          = 'GROUPSELECT',
  [CMD_GROUPADD]             = 'GROUPADD',
  [CMD_GROUPCLEAR]           = 'GROUPCLEAR',
  [CMD_REPAIR]               = 'REPAIR',
  [CMD_FIRE_STATE]           = 'FIRE_STATE',
  [CMD_MOVE_STATE]           = 'MOVE_STATE',
  [CMD_SETBASE]              = 'SETBASE',
  [CMD_INTERNAL]             = 'INTERNAL',
  [CMD_SELFD]                = 'SELFD',
  [CMD_SET_WANTED_MAX_SPEED] = 'SET_WANTED_MAX_SPEED',
  [CMD_LOAD_UNITS]           = 'LOAD_UNITS',
  [CMD_UNLOAD_UNITS]         = 'UNLOAD_UNITS',
  [CMD_UNLOAD_UNIT]          = 'UNLOAD_UNIT',
  [CMD_ONOFF]                = 'ONOFF',
  [CMD_RECLAIM]              = 'RECLAIM',
  [CMD_CLOAK]                = 'CLOAK',
  [CMD_STOCKPILE]            = 'STOCKPILE',
  [CMD_DGUN]                 = 'DGUN',
  [CMD_RESTORE]              = 'RESTORE',
  [CMD_REPEAT]               = 'REPEAT',
  [CMD_TRAJECTORY]           = 'TRAJECTORY',
  [CMD_RESURRECT]            = 'RESURRECT',
  [CMD_CAPTURE]              = 'CAPTURE',
  [CMD_AUTOREPAIRLEVEL]      = 'AUTOREPAIRLEVEL',
  [CMD_LOOPBACKATTACK]       = 'LOOPBACKATTACK'
}


CMDTYPE_ICON                       =  0
CMDTYPE_ICON_MODE                  =  5
CMDTYPE_ICON_MAP                   = 10
CMDTYPE_ICON_AREA                  = 11
CMDTYPE_ICON_UNIT                  = 12
CMDTYPE_ICON_UNIT_OR_MAP           = 13
CMDTYPE_ICON_FRONT                 = 14
CMDTYPE_COMBO_BOX                  = 15
CMDTYPE_ICON_UNIT_OR_AREA          = 16
CMDTYPE_NEXT                       = 17
CMDTYPE_PREV                       = 18
CMDTYPE_ICON_UNIT_FEATURE_OR_AREA  = 19
CMDTYPE_ICON_BUILDING              = 20
CMDTYPE_CUSTOM                     = 21
CMDTYPE_ICON_UNIT_OR_RECTANGLE     = 22
CMDTYPE_NUMBER                     = 23

CMDTYPENAMES = {
  [CMDTYPE_ICON]                      = 'ICON',
  [CMDTYPE_ICON_MODE]                 = 'ICON_MODE',
  [CMDTYPE_ICON_MAP]                  = 'ICON_MAP',
  [CMDTYPE_ICON_AREA]                 = 'ICON_AREA',
  [CMDTYPE_ICON_UNIT]                 = 'ICON_UNIT',
  [CMDTYPE_ICON_UNIT_OR_MAP]          = 'ICON_UNIT_OR_MAP',
  [CMDTYPE_ICON_FRONT]                = 'ICON_FRONT',
  [CMDTYPE_COMBO_BOX]                 = 'COMBO_BOX',
  [CMDTYPE_ICON_UNIT_OR_AREA]         = 'ICON_UNIT_OR_AREA',
  [CMDTYPE_NEXT]                      = 'NEXT',
  [CMDTYPE_PREV]                      = 'PREV',
  [CMDTYPE_ICON_UNIT_FEATURE_OR_AREA] = 'ICON_UNIT_FEATURE_OR_AREA',
  [CMDTYPE_ICON_BUILDING]             = 'ICON_BUILDING',
  [CMDTYPE_CUSTOM]                    = 'CUSTOM',
  [CMDTYPE_ICON_UNIT_OR_RECTANGLE]    = 'ICON_UNIT_OR_RECTANGLE',
  [CMDTYPE_NUMBER]                    = 'NUMBER'
}



--------------------------------------------------------------------------------
--
--  Alternate version, slower access
--

CMDS = {
  STOP                 =   0,
  WAIT                 =   5,
  TIMEWAIT             =   6,
  DEATHWAIT            =   7,
  SQUADWAIT            =   8,
  GATHERWAIT           =   9,
  MOVE                 =  10,
  PATROL               =  15,
  FIGHT                =  16,
  ATTACK               =  20,
  AREA_ATTACK          =  21,
  GUARD                =  25,
  AISELECT             =  30,
  GROUPSELECT          =  35,
  GROUPADD             =  36,
  GROUPCLEAR           =  37,
  REPAIR               =  40,
  FIRE_STATE           =  45,
  MOVE_STATE           =  50,
  SETBASE              =  55,
  INTERNAL             =  60,
  SELFD                =  65,
  SET_WANTED_MAX_SPEED =  70,
  LOAD_UNITS           =  75,
  UNLOAD_UNITS         =  80,
  UNLOAD_UNIT          =  81,
  ONOFF                =  85,
  RECLAIM              =  90,
  CLOAK                =  95,
  STOCKPILE            = 100,
  DGUN                 = 105,
  RESTORE              = 110,
  REPEAT               = 115,
  TRAJECTORY           = 120,
  RESURRECT            = 125,
  CAPTURE              = 130,
  AUTOREPAIRLEVEL      = 135,
  LOOPBACKATTACK       = 140
}

CMDNAMES = {
  [CMDS.STOP]                 = 'STOP',
  [CMDS.WAIT]                 = 'WAIT',
  [CMDS.TIMEWAIT]             = 'TIMEWAIT',
  [CMDS.DEATHWAIT]            = 'DEATHWAIT',
  [CMDS.SQUADWAIT]            = 'SQUADWAIT',
  [CMDS.GATHERWAIT]           = 'GATHERWAIT',
  [CMDS.MOVE]                 = 'MOVE',
  [CMDS.PATROL]               = 'PATROL',
  [CMDS.FIGHT]                = 'FIGHT',
  [CMDS.ATTACK]               = 'ATTACK',
  [CMDS.AREA_ATTACK]          = 'AREA_ATTACK',
  [CMDS.GUARD]                = 'GUARD',
  [CMDS.AISELECT]             = 'AISELECT',
  [CMDS.GROUPSELECT]          = 'GROUPSELECT',
  [CMDS.GROUPADD]             = 'GROUPADD',
  [CMDS.GROUPCLEAR]           = 'GROUPCLEAR',
  [CMDS.REPAIR]               = 'REPAIR',
  [CMDS.FIRE_STATE]           = 'FIRE_STATE',
  [CMDS.MOVE_STATE]           = 'MOVE_STATE',
  [CMDS.SETBASE]              = 'SETBASE',
  [CMDS.INTERNAL]             = 'INTERNAL',
  [CMDS.SELFD]                = 'SELFD',
  [CMDS.SET_WANTED_MAX_SPEED] = 'SET_WANTED_MAX_SPEED',
  [CMDS.LOAD_UNITS]           = 'LOAD_UNITS',
  [CMDS.UNLOAD_UNITS]         = 'UNLOAD_UNITS',
  [CMDS.UNLOAD_UNIT]          = 'UNLOAD_UNIT',
  [CMDS.ONOFF]                = 'ONOFF',
  [CMDS.RECLAIM]              = 'RECLAIM',
  [CMDS.CLOAK]                = 'CLOAK',
  [CMDS.STOCKPILE]            = 'STOCKPILE',
  [CMDS.DGUN]                 = 'DGUN',
  [CMDS.RESTORE]              = 'RESTORE',
  [CMDS.REPEAT]               = 'REPEAT',
  [CMDS.TRAJECTORY]           = 'TRAJECTORY',
  [CMDS.RESURRECT]            = 'RESURRECT',
  [CMDS.CAPTURE]              = 'CAPTURE',
  [CMDS.AUTOREPAIRLEVEL]      = 'AUTOREPAIRLEVEL',
  [CMDS.LOOPBACKATTACK]       = 'LOOPBACKATTACK'
}


CMDTYPES = {
  ICON                       =  0,
  ICON_MODE                  =  5,
  ICON_MAP                   = 10,
  ICON_AREA                  = 11,
  ICON_UNIT                  = 12,
  ICON_UNIT_OR_MAP           = 13,
  ICON_FRONT                 = 14,
  COMBO_BOX                  = 15,
  ICON_UNIT_OR_AREA          = 16,
  NEXT                       = 17,
  PREV                       = 18,
  ICON_UNIT_FEATURE_OR_AREA  = 19,
  ICON_BUILDING              = 20,
  CUSTOM                     = 21,
  ICON_UNIT_OR_RECTANGLE     = 22,
  NUMBER                     = 23
}
