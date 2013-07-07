--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    VFS_GetFileChecksum.lua
--  brief:
--  author:  jK
--
--  Copyright (C) 2007-2013.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (not VFS.GetFileChecksum) then
	function VFS.GetFileChecksum(file, _VFSMODE)
		local data = VFS.LoadFile(file, _VFSMODE)
		if (data) then
			local datalen     = data:len()/4 --// 'x/4' cause we use UnpackU32
			local striplength = 2 * 1024     --// 2kB

			if (striplength >= datalen) then
				local bytes = VFS.UnpackU32(data,nil,datalen)
				local checksum = math.bit_xor(0,unpack(bytes))
				return checksum
			end

			--// stack is limited, so split up the data
			local start = 1
			local crcs = {}
			repeat
				local strip = data:sub(start,start+striplength)
				local bytes = VFS.UnpackU32(strip,nil,strip:len()/4)
				local checksum = math.bit_xor(0,unpack(bytes))
				crcs[#crcs+1] = checksum
				start = start + striplength
			until (start >= datalen)

			local checksum = math.bit_xor(0,unpack(crcs))
			return checksum
		end
	end
end
