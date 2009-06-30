/*
---------------------------------------------------------------------
   Terrain Renderer using texture splatting and geomipmapping
   Copyright (c) 2006 Jelmer Cnossen

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   Jelmer Cnossen
   j.cnossen at gmail dot com
---------------------------------------------------------------------
*/
#include "StdAfx.h"

#include "TdfParser.h"

#include <cassert>
#include <deque>
#include <set>
#include <algorithm>

#include "TerrainBase.h"
#include "Terrain.h"
#include "TerrainTexture.h"

#include "TerrainTexEnvCombine.h"
#include "TerrainTextureGLSL.h"
#include "Lightcalc.h"
#include "Util.h"

#include <SDL_keysym.h>
extern unsigned char *keys;

namespace terrain {

	using namespace std;

//-----------------------------------------------------------------------
// RenderSetup
//-----------------------------------------------------------------------

	RenderSetup::~RenderSetup()
	{
		for (size_t a=0;a<passes.size();a++)
			delete passes[a].shaderSetup;
	}

	void RenderSetup::DebugOutput()
	{
		for (size_t a=0;a<passes.size();a++)
		{
			const char *str=0;
			RenderPass& p = passes[a];

			if (p.operation == Pass_Mul)
				str="Pass_Mul";
			if (p.operation == Pass_Add)
				str="Pass_Add";
			if (p.operation == Pass_Interpolate)
				str="Pass_Interpolate";
			if (p.operation == Pass_Replace)
				str="Pass_Replace";

			string shaderstr = p.shaderSetup ? p.shaderSetup->GetDebugDesc () : string("none");
			d_trace ("Pass (%lu): %s, %s. Shader: %s\n", a, str, p.invertAlpha ? "invertalpha" : "", shaderstr.c_str());
		}
		d_trace ("\n");
	}

//-----------------------------------------------------------------------
// RenderSetupCollection
//-----------------------------------------------------------------------

	RenderSetupCollection::~RenderSetupCollection()
	{
		for (size_t a=0;a<renderSetup.size();a++)
			delete renderSetup[a];
	}

//-----------------------------------------------------------------------
// Terrain Texturing class
//-----------------------------------------------------------------------


	TerrainTexture::TerrainTexture ()
	{
		tdfParser = 0;
		heightmapW = 0;
		blendmapLOD = 0;
		debugShader = -1;
		shaderHandler = 0;
		maxPasses = 0;
		currentRenderSetup = 0;
		lightmap = 0;
		flatBumpmap = 0;
		shadowMapParams = 0;
	}

	TerrainTexture::~TerrainTexture ()
	{
		delete shadowMapParams;
		delete lightmap;
		for (size_t a=0;a<texNodeSetup.size();a++)
			delete texNodeSetup[a];

		delete shaderHandler;
		texNodeSetup.clear();

		// free all blendmaps
		for (size_t a=0;a<blendMaps.size();a++)
			delete blendMaps[a];
		blendMaps.clear();
		// free all textures
		for (size_t a=0;a<textures.size();a++)
			delete textures[a];
		textures.clear();
	}

	struct TerrainTexture::GenerateInfo
	{
		deque<AlphaImage*>* bmMipmaps;
		vector<int> testlod; // the level

		// set of texture node setup objects, sorted by blend sort key
		map<uint, RenderSetupCollection*> nodesetup;
	};

