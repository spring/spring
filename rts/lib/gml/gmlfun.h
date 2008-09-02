// GML - OpenGL Multithreading Library
// for Spring http://spring.clan-sy.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used freely for any purpose, as
// long as this notice remains unchanged

#ifndef GMLFUN_H
#define GMLFUN_H

#include <set>
#include <map>

#define GML_ENABLE_DEBUG 0

#if GML_ENABLE_DEBUG
#define GML_DEBUG(str,val,type)\
if(type==GML_ENABLE_DEBUG) {\
	FILE *f=fopen("C:\\GMLDBG.TXT","a");\
	if(f) {\
		fprintf(f,"%s line %d: %s %d\n",__FILE__,__LINE__,str,val);\
		fclose(f);\
	}\
}
#else
#define GML_DEBUG(str,val,type)
#endif

extern std::map<GLenum,GLint> gmlGetIntegervCache;
extern std::map<GLenum,GLfloat> gmlGetFloatvCache;
extern std::map<GLenum,std::string> gmlGetStringCache;
#define GML_CACHE(d,v,c,K,R) if(GML_USE_CACHE) {std::map<d,v>::iterator it=c.find(K); if(it!=c.end()) {*R=(*it).second; return;}}
#define GML_CACHE_RET_STR(d,v,c,K) if(GML_USE_CACHE) {std::map<d,v>::iterator it=c.find(K); if(it!=c.end()) {return (GLubyte *)(*it).second.c_str();}}
#define GML_DEFAULT(c,r) if(GML_USE_DEFAULT && (c)) {r; return;}
#define GML_DEFAULT_RET(c,r) if(GML_USE_DEFAULT && (c)) {return r;}
#define GML_DEFAULT_ERROR() if(GML_USE_NO_ERROR) return GL_NO_ERROR;

//teximage, build2dmip
EXTERN inline int gmlNumArgsTexImage(int datatype) {
	switch(datatype) {
		case GL_COLOR_INDEX:
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_LUMINANCE:
		case GL_DEPTH_COMPONENT:
			return 1;
		case GL_LUMINANCE_ALPHA:
			return 2;
		case GL_RGB:
		case GL_BGR_EXT:
			return 3;
		case GL_RGBA:
		case GL_BGRA_EXT:
			return 4;
		default:
			GML_DEBUG("gmlNumArgsTexImage", datatype, 1)
			return 0;
	}
}

// glLight, glMaterial
EXTERN inline int gmlNumArgsLightMat(int datatype) {
	switch(datatype) {
		case GL_AMBIENT:
		case GL_DIFFUSE:
		case GL_SPECULAR:
		case GL_EMISSION: // material
		case GL_AMBIENT_AND_DIFFUSE: //mat
		case GL_POSITION:
			return 4;
		case GL_SPOT_DIRECTION:
		case GL_COLOR_INDEXES: //mat
			return 3;
		case GL_SHININESS: // mat
		case GL_SPOT_EXPONENT:
		case GL_SPOT_CUTOFF:
		case GL_CONSTANT_ATTENUATION:
		case GL_LINEAR_ATTENUATION:
		case GL_QUADRATIC_ATTENUATION:
			return 1;
		default:
			GML_DEBUG("gmlNumArgsLightMat", datatype, 1)
			return 0;
	}
}

//glFog
EXTERN inline int gmlNumArgsFog(int datatype) {
	switch(datatype) {
		case GL_FOG_MODE:
		case GL_FOG_DENSITY:
		case GL_FOG_START:
		case GL_FOG_END:
		case GL_FOG_INDEX:
			return 1;
		case GL_FOG_COLOR:
			return 4;
		default:
			GML_DEBUG("gmlNumArgsFog", datatype, 1)
			return 0;
	}
}

//glTexGen
EXTERN inline int gmlNumArgsTexGen(int datatype) {
	switch(datatype) {
		case GL_TEXTURE_GEN_MODE:
			return 1;
		case GL_OBJECT_PLANE:
		case GL_EYE_PLANE:
			return 4;
		default:
			GML_DEBUG("gmlNumArgsTexGen", datatype, 1)
			return 0;
	}
}

//glTexEnv
EXTERN inline int gmlNumArgsTexEnv(int datatype) {
	switch(datatype) {
		case GL_TEXTURE_ENV_MODE:
			return 1;
		case GL_TEXTURE_ENV_COLOR:
			return 4;
		default:
			GML_DEBUG("gmlNumArgsTexEnv", datatype, 1)
			return 0;
	}
}

//glPointParametefv
EXTERN inline int gmlNumArgsPointParam(int datatype) {
	switch(datatype) {
		case GL_POINT_SIZE_MIN:
		case GL_POINT_SIZE_MAX:
		case GL_POINT_FADE_THRESHOLD_SIZE:
		case GL_POINT_SPRITE_COORD_ORIGIN:
			return 1;
		case GL_POINT_DISTANCE_ATTENUATION:
			return 3;
		default:
			GML_DEBUG("gmlNumArgsPointParam", datatype, 1)
			return 0;
	}
}

//glTexParameterfv
EXTERN inline int gmlNumArgsTexParam(int datatype) {
	switch(datatype) {
		case GL_TEXTURE_MIN_FILTER:
		case GL_TEXTURE_MAG_FILTER:
		case GL_TEXTURE_WRAP_S:
		case GL_TEXTURE_WRAP_T:
		case GL_TEXTURE_PRIORITY:
			return 1;
		case GL_TEXTURE_BORDER_COLOR:
			return 4;
		default:
			GML_DEBUG("gmlNumArgsTexParam", datatype, 1)
			return 0;
	}
}

//glLightModelfv
EXTERN inline int gmlNumArgsLightModel(int datatype) {
	switch(datatype) {
		case GL_LIGHT_MODEL_LOCAL_VIEWER:
		case GL_LIGHT_MODEL_TWO_SIDE:
			return 1;
		case GL_LIGHT_MODEL_AMBIENT:
			return 4;
		default:
			GML_DEBUG("gmlNumArgsLightModel", datatype, 1)
			return 0;
	}
}

//glMap1f
EXTERN inline int gmlNumArgsMap1(int datatype) {
	switch(datatype) {
		case GL_MAP1_INDEX:
		case GL_MAP1_TEXTURE_COORD_1:
			return 1;
		case GL_MAP1_TEXTURE_COORD_2:
			return 2;
		case GL_MAP1_VERTEX_3:
		case GL_MAP1_NORMAL:
		case GL_MAP1_TEXTURE_COORD_3:
			return 3;
		case GL_MAP1_VERTEX_4:
		case GL_MAP1_COLOR_4:
		case GL_MAP1_TEXTURE_COORD_4:
			return 4;
		default:
			GML_DEBUG("gmlNumArgsMap1", datatype, 1)
			return 0;
	}
}

//glMap2f
EXTERN inline int gmlNumArgsMap2(int datatype) {
	switch(datatype) {
		case GL_MAP2_INDEX:
		case GL_MAP2_TEXTURE_COORD_1:
			return 1;
		case GL_MAP2_TEXTURE_COORD_2:
			return 2;
		case GL_MAP2_VERTEX_3:
		case GL_MAP2_NORMAL:
		case GL_MAP2_TEXTURE_COORD_3:
			return 3;
		case GL_MAP2_VERTEX_4:
		case GL_MAP2_COLOR_4:
		case GL_MAP2_TEXTURE_COORD_4:
			return 4;
		default:
			GML_DEBUG("gmlNumArgsMap2", datatype, 1)
			return 0;
	}
}

EXTERN inline int gmlSizeOf(int datatype) {
	switch(datatype) {
		case GL_UNSIGNED_BYTE:
			return sizeof(GLubyte);
		case GL_BYTE:
			return sizeof(GLbyte);
		case GL_BITMAP:
			return sizeof(GLubyte);
		case GL_UNSIGNED_SHORT:
			return sizeof(GLushort);
		case GL_SHORT:
			return sizeof(GLshort);
		case GL_UNSIGNED_INT:
			return sizeof(GLuint);
		case GL_INT:
			return sizeof(GLint);
		case GL_FLOAT:
			return sizeof(GLfloat);
		case GL_DOUBLE:
			return sizeof(GLdouble);
		default:
			GML_DEBUG("gmlSizeOf", datatype, 1)
			return 0;
	}
}


