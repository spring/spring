-- Author: Tobi Vollebregt

-- This is prepended before the actual unitscript, so certain functions and
-- variables can be localized, without copy pasting this into every file.

-- Do not overuse this, there is a hard limit of 200 locals per closure in Lua!

local UnitScript = Spring.UnitScript

local EmitSfx = UnitScript.EmitSfx
local Explode = UnitScript.Explode
local GetUnitValue = UnitScript.GetUnitValue
local SetUnitValue = UnitScript.SetUnitValue
local Hide = UnitScript.Hide
local Show = UnitScript.Show

local Move = UnitScript.Move
local Turn = UnitScript.Turn
local Spin = UnitScript.Spin
local StopSpin = UnitScript.StopSpin

local StartThread = UnitScript.StartThread
local Signal = UnitScript.Signal
local SetSignalMask = UnitScript.SetSignalMask
local Sleep = UnitScript.Sleep
local WaitForMove = UnitScript.WaitForMove
local WaitForTurn = UnitScript.WaitForTurn

local x_axis = 1
local y_axis = 2
local z_axis = 3
