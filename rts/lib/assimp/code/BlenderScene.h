/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file  BlenderScene.h
 *  @brief Intermediate representation of a BLEND scene.
 */
#ifndef INCLUDED_AI_BLEND_SCENE_H
#define INCLUDED_AI_BLEND_SCENE_H

namespace Assimp	{
	namespace Blender {

// Minor parts of this file are extracts from blender data structures,
// declared in the ./source/blender/makesdna directory.
// Stuff that is not used by Assimp is commented.


// NOTE
// this file serves as input data to the `./scripts/genblenddna.py`
// script. This script generates the actual binding code to read a
// blender file with a possibly different DNA into our structures.
// Only `struct` declarations are considered and the following 
// rules must be obeyed in order for the script to work properly:
//
// * C++ style comments only
//
// * Structures may include the primitive types char, int, short,
//   float, double. Signedness specifiers are not allowed on 
//	 integers. Enum types are allowed, but they must have been
//   defined in this header.
//
// * Structures may aggregate other structures, unless not defined
//   in this header.
//
// * Pointers to other structures or primitive types are allowed.
//   No references or double pointers or arrays of pointers.
//   A pointer to a T is written as boost::shared_ptr, while a
//   pointer to an array of elements is written as boost::
//   shared_array.
//
// * Arrays can have maximally two-dimensions. Any non-pointer
//   type can form them.
//
// * Multiple fields can be declare in a single line (i.e `int a,b;`)
//   provided they are neither pointers nor arrays.
//
// * One of WARN, FAIL can be appended to the declaration (
//   prior to the semiolon to specifiy the error handling policy if
//   this field is missing in the input DNA). If none of those
//   is specified the default policy is to subtitute a default
//   value for the field.
//

#define WARN // warn if field is missing, substitute default value
#define FAIL // fail the import if the field does not exist

struct Object;
struct MTex;

#define AI_BLEND_MESH_MAX_VERTS 2000000000L

// -------------------------------------------------------------------------------
struct ID : ElemBase {

	char name[24] WARN; 
	short flag;
};

// -------------------------------------------------------------------------------
struct ListBase : ElemBase {
    
	boost::shared_ptr<ElemBase> first;
	boost::shared_ptr<ElemBase> last;
};


// -------------------------------------------------------------------------------
struct PackedFile : ElemBase {
     int size WARN;
     int seek WARN;
	 boost::shared_ptr< FileOffset > data WARN;
};

// -------------------------------------------------------------------------------
struct GroupObject : ElemBase {
	
	boost::shared_ptr<GroupObject> prev,next FAIL;
	boost::shared_ptr<Object> ob;
};

// -------------------------------------------------------------------------------
struct Group : ElemBase {
	ID id FAIL;
	int layer;

	boost::shared_ptr<GroupObject> gobject;
};

// -------------------------------------------------------------------------------
struct World : ElemBase {
	ID id FAIL;
	
};

// -------------------------------------------------------------------------------
struct MVert : ElemBase {
	float co[3] FAIL;
	float no[3] FAIL;
	char flag;
	int mat_nr WARN;
	int bweight;
};

// -------------------------------------------------------------------------------
struct MEdge : ElemBase {
      int v1, v2 FAIL;
      char crease, bweight;
      short flag;
};

// -------------------------------------------------------------------------------
struct MCol : ElemBase {
	char r,g,b,a FAIL;
};

// -------------------------------------------------------------------------------
struct MFace : ElemBase {
	int v1,v2,v3,v4 FAIL;
	int mat_nr FAIL;
	char flag;
};

// -------------------------------------------------------------------------------
struct TFace : ElemBase {
	float uv[4][2] FAIL;
	int col[4] FAIL;
	char flag;
	short mode;
	short tile;
	short unwrap;
};

// -------------------------------------------------------------------------------
struct MTFace : ElemBase {

	float uv[4][2] FAIL;
	char flag;
	short mode;
	short tile;
	short unwrap;

	// boost::shared_ptr<Image> tpage;
};

// -------------------------------------------------------------------------------
struct MDeformWeight : ElemBase  {
      int    def_nr FAIL;
      float  weight FAIL;
};

// -------------------------------------------------------------------------------
struct MDeformVert : ElemBase  {

	vector<MDeformWeight> dw WARN;
	int totweight;
};

// -------------------------------------------------------------------------------
struct Material : ElemBase {
	ID id FAIL;

