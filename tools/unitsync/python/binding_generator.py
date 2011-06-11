#! /usr/bin/env python

import sys,os,shutil
import pybindgen
from pybindgen import FileCodeSink
from pybindgen.gccxmlparser import ModuleParser
from pybindgen.typehandlers import base as typehandlers

def my_module_gen(inputdir,outputdir,includedirs):
	aliases = [ ('uint8_t*', 'unsigned char*'),('uint16_t*', 'unsigned short*'),
		('uint16_t*', 'unsigned short int*')]
	for alias in aliases:
		typehandlers.add_type_alias( alias[0], alias[1] )
	generator_fn = os.path.join( outputdir, 'generate_unitsync_python_wrapper.py' )
	module_parser = ModuleParser('pyunitsync')
	with open(generator_fn,'wb') as output:#ensures file is closed after output
		module_parser.parse([os.path.join( inputdir, 'unitsync_api.h')], include_paths=includedirs ,
				includes=['"../unitsync.h"','"../unitsync_api.h"'],
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
	my_module_gen(sys.argv[1],sys.argv[2],sys.argv[3:])
