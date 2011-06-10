#! /usr/bin/env python

import sys

import pybindgen
from pybindgen import FileCodeSink
from pybindgen.gccxmlparser import ModuleParser
from pybindgen.typehandlers import base as typehandlers
  
def my_module_gen():
	typehandlers.add_type_alias('uint8_t*', 'unsigned char*')
	typehandlers.add_type_alias('uint8_t*', 'unsigned char *')
	#typehandlers.add_type_alias('uint16_t*', 'short unsigned int*')
	
	module_parser = ModuleParser('pyunitsync')
	module_parser.parse(['unitsync_api.h'], include_paths=sys.argv[2:] ,\
			includes=['"unitsync.h"','"unitsync_api.h"'],
			pygen_sink=FileCodeSink(open('%s/generate_unitsync_python_wrapper.py'%sys.argv[1],'wb')))

if __name__ == '__main__':
	my_module_gen()
