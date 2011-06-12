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
		wrapper.after_call.write_code("%s = PyBuffer_FromReadWriteMemory(retval, (%s)*sizeof(short int));" % (pybuf, self.length_expression))
		wrapper.build_params.add_parameter("N", [pybuf], prepend=True)

def my_module_gen(inputdir,outputdir,includedirs):
	aliases = [ ('uint8_t*', 'unsigned char*'),('uint16_t*', 'unsigned short*'),
		('uint16_t*', 'short unsigned int*')]
	for alias in aliases:
		typehandlers.add_type_alias( alias[0], alias[1] )
	#generator_fn = os.path.join( outputdir, 'generate_unitsync_python_wrapper.py' )
	generator_fn = os.path.join( outputdir, 'unitsync_python_wrapper.cc' )
	module_parser = ModuleParser('pyunitsync')
	
	with open(generator_fn,'wb') as output:#ensures file is closed after output
		module = module_parser.parse([os.path.join( inputdir, 'unitsync_api.h')], include_paths=includedirs ,
				includes=['"../unitsync.h"','"../unitsync_api.h"'],)
		#module = Module('pyunitsync')
		#module.add_include( '"../unitsync_api.h"' )
		#do module stuff
		#EXPORT(unsigned short*) GetMinimap(const char* fileName, int mipLevel);
		module.add_function("GetMinimap", BufferReturn("unsigned short*", "1024*1024"), [Parameter.new('const char*', 'fileName'),Parameter.new('int', 'mipLevel')])
		#module.add_function("Dummy",None,[])
		module.generate( FileCodeSink(output) )
		
	#we have to manually insert the type aliases into the generator
	#with open(generator_fn+'~','wb') as tmp:
		#with open(generator_fn) as orig:
			#for i,line in enumerate(orig):
				#tmp.write(line)
				#if i == 2: 
					#for alias in aliases:
						#tmp.write('typehandlers.add_type_alias( "%s", "%s" )\n'%(alias[0],alias[1]) )
	#shutil.move(generator_fn+'~',generator_fn)

if __name__ == '__main__':
	my_module_gen(sys.argv[1],sys.argv[2],sys.argv[3:])
