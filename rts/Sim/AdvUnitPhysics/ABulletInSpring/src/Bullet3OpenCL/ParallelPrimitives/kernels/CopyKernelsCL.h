//this file is autogenerated using stringify.bat (premake --stringify) in the build folder of this project
static const char* copyKernelsCL= \
"/*\n"
"Copyright (c) 2012 Advanced Micro Devices, Inc.  \n"
"\n"
"This software is provided 'as-is', without any express or implied warranty.\n"
"In no event will the authors be held liable for any damages arising from the use of this software.\n"
"Permission is granted to anyone to use this software for any purpose, \n"
"including commercial applications, and to alter it and redistribute it freely, \n"
"subject to the following restrictions:\n"
"\n"
"1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.\n"
"2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.\n"
"3. This notice may not be removed or altered from any source distribution.\n"
"*/\n"
"//Originally written by Takahiro Harada\n"
"\n"
"#pragma OPENCL EXTENSION cl_amd_printf : enable\n"
"#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable\n"
"\n"
"typedef unsigned int u32;\n"
"#define GET_GROUP_IDX get_group_id(0)\n"
"#define GET_LOCAL_IDX get_local_id(0)\n"
"#define GET_GLOBAL_IDX get_global_id(0)\n"
"#define GET_GROUP_SIZE get_local_size(0)\n"
"#define GROUP_LDS_BARRIER barrier(CLK_LOCAL_MEM_FENCE)\n"
"#define GROUP_MEM_FENCE mem_fence(CLK_LOCAL_MEM_FENCE)\n"
"#define AtomInc(x) atom_inc(&(x))\n"
"#define AtomInc1(x, out) out = atom_inc(&(x))\n"
"\n"
"#define make_uint4 (uint4)\n"
"#define make_uint2 (uint2)\n"
"#define make_int2 (int2)\n"
"\n"
"typedef struct\n"
"{\n"
"	int m_n;\n"
"	int m_padding[3];\n"
"} ConstBuffer;\n"
"\n"
"\n"
"\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(64,1,1)))\n"
"void Copy1F4Kernel(__global float4* dst, __global float4* src, \n"
"					ConstBuffer cb)\n"
"{\n"
"	int gIdx = GET_GLOBAL_IDX;\n"
"\n"
"	if( gIdx < cb.m_n )\n"
"	{\n"
"		float4 a0 = src[gIdx];\n"
"\n"
"		dst[ gIdx ] = a0;\n"
"	}\n"
"}\n"
"\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(64,1,1)))\n"
"void Copy2F4Kernel(__global float4* dst, __global float4* src, \n"
"					ConstBuffer cb)\n"
"{\n"
"	int gIdx = GET_GLOBAL_IDX;\n"
"\n"
"	if( 2*gIdx <= cb.m_n )\n"
"	{\n"
"		float4 a0 = src[gIdx*2+0];\n"
"		float4 a1 = src[gIdx*2+1];\n"
"\n"
"		dst[ gIdx*2+0 ] = a0;\n"
"		dst[ gIdx*2+1 ] = a1;\n"
"	}\n"
"}\n"
"\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(64,1,1)))\n"
"void Copy4F4Kernel(__global float4* dst, __global float4* src, \n"
"					ConstBuffer cb)\n"
"{\n"
"	int gIdx = GET_GLOBAL_IDX;\n"
"\n"
"	if( 4*gIdx <= cb.m_n )\n"
"	{\n"
"		int idx0 = gIdx*4+0;\n"
"		int idx1 = gIdx*4+1;\n"
"		int idx2 = gIdx*4+2;\n"
"		int idx3 = gIdx*4+3;\n"
"\n"
"		float4 a0 = src[idx0];\n"
"		float4 a1 = src[idx1];\n"
"		float4 a2 = src[idx2];\n"
"		float4 a3 = src[idx3];\n"
"\n"
"		dst[ idx0 ] = a0;\n"
"		dst[ idx1 ] = a1;\n"
"		dst[ idx2 ] = a2;\n"
"		dst[ idx3 ] = a3;\n"
"	}\n"
"}\n"
"\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(64,1,1)))\n"
"void CopyF1Kernel(__global float* dstF1, __global float* srcF1, \n"
"					ConstBuffer cb)\n"
"{\n"
"	int gIdx = GET_GLOBAL_IDX;\n"
"\n"
"	if( gIdx < cb.m_n )\n"
"	{\n"
"		float a0 = srcF1[gIdx];\n"
"\n"
"		dstF1[ gIdx ] = a0;\n"
"	}\n"
"}\n"
"\n"
"__kernel\n"
"__attribute__((reqd_work_group_size(64,1,1)))\n"
"void CopyF2Kernel(__global float2* dstF2, __global float2* srcF2, \n"
"					ConstBuffer cb)\n"
"{\n"
"	int gIdx = GET_GLOBAL_IDX;\n"
"\n"
"	if( gIdx < cb.m_n )\n"
"	{\n"
"		float2 a0 = srcF2[gIdx];\n"
"\n"
"		dstF2[ gIdx ] = a0;\n"
"	}\n"
"}\n"
"\n"
"\n"
;
