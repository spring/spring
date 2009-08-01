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
		desc      = "Manages Lua unitscripts",
		author    = "Tobi Vollebregt",
		date      = "30 July 2009",
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
-- Create, Killed, and all AimWeapon callins are always wrapped.
local thread_wrap = {
	--"StartMoving",
	--"StopMoving",
	--"Activate",
	--"Deactivate",
	--"SetDirection",
	--"SetSpeed",
	"RockUnit",
	--"HitByWeapon",
	--"HitByWeaponId",
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
	--"Go",
}

-- Callins which exist for every weapon.
-- The ones which should not be thread-wrapped are commented out.
local thread_wrap_weapon = {
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


local UNITSCRIPT_DIR = "scripts/"
local VFSMODE = VFS.ZIP_ONLY
if (Spring.IsDevLuaEnabled()) then
	VFSMODE = VFS.RAW_ONLY
end

-- needed here too, and gadget handler doesn't expose it
VFS.Include('LuaGadgets/system.lua', nil, VFSMODE)

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
	return WakeUp(thread, unitID, ...) -- TODO: compare with COB?
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
local function NewScript()
	local script = {}
	for k,v in pairs(System) do
		script[k] = v
	end
	script._G = _G  -- the global table
	script.GG = GG  -- the shared table (shared with gadgets!)
	return script
end


local function LoadScript(filename)
	local basename = filename:match("[^\\/:]*$") or filename
	local unitname = basename:sub(1,-5):lower() --assumes 3 letter extension
	local text = VFS.LoadFile(filename, VFSMODE)
	if (text == nil) then
		Spring.Echo("Failed to load: " .. filename)
		return nil
	end
	local chunk, err = loadstring(scriptHeader .. text, filename)
	if (chunk == nil) then
		Spring.Echo("Failed to load: " .. basename .. "  (" .. err .. ")")
		return nil
	end
	local env = NewScript()
	setfenv(chunk, env)
	scripts[unitname] = { chunk = chunk, env = env }
	return chunk, env
end


function gadget:Initialize()
	local scriptFiles = VFS.DirList(UNITSCRIPT_DIR, "*.lua", VFSMODE)
	for _,filename in ipairs(scriptFiles) do
		Spring.Echo("Loading unitscript " .. filename)
		LoadScript(filename)
	end
	local allUnits = Spring.GetAllUnits()
	for i=1,#allUnits do
		local unitID = allUnits[i]
		gadget:UnitCreated(unitID, Spring.GetUnitDefID(unitID))
	end
end


local StartThread = Spring.UnitScript.StartThread


local function Wrap_AimWeapon(callins, weaponNum, isShield)
	local name = "AimWeapon" .. weaponNum
	local fun = callins[name]
	if (not fun) then return end

	-- SetUnitWeaponState and SetUnitShieldState count weapons from 0
	weaponNum = weaponNum - 1

	-- SetUnitShieldState wants true or false, while
	-- SetUnitWeaponState wants 1 or 0, niiice =)
	if isShield then
		local function AimWeaponThread(unitID)
			local enabled = fun(unitID) and true or false
			return sp_SetUnitShieldState(unitID, weaponNum, enabled)
		end

		callins[name] = function(unitID)
			return StartThread(unitID, AimWeaponThread)
		end
	else
		local function AimWeaponThread(unitID, heading, pitch)
			if fun(unitID, heading, pitch) then
				return sp_SetUnitWeaponState(unitID, weaponNum, "aimReady", 1)
			end
		end

		callins[name] = function(unitID, heading, pitch)
			return StartThread(unitID, AimWeaponThread, heading, pitch)
		end
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


function gadget:UnitCreated(unitID, unitDefID)
	local ud = UnitDefs[unitDefID]
	local script = scripts[ud.name]
	if (not script) then return end

	-- Global variables in the script are still per unit.
	-- Set up a new environment that is an instance of the prototype
	-- environment, so we don't need to copy all globals for every unit.
	-- This means of course, that global variable accesses are a bit more
	-- expensive inside unit scripts, but this can be worked around easily
	-- by localizing the necessary globals.
	local env = {
		unitID = unitID,
		script = {},     -- will store the callins
	}
	setmetatable(env, { __index = script.env })
	setfenv(script.chunk, env)

	-- Execute the chunk. This puts the callins in env.script
	script.chunk(unitID)
	local callins = env.script

	-- Remove Create(), Spring calls this too early to be useful to us.
	-- (framework hasn't had chance to set up units[unitID] entry at that point.)
	local Create = callins.Create
	callins.Create = nil

	-- Add framework callins.
	callins.MoveFinished = MoveFinished
	callins.TurnFinished = TurnFinished
	callins.Destroy = Destroy

	-- Wrap certain callins in a thread and/or safety net.
	for i=1,#thread_wrap do
		Wrap(callins, thread_wrap[i])
	end
	for i=1,ud.weapons.n do
		for j=1,#thread_wrap_weapon do
			Wrap(callins, thread_wrap_weapon[j] .. i)
		end
		Wrap_AimWeapon(callins, i, WeaponDefs[ud.weapons[i].weaponDef].type == "Shield")
	end
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
	callins.Create = Create
	StartThread(unitID, Create)
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
