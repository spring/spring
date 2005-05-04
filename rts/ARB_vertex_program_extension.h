//////////////////////////////////////////////////////////////////////////////////////////
//	ARB_vertex_program_extension.h
//	Extension setup header
//	You may use this code as you see fit, provided this header is kept intact.
//	Downloaded from: users.ox.ac.uk/~univ1234
//	Created:	2nd October 2002
//////////////////////////////////////////////////////////////////////////////////////////	

#ifndef ARB_VERTEX_PROGRAM_EXTENSION_H
#define ARB_VERTEX_PROGRAM_EXTENSION_H

bool SetUpARB_vertex_program();
extern bool ARB_vertex_program_supported;

//ARB_vertex_program is not in glext.h version 16
//So, define constants and function pointer types here
//enumerants
#ifndef GL_ARB_vertex_program

#define GL_VERTEX_PROGRAM_ARB					0x8620

#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB		0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB			0x8643
#define GL_COLOR_SUM_ARB						0x8458

#define GL_PROGRAM_FORMAT_ASCII_ARB				0x8875

#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB		0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB			0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB		0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB			0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB	0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB			0x8626

#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB		0x8645

#define GL_PROGRAM_LENGTH_ARB					0x8627
#define GL_PROGRAM_FORMAT_ARB					0x8876
#define GL_PROGRAM_BINDING_ARB					0x8677
#define GL_PROGRAM_INSTRUCTIONS_ARB				0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB			0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB		0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB	0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB				0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB			0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB		0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB	0x88A7
#define GL_PROGRAM_PARAMETERS_ARB				0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB			0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB		0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB	0x88AB
#define GL_PROGRAM_ATTRIBS_ARB					0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB				0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB			0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB		0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB		0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB	0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB	0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB		0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB		0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB		0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB		0x88B6

#define GL_PROGRAM_STRING_ARB					0x8628

#define GL_PROGRAM_ERROR_POSITION_ARB			0x864B
#define GL_CURRENT_MATRIX_ARB					0x8641
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB			0x88B7
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB		0x8640
#define GL_MAX_VERTEX_ATTRIBS_ARB				0x8869
#define GL_MAX_PROGRAM_MATRICES_ARB				0x862F
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB	0x862E


#define GL_PROGRAM_ERROR_STRING_ARB				0x8874

#define GL_MATRIX0_ARB							0x88C0
#define GL_MATRIX1_ARB							0x88C1
#define GL_MATRIX2_ARB							0x88C2
#define GL_MATRIX3_ARB							0x88C3
#define GL_MATRIX4_ARB							0x88C4
#define GL_MATRIX5_ARB							0x88C5
#define GL_MATRIX6_ARB							0x88C6
#define GL_MATRIX7_ARB							0x88C7
#define GL_MATRIX8_ARB							0x88C8
#define GL_MATRIX9_ARB							0x88C9
#define GL_MATRIX10_ARB							0x88CA
#define GL_MATRIX11_ARB							0x88CB
#define GL_MATRIX12_ARB							0x88CC
#define GL_MATRIX13_ARB							0x88CD
#define GL_MATRIX14_ARB							0x88CE
#define GL_MATRIX15_ARB							0x88CF
#define GL_MATRIX16_ARB							0x88D0
#define GL_MATRIX17_ARB							0x88D1
#define GL_MATRIX18_ARB							0x88D2
#define GL_MATRIX19_ARB							0x88D3
#define GL_MATRIX20_ARB							0x88D4
#define GL_MATRIX21_ARB							0x88D5
#define GL_MATRIX22_ARB							0x88D6
#define GL_MATRIX23_ARB							0x88D7
#define GL_MATRIX24_ARB							0x88D8
#define GL_MATRIX25_ARB							0x88D9
#define GL_MATRIX26_ARB							0x88DA
#define GL_MATRIX27_ARB							0x88DB
#define GL_MATRIX28_ARB							0x88DC
#define GL_MATRIX29_ARB							0x88DD
#define GL_MATRIX30_ARB							0x88DE
#define GL_MATRIX31_ARB							0x88DF

#endif	//GL_ARB_vertex_program	(enumerants)



//Function pointer types
#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
#ifdef GL_GLEXT_PROTOTYPES		//We do not define this, so ignore this section
GLAPI void APIENTRY glVertexAttrib1sARB (GLuint, GLshort);
//............
#endif /* GL_GLEXT_PROTOTYPES */