	BaseTexture* TerrainTexture::LoadImageSource(
		const std::string& name,
		const std::string& basepath,
		Heightmap *heightmap, ILoadCallback *cb,
		Config* cfg, bool isBumpmap)
	{
		BaseTexture *btex = 0;
		if (!!atoi(tdfParser->SGetValueDef("0", basepath + name + "\\Blendmap").c_str()))
		{
			if (isBumpmap)
				throw content_error(name + " should be a bumpmap, not a blendmap.");

			Blendmap *bm = new Blendmap;
			bm->Load (name, basepath + name, heightmap, cb, tdfParser);
			blendMaps.push_back (bm);
			btex = bm;
		}
		else // a regular texturemap?
		{
			TiledTexture *tex = new TiledTexture;
			tex->Load (name, basepath + name, cb, tdfParser, isBumpmap);
			if (!tex->id) {
				delete tex;
				return 0;
			}
			textures.push_back (tex);
			btex = tex;
		}

		if (cfg->anisotropicFiltering > 0.0f) {
			glBindTexture(GL_TEXTURE_2D, btex->id);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cfg->anisotropicFiltering);
		}
		return btex;
	}

	void TerrainTexture::Load (const TdfParser *tdf, Heightmap *heightmap, TQuad *quadtree, const vector<QuadMap*>& qmaps, Config *cfg, ILoadCallback *cb, LightingInfo *li)
	{
		string basepath="MAP\\TERRAIN\\";

		if (cb) cb->PrintMsg ("  parsing texture stages...");

		if (!GLEW_ARB_multitexture)
			throw std::runtime_error ("No multitexture avaiable");
		if (!GLEW_ARB_texture_env_combine)
			throw std::runtime_error ("Texture env combine extension not avaiable");

		heightmapW = heightmap->w;
		tdfParser = tdf;

		shaderDef.Parse(*tdf, cfg->useBumpMaps);
		shaderDef.useShadowMapping = cfg->useShadowMaps;

		optimizeEpsilon = atof(tdf->SGetValueDef ("0.04", basepath + "LayerOptimizeConst").c_str());
		if (optimizeEpsilon < 0.0f) optimizeEpsilon = 0.0f;

		// Load textures
		map<string, BaseTexture*> nametbl;

		if (cb) cb->PrintMsg ("  loading textures and blendmaps...");
		bool hasBumpmaps = false;
		for (size_t a=0;a<shaderDef.stages.size();a++)
		{
			ShaderDef::Stage* st = &shaderDef.stages [a];
			// Already loaded?
			if (nametbl.find (st->sourceName) == nametbl.end())
				nametbl[st->sourceName] = LoadImageSource(st->sourceName, basepath, heightmap, cb, cfg);
			st->source = nametbl[st->sourceName];

			std::string bm;
			if (tdf->SGetValue (bm, basepath + st->sourceName + "\\Bumpmap"))
				hasBumpmaps = true;
		}

		if (cfg->useBumpMaps && hasBumpmaps) {
			for (size_t a = 0; a < shaderDef.normalMapStages.size(); a++) {
				ShaderDef::Stage* st = &shaderDef.normalMapStages [a];

				string name = st->sourceName;
				if (st->operation != ShaderDef::Alpha)
					name += "_BM"; // make it unique, this whole naming thing is very hackish though..

				// Already loaded?
				if (nametbl.find (name) == nametbl.end()) {
					// load a bumpmap unless it's an alpha operation
					st->source = LoadImageSource(st->sourceName, basepath,
						heightmap, cb, cfg, st->operation != ShaderDef::Alpha);
					if (!st->source) {
						if (!flatBumpmap) {
							flatBumpmap = TiledTexture::CreateFlatBumpmap();
							textures.push_back (flatBumpmap);
						}
						st->source = flatBumpmap;
					}
					st->sourceName = st->source->name;
					nametbl[st->sourceName] = st->source;
				}
				st->source = nametbl[st->sourceName];
			}
		}
		else cfg->useBumpMaps = false;

		// Generate blendmap mipmaps
		deque<AlphaImage*>* bmMipmaps = new deque<AlphaImage*>[blendMaps.size()];
		GenerateInfo gi;
		gi.bmMipmaps = bmMipmaps;

		if (cb) { cb->PrintMsg ("  generating blendmap mipmaps..."); }

		for (size_t a=0;a<blendMaps.size();a++) {
			Blendmap *bm = blendMaps[a];

			AlphaImage *cur = bm->image;
			bm->image = 0;

			do {
				bmMipmaps[a].push_front (cur);
				cur = cur->CreateMipmap ();
			} while (cur);

			for (size_t c=0;c<bmMipmaps[a].size();c++)
				if (bmMipmaps[a][c]->w == QUAD_W) {
					gi.testlod.push_back ((int)c);
					break;
				}
		}

		if (cb) cb->PrintMsg ("  loading blendmaps into OpenGL...");

		// Convert to textures
		for (size_t a=0;a<blendMaps.size();a++) {
			AlphaImage *bm = bmMipmaps[a].back();

			// Save image
			if (blendMaps[a]->generatorInfo) {
				char fn[32];
				SNPRINTF (fn,32, "blendmap%lu.jpg", a);
				remove(fn);
				bm->Save (fn);
			}
			// Upload
			glGenTextures (1, &blendMaps[a]->id);
			glBindTexture (GL_TEXTURE_2D, blendMaps[a]->id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			if (cfg->anisotropicFiltering > 0.0f)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cfg->anisotropicFiltering);

			deque<AlphaImage*>& mipmaps = bmMipmaps[a];
			for (size_t d=0;d<mipmaps.size();d++) {
				AlphaImage *lod = mipmaps[mipmaps.size()-d-1];
				glTexImage2D (GL_TEXTURE_2D, (int)d, GL_ALPHA, lod->w, lod->h, 0, GL_ALPHA, GL_FLOAT, lod->data);
			}
			blendMaps[a]->image = 0;
		}

		// Create texture application object
		if (!cfg->forceFallbackTexturing && GLEW_ARB_fragment_shader && GLEW_ARB_vertex_shader &&
			GLEW_ARB_shader_objects && GLEW_ARB_shading_language_100) {
			shaderHandler = new GLSLShaderHandler;
		} else {
			// texture_env_combine as fallback
			shaderHandler = new TexEnvSetupHandler;
		}

		// Calculate shadows
		if (cfg->useStaticShadow) {
			if (cb) cb->PrintMsg("  calculating lightmap");
			lightmap = new Lightmap(heightmap, 2, 1,li);
		}

		// see how lighting should be implemented, based on config and avaiable textures
		InstantiateShaders(cfg, cb);

		if (cb) { cb->PrintMsg ("  initializing terrain node shaders..."); }

		CreateTexProg (quadtree, &gi);
		shaderHandler->EndBuild();

		// count passes
		maxPasses = 0;
		for (map<uint, RenderSetupCollection*>::iterator mi=gi.nodesetup.begin();mi!=gi.nodesetup.end();++mi) {
			for (size_t i = 0; i < mi->second->renderSetup.size(); i++) {
				RenderSetup *rs = mi->second->renderSetup[i];

				if (rs->passes.size () > maxPasses) {
					maxPasses = rs->passes.size();
				}
			}
			texNodeSetup.push_back (mi->second);
		}

		if (cb) cb->PrintMsg ("  deleting temporary blendmap data...");

		// Free blendmap mipmap images
		for (size_t a=0;a<blendMaps.size();a++) {
			for (deque<AlphaImage*>::iterator i=bmMipmaps[a].begin();i!=bmMipmaps[a].end();++i) {
				delete *i;
			}
		}

		delete[] bmMipmaps;

		if (cfg->useShadowMaps) {
			shadowMapParams = new ShadowMapParams;
		}
	}

	void TerrainTexture::CreateTexProg (TQuad *node, TerrainTexture::GenerateInfo *gi)
	{
		// Test blendmaps
		for (size_t a=0;a<blendMaps.size();a++) {
			deque <AlphaImage*>& mipmaps = gi->bmMipmaps[a];
			int mipIndex = node->depth + gi->testlod[a];

			if (mipIndex >= mipmaps.size ())
				mipIndex = mipmaps.size () - 1;

			// scaling divisor, needed because the blendmap maybe lower res than the corresponding part of heightmap
			int d = ( (1<<node->depth) * QUAD_W ) / mipmaps[mipIndex]->w;

			// Calculate "constants" representing the blendmap in the area that the node covers
			blendMaps[a]->curAreaResult = mipmaps[mipIndex]->TestArea ((node->qmPos.x * QUAD_W)/d,
				(node->qmPos.y * QUAD_W)/d, ((node->qmPos.x+1)*QUAD_W)/d, ((node->qmPos.y+1)*QUAD_W)/d, optimizeEpsilon);
		}

		uint key = CalcBlendmapSortKey ();
		if (gi->nodesetup.find (key) == gi->nodesetup.end()) {
			RenderSetupCollection* tns = gi->nodesetup[key] = new RenderSetupCollection;

			tns->renderSetup.resize (shaders.size());
			uint vda = 0;

			// create a rendersetup for every shader expression
			for (size_t a = 0; a < shaders.size(); a++) {
				RenderSetup* rs = tns->renderSetup [a] = new RenderSetup;

				shaders[a]->def.Optimize(&rs->shaderDef);
				shaderHandler->BuildNodeSetup(&rs->shaderDef, rs);

				for (size_t p=0;p<tns->renderSetup [a]->passes.size();p++) {
					if(tns->renderSetup [a]->passes[p].shaderSetup)
						vda |= tns->renderSetup [a]->passes[p].shaderSetup->GetVertexDataRequirements ();
				}
			}

			// The unlit shader and far shader have to be supported, near shader is optional
			if (!tns->renderSetup[unlitShader.index] || !tns->renderSetup[farShader.index])
				throw runtime_error ("Unable to build full texture shader. ");

			tns->vertexDataReq = vda;
			tns->sortkey = key;
		}
		node->textureSetup = gi->nodesetup[key];

		if (!node->isLeaf()) {
			for (int a=0;a<4;a++)
				CreateTexProg (node->childs[a], gi);
		}

	}

	uint TerrainTexture::CalcBlendmapSortKey ()
	{
		uint key=0;
		uint mul=1;
		for (size_t a=0;a<blendMaps.size();a++) {
			key += mul * (uint)blendMaps[a]->curAreaResult;
			mul *= 3; // every blendmap::curAreaResult has 3 different states
		}
		return key;
	}

	void TerrainTexture::InstantiateShaders(Config *cfg, ILoadCallback *cb)
	{
		nearShader.def = shaderDef;
		if (cfg->useStaticShadow) {
			nearShader.def.stages.push_back (ShaderDef::Stage());
			ShaderDef::Stage& st = nearShader.def.stages.back();
			st.operation = ShaderDef::Mul;
			st.source = lightmap;
			nearShader.def.hasLighting = true;
		}
		shaders.push_back (&nearShader);
	}

	bool TerrainTexture::SetupNode (TQuad *q, int pass)
	{
		RenderSetupCollection *ns = q->textureSetup;
		if (debugShader>=0) ns = texNodeSetup[debugShader];

		RenderSetup *rs = 0;

		if (!q->renderData)
			return false;

		//assert (q->renderData);

/*		if (unlit) {
			rs = ns->renderSetup [unlitShader.index];
		} else {
			if (rd->normalMap)  {
				detailBumpmap.node = q;
				detailBumpmap.id = rd->normalMap;
				rs = ns->renderSetup [farShader.index];
			} else
				rs = ns->renderSetup [nearShader.index];
		}*/
		rs = ns->renderSetup [nearShader.index];

		if (pass >= rs->passes.size ())
			return false;

		bool r = true;
		if (rs != currentRenderSetup) {
			RenderPass *p = &rs->passes [pass];
			r = SetupShading (p, pass);

			ns->currentShaderSetup = p->shaderSetup;
			currentRenderSetup = rs;
		}

		if (q->renderData->normalMap) {
			int imageUnit, coordUnit;
			ns->currentShaderSetup->GetTextureUnits (&detailBumpmap, imageUnit, coordUnit);

			glActiveTextureARB(GL_TEXTURE0_ARB+imageUnit);
			glBindTexture(GL_TEXTURE_2D, q->renderData->normalMap);
			glActiveTextureARB(GL_TEXTURE0_ARB+coordUnit);
			detailBumpmap.SetupTexGen ();
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		return r;
	}


	bool TerrainTexture::SetupShading(RenderPass* p, int passIndex) {
		NodeSetupParams parms;
		parms.blendmaps = &blendMaps;
		parms.textures = &textures;
		parms.wsLightDir = &wsLightDir;
		parms.wsEyePos = &wsEyePos;
		parms.pass = passIndex;
		parms.shadowMapParams = shadowMapParams;

		//glDepthMask(p->depthWrite ? GL_TRUE : GL_FALSE);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);

        bool blend=true;
		switch (p->operation) {
		case Pass_Mul:
			glBlendFunc (GL_ZERO, GL_SRC_COLOR); // color=src*0+dst*src
			break;

		case Pass_Add:
			glBlendFunc (GL_ONE, GL_ONE); // color=dst*1+src*1
			break;

		case Pass_Replace:
			blend=false;
			break;

		case Pass_Interpolate:      // interpolate dest and src using src alpha
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case Pass_MulColor:
			glBlendFunc (GL_ZERO, GL_SRC_COLOR); // color=src*0+dst*src
			shaderHandler->EndTexturing();
			glEnable(GL_BLEND);
			return true;
		}

		if (blend)
			glEnable (GL_BLEND);

		return shaderHandler->SetupShader (p->shaderSetup, parms);
	}


	void TerrainTexture::EndTexturing() {
		currentRenderSetup = 0;
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		shaderHandler->EndTexturing ();
	}
	void TerrainTexture::BeginTexturing() {
		/*
		static bool z_last=false, x_last;
		if(keys[SDLK_z] && !z_last)
			DebugEvent("t_prev_shader");
		if(keys[SDLK_x] && !x_last)
			DebugEvent("t_next_shader");
		z_last=!!keys[SDLK_z];
		x_last=!!keys[SDLK_x];
		*/

		shaderHandler->BeginTexturing();
	}

	int TerrainTexture::NumPasses() {
		return maxPasses;
	}


	void TerrainTexture::EndPass() {
		currentRenderSetup = 0;
		shaderHandler->EndPass();
	}
	void TerrainTexture::BeginPass(int p) {
		shaderHandler->BeginPass(blendMaps, textures, p);
	}

	void TerrainTexture::SetShaderParams (const Vector3& lightDir, const Vector3& eyePos)
	{
		wsLightDir = lightDir;
		wsLightDir.ANormalize ();
		wsEyePos = eyePos;
	}

	void TerrainTexture::SetShadowMapParams(const ShadowMapParams* smp) {
		if (shadowMapParams)
			*shadowMapParams = *smp;
	}

	void TerrainTexture::DebugEvent (const std::string& event)
	{
		if (event == "t_prev_shader") {
			debugShader--;
			if (debugShader<-1) debugShader=texNodeSetup.size()-1;
		}
		if (event == "t_next_shader") {
			debugShader ++;
			if (debugShader==texNodeSetup.size())
				debugShader=-1;
		}
	}

#ifndef TERRAINRENDERERLIB_EXPORTS
	void TerrainTexture::DebugPrint (IFontRenderer *fontRenderer)
	{
		fontRenderer->printf(0,75,16.0f,"Numpasses: %d, curshader=%d, texture units: %d, sundir(%1.3f,%1.3f,%1.3f)", maxPasses, debugShader, shaderHandler->MaxTextureUnits (), wsLightDir.x,wsLightDir.y,wsLightDir.z);
	}
#endif

	void ShaderDef::LoadStages(int numStages,const char *stagename, const TdfParser& tdf, std::vector<ShaderDef::Stage>& stages)
	{
		for (int a=0;a<numStages;a++)
		{
			string path = "map\\terrain\\";
			char num[10];
			SNPRINTF(num, 10, "%d", a);

			string ts = path + stagename + num + "\\";

			string opstr = tdf.SGetValueDef("mul", ts + "operation");
			struct { StageOp op; const char *str; } tbl[] =
			{
				{ Mul, "mul" },
				{ Add, "add" },
				{ Alpha, "alpha" },
				{ Blend, "blend" },
				{ Mul, 0 },
			};

			StageOp operation = Mul;
			for (int i = 0; tbl[i].str; i++)
			{
				if (opstr == tbl[i].str) {
					operation = tbl[i].op;
					break;
				}
			}

			if (operation == Blend)
			{
				// insert an alpha stage before the blend stage
				stages.push_back(Stage());
				stages.back().sourceName = tdf.SGetValueDef(string(), ts + "blender");
				stages.back().operation = Alpha;
			}

			stages.push_back(Stage());
			stages.back().sourceName = tdf.SGetValueDef(string(), ts + "source");
			stages.back().operation = operation;
			if (stages.back().sourceName.empty())
				throw content_error(ts + " does not have a source texture");
		}
	}

	void ShaderDef::Parse(const TdfParser& tdf, bool needNormalMap) {
		string path = "map\\terrain\\";

		int numStages = atoi(tdf.SGetValueDef("0", path + "NumTextureStages").c_str());
		bool autoBumpMap = !!atoi(tdf.SGetValueDef("1", path + "AutoBumpmapStages").c_str());
		bool autoSpecular = !!atoi(tdf.SGetValueDef("1", path + "AutoSpecularStages").c_str());
		specularExponent = atof(tdf.SGetValueDef("8", path  + "SpecularExponent").c_str());

		LoadStages(numStages, "texstage", tdf, stages);

		if (needNormalMap) {
			// generate the bumpmap stages from the texture stages?
			if (autoBumpMap) {
				for (uint a = 0; a < stages.size(); a++) {
					Stage& st = stages[a];
					if (a && st.operation != Alpha && st.operation != Blend)
						continue;

					normalMapStages.push_back(Stage());

					Stage& bmst = normalMapStages.back();
					bmst.operation = st.operation;
					bmst.sourceName = st.sourceName;
				}
			}
			else {
				// otherwise load them from the bmstage list
				numStages = atoi(tdf.SGetValueDef("0", path + "NumBumpmapStages").c_str());
				LoadStages(numStages, "bmstage", tdf, normalMapStages);
			}
		}
	}

	void ShaderDef::Optimize(ShaderDef* dst)
	{
		OptimizeStages(stages,dst->stages);
		OptimizeStages(normalMapStages,dst->normalMapStages);
		dst->hasLighting = hasLighting;
		dst->specularExponent = specularExponent;
		dst->useShadowMapping = useShadowMapping;
	}

	void ShaderDef::OptimizeStages(std::vector<Stage>& src, std::vector<Stage>& dst)
	{
		AlphaImage::AreaTestResult atr = AlphaImage::AREA_ONE;

		// create an optimized shader definition based on replacing the blendmap with a constant
		for (size_t a=0;a<src.size();a++)
		{
			Stage &st = src[a];
			if (a == 0) atr = AlphaImage::AREA_MIXED;
			switch (st.operation) {
				case Blend: // previous * (1-alpha) + source * alpha
					if (atr == AlphaImage::AREA_MIXED)
						dst.push_back (st);
					else if(atr == AlphaImage::AREA_ONE)
					{
						// only source remains
						dst.clear();
						dst.push_back (st);
						dst.back().operation = Mul;
					}
					else if (atr == AlphaImage::AREA_ZERO)
					{
						 // only previous, remove the alpha channel
						for(int i=dst.size()-1; i>=0 && dst[i].operation == Alpha; i--)
							dst.pop_back();
					}
					break;
				case Alpha: {
					// remove redundant alpha stages
					for(int i=dst.size()-1;i>=0 && dst[i].operation == Alpha;i--)
						dst.pop_back();

					Blendmap* bm = dynamic_cast<Blendmap*>(st.source);
					if (bm) atr = bm->curAreaResult;
					else atr = AlphaImage::AREA_MIXED;
					dst.push_back (st);
					break;}
				case Add:
				case Mul:
					dst.push_back(st);
					break;
			}
		}
	}

	void ShaderDef::Output()
	{
		const char *opstr[] = { "add", "mul", "alpha" ,"blend" };
		d_trace ("Shader: %lu stages.\n", stages.size());
		for (size_t a=0;a<stages.size();a++){
			d_trace ("%lu\toperation=%s\n",a, opstr[(int)stages[a].operation]);
			d_trace ("\tsource=%s\n", stages[a].sourceName.c_str());
		}
	}
};