	float r,g,b WARN;
	float specr,specg,specb WARN;
	short har;
	float ambr,ambg,ambb WARN;
	float mirr,mirg,mirb;
	float emit WARN;
	float alpha WARN;
	float ref;
	float translucency;
	float roughness;
	float darkness;
	float refrac;

	boost::shared_ptr<Group> group;

	short diff_shader WARN;
	short spec_shader WARN;

	boost::shared_ptr<MTex> mtex[18];
};

// -------------------------------------------------------------------------------
struct Mesh : ElemBase {
	ID id FAIL;

	int totface FAIL;
	int totedge FAIL;
	int totvert FAIL;

	short subdiv;
	short subdivr;
	short subsurftype;
	short smoothresh;

	vector<MFace> mface FAIL;
	vector<MTFace> mtface;
	vector<TFace> tface;
	vector<MVert> mvert FAIL;
	vector<MEdge> medge WARN;
	vector<MDeformVert> dvert;
	vector<MCol> mcol;

	vector< boost::shared_ptr<Material> > mat FAIL;
};

// -------------------------------------------------------------------------------
struct Library : ElemBase {
	ID id FAIL;
	
	char name[240] WARN;
	char filename[240] FAIL;
	boost::shared_ptr<Library> parent WARN;
};

// -------------------------------------------------------------------------------
struct Camera : ElemBase {
	enum Type {
		  Type_PERSP	=	0
		 ,Type_ORTHO	=	1
	};

	ID id FAIL;

	// struct AnimData *adt;  

	Type type,flag WARN;
	float angle WARN;
	//float passepartalpha, angle;
	//float clipsta, clipend;
	//float lens, ortho_scale, drawsize;
	//float shiftx, shifty;

	//float YF_dofdist, YF_aperture;
	//short YF_bkhtype, YF_bkhbias;
	//float YF_bkhrot;
};


// -------------------------------------------------------------------------------
struct Lamp : ElemBase {

	enum FalloffType {
		 FalloffType_Constant	= 0x0
		,FalloffType_InvLinear	= 0x1
		,FalloffType_InvSquare	= 0x2
		//,FalloffType_Curve	= 0x3
		//,FalloffType_Sliders	= 0x4
	};

	enum Type {
		 Type_Local			= 0x0
		,Type_Sun			= 0x1
		,Type_Spot			= 0x2
		,Type_Hemi			= 0x3
		,Type_Area			= 0x4
		//,Type_YFPhoton	= 0x5
	};

      ID id FAIL;
      //AnimData *adt;  
      
      Type type FAIL;
	  short flags;

      //int mode;
      
      short colormodel, totex;
      float r,g,b,k WARN;
      //float shdwr, shdwg, shdwb;
      
      float energy, dist, spotsize, spotblend;
      //float haint;
         
      float att1, att2; 
      //struct CurveMapping *curfalloff;
      FalloffType falloff_type;
      
      //float clipsta, clipend, shadspotsize;
      //float bias, soft, compressthresh;
      //short bufsize, samp, buffers, filtertype;
      //char bufflag, buftype;
      
      //short ray_samp, ray_sampy, ray_sampz;
      //short ray_samp_type;
      //short area_shape;
	  //float area_size, area_sizey, area_sizez;
	  //float adapt_thresh;
	  //short ray_samp_method;

	  //short texact, shadhalostep;

	  //short sun_effect_type;
	  //short skyblendtype;
	  //float horizon_brightness;
	  //float spread;
	  float sun_brightness;
	  //float sun_size;
	  //float backscattered_light;
	  //float sun_intensity;
	  //float atm_turbidity;
	  //float atm_inscattering_factor;
	  //float atm_extinction_factor;
	  //float atm_distance_factor;
	  //float skyblendfac;
	  //float sky_exposure;
	  //short sky_colorspace;

	  // int YF_numphotons, YF_numsearch;
	  // short YF_phdepth, YF_useqmc, YF_bufsize, YF_pad;
	  // float YF_causticblur, YF_ltradius;

	  // float YF_glowint, YF_glowofs;
      // short YF_glowtype, YF_pad2;
      
      //struct Ipo *ipo;                    
      //struct MTex *mtex[18];              
      // short pr_texture;
      
