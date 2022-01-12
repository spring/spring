--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    addonRevisions.lua
--  brief:
--  author:  jK
--
--  Copyright (C) 2007-2013.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

require "crashHandler.lua"
require "table.lua"


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Addon Info

AddonRevs = {}

local startTimer = Spring.GetTimer()

local function GetDefaultKnownInfo(filepath, basename)
	return {
		filepath = filepath,
		basename = basename,
		name     = basename,
		version  = "0.1",
		layer    = 0,
		desc     = "",
		author   = "",
		license  = "",
		enabled  = false,
		api      = false,
		handler  = false,
		before   = {},
		after    = {},
		depend   = {},
		_rev     = 0,
	}
end


function AddonRevs.LoadAddonInfoRev2(filepath, _VFSMODE)
	local basename = Basename(filepath)

	local ki = GetDefaultKnownInfo(filepath, basename)
	ki._rev = 2

	_VFSMODE = _VFSMODE or VFSMODE
	local loadEnv = {addon = {InGetInfo = true}, math = math}

	local success, rvalue = pcall(VFS.Include, filepath, loadEnv, _VFSMODE)
		if not success then
			return "Failed to load: " .. basename .. "  (" .. tostring(rvalue) .. ")"
		end
		if rvalue == false then
			return true --// addon asked for a silent death
		end
		if type(rvalue) ~= "table" then
			return "Wrong return value: " .. basename
		end

	tcopy(ki, rvalue)
	return false, ki
end


function AddonRevs.LoadAddonInfoRev1(filepath, _VFSMODE)
	local addon = AddonRevs.ParseAddon(1, filepath, _VFSMODE)
	if addon then
		local basename = Basename(filepath)

		local ki = GetDefaultKnownInfo(filepath, basename)
		ki._rev = 1

		local rvalue
		if (addon.GetInfo) then
			rvalue = SafeCallAddon(addon, "GetInfo")
			if type(rvalue) ~= "table" then
				return "Failed to call GetInfo() in: " .. basename
			end
		else
			return "Missing GetInfo() in: " .. basename
		end

		tcopy(ki, rvalue)
		return false, ki
	end
end


function AddonRevs.ValidateKnownInfo(ki, _VFSMODE)
	if not ki then
		return "No KnownInfo given"
	end

	--// load/create data
	local knownInfo = handler.knownInfos[ki.name]
	if (not knowInfo) then
		knownInfo = {}
		handler.knownInfos[ki.name] = knownInfo
	end

	--// check for duplicated name
	if (knownInfo.filepath)and(knownInfo.filepath ~= ki.filepath) then
		return "Failed to load: " .. ki.basename .. " (duplicate name)"
	end

	--// create/update knownInfo table
	tcopy(knownInfo, ki) --// update table
end



--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Addon Parsing

function AddonRevs.NewAddonRev2()
	local addonEnv = {}
	local addon = addonEnv
	addonEnv.widget = addon
	addonEnv.addon  = addon  --// makes `function Initizalize` & `function addon.Initialize` point to the same data

	--// copy the engine enviroment to the addon
		tcopy(addonEnv, handler.EG)

	--// the shared table
		addonEnv.SG = handler.SG

	--// addon related methods
		tmergein(addonEnv.addon, {
			Remove = function() handler:Remove(addon, "auto") end,
			IsMouseOwner = function() return (handler.mouseOwner == addon) end,
			DisownMouse  = function()
				if (handler.mouseOwner == addon) then
					handler.mouseOwner = nil
				end
			end,
			UpdateCallIn = function(name) handler:UpdateAddonCallIn(name, addon) end,
			RemoveCallIn = function(name) handler:RemoveAddonCallIn(name, addon) end,
			AddAction    = function(cmd, func, data, types) return actionHandler.AddAction(addon, cmd, func, data, types) end,
			RemoveAction = function(cmd, types)             return actionHandler.RemoveAction(addon, cmd, types) end,
--[[
FIXME???
			AddLayoutCommand = function(_, cmd)
				if (handler.inCommandsChanged) then
					table.insert(handler.customCommands, cmd)
				else
					Spring.Echo("AddLayoutCommand() can only be used in CommandsChanged()")
				end
			end,
			GetCommands  = function() return handler.commands end,
--]]
			RegisterGlobal   = function(name, value) return handler:RegisterGlobal(addon, name, value) end,
			DeregisterGlobal = function(name)        return handler:DeregisterGlobal(addon, name) end,
			SetGlobal        = function(name, value) return handler:SetGlobal(addon, name, value) end,
		})

	--// insert handler
		addonEnv.handler = handler
		addonEnv[handler.name] = handler
	return addon
