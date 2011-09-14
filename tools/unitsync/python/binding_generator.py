#! /usr/bin/env python

import sys,os,shutil
import pybindgen
from pybindgen.gccxmlparser import ModuleParser
from pybindgen.typehandlers import base as typehandlers
from pybindgen import ReturnValue, Parameter, Module, Function, FileCodeSink
from pybindgen import CppMethod, CppConstructor, CppClass, Enum

class BufferReturn(ReturnValue):
	CTYPES = []

	def __init__(self, ctype, length_expression):
		super(BufferReturn, self).__init__(ctype, is_const=False)
		self.length_expression = length_expression

	def convert_c_to_python(self, wrapper):
		pybuf = wrapper.after_call.declare_variable("PyObject*", "pybuf")
		wrapper.after_call.write_code("%s = PyBuffer_FromReadWriteMemory(retval, (%s)*sizeof(unsigned short int));" % (pybuf, self.length_expression))
		wrapper.build_params.add_parameter("N", [pybuf], prepend=True)

def my_module_gen(inputdir,outputdir,includedirs):
	aliases = [ ('uint8_t*', 'unsigned char*')]
	for alias in aliases:
		typehandlers.add_type_alias( alias[0], alias[1] )
	generator_fn = os.path.join( outputdir, 'unitsync_python_wrapper.cc' )
	module_parser = ModuleParser('pyunitsync')
	
	with open(generator_fn,'wb') as output:#ensures file is closed after output
		module = module_parser.parse([os.path.join( inputdir, 'unitsync_api.h')], include_paths=includedirs ,
				includes=['"../unitsync.h"','"../unitsync_api.h"','<Numeric/arrayobject.h>'],)
		module.add_function("GetMinimap", BufferReturn("unsigned short*", "1024*1024"), [Parameter.new('const char*', 'fileName'),Parameter.new('int', 'mipLevel')])
		module.generate( FileCodeSink(output) )

if __name__ == '__main__':
	my_module_gen(sys.argv[1],sys.argv[2],sys.argv[3:])