#define GML_MAKEVAR() int type;
#define GML_MAKEVAR_A(ftype1) GML_MAKEVAR() ftype1 A;
#define GML_MAKEVAR_B(ftype1,ftype2) GML_MAKEVAR_A(ftype1) ftype2 B;
#define GML_MAKEVAR_C(ftype1,ftype2,ftype3) GML_MAKEVAR_B(ftype1,ftype2) ftype3 C;
#define GML_MAKEVAR_D(ftype1,ftype2,ftype3,ftype4) GML_MAKEVAR_C(ftype1,ftype2,ftype3) ftype4 D;
#define GML_MAKEVAR_E(ftype1,ftype2,ftype3,ftype4,ftype5) GML_MAKEVAR_D(ftype1,ftype2,ftype3,ftype4) ftype5 E;
#define GML_MAKEVAR_F(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6) GML_MAKEVAR_E(ftype1,ftype2,ftype3,ftype4,ftype5) ftype6 F;
#define GML_MAKEVAR_G(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7) GML_MAKEVAR_F(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6) ftype7 G;
#define GML_MAKEVAR_H(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8) GML_MAKEVAR_G(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7) ftype8 H;
#define GML_MAKEVAR_I(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9) GML_MAKEVAR_H(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8) ftype9 I;
#define GML_MAKEVAR_J(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,ftype10) GML_MAKEVAR_I(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9) ftype10 J;
#define GML_MAKEVAR_SIZE() int size;
#define GML_MAKEVAR_RET(ft) volatile ft ret;

#define GML_MAKEASS_A() p->A=A;
#define GML_MAKEASS_B() GML_MAKEASS_A() p->B=B;
#define GML_MAKEASS_C() GML_MAKEASS_B() p->C=C;
#define GML_MAKEASS_D() GML_MAKEASS_C() p->D=D;
#define GML_MAKEASS_E() GML_MAKEASS_D() p->E=E;
#define GML_MAKEASS_F() GML_MAKEASS_E() p->F=F;
#define GML_MAKEASS_G() GML_MAKEASS_F() p->G=G;
#define GML_MAKEASS_H() GML_MAKEASS_G() p->H=H;
#define GML_MAKEASS_I() GML_MAKEASS_H() p->I=I;
#define GML_MAKEASS_J() GML_MAKEASS_I() p->J=J;

#define GML_RETVAL(ft) return (ft)*(volatile ft *)&(p->ret);

#define GML_RELOC()\
	while(qd->WritePos+datasize>=qd->WriteSize)\
		qd->WaitRealloc();