      //struct PreviewImage *preview;
};

// -------------------------------------------------------------------------------
struct ModifierData : ElemBase  {
	enum ModifierType {
      eModifierType_None = 0,
      eModifierType_Subsurf,
      eModifierType_Lattice,
      eModifierType_Curve,
      eModifierType_Build,
      eModifierType_Mirror,
      eModifierType_Decimate,
      eModifierType_Wave,
      eModifierType_Armature,
      eModifierType_Hook,
      eModifierType_Softbody,
      eModifierType_Boolean,
      eModifierType_Array,
      eModifierType_EdgeSplit,
      eModifierType_Displace,
      eModifierType_UVProject,
      eModifierType_Smooth,
      eModifierType_Cast,
      eModifierType_MeshDeform,
      eModifierType_ParticleSystem,
      eModifierType_ParticleInstance,
      eModifierType_Explode,
      eModifierType_Cloth,
      eModifierType_Collision,
      eModifierType_Bevel,
      eModifierType_Shrinkwrap,
      eModifierType_Fluidsim,
      eModifierType_Mask,
      eModifierType_SimpleDeform,
      eModifierType_Multires,
      eModifierType_Surface,
      eModifierType_Smoke,
      eModifierType_ShapeKey
	};

	boost::shared_ptr<ElemBase> next WARN;
	boost::shared_ptr<ElemBase> prev WARN;

	int type, mode;
	char name[32];
};

// -------------------------------------------------------------------------------
struct SubsurfModifierData : ElemBase  {

	enum Type {
		
		TYPE_CatmullClarke = 0x0,
		TYPE_Simple = 0x1
	};

	enum Flags {
		// some ommitted
		FLAGS_SubsurfUV		=1<<3
	};

	ModifierData modifier FAIL;
	short subdivType WARN;
	short levels FAIL;
	short renderLevels ;
	short flags;
};

// -------------------------------------------------------------------------------
struct MirrorModifierData : ElemBase {

	enum Flags {
		Flags_CLIPPING      =1<<0,
		Flags_MIRROR_U      =1<<1,
		Flags_MIRROR_V      =1<<2,
		Flags_AXIS_X        =1<<3,
		Flags_AXIS_Y        =1<<4,
		Flags_AXIS_Z        =1<<5,
		Flags_VGROUP        =1<<6
	};

	ModifierData modifier FAIL;

	short axis, flag;
	float tolerance;
	boost::shared_ptr<Object> mirror_ob;
};

// -------------------------------------------------------------------------------
struct Object : ElemBase  {
	ID id FAIL;

	enum Type {
		 Type_EMPTY		=	0
		,Type_MESH		=	1
		,Type_CURVE		=	2
		,Type_SURF		=   3
		,Type_FONT		=   4
		,Type_MBALL		=	5

		,Type_LAMP		=	10
		,Type_CAMERA	=   11

		,Type_WAVE		=   21
		,Type_LATTICE	=   22
	};

	Type type FAIL;
	float obmat[4][4] WARN;
	float parentinv[4][4] WARN;
	char parsubstr[32] WARN;
	
	boost::shared_ptr<Object> parent WARN;
	boost::shared_ptr<Object> track WARN;

	boost::shared_ptr<Object> proxy,proxy_from,proxy_group WARN;
	boost::shared_ptr<Group> dup_group WARN;
	boost::shared_ptr<ElemBase> data FAIL;

	ListBase modifiers;
};


// -------------------------------------------------------------------------------
struct Base : ElemBase {
	boost::shared_ptr<Base> prev WARN;
	boost::shared_ptr<Base> next WARN;
	boost::shared_ptr<Object> object WARN;
};

// -------------------------------------------------------------------------------
struct Scene : ElemBase {
	ID id FAIL;

	boost::shared_ptr<Object> camera WARN;
	boost::shared_ptr<World> world WARN;
	boost::shared_ptr<Base> basact WARN;

	ListBase base;
};


// -------------------------------------------------------------------------------
struct Image : ElemBase {
	ID id FAIL;

	char name[240] WARN;               

	//struct anim *anim;

	short ok, flag;
	short source, type, pad, pad1;
	int lastframe;

	short tpageflag, totbind;
	short xrep, yrep;
	short twsta, twend;
	//unsigned int bindcode;  
	//unsigned int *repbind; 

	boost::shared_ptr<PackedFile> packedfile;
	//struct PreviewImage * preview;

