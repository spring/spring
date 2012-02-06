
if not Spring.GetConfigInt("LuaSocketEnabled", 0) == 1 then
	Spring.Echo("LuaSocketEnabled is disabled")
	return false
end

function widget:GetInfo()
return {
	name    = "Test-Widget for luasocket",
	desc    = "a simple test widget to show capabilities of luasocket",
	author  = "abma",
	date    = "Jul. 2011",
	license = "GNU GPL, v2 or later",
	layer   = 0,
	enabled = true,
}
end

--VFS.Include(LUAUI_DIRNAME .. "Widgets/socket/socket.lua")

local socket = socket

local client
local set
local headersent

local host = "localhost"
local port = 80
local file = "/"

local function dumpConfig()
	-- dump all luasocket related config settings to console
	for _, conf in ipairs({"TCPAllowConnect", "TCPAllowListen", "UDPAllowConnect", "UDPAllowListen"  }) do
		Spring.Echo(conf .. " = " .. Spring.GetConfigString(conf, ""))
	end

end

function widget:Initialize()
	dumpConfig()
	client=socket.tcp()
	client:settimeout(0)
	--Spring.Echo(socket.dns.toip("localhost"))
	--FIXME dns-request seems to block
	res, err = client:connect(host, port)
	if not res and not res=="timeout" then
		Spring.Echo("Error in connect: "..err)
		return
	end
	set = newset()
	set:insert(client)
end

function newset()
    local reverse = {}
    local set = {}
    return setmetatable(set, {__index = {
        insert = function(set, value)
            if not reverse[value] then
                table.insert(set, value)
                reverse[value] = table.getn(set)
            end
        end,
        remove = function(set, value)
            local index = reverse[value]
            if index then
                reverse[value] = nil
                local top = table.remove(set)
                if top ~= value then
                    reverse[top] = index
                    set[index] = top
                end
            end
        end
    }})
end

function widget:DrawScreen(n)
	if set==nil or #set<=0 then
		return
	end
	-- get sockets ready for read
	local readable, writeable, error = socket.select(set, set, 0)
	if error~=nil then
		-- some error happened in select
		if error=="timeout" then
			-- nothing to do, return
			return
		end
		Spring.Echo("Error in select: " .. error)
	end
	for __, input in ipairs(readable) do
		local s, status, partial = input:receive('*a') --try to read all data
		if status == "timeout" then
			Spring.Echo(s or partial)
		elseif status == "closed" then
			Spring.Echo("closed connection")
			input:close()
			set:remove(input)
		end
	end
	for __, output in ipairs(writeable) do
		if headersent==nil then
			-- socket is writeable
			headersent=1
			Spring.Echo("sending http request")
			output:send("GET " .. file .. " HTTP/1.0\r\nHost: " .. host ..  " \r\n\r\n")
		end
	end
end