#define GML_PREP_FIXED(name)\
	gmlQueue *qd=&gmlQueues[gmlThreadNumber];\
	int datasize=sizeof(gml##name##Data);\
	GML_RELOC()\
	gml##name##Data *p=(gml##name##Data *)qd->WritePos;\
	p->type=gml##name##Enum;

#define GML_UPD_POS()\
	qd->WritePos+=datasize;

#define GML_UPD_SIZE()\
	p->size=datasize;\

#define GML_PREP_VAR(name,sizefun)\
	gmlQueue *qd=&gmlQueues[gmlThreadNumber];\
	int size=sizefun;\
	int datasize=sizeof(gml##name##Data)+size;\
	GML_RELOC()\
	gml##name##Data *p=(gml##name##Data *)qd->WritePos;\
	p->type=gml##name##Enum;

#define GML_PREP_VAR_SIZE(name,sizefun)\
	GML_PREP_VAR(name,sizefun)\
	GML_UPD_SIZE()

#define GML_COND(stmt)\
	GML_IF_SERVER_THREAD() {\
		stmt;\
		return;\
	}

#define GML_COND_RET(stmt)\
	GML_IF_SERVER_THREAD() {\
		return stmt;\
	}

EXTERN inline void gmlSync(gmlQueue *qd) {
	qd->SyncRequest();
}

#define GML_SYNC_COND(arg,x) arg;

#define GML_SYNC() gmlSync(qd)

#define GML_FUN(name,ftype) EXTERN const int gml##name##Enum=__LINE__-__FIRSTLINE__;\
	EXTERN inline ftype gml##name

#define GML_RETFUN(name,ftr) EXTERN inline ftr gml##name

#define GML_MAKEFUN0(name) struct gml##name##Data {\
	GML_MAKEVAR()\
};\
GML_FUN(name,void)() {\
	GML_COND(gl##name())\
	GML_PREP_FIXED(name)\
	GML_UPD_POS()\
}

#define GML_MAKEFUN0R(name,ftypeR,cache) struct gml##name##Data {\
	GML_MAKEVAR()\
	GML_MAKEVAR_RET(ftypeR)\
};\
GML_FUN(name,ftypeR)() {\
	GML_COND_RET(gl##name())\
	cache\
	GML_PREP_FIXED(name)\
	GML_UPD_POS()\
	GML_SYNC();\
	GML_RETVAL(ftypeR)\
}

#define GML_MAKEFUN1(name,ftype1) struct gml##name##Data {\
	GML_MAKEVAR_A(ftype1)\
};\
GML_FUN(name,void)(ftype1 A) {\
	GML_COND(gl##name(A))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_A()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN1R(name,ftype1,ftypeR,cache) struct gml##name##Data {\
	GML_MAKEVAR_A(ftype1)\
	GML_MAKEVAR_RET(ftypeR)\
};\
GML_FUN(name,ftypeR)(ftype1 A) {\
	GML_COND_RET(gl##name(A))\
	cache\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_A()\
	GML_UPD_POS()\
	GML_SYNC();\
	GML_RETVAL(ftypeR)\
}

#define GML_MAKEFUN2(name,ftype1,ftype2,cache,...) struct gml##name##Data {\
	GML_MAKEVAR_B(ftype1,ftype2)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B) {\
	GML_COND(gl##name(A,B))\
	cache\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_B()\
	GML_UPD_POS()\
	GML_SYNC_COND(__VA_ARGS__,)\
}

#define GML_MAKEFUN2B(name,ftype1,ftype2) struct gml##name##Data {\
	GML_MAKEVAR_B(ftype1,ftype2)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B) {\
	GML_COND(gl##name(A,B))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_B()\
	switch(A) {\
		case GL_ARRAY_BUFFER:\
			qd->ArrayBuffer=B; break;\
		case GL_ELEMENT_ARRAY_BUFFER:\
			qd->ElementArrayBuffer=B; break;\
		case GL_PIXEL_PACK_BUFFER:\
			qd->PixelPackBuffer=B; break;\
		case GL_PIXEL_UNPACK_BUFFER:\
			qd->PixelUnpackBuffer=B; break;\
	}\
	GML_UPD_POS()\
}


#define GML_MAKEFUN2R(name,ftype1,ftype2,ftypeR) struct gml##name##Data {\
	GML_MAKEVAR_B(ftype1,ftype2)\
	GML_MAKEVAR_RET(ftypeR)\
};\
GML_FUN(name,ftypeR)(ftype1 A,ftype2 B) {\
	GML_COND_RET(gl##name(A,B))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_B()\
	GML_UPD_POS()\
	GML_SYNC();\
	GML_RETVAL(ftypeR)\
}

#define GML_MAKEFUN3(name,ftype1,ftype2,ftype3,cache,...) struct gml##name##Data {\
	GML_MAKEVAR_C(ftype1,ftype2,ftype3)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C) {\
	GML_COND(gl##name(A,B,C))\
	cache\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_C()\
	GML_UPD_POS()\
	GML_SYNC_COND(__VA_ARGS__,)\
}

#define GML_MAKEFUN4(name,ftype1,ftype2,ftype3,ftype4,...) struct gml##name##Data {\
	GML_MAKEVAR_D(ftype1,ftype2,ftype3,ftype4)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D) {\
	GML_COND(gl##name(A,B,C,D))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_D()\
	GML_UPD_POS()\
	GML_SYNC_COND(__VA_ARGS__,)\
}

#define GML_MAKEFUN5(name,ftype1,ftype2,ftype3,ftype4,ftype5) struct gml##name##Data {\
	GML_MAKEVAR_E(ftype1,ftype2,ftype3,ftype4,ftype5)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E) {\
	GML_COND(gl##name(A,B,C,D,E))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_E()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN6(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6) struct gml##name##Data {\
	GML_MAKEVAR_F(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F) {\
	GML_COND(gl##name(A,B,C,D,E,F))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_F()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN7(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,...) struct gml##name##Data {\
	GML_MAKEVAR_G(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G) {\
	GML_COND(gl##name(A,B,C,D,E,F,G))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_G()\
	GML_UPD_POS()\
	GML_SYNC_COND(__VA_ARGS__,)\
}

#define GML_MAKEFUN8(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8) struct gml##name##Data {\
	GML_MAKEVAR_H(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 H) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_H()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN9(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9) struct gml##name##Data {\
	GML_MAKEVAR_I(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 H, ftype9 I) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H,I))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_I()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN9R(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,ftypeR) struct gml##name##Data {\
	GML_MAKEVAR_I(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9)\
	GML_MAKEVAR_RET(ftypeR)\
};\
GML_FUN(name,ftypeR)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 H, ftype9 I) {\
	GML_COND_RET(gl##name(A,B,C,D,E,F,G,H,I))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_I()\
	GML_UPD_POS()\
	GML_SYNC();\
	GML_RETVAL(ftypeR)\
}


#define GML_MAKEFUN10(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,ftype10) struct gml##name##Data {\
	GML_MAKEVAR_J(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,ftype10)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 H, ftype9 I, ftype10 J) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H,I,J))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_J()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN7S(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,sizefun) struct gml##name##Data {\
	GML_MAKEVAR_F(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6)\
	GML_MAKEVAR_SIZE()\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 *G) {\
	GML_COND(gl##name(A,B,C,D,E,F,G))\
	GML_PREP_VAR_SIZE(name,sizefun)\
	GML_MAKEASS_F()\
	memcpy(p+1,G,size);\
	GML_UPD_POS()\
}

#define GML_PUB_COPY(name,var,ftype)\
	p->var=NULL;\
	if(qd->PixelUnpackBuffer) {\
		datasize=sizeof(gml##name##Data);\
		p->var=(ftype)((BYTE *)var+1);\
	}\
	else if(var!=NULL)\
		memcpy(p+1,var,size);

#define GML_MAKEFUN8S(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,sizefun) struct gml##name##Data {\
	GML_MAKEVAR_H(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8 *)\
	GML_MAKEVAR_SIZE()\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 *H) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H))\
	GML_PREP_VAR(name,sizefun)\
	GML_MAKEASS_G()\
	GML_PUB_COPY(name,H,ftype8 *)\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN9S(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,sizefun) struct gml##name##Data {\
	GML_MAKEVAR_I(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9 *)\
	GML_MAKEVAR_SIZE()\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 H, ftype9 *I) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H,I))\
	GML_PREP_VAR(name,sizefun)\
	GML_MAKEASS_H()\
	GML_PUB_COPY(name,I,ftype9 *)\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN10S(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,ftype10,sizefun) struct gml##name##Data {\
	GML_MAKEVAR_J(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,ftype10 *)\
	GML_MAKEVAR_SIZE()\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 H, ftype9 I, ftype10 *J) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H,I,J))\
	GML_PREP_VAR(name,sizefun)\
	GML_MAKEASS_I()\
	GML_PUB_COPY(name,J,ftype10 *)\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN1V(name,ftype1,ftypeX,count) struct gml##name##Data {\
	GML_MAKEVAR()\
	GML_MAKEVAR_SIZE()\
	ftypeX A;\
};\
GML_FUN(name,void)(ftype1* A) {\
	GML_COND(gl##name(A))\
	GML_PREP_VAR_SIZE(name,(count-1)*sizeof(ftypeX))\
	memcpy(&(p->A),A,size+sizeof(ftypeX));\
	GML_UPD_POS()\
}

#define GML_MAKEFUN2V(name,ftype1,ftype2,ftypeX,count) struct gml##name##Data {\
	GML_MAKEVAR_A(ftype1)\
	GML_MAKEVAR_SIZE()\
	ftypeX B;\
};\
GML_FUN(name,void)(ftype1 A, ftype2* B) {\
	GML_COND(gl##name(A,B))\
	GML_PREP_VAR_SIZE(name,(count-1)*sizeof(ftypeX))\
	GML_MAKEASS_A()\
	memcpy(&(p->B),B,size+sizeof(ftypeX));\
	GML_UPD_POS()\
}

#define GML_MAKEFUN3V(name,ftype1,ftype2,ftype3,ftypeX,count) struct gml##name##Data {\
	GML_MAKEVAR_B(ftype1,ftype2)\
	GML_MAKEVAR_SIZE()\
	ftypeX C;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3* C) {\
	GML_COND(gl##name(A,B,C))\
	GML_PREP_VAR_SIZE(name,(count-1)*sizeof(ftypeX))\
	GML_MAKEASS_B()\
	memcpy(&(p->C),C,size+sizeof(ftypeX));\
	GML_UPD_POS()\
}

#define GML_MAKEFUN4V(name,ftype1,ftype2,ftype3,ftype4,ftypeX,count) struct gml##name##Data {\
	GML_MAKEVAR_C(ftype1,ftype2,ftype3)\
	GML_MAKEVAR_SIZE()\
	ftypeX D;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 *D) {\
	GML_COND(gl##name(A,B,C,D))\
	GML_PREP_VAR_SIZE(name,(count-1)*sizeof(ftypeX))\
	GML_MAKEASS_C()\
	memcpy(&(p->D),D,size+sizeof(ftypeX));\
	GML_UPD_POS()\
}

#define GML_MAKEFUN4VS(name,ftype1,ftype2,ftype3,ftype4,ftypeX,count) struct gml##name##Data {\
	GML_MAKEVAR_B(ftype1,ftype2)\
	ftype4 D;\
	GML_MAKEVAR_SIZE()\
	ftypeX C;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 *C, ftype4 D) {\
	GML_COND(gl##name(A,B,C,D))\
	GML_PREP_VAR_SIZE(name,(count-1)*sizeof(ftypeX))\
	GML_MAKEASS_B()\
	p->D=D;\
	if(C!=NULL)\
		memcpy(&(p->C),C,size+sizeof(ftypeX));\
	GML_UPD_POS()\
}

#define GML_MAKEFUN4VSS(name,ftype1,ftype2,ftype3,ftype4,count) struct gml##name##Data {\
	GML_MAKEVAR_B(ftype1,ftype2)\
	int lensize;\
	GML_MAKEVAR_SIZE()\
	ftype3 *C;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 **C, ftype4 *D) {\
	GML_COND(gl##name(A,B,C,D))\
	GML_PREP_VAR(name,(count-1)*sizeof(ftype3 *))\
	GML_MAKEASS_B()\
	p->lensize=datasize;\
	BYTE *e=(BYTE *)p+datasize;\
	for(int i=0; i<B; ++i) {\
		BOOL_ len=!D || D[i]<0;\
		GLint sl=(len?strlen(C[i]):D[i])+1;\
		datasize+=sl;\
		((intptr_t *)&(p->C))[i]=sl;\
			--sl;\
		while(qd->WritePos+datasize>=qd->WriteSize)\
			p=(gml##name##Data *)qd->WaitRealloc(&e);\
		memcpy(e,C[i],sl);\
		e+=sl;\
		*e='\0';\
		++e;\
	}\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}

#define GML_PUB_PCOPY(name,var,pvar,ftype)\
	p->pvar=NULL;\
	if(qd->PixelUnpackBuffer) {\
		datasize=sizeof(gml##name##Data);\
		p->pvar=(ftype *)var+1;\
	}\
	else if(var!=NULL)\
		memcpy(&(p->var),var,size+sizeof(ftype));

#define GML_STDCOPY1(type,count,stride,nargs)\
	for(int i=0; i<count; ++i) {\
		type *v2=v;\
		for(int j=0; j<nargs; ++j) {\
			*e=*v2;\
			++e;\
			++v2;\
		}\
		v+=stride;\
	}

#define GML_MAKEFUN6VST(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftypeX,count,stride,numargs) struct gml##name##Data {\
	GML_MAKEVAR_E(ftype1,ftype2,ftype3,ftype4,ftype5)\
	GML_MAKEVAR_SIZE()\
	ftypeX F;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 *F) {\
	GML_COND(gl##name(A,B,C,D,E,F))\
	int nargs=numargs;\
	GML_PREP_VAR_SIZE(name,(nargs*count-1)*sizeof(ftypeX))\
	GML_MAKEASS_E()\
	p->stride=nargs;\
	ftypeX *e=&(p->F);\
	ftype6 *v=F;\
	GML_STDCOPY1(ftype6,count,stride,nargs)\
	GML_UPD_POS()\
}

#define GML_STDCOPY2(type,count1,stride1,count2,stride2,nargs)\
	for(int i=0; i<count1; ++i) {\
		type *v2=v;\
		for(int j=0; j<count2; ++j) {\
			type *v3=v2;\
			for(int k=0; k<nargs; ++k) {\
				*e=*v3;\
				++e;\
				++v3;\
			}\
			v2+=stride2;\
		}\
		v+=stride1;\
	}

#define GML_MAKEFUN10VST(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,ftype10,ftypeX,count1,stride1,count2,stride2,numargs) struct gml##name##Data {\
	GML_MAKEVAR_I(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9)\
	GML_MAKEVAR_SIZE()\
	ftypeX J;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 H, ftype9 I, ftype10 *J) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H,I,J))\
	int nargs=numargs;\
	GML_PREP_VAR_SIZE(name,(nargs*count1*count2-1)*sizeof(ftypeX))\
	GML_MAKEASS_I()\
	p->stride1=nargs*count2;\
	p->stride2=nargs;\
	ftypeX *e=&(p->J);\
	ftype10 *v=J;\
	GML_STDCOPY2(ftype10,count1,stride1,count2,stride2,nargs)\
	GML_UPD_POS()\
}