	float lastupdate;
	int lastused;
	short animspeed;

	short gen_x, gen_y, gen_type; 
};

// -------------------------------------------------------------------------------
struct Tex : ElemBase {

	// actually, the only texture type we support is Type_IMAGE
	enum Type {
		 Type_CLOUDS		= 1
		,Type_WOOD			= 2
		,Type_MARBLE		= 3
		,Type_MAGIC			= 4
		,Type_BLEND			= 5
		,Type_STUCCI		= 6
		,Type_NOISE			= 7
		,Type_IMAGE			= 8
		,Type_PLUGIN		= 9
		,Type_ENVMAP		= 10
		,Type_MUSGRAVE		= 11
		,Type_VORONOI		= 12
		,Type_DISTNOISE		= 13
		,Type_POINTDENSITY	= 14
		,Type_VOXELDATA		= 15
	};

	ID id FAIL;
	// AnimData *adt; 

	//float noisesize, turbul;
	//float bright, contrast, rfac, gfac, bfac;
	//float filtersize;

	//float mg_H, mg_lacunarity, mg_octaves, mg_offset, mg_gain;
	//float dist_amount, ns_outscale;

	//float vn_w1;
	//float vn_w2;
	//float vn_w3;
	//float vn_w4;
	//float vn_mexp;
	//short vn_distm, vn_coltype;

	//short noisedepth, noisetype;
	//short noisebasis, noisebasis2;

	//short imaflag, flag;
	Type type FAIL;
	//short stype;

	//float cropxmin, cropymin, cropxmax, cropymax;
	//int texfilter;
	//int afmax;  
	//short xrepeat, yrepeat;
	//short extend;

	//short fie_ima;
	//int len;
	//int frames, offset, sfra;

	//float checkerdist, nabla;
	//float norfac;

	//ImageUser iuser;

	//bNodeTree *nodetree;
	//Ipo *ipo;                  
	boost::shared_ptr<Image> ima WARN;
	//PluginTex *plugin;
	//ColorBand *coba;
	//EnvMap *env;
	//PreviewImage * preview;
	//PointDensity *pd;
	//VoxelData *vd;

	//char use_nodes;
};

// -------------------------------------------------------------------------------
struct MTex : ElemBase {

	enum Projection {
		 Proj_N = 0
		,Proj_X = 1
		,Proj_Y = 2
		,Proj_Z = 3
	};

	enum Flag {
		 Flag_RGBTOINT		= 0x1
		,Flag_STENCIL		= 0x2
		,Flag_NEGATIVE		= 0x4
		,Flag_ALPHAMIX		= 0x8
		,Flag_VIEWSPACE		= 0x10
	};

	enum BlendType {
		 BlendType_BLEND			= 0
		,BlendType_MUL				= 1
		,BlendType_ADD				= 2
		,BlendType_SUB				= 3
		,BlendType_DIV				= 4
		,BlendType_DARK				= 5
		,BlendType_DIFF				= 6
		,BlendType_LIGHT			= 7
		,BlendType_SCREEN			= 8
		,BlendType_OVERLAY			= 9
		,BlendType_BLEND_HUE		= 10
		,BlendType_BLEND_SAT		= 11
		,BlendType_BLEND_VAL		= 12
		,BlendType_BLEND_COLOR		= 13
	};

	// short texco, mapto, maptoneg;

	BlendType blendtype;
	boost::shared_ptr<Object> object;
	boost::shared_ptr<Tex> tex;
	char uvname[32];

	Projection projx,projy,projz;
	char mapping;
	float ofs[3], size[3], rot;

	int texflag;
	short colormodel, pmapto, pmaptoneg;
	//short normapspace, which_output;
	//char brush_map_mode;
	float r,g,b,k WARN;
	//float def_var, rt;

	//float colfac, varfac;

	//float norfac, dispfac, warpfac;
	float colspecfac, mirrfac, alphafac;
	float difffac, specfac, emitfac, hardfac;
	//float raymirrfac, translfac, ambfac;
	//float colemitfac, colreflfac, coltransfac;
	//float densfac, scatterfac, reflfac;

	//float timefac, lengthfac, clumpfac;
	//float kinkfac, roughfac, padensfac;
	//float lifefac, sizefac, ivelfac, pvelfac;
	//float shadowfac;
	//float zenupfac, zendownfac, blendfac;
};


	}
}
#endif
