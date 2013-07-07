--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    actions.lua
--  brief:   action interface for text commands, and bound commands
--  author:  Dave Rodgers, jK
--
--  Copyright (C) 2007-2011.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--TODO more unification with LuaGadget's one
--TODO add LuaMessages

if (actionHandler) then
	return actionHandler
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local textActions       = {}
local keyPressActions   = {}
local keyRepeatActions  = {}
local keyReleaseActions = {}
--local syncActions = {}


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Helpers

local function MakeWords(line)
	local words = {}
	for w in line:gmatch("[^%s]+") do
		table.insert(words, w)
	end
	return words
end


local function ParseTypes(types, def)
	if (type(types) ~= "string") then
		types = def
	end
	local text       = (types:find("t") ~= nil)
	local keyPress   = (types:find("p") ~= nil)
	local keyRepeat  = (types:find("R") ~= nil)
	local keyRelease = (types:find("r") ~= nil)
	return text, keyPress, keyRepeat, keyRelease
end


local function MakeKeySetString(key, mods)
	local keyset = ""
	if (mods.alt)   then keyset = keyset .. "A+" end
	if (mods.ctrl)  then keyset = keyset .. "C+" end
	if (mods.meta)  then keyset = keyset .. "M+" end
	if (mods.shift) then keyset = keyset .. "S+" end
	local userSym, defSym = Spring.GetKeySymbol(key)
	return (keyset .. defSym)
end


local function InsertCallInfo(callInfoList, addon, func, data)
	local layer = addon._info.layer
	local index = 1
	for i,ci in ipairs(callInfoList) do
		local w = ci[1]
		if (w == addon) then
			return false  --  already in the table
		end
		if (layer >= w._info.layer) then
			index = i + 1
		end
	end
	table.insert(callInfoList, index, { addon, func, data })
	return true
end


local function InsertAction(map, cmd, addon, func, data)
	local callInfoList = map[cmd]
	if not callInfoList then
		callInfoList = {}
		map[cmd] = callInfoList
	end
	return InsertCallInfo(callInfoList, addon, func, data)
end


local function RemoveCallInfo(callInfoList, addon)
	local count = 0
	for i,callInfo in ipairs(callInfoList) do
		local w = callInfo[1]
		if (w == addon) then
			table.remove(callInfoList, i)
			count = count + 1
			-- break
		end
	end
	return count
end


local function ClearActionList(actionMap, addon)
	for cmd, callInfoList in pairs(actionMap) do
		RemoveCallInfo(callInfoList, addon)
	end
end


