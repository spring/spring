-- Author: Tobi Vollebregt

--[[
Simple gadget which includes all *.lua files from UNITSCRIPT_LIBRARY_DIR
(defaults to: 'scripts/library/')

Theses files can define some useful snippets of Lua unit script code, and put
them in the global GG.UnitScript table, from where the scripts may use them.
]]--

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
	return {
		name      = "Lua unitscript snippet library",
		desc      = "Lua unitscript snippet library",
		author    = "Tobi Vollebregt",
		date      = "1 August 2009",
		license   = "GPL v2",
		layer     = -1,  --  must come before unit_script.lua
		enabled   = true --  loaded by default?
	}
end


if (not gadgetHandler:IsSyncedCode()) then
	return false
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local UNITSCRIPT_LIBRARY_DIR = UNITSCRIPT_LIBRARY_DIR or "scripts/library/"
local VFSMODE = VFS.ZIP_ONLY
if (Spring.IsDevLuaEnabled()) then
	VFSMODE = VFS.RAW_ONLY
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local scriptHeader = VFS.LoadFile("gamedata/unit_script_library_header.lua", VFSMODE)

-- Newlines (and comments) are stripped to not change line numbers in stacktraces.
scriptHeader = scriptHeader:gsub("%-%-[^\r\n]*", ""):gsub("[\r\n]", " ")


local function LoadScript(filename)
	local basename = filename:match("[^\\/:]*$") or filename
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
	local good, err = xpcall(chunk, debug.traceback)
	if (not good) then
		Spring.Echo("Failed to load: " .. basename .. "  (" .. err .. ")")
		return nil
	end
	return true
end


function gadget:Initialize()
	-- By convention, this is where snippets should be stored.
	if (not GG.UnitScript) then
		GG.UnitScript = {}
	end

	-- Load the snippets.
	local scriptFiles = VFS.DirList(UNITSCRIPT_LIBRARY_DIR, "*.lua", VFSMODE)
	for _,filename in ipairs(scriptFiles) do
		Spring.Echo("Loading unitscript snippet " .. filename)
		LoadScript(filename)
	end
end