typedef void (APIENTRY * PFNGLVERTEXATTRIB1SARBPROC) (GLuint index, GLshort x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1FARBPROC) (GLuint index, GLfloat x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1DARBPROC) (GLuint index, GLdouble x);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2SARBPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2FARBPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2DARBPROC) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUBARBPROC) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);

typedef void (APIENTRY * PFNGLVERTEXATTRIB1SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB1DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB2DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB3DVARBPROC) (GLuint index, const GLdouble *v);

typedef void (APIENTRY * PFNGLVERTEXATTRIB4BVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4IVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4UBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4USVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4UIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NBVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NSVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NIVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUSVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIB4NUIVARBPROC) (GLuint index, const GLuint *v);

typedef void (APIENTRY * PFNGLVERTEXATTRIBPOINTERARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);

typedef void (APIENTRY * PFNGLENABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRY * PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);

typedef void (APIENTRY * PFNGLPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid * string);

typedef void (APIENTRY * PFNGLBINDPROGRAMARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRY * PFNGLDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint* program);

typedef void (APIENTRY * PFNGLGENPROGRAMSARBPROC) (GLsizei n, GLuint * programs);

typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble * params);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat * params);

typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble * params);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat * params);

typedef void (APIENTRY * PFNGLGETPROGRAMENVPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble * params);
typedef void (APIENTRY * PFNGLGETPROGRAMENVPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat * params);

typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble * params);
typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat * params);

typedef void (APIENTRY * PFNGLGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint * params);

typedef void (APIENTRY * PFNGLGETPROGRAMSTRINGARBPROC) (GLenum target, GLenum pname, GLvoid * string);

typedef void (APIENTRY * PFNGLGETVERTEXATTRIBDVARBPROC) (GLuint index, GLenum pname, GLdouble * params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBFVARBPROC) (GLuint index, GLenum pname, GLfloat * params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBIVARBPROC) (GLuint index, GLenum pname, GLint * params);

typedef void (APIENTRY * PFNGLGETVERTEXATTRIBPOINTERVARBPROC) (GLuint index, GLenum pname, GLvoid ** pointer);

typedef GLboolean (APIENTRY * PFNGLISPROGRAMARBPROC) (GLuint program);

#endif



//Function pointers
extern PFNGLVERTEXATTRIB1SARBPROC					glVertexAttrib1sARB;
extern PFNGLVERTEXATTRIB1FARBPROC					glVertexAttrib1fARB;
extern PFNGLVERTEXATTRIB1DARBPROC					glVertexAttrib1dARB;
extern PFNGLVERTEXATTRIB2SARBPROC					glVertexAttrib2sARB;
extern PFNGLVERTEXATTRIB2FARBPROC					glVertexAttrib2fARB;
extern PFNGLVERTEXATTRIB2DARBPROC					glVertexAttrib2dARB;
extern PFNGLVERTEXATTRIB3SARBPROC					glVertexAttrib3sARB;
extern PFNGLVERTEXATTRIB3FARBPROC					glVertexAttrib3fARB;
extern PFNGLVERTEXATTRIB3DARBPROC					glVertexAttrib3dARB;
extern PFNGLVERTEXATTRIB4SARBPROC					glVertexAttrib4sARB;
extern PFNGLVERTEXATTRIB4FARBPROC					glVertexAttrib4fARB;
extern PFNGLVERTEXATTRIB4DARBPROC					glVertexAttrib4dARB;
extern PFNGLVERTEXATTRIB4NUBARBPROC					glVertexAttrib4NubARB;

extern PFNGLVERTEXATTRIB1SVARBPROC					glVertexAttrib1svARB;
extern PFNGLVERTEXATTRIB1FVARBPROC					glVertexAttrib1fvARB;
extern PFNGLVERTEXATTRIB1DVARBPROC					glVertexAttrib1dvARB;
extern PFNGLVERTEXATTRIB2SVARBPROC					glVertexAttrib2svARB;
extern PFNGLVERTEXATTRIB2FVARBPROC					glVertexAttrib2fvARB;
extern PFNGLVERTEXATTRIB2DVARBPROC					glVertexAttrib2dvARB;
extern PFNGLVERTEXATTRIB3SVARBPROC					glVertexAttrib3svARB;
extern PFNGLVERTEXATTRIB3FVARBPROC					glVertexAttrib3fvARB;
extern PFNGLVERTEXATTRIB3DVARBPROC					glVertexAttrib3dvARB;