local function RemoveAction(map, addon, cmd)
	local callInfoList = map[cmd]
	if (callInfoList == nil) then
		return false
	end
	local count = RemoveCallInfo(callInfoList, addon)
	if (#callInfoList <= 0) then
		map[cmd] = nil
	end
	return (count > 0)
end


local function TryAction(actionMap, cmd, optLine, optWords, isRepeat, release)
	local callInfoList = actionMap[cmd]
	if not callInfoList then
		return false
	end
	for i,callInfo in ipairs(callInfoList) do
		--local addon = callInfo[1]
		local func   = callInfo[2]
		local data   = callInfo[3]
		if (func(cmd, optLine, optWords, data, isRepeat, release)) then
			return true
		end
	end
	return false
end




--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Insertions
--

local function AddAddonAction(addon, cmd, func, data, types, _)
	assert(_ == nil, "actionHandler:Foobar() is deprecated, use actionHandler.Foobar()!")

	-- make sure that this is a fully initialized addon
	if (not addon._info) then
		error(LUA_NAME .. "error adding action: please use addon:Initialize()")
	end

	-- default to text and keyPress  (not repeat or releases)
	local text, keyPress, keyRepeat, keyRelease = ParseTypes(types, "tp")

	local tSuccess, pSuccess, RSuccess, rSuccess = false, false, false, false

	if (text)       then tSuccess = InsertAction(textActions, cmd, addon, func, data)       end
	if (keyPress)   then pSuccess = InsertAction(keyPressActions, cmd, addon, func, data)   end
	if (keyRepeat)  then RSuccess = InsertAction(keyRepeatActions, cmd, addon, func, data)  end
	if (keyRelease) then rSuccess = InsertAction(keyReleaseActions, cmd, addon, func, data) end

	return tSuccess, pSuccess, RSuccess, rSuccess
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Removals
--

local function RemoveAddonAction(addon, cmd, types, _)
	assert(_ == nil, "actionHandler:Foobar() is deprecated, use actionHandler.Foobar()!")

	-- default to removing all
	local text, keyPress, keyRepeat, keyRelease = ParseTypes(types, "tpRr")

	local tSuccess, pSuccess, RSuccess, rSuccess = false, false, false, false

	if (text)       then tSuccess = RemoveAction(textActions, addon, cmd)       end
	if (keyPress)   then pSuccess = RemoveAction(keyPressActions, addon, cmd)   end
	if (keyRepeat)  then RSuccess = RemoveAction(keyRepeatActions, addon, cmd)  end
	if (keyRelease) then rSuccess = RemoveAction(keyReleaseActions, addon, cmd) end

	return tSuccess, pSuccess, RSuccess, rSuccess
end


local function RemoveAddonActions(addon, _)
	assert(_ == nil, "actionHandler:Foobar() is deprecated, use actionHandler.Foobar()!")

	ClearActionList(textActions, addon)
	ClearActionList(keyPressActions, addon)
	ClearActionList(keyRepeatActions, addon)
	ClearActionList(keyReleaseActions, addon)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Calls
--


local function KeyAction(press, key, mods, isRepeat, _)
	assert(_ == nil, "actionHandler:Foobar() is deprecated, use actionHandler.Foobar()!")

	local keyset = MakeKeySetString(key, mods)
	local defBinds = Spring.GetKeyBindings(keyset)
	if (defBinds) then
		local actionSet
		if (press) then
			actionSet = isRepeat and keyRepeatActions or keyPressActions
		else
			actionSet = keyReleaseActions
		end
		for b,bAction in ipairs(defBinds) do
			local bCmd, bOpts = next(bAction, nil)
			local words = MakeWords(bOpts)
			if (TryAction(actionSet, bCmd, bOpts, words, isRepeat, not press)) then
				return true
			end
		end
	end
	return false
end


local function TextAction(line, _)
	assert(_ == nil, "actionHandler:Foobar() is deprecated, use actionHandler.Foobar()!")

	local words = MakeWords(line)
	local cmd = words[1]
	if not cmd then
		return false
	end
	-- remove the command from the words list and the raw line
	table.remove(words, 1)
	local _,_,line = line:find("[^%s]+[%s]+(.*)")
	if not line then
		line = ""  -- no args
	end

	return TryAction(textActions, cmd, line, words, false, nil)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

actionHandler = {
	KeyAction  = KeyAction,
	TextAction = TextAction,

	AddAction = AddAddonAction,
	RemoveAction  = RemoveAddonAction,
	RemoveAddonActions = RemoveAddonActions,

	--used by rev1 addons
	oldSyntax = {
		KeyAction           = function(_, ...) return KeyAction(...) end,
		TextAction          = function(_, ...) return TextAction(...) end,
		AddAction           = function(_, ...) return AddAddonAction(...) end,
		RemoveAction        = function(_, ...) return RemoveAddonAction(...) end,
		RemoveWidgetActions = function(_, ...) return RemoveAddonActions(...) end,
	}

	--LuaRules
	--GotChatMsg     = GotChatMsg
	--RecvFromSynced = RecvFromSynced
	--HaveChatAction = function() return (next(chatActions) ~= nil) end,
	--HaveSyncAction = function() return (next(syncActions) ~= nil) end,
}

return actionHandler

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