#define GML_MAKEFUN7VP(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftypeX,count) struct gml##name##Data {\
	GML_MAKEVAR_F(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6)\
	GML_MAKEVAR_SIZE()\
	ftypeX *GP;\
	ftypeX G;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 *G) {\
	GML_COND(gl##name(A,B,C,D,E,F,G))\
	GML_PREP_VAR(name,(count-1)*sizeof(ftypeX))\
	GML_MAKEASS_F()\
	GML_PUB_PCOPY(name,G,GP,ftypeX)\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN8VP(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftypeX,count) struct gml##name##Data {\
	GML_MAKEVAR_G(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7)\
	GML_MAKEVAR_SIZE()\
	ftypeX *HP;\
	ftypeX H;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 *H) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H))\
	GML_PREP_VAR(name,(count-1)*sizeof(ftypeX))\
	GML_MAKEASS_G()\
	GML_PUB_PCOPY(name,H,HP,ftypeX)\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN9VP(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8,ftype9,ftypeX,count) struct gml##name##Data {\
	GML_MAKEVAR_H(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,ftype7,ftype8)\
	GML_MAKEVAR_SIZE()\
	ftypeX *IP;\
	ftypeX I;\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 F, ftype7 G, ftype8 H, ftype9 *I) {\
	GML_COND(gl##name(A,B,C,D,E,F,G,H,I))\
	GML_PREP_VAR(name,(count-1)*sizeof(ftypeX))\
	GML_MAKEASS_H()\
	GML_PUB_PCOPY(name,I,IP,ftypeX)\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}

#define GML_MAKEFUN1CS(name,ftype1,arg) struct gml##name##Data {\
	GML_MAKEVAR_A(ftype1)\
};\
GML_FUN(name,void)(ftype1 A) {\
	GML_COND(gl##name(A))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_A()\
	qd->ClientState arg (1<<(A-GL_VERTEX_ARRAY));\
	GML_UPD_POS()\
}

#define GML_MAKEFUN1VA(name,ftype1,arg,fun) struct gml##name##Data {\
	GML_MAKEVAR_A(ftype1)\
};\
GML_FUN(name,void)(ftype1 A) {\
	GML_COND(gl##name(A))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_A()\
	qd->arg##set.fun(A);\
	GML_UPD_POS()\
}

#define GML_UPD_CS(arg)\
	if(qd->ArrayBuffer)\
		qd->ClientState |= GML_##arg##_ARRAY_BUFFER;\
	else\
		qd->ClientState &= ~GML_##arg##_ARRAY_BUFFER;

#define GML_MAKEFUN2P(name,ftype1,ftype2,arg) struct gml##name##Data {\
	GML_MAKEVAR_B(ftype1,ftype2 *)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 *B) {\
	GML_COND(gl##name(A,B))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_B()\
	qd->arg##stride=A;\
	qd->arg##pointer=B;\
	GML_UPD_CS(arg)\
	GML_UPD_POS()\
}

#define GML_MAKEFUN3P(name,ftype1,ftype2,ftype3,arg) struct gml##name##Data {\
	GML_MAKEVAR_C(ftype1,ftype2,ftype3 *)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 *C) {\
	GML_COND(gl##name(A,B,C))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_C()\
	qd->arg##type=A;\
	qd->arg##stride=B;\
	qd->arg##pointer=C;\
	GML_UPD_CS(arg)\
	GML_UPD_POS()\
}

#define GML_MAKEFUN4P(name,ftype1,ftype2,ftype3,ftype4,arg) struct gml##name##Data {\
	GML_MAKEVAR_D(ftype1,ftype2,ftype3,ftype4 *)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 *D) {\
	GML_COND(gl##name(A,B,C,D))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_D()\
	qd->arg##size=A;\
	qd->arg##type=B;\
	qd->arg##stride=C;\
	qd->arg##pointer=D;\
	GML_UPD_CS(arg)\
	GML_UPD_POS()\
}

#define GML_MAKEFUN6P(name,ftype1,ftype2,ftype3,ftype4,ftype5,ftype6,arg) struct gml##name##Data {\
	GML_MAKEVAR_F(ftype1,ftype2,ftype3,ftype4,ftype5,ftype6 *)\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 D, ftype5 E, ftype6 *F) {\
	GML_COND(gl##name(A,B,C,D,E,F))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_F()\
	qd->arg##map[A]=arg##data(B,C,D,E,F,qd->ArrayBuffer);\
	GML_UPD_POS()\
}

#define GML_MEMCOPY()\
	for(int i=0; i<C; ++i) {\
		BYTE *v2=v;\
		for(int j=0; j<itemsize; ++j) {\
			*e=*v2;\
			++e;\
			++v2;\
		}\
		v+=itemstride;\
	}

#define GML_IDXLOOP(ltype)\
	for(int i=0; i<B; ++i) {\
		BYTE *v2=v+(*(ltype *)dt)*itemstride;\
		dt+=sizeof(ltype);\
		for(int j=0; j<itemsize; ++j) {\
			*e=*v2;\
			++e;\
			++v2;\
		}\
	}

#define GML_IDXCOPY()\
	BYTE *dt=(BYTE *)D;\
	switch(C) {\
		case GL_UNSIGNED_INT:\
			GML_IDXLOOP(GLuint)\
			break;\
		case GL_UNSIGNED_SHORT:\
			GML_IDXLOOP(GLushort)\
			break;\
		case GL_UNSIGNED_BYTE:\
			GML_IDXLOOP(GLubyte)\
			break;\
	}

#define GML_MAKESUBFUNDA(name,pre,arg,sizefun,sizeass,typeass,first,count,copyfun)\
	if(clientstate & (1<<(pre-GL_VERTEX_ARRAY))) {\
		int itemsize=sizefun;\
		int itemstride=qd->arg##stride;\
		if(itemstride==0)\
			itemstride=itemsize;\
		p->arg##totalsize=0;\
		p->arg##pointer=(GLvoid *)((BYTE *)qd->arg##pointer+first*itemstride);\
		sizeass;\
		typeass;\
		if(!(qd->ClientState & GML_##arg##_ARRAY_BUFFER)) {\
			p->arg##totalsize=itemsize*count;\
			datasize+=p->arg##totalsize;\
			while(qd->WritePos+datasize>=qd->WriteSize)\
				p=(gml##name##Data *)qd->WaitRealloc(&e);\
			BYTE *v=(BYTE *)p->arg##pointer;\
			copyfun\
		}\
	}

#define GML_MAKEPOINTERDATA()\
	GLenum ClientState;\
	GLint VPsize;\
	GLenum VPtype;\
	GLvoid * VPpointer;\
	int VPtotalsize;\
	GLint CPsize;\
	GLenum CPtype;\
	GLvoid * CPpointer;\
	int CPtotalsize;\
	GLint TCPsize;\
	GLenum TCPtype;\
	GLvoid * TCPpointer;\
	int TCPtotalsize;\
	GLenum IPtype;\
	GLvoid * IPpointer;\
	int IPtotalsize;\
	GLenum NPtype;\
	GLvoid *NPpointer;\
	int NPtotalsize;\
	GLvoid * EFPpointer;\
	int EFPtotalsize;\
	int VAcount;

#define GML_MAKESUBFUNVA(name,first,count,copyfun)\
	p->VAcount=qd->VAset.size();\
	std::set<GLuint>::iterator si=qd->VAset.begin();\
	while(si!=qd->VAset.end()) {\
		std::map<GLuint,VAdata>::iterator mi=qd->VAmap.find(*si);\
		VAdata *vd=&(mi->second);\
		int itemstride=vd->stride;\
 		int itemsize=vd->size*gmlSizeOf(vd->type);\
		if(itemstride==0)\
			itemstride=itemsize;\
		if(vd->buffer)\
			itemsize=0;\
		int totalsize=itemsize*count+sizeof(VAstruct);\
		datasize+=totalsize;\
		while(qd->WritePos+datasize>=qd->WriteSize)\
			p=(gml##name##Data *)qd->WaitRealloc(&e);\
		VAstruct *vs=(VAstruct *)e;\
		e+=sizeof(VAstruct);\
		vs->target=*si;\
		vs->size=vd->size;\
		vs->type=vd->type;\
		vs->normalized=vd->normalized;\
		vs->totalsize=totalsize;\
		vs->pointer=(GLvoid *)((BYTE *)vd->pointer+first*itemstride);\
		vs->buffer=vd->buffer;\
		if(!vd->buffer) {\
			BYTE *v=(BYTE *)vs->pointer;\
			copyfun\
		}\
		++si;\
	}


#define GML_MAKEFUN3VDA(name,ftype1,ftype2,ftype3) struct gml##name##Data {\
	GML_MAKEVAR_C(ftype1,ftype2,ftype3)\
	GML_MAKEPOINTERDATA()\
	GML_MAKEVAR_SIZE()\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C) {\
	GML_COND(gl##name(A,B,C))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_C()\
	GLenum clientstate=qd->ClientState & ~(qd->ClientState>>16);\
	p->ClientState=qd->ClientState;\
	BYTE *e=(BYTE *)(p+1);\
	GML_MAKESUBFUNDA(name,GL_VERTEX_ARRAY,VP,qd->VPsize*gmlSizeOf(qd->VPtype),p->VPsize=qd->VPsize,p->VPtype=qd->VPtype,B,C,GML_MEMCOPY())\
	GML_MAKESUBFUNDA(name,GL_COLOR_ARRAY,CP,qd->CPsize*gmlSizeOf(qd->CPtype),p->CPsize=qd->CPsize,p->CPtype=qd->CPtype,B,C,GML_MEMCOPY())\
	GML_MAKESUBFUNDA(name,GL_TEXTURE_COORD_ARRAY,TCP,qd->TCPsize*gmlSizeOf(qd->TCPtype),p->TCPsize=qd->TCPsize,p->TCPtype=qd->TCPtype,B,C,GML_MEMCOPY())\
	GML_MAKESUBFUNDA(name,GL_INDEX_ARRAY,IP,gmlSizeOf(qd->IPtype),,p->IPtype=qd->IPtype,B,C,GML_MEMCOPY())\
	GML_MAKESUBFUNDA(name,GL_NORMAL_ARRAY,NP,3*gmlSizeOf(qd->NPtype),,p->NPtype=qd->NPtype,B,C,GML_MEMCOPY())\
	GML_MAKESUBFUNDA(name,GL_EDGE_FLAG_ARRAY,EFP,sizeof(GLboolean),,,B,C,GML_MEMCOPY())\
	GML_MAKESUBFUNVA(name,B,C,GML_MEMCOPY())\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}


#define GML_MAKEFUN4VDE(name,ftype1,ftype2,ftype3,ftype4) struct gml##name##Data {\
	GML_MAKEVAR_D(ftype1,ftype2,ftype3,ftype4 *)\
	GML_MAKEPOINTERDATA()\
	GML_MAKEVAR_SIZE()\
};\
GML_FUN(name,void)(ftype1 A, ftype2 B, ftype3 C, ftype4 *D) {\
	GML_COND(gl##name(A,B,C,D))\
	GML_PREP_FIXED(name)\
	GML_MAKEASS_D()\
	BYTE *e=(BYTE *)(p+1);\
	GLenum clientstate=qd->ClientState & ~(qd->ClientState>>16);\
	p->ClientState=qd->ClientState;\
	if(qd->ElementArrayBuffer)\
		p->ClientState |= GML_ELEMENT_ARRAY_BUFFER;\
	GML_MAKESUBFUNDA(name,GL_VERTEX_ARRAY,VP,qd->VPsize*gmlSizeOf(qd->VPtype),p->VPsize=qd->VPsize,p->VPtype=qd->VPtype,0,B,GML_IDXCOPY())\
	GML_MAKESUBFUNDA(name,GL_COLOR_ARRAY,CP,qd->CPsize*gmlSizeOf(qd->CPtype),p->CPsize=qd->CPsize,p->CPtype=qd->CPtype,0,B,GML_IDXCOPY())\
	GML_MAKESUBFUNDA(name,GL_TEXTURE_COORD_ARRAY,TCP,qd->TCPsize*gmlSizeOf(qd->TCPtype),p->TCPsize=qd->TCPsize,p->TCPtype=qd->TCPtype,0,B,GML_IDXCOPY())\
	GML_MAKESUBFUNDA(name,GL_INDEX_ARRAY,IP,gmlSizeOf(qd->IPtype),,p->IPtype=qd->IPtype,0,B,GML_IDXCOPY())\
	GML_MAKESUBFUNDA(name,GL_NORMAL_ARRAY,NP,3*gmlSizeOf(qd->NPtype),,p->NPtype=qd->NPtype,0,B,GML_IDXCOPY())\
	GML_MAKESUBFUNDA(name,GL_EDGE_FLAG_ARRAY,EFP,sizeof(GLboolean),,,0,B,GML_IDXCOPY())\
	GML_MAKESUBFUNVA(name,0,B,GML_IDXCOPY())\
	GML_UPD_SIZE()\
	GML_UPD_POS()\
}



const int __FIRSTLINE__=__LINE__;
GML_MAKEFUN1(Disable,GLenum)
GML_MAKEFUN1(Enable,GLenum)
GML_MAKEFUN2(BindTexture,GLenum,GLuint,)
GML_MAKEFUN3(TexParameteri,GLenum,GLenum,GLint,)
GML_MAKEFUN1(ActiveTextureARB,GLenum)
GML_MAKEFUN4(Color4f,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN3(Vertex3f,GLfloat,GLfloat,GLfloat,)
GML_MAKEFUN3(TexEnvi,GLenum,GLenum,GLint,)
GML_MAKEFUN2(TexCoord2f,GLfloat,GLfloat,)
GML_MAKEFUN6(ProgramEnvParameter4fARB,GLenum,GLuint,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN0(End)
GML_MAKEFUN1(Begin,GLenum)
GML_MAKEFUN1(MatrixMode,GLenum)
GML_MAKEFUN2(Vertex2f,GLfloat,GLfloat,)
GML_MAKEFUN0(PopMatrix)
GML_MAKEFUN0(PushMatrix)
GML_MAKEFUN0(LoadIdentity)
GML_MAKEFUN3(Translatef,GLfloat,GLfloat,GLfloat,)
GML_MAKEFUN2(BlendFunc,GLenum,GLenum,)
GML_MAKEFUN1(CallList,GLuint)
GML_MAKEFUN3(Color3f,GLfloat,GLfloat,GLfloat,)
GML_MAKEFUN9S(TexImage2D,GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid,D*E*gmlNumArgsTexImage(G)*gmlSizeOf(H))
GML_MAKEFUN1V(Color4fv,const GLfloat,GLfloat,4)
GML_MAKEFUN2(BindProgramARB,GLenum,GLuint,)
GML_MAKEFUN3(Scalef,GLfloat,GLfloat,GLfloat,)
GML_MAKEFUN4(Viewport,GLint,GLint,GLsizei,GLsizei)
GML_MAKEFUN2V(DeleteTextures,GLsizei,const GLuint,GLuint,A)
GML_MAKEFUN3(MultiTexCoord2fARB,GLenum,GLfloat,GLfloat,)
GML_MAKEFUN2(AlphaFunc,GLenum,GLclampf,)
GML_MAKEFUN1(DepthMask,GLboolean)
GML_MAKEFUN1(LineWidth,GLfloat)
GML_MAKEFUN2(BindFramebufferEXT,GLenum,GLuint,)
GML_MAKEFUN4(Rotatef,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN2(DeleteLists,GLuint,GLsizei,)
GML_MAKEFUN1CS(DisableClientState,GLenum,&=~)
GML_MAKEFUN1CS(EnableClientState,GLenum,|=)
GML_MAKEFUN4(Rectf,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN3V(Lightfv,GLenum,GLenum,const GLfloat,GLfloat,gmlNumArgsLightMat(B))
GML_MAKEFUN7S(uBuild2DMipmaps,GLenum,GLint,GLsizei,GLsizei,GLenum,GLenum,const GLvoid,C*D*gmlNumArgsTexImage(E)*gmlSizeOf(F))
GML_MAKEFUN1(Clear,GLbitfield)
GML_MAKEFUN0(EndList)
GML_MAKEFUN2(NewList,GLuint,GLenum,)
GML_MAKEFUN4(ClearColor,GLclampf,GLclampf,GLclampf,GLclampf)
GML_MAKEFUN2(PolygonMode,GLenum,GLenum,)
GML_MAKEFUN1(ActiveTexture,GLenum)
GML_MAKEFUN2(Fogf,GLenum,GLfloat,)
GML_MAKEFUN1V(MultMatrixf,const GLfloat,GLfloat,16)
GML_MAKEFUN6(Ortho,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble)
GML_MAKEFUN0(PopAttrib)
GML_MAKEFUN3V(Materialfv,GLenum,GLenum,const GLfloat,GLfloat,gmlNumArgsLightMat(B))
GML_MAKEFUN2(PolygonOffset,GLfloat,GLfloat,)
GML_MAKEFUN1(PushAttrib,GLbitfield)
GML_MAKEFUN1(CullFace,GLenum)
GML_MAKEFUN4(ColorMask,GLboolean,GLboolean,GLboolean,GLboolean)
GML_MAKEFUN1V(Vertex3fv,const GLfloat,GLfloat,3)
GML_MAKEFUN3V(TexGenfv,GLenum,GLenum,const GLfloat,GLfloat,gmlNumArgsTexGen(B))
GML_MAKEFUN2(Vertex2d,GLdouble,GLdouble,)
GML_MAKEFUN4P(VertexPointer,GLint,GLenum,GLsizei,GLvoid, VP)
GML_MAKEFUN3VDA(DrawArrays,GLenum,GLint,GLsizei)
GML_MAKEFUN2V(Fogfv,GLenum,const GLfloat,GLfloat,gmlNumArgsFog(A))
GML_MAKEFUN5(FramebufferTexture2DEXT,GLenum,GLenum,GLenum,GLuint,GLint)
GML_MAKEFUN4P(TexCoordPointer,GLint,GLenum,GLsizei,GLvoid, TCP)
GML_MAKEFUN9S(TexSubImage2D,GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const GLvoid,E*F*gmlNumArgsTexImage(G)*gmlSizeOf(H))
GML_MAKEFUN2V(ClipPlane,GLenum,const GLdouble,GLdouble,4)
GML_MAKEFUN4(Color4d,GLdouble,GLdouble,GLdouble,GLdouble)
GML_MAKEFUN2(LightModeli,GLenum,GLint,)
GML_MAKEFUN3(TexGeni,GLenum,GLenum,GLint,)
GML_MAKEFUN3(TexParameterf,GLenum,GLenum,GLfloat,)
GML_MAKEFUN8(CopyTexSubImage2D,GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei)
GML_MAKEFUN2V(DeleteFramebuffersEXT,GLsizei,const GLuint,GLuint,A)
GML_MAKEFUN1V(LoadMatrixf,const GLfloat,GLfloat,16)
GML_MAKEFUN1(ShadeModel,GLenum)
GML_MAKEFUN1(UseProgram,GLuint)
GML_MAKEFUN1(ClientActiveTextureARB,GLenum)
GML_MAKEFUN2V(DeleteRenderbuffersEXT,GLsizei,const GLuint,GLuint,A)
GML_MAKEFUN0(Flush)
GML_MAKEFUN3(Normal3f,GLfloat,GLfloat,GLfloat,)
GML_MAKEFUN1(UseProgramObjectARB,GLhandleARB)
GML_MAKEFUN8VP(CompressedTexImage2DARB,GLenum,GLint,GLenum,GLsizei,GLsizei,GLint,GLsizei,const GLvoid,BYTE,G)
GML_MAKEFUN1(DeleteObjectARB,GLhandleARB)
GML_MAKEFUN2(Fogi,GLenum,GLint,)
GML_MAKEFUN1V(MultMatrixd,const GLdouble,GLdouble,16)
GML_MAKEFUN2(PixelStorei,GLenum,GLint,)
GML_MAKEFUN2(PointParameterf,GLenum,GLfloat,)
GML_MAKEFUN3(TexCoord3f,GLfloat,GLfloat,GLfloat,)
GML_MAKEFUN2(Uniform1i,GLint,GLint,)
GML_MAKEFUN2(BindRenderbufferEXT,GLenum,GLuint,)
GML_MAKEFUN1V(Color3fv,const GLfloat,GLfloat,3)
GML_MAKEFUN1(DepthFunc,GLenum)
GML_MAKEFUN2(Hint,GLenum,GLenum,)
GML_MAKEFUN1(LogicOp,GLenum)
GML_MAKEFUN3(StencilOp,GLenum,GLenum,GLenum,)
GML_MAKEFUN3V(TexEnvfv,GLenum,GLenum,const GLfloat,GLfloat,gmlNumArgsTexEnv(B))
GML_MAKEFUN4V(UniformMatrix4fv,GLint,GLsizei,GLboolean,const GLfloat,GLfloat,16*B)
GML_MAKEFUN4(uOrtho2D,GLdouble,GLdouble,GLdouble,GLdouble)
GML_MAKEFUN2(AttachObjectARB,GLhandleARB,GLhandleARB,)
GML_MAKEFUN2B(BindBufferARB,GLenum,GLuint)
GML_MAKEFUN1V(Color3ubv,const GLubyte,GLubyte,3)
GML_MAKEFUN2(DetachObjectARB,GLhandleARB,GLhandleARB,)
GML_MAKEFUN4(FramebufferRenderbufferEXT,GLenum,GLenum,GLenum,GLuint)
GML_MAKEFUN2(LineStipple,GLint,GLushort,)
GML_MAKEFUN1V(LoadMatrixd,const GLdouble,GLdouble,16)
GML_MAKEFUN2(SetFenceNV,GLuint,GLenum,)
GML_MAKEFUN3(StencilFunc,GLenum,GLint,GLuint,)
GML_MAKEFUN10S(TexImage3D,GLenum,GLint,GLenum,GLsizei,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid,D*E*F*gmlNumArgsTexImage(H)*gmlSizeOf(I))
GML_MAKEFUN2(Uniform1f,GLint,GLfloat,)
GML_MAKEFUN1(ClearStencil,GLint)
GML_MAKEFUN4P(ColorPointer,GLint,GLenum,GLsizei,GLvoid, CP)
GML_MAKEFUN1(DeleteShader,GLuint)
GML_MAKEFUN4VDE(DrawElements,GLenum,GLsizei,GLenum,GLvoid)
GML_MAKEFUN1(GenerateMipmapEXT,GLenum)
GML_MAKEFUN3(Materialf,GLenum,GLenum,GLfloat,)
GML_MAKEFUN3P(NormalPointer,GLenum,GLsizei,GLvoid, NP)
GML_MAKEFUN3V(ProgramEnvParameter4fvARB,GLenum,GLuint,const GLfloat,GLfloat,4)
GML_MAKEFUN4(RenderbufferStorageEXT,GLenum,GLenum,GLsizei,GLsizei)
GML_MAKEFUN1(StencilMask,GLuint)
GML_MAKEFUN4(Uniform3f,GLint,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN4(uPerspective,GLdouble,GLdouble,GLdouble,GLdouble)
GML_MAKEFUN1(ActiveStencilFaceEXT,GLenum)
GML_MAKEFUN2(AttachShader,GLuint,GLuint,)
GML_MAKEFUN10(BlitFramebufferEXT,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum)
GML_MAKEFUN4VS(BufferDataARB,GLenum,GLsizei,const GLvoid,GLenum,BYTE,B)
GML_MAKEFUN1(ClearDepth,GLclampd)
GML_MAKEFUN3(Color3ub,GLubyte,GLubyte,GLubyte,)
GML_MAKEFUN7VP(CompressedTexImage1DARB,GLenum,GLint,GLenum,GLsizei,GLint,GLsizei,const GLvoid,BYTE,F)
GML_MAKEFUN9VP(CompressedTexImage3DARB,GLenum,GLint,GLenum,GLsizei,GLsizei,GLsizei,GLint,GLsizei,const GLvoid,BYTE,H)
GML_MAKEFUN1(DrawBuffer,GLenum)
GML_MAKEFUN1(FrontFace,GLenum)
GML_MAKEFUN6(Frustum,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble)
GML_MAKEFUN1(LinkProgramARB,GLhandleARB)
GML_MAKEFUN2(MultiTexCoord1f,GLenum,GLfloat,)
GML_MAKEFUN3(MultiTexCoord2f,GLenum,GLfloat,GLfloat,)
GML_MAKEFUN4(MultiTexCoord3f,GLenum,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN5(MultiTexCoord4f,GLenum,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN2V(PointParameterfv,GLenum,GLfloat,GLfloat,gmlNumArgsPointParam(A))
GML_MAKEFUN1(PointSize,GLfloat)
GML_MAKEFUN4V(ProgramStringARB,GLenum,GLenum,GLsizei,const GLvoid,BYTE,C)
GML_MAKEFUN3(SecondaryColor3f,GLfloat,GLfloat,GLfloat,)
GML_MAKEFUN1(TexCoord1f,GLfloat)
GML_MAKEFUN4(TexCoord4f,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN3(TexEnvf,GLenum,GLenum,GLfloat,)
GML_MAKEFUN3(TexGenf,GLenum,GLenum,GLfloat,)
GML_MAKEFUN8S(TexImage1D,GLenum,GLint,GLint,GLsizei,GLint,GLenum,GLenum,const GLvoid,D*gmlNumArgsTexImage(F)*gmlSizeOf(G))
GML_MAKEFUN2(Uniform1iARB,GLint,GLint,)
GML_MAKEFUN3(Uniform2f,GLint,GLfloat,GLfloat,)
GML_MAKEFUN3(Uniform2fARB,GLint,GLfloat,GLfloat,)
GML_MAKEFUN3(Uniform2i,GLint,GLint,GLint,)
GML_MAKEFUN4(Uniform3fARB,GLint,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN4(Uniform3i,GLint,GLint,GLint,GLint)
GML_MAKEFUN5(Uniform4f,GLint,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN5(Uniform4i,GLint,GLint,GLint,GLint,GLint)
GML_MAKEFUN4V(UniformMatrix2fv,GLint,GLsizei,GLboolean,const GLfloat,GLfloat,4*B)
GML_MAKEFUN4V(UniformMatrix3fv,GLint,GLsizei,GLboolean,const GLfloat,GLfloat,9*B)
GML_MAKEFUN4(Vertex4f,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN1(uDeleteQuadric,GLUquadric *)
GML_MAKEFUN2(uQuadricDrawStyle,GLUquadric *,GLenum,)
GML_MAKEFUN4(uSphere,GLUquadric *,GLdouble,GLint,GLint)
GML_MAKEFUN4(ClearAccum,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN4(Color4ub,GLubyte,GLubyte,GLubyte,GLubyte)
GML_MAKEFUN1V(Color4ubv,const GLubyte,GLubyte,4)
GML_MAKEFUN1(CompileShader,GLuint)
GML_MAKEFUN1(CompileShaderARB,GLhandleARB)
GML_MAKEFUN8(CopyTexImage2D,GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLsizei,GLint)
GML_MAKEFUN2V(DeleteBuffersARB,GLsizei,const GLuint,GLuint,A)
GML_MAKEFUN2V(DeleteFencesNV,GLsizei,const GLuint,GLuint,A)
GML_MAKEFUN1(DeleteProgram,GLuint)
GML_MAKEFUN2V(DeleteProgramsARB,GLsizei,const GLuint,GLuint,A)
GML_MAKEFUN2(DetachShader,GLuint,GLuint,)
GML_MAKEFUN1VA(DisableVertexAttribArrayARB,GLuint,VA,erase)
GML_MAKEFUN2V(DrawBuffersARB,GLsizei,const GLenum,GLenum,A)
GML_MAKEFUN1(EdgeFlag,GLboolean)
GML_MAKEFUN1VA(EnableVertexAttribArrayARB,GLuint,VA,insert)
GML_MAKEFUN0(Finish)
GML_MAKEFUN1(FinishFenceNV,GLuint)
GML_MAKEFUN1(FogCoordf,GLfloat)
GML_MAKEFUN3(Lightf,GLenum,GLenum,GLfloat,)
GML_MAKEFUN1(LinkProgram,GLuint)
GML_MAKEFUN1V(Normal3fv,const GLfloat,GLfloat,3)
GML_MAKEFUN2(RasterPos2i,GLint,GLint,)
GML_MAKEFUN1(ReadBuffer,GLenum)
GML_MAKEFUN4(Scissor,GLint,GLint,GLsizei,GLsizei)
GML_MAKEFUN4VSS(ShaderSource,GLuint,GLsizei,const GLchar,GLint,B)
GML_MAKEFUN4VSS(ShaderSourceARB,GLhandleARB,GLsizei,const GLcharARB,GLint,B)
GML_MAKEFUN1V(TexCoord2fv,const GLfloat,GLfloat,2)
GML_MAKEFUN3V(TexParameterfv,GLenum,GLenum,const GLfloat,GLfloat,gmlNumArgsTexParam(B))
GML_MAKEFUN3(Translated,GLdouble,GLdouble,GLdouble,)
GML_MAKEFUN3V(Uniform1fv,GLint,GLsizei,const GLfloat,GLfloat,B)
GML_MAKEFUN5(Uniform4fARB,GLint,GLfloat,GLfloat,GLfloat,GLfloat)
GML_MAKEFUN4V(UniformMatrix4fvARB,GLint,GLsizei,GLboolean,const GLfloat,GLfloat,16*B);
GML_MAKEFUN6P(VertexAttribPointerARB,GLuint,GLint,GLenum,GLboolean,GLsizei,const GLvoid,VA)
GML_MAKEFUN9(uLookAt,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble)
GML_MAKEFUN2V(LightModelfv,GLenum,const GLfloat,GLfloat,gmlNumArgsLightModel(A))
GML_MAKEFUN2V(DeleteQueries,GLsizei,const GLuint,GLuint,A)//
GML_MAKEFUN1(BlendEquation,GLenum)
GML_MAKEFUN2(StencilMaskSeparate,GLenum,GLuint,)
GML_MAKEFUN4(StencilFuncSeparate,GLenum,GLenum,GLint,GLuint)
GML_MAKEFUN4(StencilOpSeparate,GLenum,GLenum,GLenum,GLenum)
GML_MAKEFUN2(BeginQuery,GLenum,GLuint,)
GML_MAKEFUN1(EndQuery,GLenum)
GML_MAKEFUN3(GetQueryObjectuiv,GLuint,GLenum,GLuint *,,GML_SYNC())
GML_MAKEFUN2(BlendEquationSeparate,GLenum,GLenum,)
GML_MAKEFUN4(BlendFuncSeparate,GLenum,GLenum,GLenum,GLenum)
GML_MAKEFUN6(uCylinder,GLUquadric *,GLdouble,GLdouble,GLdouble,GLint,GLint)
GML_MAKEFUN2V(DeleteBuffers,GLsizei,const GLuint,GLuint,A)//
GML_MAKEFUN2B(BindBuffer,GLenum,GLuint)
GML_MAKEFUN4VS(BufferData,GLenum,GLsizeiptr,const GLvoid,GLenum,BYTE,B)
GML_MAKEFUN2R(MapBuffer,GLenum,GLenum,GLvoid *)
GML_MAKEFUN1R(UnmapBuffer,GLenum,GLboolean,)
GML_MAKEFUN8VP(CompressedTexImage2D,GLenum,GLint,GLenum,GLsizei,GLsizei,GLint,GLsizei,const GLvoid,BYTE,G)
GML_MAKEFUN1R(IsShader,GLuint, GLboolean,)
GML_MAKEFUN3(Vertex3i,GLint,GLint,GLint,)
GML_MAKEFUN2(GetIntegerv,GLenum,GLint *,GML_CACHE(GLenum,GLint,gmlGetIntegervCache,A,B),GML_SYNC())//
GML_MAKEFUN1R(CheckFramebufferStatusEXT,GLenum, GLenum,GML_DEFAULT_RET(A==GL_FRAMEBUFFER_EXT,GL_FRAMEBUFFER_COMPLETE_EXT))
GML_MAKEFUN2(GetFloatv,GLenum,GLfloat *,GML_CACHE(GLenum,GLfloat,gmlGetFloatvCache,A,B),GML_SYNC())
GML_MAKEFUN1R(GetString,GLenum,const GLubyte *,GML_CACHE_RET_STR(GLenum,std::string,gmlGetStringCache,A))
GML_MAKEFUN2R(GetUniformLocationARB,GLhandleARB,const GLcharARB *, GLint)
GML_MAKEFUN7(ReadPixels,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,GLvoid *,GML_SYNC())
GML_MAKEFUN0R(GetError,GLenum,GML_DEFAULT_ERROR())
GML_MAKEFUN3(GetObjectParameterivARB,GLhandleARB,GLenum,GLint *,GML_DEFAULT(B==GL_OBJECT_COMPILE_STATUS_ARB || B==GL_OBJECT_LINK_STATUS_ARB,*C=GL_TRUE),GML_SYNC())
GML_MAKEFUN2R(GetUniformLocation,GLint,const GLchar *, GLint)
GML_MAKEFUN2(GetDoublev,GLenum, GLdouble *,,GML_SYNC())
GML_MAKEFUN3(GetProgramiv,GLuint,GLenum,GLint *,,GML_SYNC())
GML_MAKEFUN7(GetActiveUniform,GLuint,GLuint,GLsizei,GLsizei *,GLint *,GLenum *,GLchar *,GML_SYNC())
GML_MAKEFUN2R(GetAttribLocationARB,GLhandleARB,const GLcharARB *, GLint)
GML_MAKEFUN4(GetInfoLogARB,GLhandleARB,GLsizei,GLsizei *,GLcharARB *,GML_SYNC())
GML_MAKEFUN4(GetProgramInfoLog,GLuint,GLsizei,GLsizei *,GLchar *,GML_SYNC())
GML_MAKEFUN3(GetProgramivARB,GLenum,GLenum,GLint *,,GML_SYNC())
GML_MAKEFUN4(GetShaderInfoLog,GLuint,GLsizei,GLsizei *,GLchar *,GML_SYNC())
GML_MAKEFUN3(GetShaderiv,GLuint,GLenum,GLint *,GML_DEFAULT(B==GL_COMPILE_STATUS,*C=GL_TRUE),GML_SYNC())
GML_MAKEFUN1R(IsRenderbufferEXT,GLuint,GLboolean,)
GML_MAKEFUN2R(MapBufferARB,GLenum,GLenum,GLvoid *)
GML_MAKEFUN9R(uProject,GLdouble,GLdouble,GLdouble,const GLdouble *,const GLdouble *,const GLint *,GLdouble *,GLdouble *,GLdouble *,int)
GML_MAKEFUN9R(uScaleImage,GLenum,GLint,GLint,GLenum,const void *,GLint,GLint,GLenum,void *,int)
GML_MAKEFUN1R(TestFenceNV,GLuint,GLboolean,)
GML_MAKEFUN3P(IndexPointer,GLenum,GLsizei,GLvoid, IP)//
GML_MAKEFUN2P(EdgeFlagPointer,GLsizei,GLboolean, EFP)
GML_MAKEFUN4(TrackMatrixNV,GLenum,GLuint,GLenum,GLenum)
GML_MAKEFUN3(ProgramParameteriEXT,GLuint,GLenum,GLint,)
GML_MAKEFUN4(BlendColor,GLclampf,GLclampf,GLclampf,GLclampf)
GML_MAKEFUN6VST(Map1f,GLenum,GLfloat,GLfloat,GLint,GLint,const GLfloat,GLfloat,E,D,gmlNumArgsMap1(A))
GML_MAKEFUN10VST(Map2f,GLenum,GLfloat,GLfloat,GLint,GLint,GLfloat,GLfloat,GLint,GLint,const GLfloat,GLfloat,E,D,I,H,gmlNumArgsMap2(A))
GML_MAKEFUN3(MapGrid1f,GLint,GLfloat,GLfloat,)
GML_MAKEFUN6(MapGrid2f,GLint,GLfloat,GLfloat,GLint,GLfloat,GLfloat)
GML_MAKEFUN3(EvalMesh1,GLenum,GLint,GLint,)
GML_MAKEFUN5(EvalMesh2,GLenum,GLint,GLint,GLint,GLint)
GML_MAKEFUN1(EvalCoord1f,GLfloat)
GML_MAKEFUN2(EvalCoord2f,GLfloat,GLfloat,)
GML_MAKEFUN1(EvalPoint1,GLint)
GML_MAKEFUN2(EvalPoint2,GLint,GLint,)
GML_MAKEFUN1R(RenderMode,GLenum,GLint,)
GML_MAKEFUN2(SelectBuffer,GLsizei,GLuint *,,GML_SYNC())
GML_MAKEFUN0(InitNames)
GML_MAKEFUN1(LoadName,GLuint)
GML_MAKEFUN1(PushName,GLuint)
GML_MAKEFUN0(PopName)

#endif
