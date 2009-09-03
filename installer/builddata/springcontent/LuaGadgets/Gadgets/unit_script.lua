-- Author: Tobi Vollebregt

--[[
Please, think twice before editing this file. Compared to most gadgets, there
are some complex things going on.  A good understanding of Lua's coroutines is
required to make nontrivial modifications to this file.

In other words, HERE BE DRAGONS =)

Known issues:
- {Query,AimFrom,Aim,Fire}{Primary,Secondary,Tertiary} are not handled.
  (use {Query,AimFrom,Aim,Fire}{Weapon1,Weapon2,Weapon3} instead!)
- Errors in Killed do not show a traceback.
- Errors in callins which aren't wrapped in a thread do not show a traceback.
- Which callins are wrapped in a thread and which aren't is a bit arbitrary.
- MoveFinished, TurnFinished and Destroy are overwritten by the framework.
- There is no way to reload the script of a single unit. (use /luarules reload)
- Error checking is lacking.  (In particular for incorrect unitIDs.)

To do:
- Test real world performance (compared to COB)
]]--

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
	return {
		name      = "Lua unitscript framework",
		desc      = "Manages Lua unit scripts",
		author    = "Tobi Vollebregt",
		date      = "2 September 2009",
		license   = "GPL v2",
		layer     = 0,
		enabled   = true --  loaded by default?
	}
end


if (not gadgetHandler:IsSyncedCode()) then
	return false
end


