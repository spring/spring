#!/usr/bin/env python2

# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
# written by ashdnazg to help convert unit scripts after 4785bdcc
import sys
import struct
import glob
import os.path

COB_HEADER_FORMAT = "<13L"
COB_HEADER_FIELDS = (
	"VersionSignature",
	"NumberOfScripts",
	"NumberOfPieces",
	"TotalScriptLen",
	"NumberOfStaticVars",
	"Unknown_2",
	"OffsetToScriptCodeIndexArray",
	"OffsetToScriptNameOffsetArray",
	"OffsetToPieceNameOffsetArray",
	"OffsetToScriptCode",
	"Unknown_3",
	"OffsetToSoundNameArray",
	"NumberOfSounds"
)

OLD_RETURN = struct.pack("<3L", 0x10021001, 0, 0x10065000)
BAD_RETURN = struct.pack("<2L", 0x10022000, 0x10065000)
NEW_RETURN = struct.pack("<3L", 0x10021002, 1, 0x10065000)

def unpack(bytes, offset, format):
	return struct.unpack(format, bytes[offset:offset+struct.calcsize(format)])

def unpack_string(bytes, offset):
	b = bytes[offset:]
	return b[:b.index('\0')]

def process_cob(path):
	print("processing %s" % (path,))
	content = open(path, "rb").read()
	header_values = unpack(content, 0, COB_HEADER_FORMAT)
	header = dict(zip(COB_HEADER_FIELDS, header_values))

	name_offset_array = header["OffsetToScriptNameOffsetArray"]
	code_offset_array = header["OffsetToScriptCodeIndexArray"]

	script_names = []
	script_code_offsets = []
	for i in range(header["NumberOfScripts"]):
		(script_name_offset,) = unpack(content, name_offset_array + i * 4, "<L")
		(script_code_offset,) = unpack(content, code_offset_array + i * 4, "<L")
		script_names.append(unpack_string(content, script_name_offset))
		script_code_offsets.append(script_code_offset)

	script_code_offsets.append(header["TotalScriptLen"])

	if "Killed" not in script_names:
		return
	killed_index = script_names.index("Killed")
	killed_offset = script_code_offsets[killed_index] * 4
	killed_len = script_code_offsets[killed_index + 1] * 4 - killed_offset
	base_offset = header["OffsetToScriptCode"]
	killed_content = content[base_offset + killed_offset:base_offset + killed_offset + killed_len]
	killed_new_content = killed_content.replace(OLD_RETURN, NEW_RETURN)
	if BAD_RETURN in killed_new_content:
		print "Warning: need to fix script %s manually" % (path,)

	if killed_content == killed_new_content:
		return

	new_content = content[:base_offset + killed_offset] + killed_new_content + content[base_offset + killed_offset + killed_len:]
	open(path, "wb").write(new_content)



def main(root_folder):
	cob_paths = glob.glob(os.path.join(root_folder, "*.[Cc][Oo][Bb]"))
	for cob_path in cob_paths:
		process_cob(cob_path)


if __name__ == '__main__':
	if len(sys.argv) < 2:
		print("Usage: %s <cob root folder>" % (sys.argv[0],))
		exit(1)
	main(sys.argv[1])