extern PFNGLVERTEXATTRIB4BVARBPROC					glVertexAttrib4bvARB;
extern PFNGLVERTEXATTRIB4SVARBPROC					glVertexAttrib4svARB;
extern PFNGLVERTEXATTRIB4IVARBPROC					glVertexAttrib4ivARB;
extern PFNGLVERTEXATTRIB4UBVARBPROC					glVertexAttrib4ubvARB;
extern PFNGLVERTEXATTRIB4USVARBPROC					glVertexAttrib4usvARB;
extern PFNGLVERTEXATTRIB4UIVARBPROC					glVertexAttrib4uivARB;
extern PFNGLVERTEXATTRIB4FVARBPROC					glVertexAttrib4fvARB;
extern PFNGLVERTEXATTRIB4DVARBPROC					glVertexAttrib4dvARB;
extern PFNGLVERTEXATTRIB4NBVARBPROC					glVertexAttrib4NbvARB;
extern PFNGLVERTEXATTRIB4NSVARBPROC					glVertexAttrib4NsvARB;
extern PFNGLVERTEXATTRIB4NIVARBPROC					glVertexAttrib4NivARB;
extern PFNGLVERTEXATTRIB4NUBVARBPROC				glVertexAttrib4NubvARB;
extern PFNGLVERTEXATTRIB4NUSVARBPROC				glVertexAttrib4NusvARB;
extern PFNGLVERTEXATTRIB4NUIVARBPROC				glVertexAttrib4NuivARB;

extern PFNGLVERTEXATTRIBPOINTERARBPROC				glVertexAttribPointerARB;

extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC			glEnableVertexAttribArrayARB;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC			glDisableVertexAttribArrayARB;

extern PFNGLPROGRAMSTRINGARBPROC					glProgramStringARB;

extern PFNGLBINDPROGRAMARBPROC						glBindProgramARB;
extern PFNGLDELETEPROGRAMSARBPROC					glDeleteProgramsARB;
extern PFNGLGENPROGRAMSARBPROC						glGenProgramsARB;

extern PFNGLPROGRAMENVPARAMETER4DARBPROC			glProgramEnvParameter4dARB;
extern PFNGLPROGRAMENVPARAMETER4DVARBPROC			glProgramEnvParameter4dvARB;
extern PFNGLPROGRAMENVPARAMETER4FARBPROC			glProgramEnvParameter4fARB;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC			glProgramEnvParameter4fvARB;

extern PFNGLPROGRAMLOCALPARAMETER4DARBPROC			glProgramLocalParameter4dARB;
extern PFNGLPROGRAMLOCALPARAMETER4DVARBPROC			glProgramLocalParameter4dvARB;
extern PFNGLPROGRAMLOCALPARAMETER4FARBPROC			glProgramLocalParameter4fARB;
extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC			glProgramLocalParameter4fvARB;

extern PFNGLGETPROGRAMENVPARAMETERDVARBPROC			glGetProgramEnvParameterdvARB;
extern PFNGLGETPROGRAMENVPARAMETERFVARBPROC			glGetProgramEnvParameterfvARB;

extern PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC		glGetProgramLocalParameterdvARB;
extern PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC		glGetProgramLocalParameterfvARB;

extern PFNGLGETPROGRAMIVARBPROC						glGetProgramivARB;

extern PFNGLGETPROGRAMSTRINGARBPROC					glGetProgramStringARB;

extern PFNGLGETVERTEXATTRIBDVARBPROC				glGetVertexAttribdvARB;
extern PFNGLGETVERTEXATTRIBFVARBPROC				glGetVertexAttribfvARB;
extern PFNGLGETVERTEXATTRIBIVARBPROC				glGetVertexAttribivARB;

extern PFNGLGETVERTEXATTRIBPOINTERVARBPROC			glGetVertexAttribPointervARB;

extern PFNGLISPROGRAMARBPROC						glIsProgramARB;

#endif	//ARB_VERTEX_PROGRAM_EXTENSION_H