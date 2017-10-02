/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "System/TdfParser.h"

#include "TerrainBase.h"
#include "Terrain.h"
#include "TerrainTexture.h"

#include "TerrainTexEnvCombine.h"
#include "TerrainTextureGLSL.h"
#include "Lightcalc.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"

#include <cassert>
#include <deque>
#include <map>
#include <vector>
#include <algorithm>

#include <SDL_keysym.h>

namespace terrain {

//-----------------------------------------------------------------------
// RenderSetup
//-----------------------------------------------------------------------

	RenderSetup::~RenderSetup()
	{
		Clear();
	}

	void RenderSetup::Clear()
	{
		for (size_t a = 0; a < passes.size(); a++)
			delete passes[a].shaderSetup;
		passes.clear();
	}

	void RenderSetup::DebugOutput()
	{
		for (size_t a = 0; a < passes.size(); a++) {
			const char* str = 0;
			RenderPass& p = passes[a];

			if (p.operation == Pass_Mul)
				str="Pass_Mul";
			if (p.operation == Pass_Add)
				str="Pass_Add";
			if (p.operation == Pass_Interpolate)
				str="Pass_Interpolate";
			if (p.operation == Pass_Replace)
				str="Pass_Replace";

			std::string shaderstr = p.shaderSetup ? p.shaderSetup->GetDebugDesc() : std::string("none");
			LOG_L(L_DEBUG, "Pass (" _STPF_ "): %s, %s. Shader: %s",
					a, str, (p.invertAlpha ? "invertalpha" : ""),
					shaderstr.c_str());
		}
		LOG_L(L_DEBUG, "Shader passes finnished.");
	}

//-----------------------------------------------------------------------
// RenderSetupCollection
//-----------------------------------------------------------------------

	RenderSetupCollection::~RenderSetupCollection()
	{
		for (size_t a = 0; a < renderSetup.size(); a++)
			delete renderSetup[a];
	}

//-----------------------------------------------------------------------
// Terrain Texturing class
//-----------------------------------------------------------------------


	TerrainTexture::TerrainTexture():
		debugShader(-1),
		heightmapW(0),
		blendmapLOD(0),
		useBumpMaps(false),
		optimizeEpsilon(0.0f),
		currentRenderSetup(NULL),
		shaderHandler(NULL),
		maxPasses(0),
		tdfParser(NULL),
		lightmap(NULL),
		flatBumpmap(NULL),
		shadowMapParams(0)
	{
	}

	TerrainTexture::~TerrainTexture()
	{
		delete shadowMapParams;
		delete lightmap;
		for (size_t a = 0; a < texNodeSetup.size(); a++)
			delete texNodeSetup[a];

		delete shaderHandler;
		texNodeSetup.clear();

		// free all blendmaps
		for (size_t a = 0; a < blendMaps.size(); a++)
			delete blendMaps[a];
		blendMaps.clear();
		// free all textures
		for (size_t a = 0; a < textures.size(); a++)
			delete textures[a];
		textures.clear();
	}

	struct TerrainTexture::GenerateInfo
	{
		std::deque<AlphaImage*>* bmMipmaps;
		std::vector<int> testlod; // the level

		// set of texture node setup objects, sorted by blend sort key
		std::map<uint, RenderSetupCollection*> nodesetup;
	};