-- This lists all callins which may be wrapped in a coroutine (thread).
-- The ones which should not be thread-wrapped are commented out.
-- Create, Killed, AimWeapon and AimShield callins are always wrapped.
local thread_wrap = {
	--"StartMoving",
	--"StopMoving",
	--"Activate",
	--"Deactivate",
	--"WindChanged",
	--"ExtractionRateChanged",
	"RockUnit",
	--"HitByWeapon",
	--"MoveRate0",
	--"MoveRate1",
	--"MoveRate2",
	--"MoveRate3",
	--"setSFXoccupy",
	--"QueryLandingPad",
	"Falling",
	"Landed",
	"BeginTransport",
	--"QueryTransport",
	"TransportPickup",
	"StartUnload",
	"EndTransport",
	"TransportDrop",
	"StartBuilding",
	"StopBuilding",
	--"QueryNanoPiece",
	--"QueryBuildInfo",
	--"QueryWeapon",
	--"AimFromWeapon",
	"FireWeapon",
	--"EndBurst",
	--"Shot",
	--"BlockShot",
	--"TargetWeight",
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Localize often used methods.
local pairs = pairs

local co_create = coroutine.create
local co_resume = coroutine.resume
local co_yield = coroutine.yield
local co_running = coroutine.running

local bit_and = math.bit_and
local floor = math.floor

local sp_GetGameFrame = Spring.GetGameFrame
local sp_GetUnitWeaponState = Spring.GetUnitWeaponState
local sp_SetUnitWeaponState = Spring.SetUnitWeaponState
local sp_SetUnitShieldState = Spring.SetUnitShieldState

-- Keep local reference to engine's WaitForMove/WaitForTurn,
-- as we overwrite them with (safer) framework version later on.
local sp_WaitForMove = Spring.UnitScript.WaitForMove
local sp_WaitForTurn = Spring.UnitScript.WaitForTurn
local sp_SetPieceVisibility = Spring.UnitScript.SetPieceVisibility
local sp_SetDeathScriptFinished = Spring.UnitScript.SetDeathScriptFinished


local UNITSCRIPT_DIR = (UNITSCRIPT_DIR or "scripts/"):lower()
local VFSMODE = VFS.ZIP_ONLY
if (Spring.IsDevLuaEnabled()) then
	VFSMODE = VFS.RAW_ONLY
end

-- needed here too, and gadget handler doesn't expose it
VFS.Include('LuaGadgets/system.lua', nil, VFSMODE)
VFS.Include('gamedata/VFSUtils.lua', nil, VFSMODE)

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--[[
Data structure to administrate the threads of each managed unit.
We store a set of all threads for each unit, and in two separate tables
the threads which are waiting for a turn or move animation to finish.

The 'thread' stored in waitingForMove/waitingForTurn/sleepers is the table
wrapping the actual coroutine object.  This way the signal_mask etc. is
available too.

The threads table is a weak table.  This saves us from having to manually clean
up dead threads: any thread which is not sleeping or waiting is in none of
(sleepers,waitingForMove,waitingForTurn) => it is only in the threads table
=> garbage collector will harvest it because the table is weak.

Beware the threads are indexed by thread (coroutine), so careless
iteration of threads WILL cause desync!

Format: {
	[unitID] = {
		env = { the unit's environment table },
		waitingForMove = { [piece*3+axis] = thread, ... },
		waitingForTurn = { [piece*3+axis] = thread, ... },
		threads = { [thread] = { thread = thread, signal_mask = number, unitID = number }, ... },
	},
}
where ~ refers to the unit table again (allows finding the unit given a thread)
--]]
local units = {}

--[[
This is the bed, it stores all the sleeping threads,
indexed by the frame in which they need to be woken up.

Format: {
	[framenum] = { thread1, thread2, ... },
}
--]]
local sleepers = {}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Helper for Destroy and Signal.
local function Remove(tab, item)
	for i=1,#tab do
		if (tab[i] == item) then
			table.remove(tab, i)
			return
		end
	end
end

-- This is put in every script to clean up if the script gets destroyed.
local function Destroy(unitID)
	if units[unitID] then
		for _,thread in pairs(units[unitID].threads) do
			if thread.container then
				Remove(thread.container, thread) -- FIXME: performance?
			end
		end
		units[unitID] = nil
	end
end

-- Helper for AnimFinished, StartThread and gadget:GameFrame.
-- Resumes a sleeping or waiting thread; displays any errors.
local function WakeUp(thread, ...)
	thread.container = nil
	local co = thread.thread
	local good, err = co_resume(co, ...)
	if (not good) then
		Spring.Echo(err)
		Spring.Echo(debug.traceback(co))
	end
end

-- Helper for MoveFinished and TurnFinished
local function AnimFinished(threads, waitingForAnim, piece, axis)
	local index = piece * 3 + axis
	local threads = waitingForAnim[index]
	if threads then
		waitingForAnim[index] = {}
		for i=1,#threads do
			WakeUp(threads[i])
		end
	end
end

-- MoveFinished and TurnFinished are put in every script by the framework.
-- They resume the threads which were waiting for the move/turn.
local function MoveFinished(unitID, piece, axis)
	local u = units[unitID]
	return AnimFinished(u.threads, u.waitingForMove, piece, axis)
end

local function TurnFinished(unitID, piece, axis)
	local u = units[unitID]
	return AnimFinished(u.threads, u.waitingForTurn, piece, axis)
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Helper for WaitForMove and WaitForTurn
-- Unsafe, because it does not check whether the animation to wait for actually exists.
local function WaitForAnim(threads, waitingForAnim, piece, axis)
	local index = piece * 3 + axis
	local wthreads = waitingForAnim[index]
	if (not wthreads) then
		wthreads = {}
		waitingForAnim[index] = wthreads
	end
	local thread = threads[co_running() or error("not in a thread", 2)]
	wthreads[#wthreads+1] = thread
	thread.container = threads
	-- yield the running thread:
	-- it will be resumed once the wait finished (in AnimFinished).
	co_yield()
end

-- overwrites engine's WaitForMove
function Spring.UnitScript.WaitForMove(unitID, piece, axis)
	if sp_WaitForMove(unitID, piece, axis) then
		local u = units[unitID]
		return WaitForAnim(u.threads, u.waitingForMove, piece, axis)
	end
end

-- overwrites engine's WaitForTurn
function Spring.UnitScript.WaitForTurn(unitID, piece, axis)
	if sp_WaitForTurn(unitID, piece, axis) then
		local u = units[unitID]
		return WaitForAnim(u.threads, u.waitingForTurn, piece, axis)
	end
end

function Spring.UnitScript.Sleep(unitID, milliseconds)
	local n = floor(milliseconds / 33)
	if (n <= 0) then n = 1 end
	n = n + sp_GetGameFrame()
	local zzz = sleepers[n]
	if (not zzz) then
		zzz = {}
		sleepers[n] = zzz
	end
	local thread = units[unitID].threads[co_running() or error("not in a thread", 2)]
	zzz[#zzz+1] = thread
	thread.container = zzz
	-- yield the running thread:
	-- it will be resumed in frame #n (in gadget:GameFrame).
	co_yield()
end

function Spring.UnitScript.StartThread(unitID, fun, ...)
	local co = co_create(fun)
	local u = units[unitID]
	local thread = {
		thread = co,
		-- signal_mask is inherited from current thread, if any
		signal_mask = (co_running() and u.threads[co_running()].signal_mask or 0)
	}
	u.threads[co] = thread
	-- COB doesn't start thread immediately: it only sets up stack and
	-- pushes parameters on it for first time the thread is scheduled.
	-- Here it is easier however to start thread immediately, so we don't need
	-- to remember the parameters for the first co_resume call somewhere.
	-- I think in practice the difference in behavior isn't an issue.
	return WakeUp(thread, unitID, ...)
end

function Spring.UnitScript.SetSignalMask(unitID, mask)
	local thread = units[unitID].threads[co_running()]
	if thread then
		thread.signal_mask = mask
	end
end

function Spring.UnitScript.Signal(unitID, mask)
	-- beware, unsynced loop order
	-- (doesn't matter here as long as all threads get removed)
	for _,thread in pairs(units[unitID].threads) do
		if (bit_and(thread.signal_mask, mask) ~= 0) then
			if thread.container then
				Remove(thread.container, thread) -- FIXME: performance?
			end
		end
	end
end

function Spring.UnitScript.Hide(unitID, piece)
	return sp_SetPieceVisibility(unitID, piece, false)
end

function Spring.UnitScript.Show(unitID, piece)
	return sp_SetPieceVisibility(unitID, piece, true)
end

-- may be useful to other gadgets
function Spring.UnitScript.GetScriptEnv(unitID)
	return units[unitID].env
end

function Spring.UnitScript.GetLongestReloadTime(unitID)
	local longest = 0
	for i=0,31 do
		local reloadTime = sp_GetUnitWeaponState(unitID, i, "reloadTime")
		if (not reloadTime) then break end
		if (reloadTime > longest) then longest = reloadTime end
	end
	return 1000 * longest
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local scriptHeader = VFS.LoadFile("gamedata/unit_script_header.lua", VFSMODE)

-- Newlines (and comments) are stripped to not change line numbers in stacktraces.
scriptHeader = scriptHeader:gsub("%-%-[^\r\n]*", ""):gsub("[\r\n]", " ")


--[[
Dictionary mapping script name (without path or extension) to a Lua chunk which
returns a new closure (read; instance) of this unitscript, and a prototype
environment for each instance of the unit.

Format: {
	[unitID] = { chunk = chunk, env = env },
}
--]]
local scripts = {}


-- Creates a new environment for a unit script.
-- This environment is used as prototype for the unit script instances.
-- (To save on time copying and space for a copy for each and every unit.)
local function NewScript()
	local script = {}
	for k,v in pairs(System) do
		script[k] = v
	end
	script._G = _G  -- the global table
	script.GG = GG  -- the shared table (shared with gadgets!)
	return script
end


local function Basename(filename)
	return filename:match("[^\\/:]*$") or filename
end


local function LoadChunk(filename)
	local text = VFS.LoadFile(filename, VFSMODE)
	if (text == nil) then
		Spring.Echo("Failed to load: " .. filename)
		return nil
	end
	local chunk, err = loadstring(scriptHeader .. text, filename)
	if (chunk == nil) then
		Spring.Echo("Failed to load: " .. Basename(basename) .. "  (" .. err .. ")")
		return nil
	end
	return chunk
end


local function LoadScript(scriptName, filename)
	local chunk = LoadChunk(filename)
	local env = NewScript()
	setfenv(chunk, env)
	scripts[scriptName] = { chunk = chunk, env = env }
	return chunk, env
end


function gadget:Initialize()
	-- This initialization code has following properties:
	--  * all used scripts are loaded => early syntax error detection
	--  * unused scripts aren't loaded
	--  * files can be arbitrarily ordered in subdirs (like defs)
	--  * exact path doesn't need to be specified
	--  * exact path can be specified to resolve ambiguous basenames
	--  * engine default scriptName (with .cob extension) works

	-- Recursively collect files below UNITSCRIPT_DIR.
	local scriptFiles = {}
	for _,filename in ipairs(RecursiveFileSearch(UNITSCRIPT_DIR, "*.lua", VFSMODE)) do
		local basename = Basename(filename)
		scriptFiles[filename] = filename  -- for exact match
		scriptFiles[basename] = filename  -- for basename match
	end

	-- Go through all UnitDefs and load scripts.
	-- Names are tested in following order:
	--  * exact match
	--  * basename match
	--  * exact match where .cob->.lua
	--  * basename match where .cob->.lua
	for i=1,#UnitDefs do
		local unitDef = UnitDefs[i]
		if (unitDef and not scripts[unitDef.scriptName]) then
			local fn  = UNITSCRIPT_DIR .. unitDef.scriptName:lower()
			local bn  = Basename(fn)
			local cfn = fn:gsub("%.cob$", "%.lua")
			local cbn = bn:gsub("%.cob$", "%.lua")
			local filename = scriptFiles[fn] or scriptFiles[bn] or
			                 scriptFiles[cfn] or scriptFiles[cbn]
			if filename then
				Spring.Echo("Loading unitscript " .. filename)
				LoadScript(unitDef.scriptName, filename)
			end
		end
	end

	-- Fake UnitCreated events for existing units. (for '/luarules reload')
	local allUnits = Spring.GetAllUnits()
	for i=1,#allUnits do
		local unitID = allUnits[i]
		gadget:UnitCreated(unitID, Spring.GetUnitDefID(unitID))
	end
end

--------------------------------------------------------------------------------

local StartThread = Spring.UnitScript.StartThread


local function Wrap_AimWeapon(callins)
	local fun = callins["AimWeapon"]
	if (not fun) then return end

	-- SetUnitShieldState wants true or false, while
	-- SetUnitWeaponState wants 1 or 0, niiice =)
	local function AimWeaponThread(unitID, weaponNum, heading, pitch)
		if fun(unitID, weaponNum, heading, pitch) then
			-- SetUnitWeaponState counts weapons from 0
			return sp_SetUnitWeaponState(unitID, weaponNum - 1, "aimReady", 1)
		end
	end

	callins["AimWeapon"] = function(unitID, weaponNum, heading, pitch)
		return StartThread(unitID, AimWeaponThread, weaponNum, heading, pitch)
	end
end


local function Wrap_AimShield(callins)
	local fun = callins["AimShield"]
	if (not fun) then return end

	-- SetUnitShieldState wants true or false, while
	-- SetUnitWeaponState wants 1 or 0, niiice =)
	local function AimWeaponThread(unitID, weaponNum)
		local enabled = fun(unitID, weaponNum) and true or false
		-- SetUnitShieldState counts weapons from 0
		return sp_SetUnitShieldState(unitID, weaponNum, enabled)
	end

	callins["AimShield"] = function(unitID, weaponNum)
		return StartThread(unitID, AimWeaponThread, weaponNum)
	end
end


local function Wrap_Killed(callins)
	local fun = callins["Killed"]
	if (not fun) then return end

	local function KilledThread(unitID, recentDamage, maxHealth)
		-- It is *very* important the sp_SetDeathScriptFinished is executed, hence pcall.
		--local good, wreckLevel = xpcall(fun, debug.traceback, unitID, recentDamage, maxHealth)
		local good, wreckLevel = pcall(fun, unitID, recentDamage, maxHealth)
		if (not good) then
			-- wreckLevel is the error message in this case =)
			Spring.Echo(wreckLevel)
			wreckLevel = -1 -- no wreck, no error
		end
		sp_SetDeathScriptFinished(unitID, wreckLevel)
	end

	callins["Killed"] = function(unitID, recentDamage, maxHealth)
		StartThread(unitID, KilledThread, recentDamage, maxHealth)
		return -- no return value signals Spring to wait for SetDeathScriptFinished call.
	end
end


local function Wrap(callins, name)
	local fun = callins[name]
	if (not fun) then return end

	callins[name] = function(unitID, ...)
		return StartThread(unitID, fun, ...)
	end
end

--------------------------------------------------------------------------------

--[[
Storage for MemoizedInclude.
Format: { [filename] = chunk }
--]]
local include_cache = {}


-- core of include() function for unit scripts
local function ScriptInclude(filename)
	local chunk = LoadChunk(filename)
	include_cache[filename] = chunk
	return chunk
end


-- memoize it so we don't need to decompress and parse the .lua file everytime..
local function MemoizedInclude(filename, env)
	local chunk = include_cache[filename] or ScriptInclude(filename)
	--overwrite environment so it access environment of current unit
	setfenv(chunk, env)
	return chunk()
end

--------------------------------------------------------------------------------

function gadget:UnitCreated(unitID, unitDefID)
	local script = scripts[UnitDefs[unitDefID].scriptName]
	if (not script) then return end

	-- Global variables in the script are still per unit.
	-- Set up a new environment that is an instance of the prototype
	-- environment, so we don't need to copy all globals for every unit.

	-- This means of course, that global variable accesses are a bit more
	-- expensive inside unit scripts, but this can be worked around easily
	-- by localizing the necessary globals.
	local pieces = Spring.GetUnitPieceMap(unitID)
	local env = {
		unitID = unitID,
		unitDefID = unitDefID,
		include = function(f) return MemoizedInclude(f, env) end,
		piece = function(name)
					if not pieces[name] then
						error("piece not found: " .. tostring(name), 2)
					end
					return pieces[name]
				end,
		script = {},     -- will store the callins
	}
	setmetatable(env, { __index = script.env })
	setfenv(script.chunk, env)

	-- Execute the chunk. This puts the callins in env.script
	script.chunk(unitID)
	local callins = env.script

	-- Add framework callins.
	callins.MoveFinished = MoveFinished
	callins.TurnFinished = TurnFinished
	callins.Destroy = Destroy

	-- Wrap certain callins in a thread and/or safety net.
	for i=1,#thread_wrap do
		Wrap(callins, thread_wrap[i])
	end
	Wrap_AimWeapon(callins)
	Wrap_AimShield(callins)
	Wrap_Killed(callins)

	-- Register the callins with Spring.
	Spring.UnitScript.CreateScript(unitID, callins)

	-- Register (must be last: it shouldn't be done in case of error.)
	units[unitID] = {
		env = env,
		waitingForMove = {},
		waitingForTurn = {},
		threads = setmetatable({}, {__mode = "kv"}), -- weak table
	}

	-- Now it's safe to start a thread which will run Create().
	-- (Spring doesn't run it, and if it did, it would do so too early to be useful.)
	StartThread(unitID, callins.Create)
end


function gadget:GameFrame(n)
	local zzz = sleepers[n]
	if zzz then
		sleepers[n] = nil
		-- Wake up the lazy bastards.
		for i=1,#zzz do
			WakeUp(zzz[i])
		end
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
