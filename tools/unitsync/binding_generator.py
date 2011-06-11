#! /usr/bin/env python

import sys
import pybindgen
from pybindgen import FileCodeSink
from pybindgen.gccxmlparser import ModuleParser
from pybindgen.typehandlers import base as typehandlers
import shutil

def my_module_gen():
	aliases = [ ('uint8_t*', 'unsigned char*'),('uint16_t*', 'unsigned short*'),
		('uint16_t*', 'unsigned short int*')]
	for alias in aliases:
		typehandlers.add_type_alias( alias[0], alias[1] )
	generator_fn = '%s/generate_unitsync_python_wrapper.py'%sys.argv[1]
	module_parser = ModuleParser('pyunitsync')
	with open(generator_fn,'wb') as output:#ensures file is closed after output
		module_parser.parse(['unitsync_api.h'], include_paths=sys.argv[2:] ,
				includes=['"unitsync.h"','"unitsync_api.h"'],
				pygen_sink=FileCodeSink(output))
	#we have to manually insert the type aliases into the generator
	with open(generator_fn+'~','wb') as tmp:
		with open(generator_fn) as orig:
			for i,line in enumerate(orig):
				tmp.write(line)
				if i == 2: 
					for alias in aliases:
						tmp.write('typehandlers.add_type_alias( "%s", "%s" )\n'%(alias[0],alias[1]) )
	shutil.move(generator_fn+'~',generator_fn)

if __name__ == '__main__':
	my_module_gen()