	BaseTexture* TerrainTexture::LoadImageSource(
		const std::string& name,
		const std::string& basepath,
		Heightmap* heightmap, ILoadCallback* cb,
		Config* cfg, bool isBumpmap)
	{
		BaseTexture* btex = NULL;
		if (!!atoi(tdfParser->SGetValueDef("0", basepath + name + "\\Blendmap").c_str()))
		{
			if (isBumpmap)
				throw content_error(name + " should be a bumpmap, not a blendmap.");

			Blendmap* bm = new Blendmap;
			bm->Load(name, basepath + name, heightmap, cb, tdfParser);
			blendMaps.push_back(bm);
			btex = bm;
		}
		else // a regular texturemap?
		{
			TiledTexture* tex = new TiledTexture;
			tex->Load(name, basepath + name, cb, tdfParser, isBumpmap);
			if (!tex->id) {
				delete tex;
				return 0;
			}
			textures.push_back(tex);
			btex = tex;
		}

		if (cfg->anisotropicFiltering > 0.0f) {
			glBindTexture(GL_TEXTURE_2D, btex->id);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cfg->anisotropicFiltering);
		}
		return btex;
	}

	void TerrainTexture::Load(const TdfParser* tdf, Heightmap* heightmap,
			TQuad* quadtree, const std::vector<QuadMap*>& qmaps, Config* cfg,
			ILoadCallback* cb, LightingInfo* li)
	{
		std::string basepath = "MAP\\TERRAIN\\";

		if (cb) cb->PrintMsg("  parsing texture stages...");

		heightmapW = heightmap->w;
		tdfParser = tdf;

		shaderDef.Parse(*tdf, cfg->useBumpMaps);
		shaderDef.useShadowMapping = cfg->useShadowMaps;

		optimizeEpsilon = atof(tdf->SGetValueDef("0.04", basepath + "LayerOptimizeConst").c_str());
		if (optimizeEpsilon < 0.0f) optimizeEpsilon = 0.0f;

		// Load textures
		std::map<std::string, BaseTexture*> nametbl;

		if (cb) cb->PrintMsg("  loading textures and blendmaps...");
		bool hasBumpmaps = false;
		for (size_t a=0;a<shaderDef.stages.size();a++)
		{
			ShaderDef::Stage* st = &shaderDef.stages [a];
			// Already loaded?
			if (nametbl.find(st->sourceName) == nametbl.end())
				nametbl[st->sourceName] = LoadImageSource(st->sourceName, basepath, heightmap, cb, cfg);
			st->source = nametbl[st->sourceName];

			std::string bm;
			if (tdf->SGetValue(bm, basepath + st->sourceName + "\\Bumpmap"))
				hasBumpmaps = true;
		}

		if (cfg->useBumpMaps && hasBumpmaps) {
			for (size_t a = 0; a < shaderDef.normalMapStages.size(); a++) {
				ShaderDef::Stage* st = &shaderDef.normalMapStages [a];

				std::string name = st->sourceName;
				if (st->operation != ShaderDef::Alpha)
					name += "_BM"; // make it unique, this whole naming thing is very hackish though..

				// Already loaded?
				if (nametbl.find(name) == nametbl.end()) {
					// load a bumpmap unless it's an alpha operation
					st->source = LoadImageSource(st->sourceName, basepath,
						heightmap, cb, cfg, st->operation != ShaderDef::Alpha);
					if (!st->source) {
						if (!flatBumpmap) {
							flatBumpmap = TiledTexture::CreateFlatBumpmap();
							textures.push_back(flatBumpmap);
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
		std::deque<AlphaImage*>* bmMipmaps = new std::deque<AlphaImage*>[blendMaps.size()];
		GenerateInfo gi;
		gi.bmMipmaps = bmMipmaps;

		if (cb) { cb->PrintMsg("  generating blendmap mipmaps..."); }

		for (size_t a=0;a<blendMaps.size();a++) {
			Blendmap* bm = blendMaps[a];

			AlphaImage* cur = bm->image;
			bm->image = 0;

			do {
				bmMipmaps[a].push_front(cur);
				cur = cur->CreateMipmap();
			} while (cur);

			for (size_t c=0;c<bmMipmaps[a].size();c++)
				if (bmMipmaps[a][c]->w == QUAD_W) {
					gi.testlod.push_back((int)c);
					break;
				}
		}

		if (cb) cb->PrintMsg("  loading blendmaps into OpenGL...");

		// Convert to textures
		for (size_t a = 0; a < blendMaps.size(); a++) {
			AlphaImage* bm = bmMipmaps[a].back();

			// Save image
			if (blendMaps[a]->generatorInfo) {
				char fn[32];
				SNPRINTF(fn,32, "blendmap" _STPF_ ".jpg", a);
				remove(fn);
				bm->Save(fn);
			}
			// Upload
			glGenTextures(1, &blendMaps[a]->id);
			glBindTexture(GL_TEXTURE_2D, blendMaps[a]->id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			if (cfg->anisotropicFiltering > 0.0f)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cfg->anisotropicFiltering);

			std::deque<AlphaImage*>& mipmaps = bmMipmaps[a];
			for (size_t d = 0; d < mipmaps.size(); d++) {
				AlphaImage* lod = mipmaps[mipmaps.size()-d-1];
				glTexImage2D(GL_TEXTURE_2D, (int)d, GL_ALPHA, lod->w, lod->h, 0, GL_ALPHA, GL_FLOAT, lod->data);
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

		// see how lighting should be implemented, based on config and available textures
		InstantiateShaders(cfg, cb);

		if (cb) { cb->PrintMsg("  initializing terrain node shaders..."); }

		shaderHandler->BeginBuild();
		CreateTexProg(quadtree, &gi);
		shaderHandler->EndBuild();

		// count passes
		maxPasses = 0;
		for (std::map<uint, RenderSetupCollection*>::iterator mi=gi.nodesetup.begin();mi!=gi.nodesetup.end();++mi) {
			for (size_t i = 0; i < mi->second->renderSetup.size(); i++) {
				RenderSetup* rs = mi->second->renderSetup[i];

				if (rs->passes.size() > maxPasses) {
					maxPasses = rs->passes.size();
				}
			}
			texNodeSetup.push_back(mi->second);
		}

		if (cb) cb->PrintMsg("  deleting temporary blendmap data...");

		// Free blendmap mipmap images
		for (size_t a = 0; a < blendMaps.size(); a++) {
			for (std::deque<AlphaImage*>::iterator i = bmMipmaps[a].begin(); i != bmMipmaps[a].end(); ++i) {
				delete *i;
			}
		}

		delete[] bmMipmaps;

		shadowMapParams = new ShadowMapParams;
	}

	void TerrainTexture::ReloadShaders(TQuad* quadtree, Config* cfg)
	{
		GenerateInfo gi;

		// This are the configuration keys that are re-read on shader reload.
		// Changes of any other configuration keys are ignored.
		shaderDef.useShadowMapping = cfg->useShadowMaps;

		shaderHandler->BeginBuild();
		ReloadTexProg(quadtree, &gi);
		shaderHandler->EndBuild();

		// FIXME: count passes again?
	}

	void TerrainTexture::ReloadTexProg(TQuad* node, TerrainTexture::GenerateInfo* gi)
	{
		RenderSetupCollection* tns = node->textureSetup;

		if (gi->nodesetup.find(tns->sortkey) == gi->nodesetup.end()) {
			gi->nodesetup[tns->sortkey] = tns;

			for (size_t a = 0; a < shaders.size(); a++) {
				RenderSetup* rs = tns->renderSetup [a];

				// see ReloadShaders
				rs->shaderDef.useShadowMapping = shaderDef.useShadowMapping;

				rs->Clear();
				shaderHandler->BuildNodeSetup(&rs->shaderDef, rs);

				// FIXME: check whether vertex data requirements remain the same?
			}
		}

		if (!node->isLeaf()) {
			for (int a=0;a<4;a++)
				ReloadTexProg(node->children[a], gi);
		}
	}

	void TerrainTexture::CreateTexProg(TQuad* node, TerrainTexture::GenerateInfo* gi)
	{
		// Test blendmaps
		for (size_t a = 0; a < blendMaps.size(); a++) {
			std::deque<AlphaImage*>& mipmaps = gi->bmMipmaps[a];
			int mipIndex = node->depth + gi->testlod[a];

			if (mipIndex >= mipmaps.size())
				mipIndex = mipmaps.size() - 1;

			// scaling divisor, needed because the blendmap maybe lower res than the corresponding part of heightmap
			int d = ( (1<<node->depth) * QUAD_W ) / mipmaps[mipIndex]->w;

			// Calculate "constants" representing the blendmap in the area that the node covers
			blendMaps[a]->curAreaResult = mipmaps[mipIndex]->TestArea((node->qmPos.x * QUAD_W)/d,
				(node->qmPos.y * QUAD_W)/d, ((node->qmPos.x+1)*QUAD_W)/d, ((node->qmPos.y+1)*QUAD_W)/d, optimizeEpsilon);
		}

		uint key = CalcBlendmapSortKey();
		if (gi->nodesetup.find(key) == gi->nodesetup.end()) {
			RenderSetupCollection* tns = gi->nodesetup[key] = new RenderSetupCollection;

			tns->renderSetup.resize(shaders.size());
			uint vda = 0;

			// create a rendersetup for every shader expression
			for (size_t a = 0; a < shaders.size(); a++) {
				RenderSetup* rs = tns->renderSetup [a] = new RenderSetup;

				shaders[a]->def.Optimize(&rs->shaderDef);
				shaderHandler->BuildNodeSetup(&rs->shaderDef, rs);

				for (size_t p=0;p<tns->renderSetup [a]->passes.size();p++) {
					if(tns->renderSetup [a]->passes[p].shaderSetup)
						vda |= tns->renderSetup [a]->passes[p].shaderSetup->GetVertexDataRequirements();
				}
			}

			// The unlit shader and far shader have to be supported, near shader is optional
			if (!tns->renderSetup[unlitShader.index] || !tns->renderSetup[farShader.index])
				throw std::runtime_error("Unable to build full texture shader. ");

			tns->vertexDataReq = vda;
			tns->sortkey = key;
		}
		node->textureSetup = gi->nodesetup[key];

		if (!node->isLeaf()) {
			for (int a = 0; a < 4; a++)
				CreateTexProg(node->children[a], gi);
		}

	}

	uint TerrainTexture::CalcBlendmapSortKey()
	{
		uint key=0;
		uint mul=1;
		for (size_t a = 0; a < blendMaps.size(); a++) {
			key += mul * (uint)blendMaps[a]->curAreaResult;
			mul *= 3; // every blendmap::curAreaResult has 3 different states
		}
		return key;
	}

	void TerrainTexture::InstantiateShaders(Config* cfg, ILoadCallback* cb)
	{
		nearShader.def = shaderDef;
		if (cfg->useStaticShadow) {
			nearShader.def.stages.push_back(ShaderDef::Stage());
			ShaderDef::Stage& st = nearShader.def.stages.back();
			st.operation = ShaderDef::Mul;
			st.source = lightmap;
			nearShader.def.hasLighting = true;
		}
		shaders.push_back(&nearShader);
	}

	bool TerrainTexture::SetupNode(TQuad* q, int pass)
	{
		RenderSetupCollection* ns = q->textureSetup;
		if (debugShader >= 0) ns = texNodeSetup[debugShader];

		RenderSetup* rs = NULL;

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

		if (pass >= rs->passes.size())
			return false;

		bool r = true;
		if (rs != currentRenderSetup) {
			RenderPass* p = &rs->passes [pass];
			r = SetupShading(p, pass);

			ns->currentShaderSetup = p->shaderSetup;
			currentRenderSetup = rs;
		}

		if (q->renderData->normalMap) {
			int imageUnit, coordUnit;
			ns->currentShaderSetup->GetTextureUnits(&detailBumpmap, imageUnit, coordUnit);

			glActiveTextureARB(GL_TEXTURE0_ARB+imageUnit);
			glBindTexture(GL_TEXTURE_2D, q->renderData->normalMap);
			glActiveTextureARB(GL_TEXTURE0_ARB+coordUnit);
			detailBumpmap.SetupTexGen();
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
			glBlendFunc(GL_ZERO, GL_SRC_COLOR); // color=src*0+dst*src
			break;

		case Pass_Add:
			glBlendFunc(GL_ONE, GL_ONE); // color=dst*1+src*1
			break;

		case Pass_Replace:
			blend=false;
			break;

		case Pass_Interpolate:      // interpolate dest and src using src alpha
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case Pass_MulColor:
			glBlendFunc(GL_ZERO, GL_SRC_COLOR); // color=src*0+dst*src
			shaderHandler->EndTexturing();
			glEnable(GL_BLEND);
			return true;
		}

		if (blend)
			glEnable(GL_BLEND);

		return shaderHandler->SetupShader(p->shaderSetup, parms);
	}


	void TerrainTexture::EndTexturing() {
		currentRenderSetup = 0;
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		shaderHandler->EndTexturing();
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

	int TerrainTexture::NumPasses() const {
		return maxPasses;
	}


	void TerrainTexture::EndPass() {
		currentRenderSetup = 0;
		shaderHandler->EndPass();
	}
	void TerrainTexture::BeginPass(int p) {
		shaderHandler->BeginPass(blendMaps, textures, p);
	}

	void TerrainTexture::SetShaderParams(const Vector3& lightDir, const Vector3& eyePos)
	{
		wsLightDir = lightDir;
		wsLightDir.ANormalize();
		wsEyePos = eyePos;
	}

	void TerrainTexture::SetShadowMapParams(const ShadowMapParams* smp) {
		if (shadowMapParams)
			*shadowMapParams = *smp;
	}

	void TerrainTexture::DebugEvent(const std::string& event)
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
	void TerrainTexture::DebugPrint(IFontRenderer* fontRenderer)
	{
		if (fontRenderer != NULL) {
			fontRenderer->printf(0, 75, 16.0f,
					"Numpasses: %d, curshader=%d, texture units: %d, sundir(%1.3f,%1.3f,%1.3f)",
					maxPasses, debugShader, shaderHandler->MaxTextureUnits(),
					wsLightDir.x, wsLightDir.y, wsLightDir.z);
		}
	}
#endif

	void ShaderDef::LoadStages(int numStages,const char* stagename, const TdfParser& tdf, std::vector<ShaderDef::Stage>& stages)
	{
		for (int a = 0; a < numStages; a++)
		{
			std::string path = "map\\terrain\\";
			char num[10];
			SNPRINTF(num, 10, "%d", a);

			std::string ts = path + stagename + num + "\\";

			std::string opstr = tdf.SGetValueDef("mul", ts + "operation");
			struct { StageOp op; const char* str; } tbl[] =
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
				stages.back().sourceName = tdf.SGetValueDef(std::string(), ts + "blender");
				stages.back().operation = Alpha;
			}

			stages.push_back(Stage());
			stages.back().sourceName = tdf.SGetValueDef(std::string(), ts + "source");
			stages.back().operation = operation;
			if (stages.back().sourceName.empty())
				throw content_error(ts + " does not have a source texture");
		}
	}

	void ShaderDef::Parse(const TdfParser& tdf, bool needNormalMap) {
		std::string path = "map\\terrain\\";

		int numStages = atoi(tdf.SGetValueDef("0", path + "NumTextureStages").c_str());
		bool autoBumpMap = !!atoi(tdf.SGetValueDef("1", path + "AutoBumpmapStages").c_str());
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
						dst.push_back(st);
					else if(atr == AlphaImage::AREA_ONE)
					{
						// only source remains
						dst.clear();
						dst.push_back(st);
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
					dst.push_back(st);
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
		const char* opstr[] = { "add", "mul", "alpha" ,"blend" };
		LOG("Shader: " _STPF_ " stages.", stages.size());
		for (size_t a = 0; a < stages.size(); a++){
			LOG(_STPF_ "\toperation=%s", a, opstr[(int)stages[a].operation]);
			LOG("\tsource=%s", stages[a].sourceName.c_str());
		}
	}
};