end


function AddonRevs.NewAddonRev1()
	local addonEnv = {}
	local addon = addonEnv --// easy self referencing
	addonEnv.addon  = addon
	addonEnv.widget = addon

	--// copy the engine enviroment to the addon
	tcopy(addonEnv, handler.EG)

	--// the shared table
	addonEnv.SG = handler.SG
	addonEnv.WG = handler.SG

	--// wrapped calls (closures)
	local h = {}
	addonEnv.handler = h
	addonEnv[handler.name] = h
	addonEnv.include = function(f) return include(f, addon) end
	h.ForceLayout  = handler.EG.ForceLayout
	h.RemoveWidget = function() handler:Remove(addon, "auto") end
	h.GetCommands  = function() return handler.commands end
	h.GetViewSizes = function() return gl.GetViewSizes() end
	h.GetHourTimer = function() return Spring.DiffTimers(Spring.GetTimer(), startTimer) % 3600 end
	h.IsMouseOwner = function() return (handler.mouseOwner == addon) end
	h.DisownMouse  = function()
		if (handler.mouseOwner == addon) then
			handler.mouseOwner = nil
		end
	end

	h.UpdateCallIn = function(_, name) handler:UpdateAddonCallIn(name, addon) end
	h.RemoveCallIn = function(_, name) handler:RemoveAddonCallIn(name, addon) end

	h.AddAction    = function(_, cmd, func, data, types) return actionHandler.AddAction(addon, cmd, func, data, types) end
	h.RemoveAction = function(_, cmd, types)             return actionHandler.RemoveAction(addon, cmd, types) end
	h.actionHandler    = actionHandler.oldSyntax

	h.AddLayoutCommand = function(_, cmd)
		if (handler.inCommandsChanged) then
			table.insert(handler.customCommands, cmd)
		else
			Spring.Echo("AddLayoutCommand() can only be used in CommandsChanged()")
		end
	end

	h.RegisterGlobal   = function(_, name, value) return handler:RegisterGlobal(addon, name, value) end
	h.DeregisterGlobal = function(_, name)        return handler:DeregisterGlobal(addon, name) end
	h.SetGlobal        = function(_, name, value) return handler:SetGlobal(addon, name, value) end

	return addonEnv
end


function AddonRevs.ParseAddon(rev, filepath, _VFSMODE)
	_VFSMODE = _VFSMODE or VFSMODE
	local basename = Basename(filepath)

	--// load the code
	local addonEnv = handler:NewAddon(rev)
	local success, err = pcall(VFS.Include, filepath, addonEnv, _VFSMODE)
	if (not success) then
		Spring.Log(LUA_NAME, "error", 'Failed to load: ' .. basename .. '  (' .. tostring(err) .. ')')
		return nil
	end
	if (err == false) then
		return nil --// addon asked for a silent death
	end

	local addon = addonEnv.addon

	--// Validate Callins
	err = handler:ValidateAddon(addon)
	if (err) then
		Spring.Log(LUA_NAME, "error", 'Failed to load: ' .. basename .. '  (' .. tostring(err) .. ')')
		return nil
	end

	return addon
end
