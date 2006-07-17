/*---------------------------------------------------------------------
 Terrain Renderer using texture splatting and geomipmapping

 Copyright (2006) Jelmer Cnossen
 This code is released under GPL license (See LICENSE.html for info)
---------------------------------------------------------------------*/

#include "StdAfx.h"

#include "TdfParser.h"

#include <cassert>
#include <deque>

#include "TerrainBase.h"
#include "Terrain.h"
#include "TerrainTexture.h"

#include "TerrainTexEnvCombine.h"
//#include "TerrainTextureGLSL.h"
#include "Lightcalc.h"

namespace terrain {

	using namespace std;

//-----------------------------------------------------------------------
// RenderSetup
//-----------------------------------------------------------------------

	RenderSetup::~RenderSetup()
	{
		for (int a=0;a<passes.size();a++)
			delete passes[a].shaderSetup;
	}

	void RenderSetup::DebugOutput()
	{
		for (int a=0;a<passes.size();a++) 
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
			d_trace ("Pass (%d): %s, %s. Shader: %s\n", a, str, p.invertAlpha ? "invertalpha" : "", shaderstr.c_str());
		}
		d_trace ("\n");
	}

//-----------------------------------------------------------------------
// RenderSetupCollection
//-----------------------------------------------------------------------

	RenderSetupCollection::~RenderSetupCollection()
	{
		for (int a=0;a<renderSetup.size();a++)
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
	}

	TerrainTexture::~TerrainTexture ()
	{
		delete lightmap;
		for (int a=0;a<texNodeSetup.size();a++)
			delete texNodeSetup[a];

		delete shaderHandler;
		texNodeSetup.clear();

		// free all blendmaps
		for (int a=0;a<blendMaps.size();a++)
			delete blendMaps[a];
		blendMaps.clear();
		// free all textures
		for (int a=0;a<textures.size();a++)
			delete textures[a];
		textures.clear();
	}

	void TerrainTexture::LoadBumpmaps (Config *cfg)
	{
		for (int a=0;a<textures.size();a++)
		{
			TiledTexture *tt = textures[a];
			if (tt->bumpMapName.empty ())
				continue;

			uint texid = LoadTexture (tt->bumpMapName, true);

			if (texid)
			{
				if (cfg->anisotropicFiltering > 0.0f)
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cfg->anisotropicFiltering);

				tt->bumpMap = new BaseTexture;
				tt->bumpMap->name = tt->name + "_BM";
				tt->bumpMap->tilesize = tt->tilesize;
				tt->bumpMap->id = texid;
			}
		}
	}

	struct TerrainTexture::GenerateInfo
	{
		deque<AlphaImage*>* bmMipmaps;
		vector<int> testlod; // the level 

		// set of texture node setup objects, sorted by blend sort key
		map<uint, RenderSetupCollection*> nodesetup;
	};

	BaseTexture* TerrainTexture::LoadImageSource(const std::string& name, const std::string& basepath, Heightmap *heightmap, ILoadCallback *cb, Config* cfg)
	{
		BaseTexture *btex = 0;
		if (!!atoi(tdfParser->SGetValueDef("0", basepath + name + "\\Blendmap").c_str()))
		{
			Blendmap *bm = new Blendmap;
			bm->Load (name, basepath + name, heightmap, cb, tdfParser);
			blendMaps.push_back (bm);
			btex = bm;
		}
		else // a regular texturemap?
		{
			TiledTexture *tex = new TiledTexture;
			tex->Load (name, basepath + name, cb, tdfParser);
			textures.push_back (tex);
			btex = tex;
		}

		if (cfg->anisotropicFiltering > 0.0f) {
			glBindTexture(GL_TEXTURE_2D, btex->id);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cfg->anisotropicFiltering);
		}
		return btex;
	}

	void TerrainTexture::Load (TdfParser *tdf, Heightmap *heightmap, TQuad *quadtree, const vector<QuadMap*>& qmaps, Config *cfg, ILoadCallback *cb, LightingInfo *li)
	{
		string basepath="MAP\\TERRAIN\\";

		if (cb) cb->PrintMsg ("  parsing color expression...");

		if (!GLEW_ARB_multitexture)
			throw std::runtime_error ("No multitexture avaiable");
		if (!GLEW_ARB_texture_env_combine)
			throw std::runtime_error ("Texture env combine extension not avaiable");

		heightmapW = heightmap->w;
		tdfParser = tdf;

		shaderDef.Parse(*tdf);

		// Load textures
		map<string, BaseTexture*> nametbl;
		int texUnit=0;

		if (cb) cb->PrintMsg ("  loading textures and blendmaps...");
		for (int a=0;a<shaderDef.stages.size();a++)
		{
			ShaderDef::Stage* st = &shaderDef.stages [a];
			// Already loaded?
			if (nametbl.find (st->sourceName) == nametbl.end()) 
				st->source = nametbl[st->sourceName] = LoadImageSource(st->sourceName, basepath, heightmap, cb, cfg);
		}

		// Generate blendmap mipmaps
		deque<AlphaImage*>* bmMipmaps = new deque<AlphaImage*>[blendMaps.size()];
		GenerateInfo gi;
		gi.bmMipmaps = bmMipmaps;

		if (cb) cb->PrintMsg ("  generating blendmap mipmaps...");

		for (int a=0;a<blendMaps.size();a++)
		{
			Blendmap *bm = blendMaps[a];

			AlphaImage *cur = bm->image;
			bm->image = 0;

			do {
				bmMipmaps[a].push_front (cur);
				cur = cur->CreateMipmap ();
			} while (cur);

			for (int c=0;c<bmMipmaps[a].size();c++)
				if (bmMipmaps[a][c]->w == QUAD_W) {
					gi.testlod.push_back (c);
					break;
				}
		}

		if (cb) cb->PrintMsg ("  loading blendmaps into OpenGL...");
		
		// Convert to textures
		for (int a=0;a<blendMaps.size();a++) 
		{
			AlphaImage *bm = bmMipmaps[a].back();

			// Save image
			if (blendMaps[a]->generatorInfo)
			{
				char fn[32];
				SNPRINTF (fn,32, "blendmap%d.jpg", a);
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
			for (int d=0;d<mipmaps.size();d++) {
				AlphaImage *lod = mipmaps[mipmaps.size()-d-1];
				glTexImage2D (GL_TEXTURE_2D, d, GL_ALPHA, lod->w, lod->h, 0, GL_ALPHA, GL_FLOAT, lod->data);
			}
			blendMaps[a]->image = 0;
		}

		// Create texture application object
/*		if (GLEW_ARB_fragment_shader && GLEW_ARB_vertex_shader && 
			GLEW_ARB_shader_objects && GLEW_ARB_shading_language_100 && !forceFallback) {
			shaderHandler = new GLSLShaderHandler;
		} else {*/
			// texture_env_combine as fallback
			shaderHandler = new TexEnvSetupHandler;
		//}

		// Calculate shadows
		if (cfg->useStaticShadow) {
			if (cb) cb->PrintMsg("  calculating lightmap");
			lightmap = new Lightmap(heightmap, 1, li);
		}

		// see how lighting should be implemented, based on config and avaiable textures
		InstantiateShaders(cfg, cb);

        if (cb) cb->PrintMsg ("  initializing terrain node shaders...");

		CreateTexProg (quadtree, &gi);

		maxPasses = 0;
		for (map<uint, RenderSetupCollection*>::iterator mi=gi.nodesetup.begin();mi!=gi.nodesetup.end();++mi)
		{
			for (int i = 0; i < mi->second->renderSetup.size(); i++) {
				RenderSetup *rs = mi->second->renderSetup[i];

				if (rs->passes.size () > maxPasses)
					maxPasses = rs->passes.size();

				rs->shaderDef.Output();
				rs->DebugOutput();
			}
			texNodeSetup.push_back (mi->second);
		}

        if (cb) cb->PrintMsg ("  deleting temporary blendmap data...");

		// Free blendmap mipmap images
		for (int a=0;a<blendMaps.size();a++) {
			for (deque<AlphaImage*>::iterator i=bmMipmaps[a].begin();i!=bmMipmaps[a].end();++i)
				delete *i;
		}
	    
		delete[] bmMipmaps;
	}

	void TerrainTexture::CreateTexProg (TQuad *node, TerrainTexture::GenerateInfo *gi)
	{
		// Test blendmaps
		for (int a=0;a<blendMaps.size();a++) {
			deque <AlphaImage*>& mipmaps = gi->bmMipmaps[a];
			int mipIndex = node->depth + gi->testlod[a];

			if (mipIndex >= mipmaps.size ()) 
				mipIndex = mipmaps.size () - 1;

			// scaling divisor, needed because the blendmap maybe lower res than the corresponding part of heightmap
			int d = ( (1<<node->depth) * QUAD_W ) / mipmaps[mipIndex]->w;

			// Calculate "constants" representing the blendmap in the area that the node covers
			blendMaps[a]->curAreaResult = mipmaps[mipIndex]->TestArea ((node->qmPos.x * QUAD_W)/d, 
				(node->qmPos.y * QUAD_W)/d, ((node->qmPos.x+1)*QUAD_W)/d, ((node->qmPos.y+1)*QUAD_W)/d);
		}

		uint key = CalcBlendmapSortKey ();
		if (gi->nodesetup.find (key) == gi->nodesetup.end()) {
			RenderSetupCollection* tns = gi->nodesetup[key] = new RenderSetupCollection;

			tns->renderSetup.resize (shaders.size());
			uint vda = 0;

			// create a rendersetup for every shader expression
			for (int a=0;a<shaders.size();a++)
			{
				RenderSetup *rs = tns->renderSetup [a] = new RenderSetup;

//				shaders[a]->def.Optimize(&rs->shaderDef);
				rs->shaderDef = shaders[a]->def;
				shaderHandler->BuildNodeSetup(&rs->shaderDef, rs);

				for (int p=0;p<tns->renderSetup [a]->passes.size();p++) {
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
		for (int a=0;a<blendMaps.size();a++) {
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

	bool TerrainTexture::SetupNode (TQuad *q, int pass, bool unlit)
	{
		RenderSetupCollection *ns = q->textureSetup;
		if (debugShader>=0) ns = texNodeSetup[debugShader];

		RenderSetup *rs = 0;

		assert (q->renderData);

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

	bool TerrainTexture::SetupShading (RenderPass *p, int passIndex)
	{
		NodeSetupParams parms;
		parms.blendmaps = &blendMaps;
		parms.textures = &textures;
		parms.wsLightVec = &wsLightVec;
		parms.wsEyeVec = &wsEyeVec;
		parms.pass = passIndex;

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
			shaderHandler->CleanupStates();
			glEnable(GL_BLEND);
			return true;
		}

		if (blend)
			glEnable (GL_BLEND);

		return shaderHandler->SetupShader (p->shaderSetup, parms);
	}

	void TerrainTexture::CleanupTexturing ()
	{
		currentRenderSetup = 0;
		glDisable (GL_BLEND);

		shaderHandler->CleanupStates ();
	}

	int TerrainTexture::NumPasses ()
	{
		return maxPasses;
	}

	void TerrainTexture::EndPass()
	{
		currentRenderSetup = 0;
	}

	void TerrainTexture::SetShaderParams (const Vector3& lightVec, const Vector3& eyeVec)
	{
		wsLightVec = lightVec;
		wsLightVec.Normalize ();
		wsEyeVec = eyeVec;
		wsEyeVec.Normalize ();
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
		fontRenderer->printf(0,75,16.0f,"Numpasses: %d, curshader=%d, texture units: %d, sundir(%1.3f,%1.3f,%1.3f)", maxPasses, debugShader, shaderHandler->MaxTextureUnits (), wsLightVec.x,wsLightVec.y,wsLightVec.z);
	}
#endif

	void ShaderDef::Parse(TdfParser& tdf)
	{
		string path = "map\\terrain\\";

		int numStages = atoi(tdf.SGetValueDef("0", path + "NumTextureStages").c_str());
		bool autoBumpMap = !!atoi(tdf.SGetValueDef("0", path + "AutoBumpmapStages").c_str());

		for (int a=0;a<numStages;a++)
		{
			char num[10];
			itoa(a, num, 10);
			
			string ts = path + "texstage" + num + "\\";

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
		}

		// generate the bumpmap stages from the texture stages?
		if (autoBumpMap)
		{

		}
		else { 
			// otherwise load them from the bmstage list
		}
	}

	void ShaderDef::Optimize(ShaderDef* dst)
	{
		AlphaImage::AreaTestResult atr = AlphaImage::AREA_ONE;

		// create an optimized shader definition based on replacing the blendmap with a constant
		for (int a=0;a<stages.size();a++) 
		{
			Stage &st = stages[a];
			switch (st.operation) {
				case Blend: // previous * (1-alpha) + source * alpha
					if (atr == AlphaImage::AREA_MIXED) 
						dst->stages.push_back (st);
					else if(atr == AlphaImage::AREA_ONE) {
						// only source remains
						dst->stages.clear();
						dst->stages.push_back (st);
						dst->stages.back().operation = Mul;
					}
					else if (atr == AlphaImage::AREA_ZERO) 
					{
						 // only previous, remove the alpha channel
						for(int i=dst->stages.size()-1; i>=0 && dst->stages[i].operation == Alpha; i--)
							dst->stages.pop_back();
					}
					break;
				case Alpha: {
					// remove redundant alpha stages
					for(int i=dst->stages.size()-1;i>=0 && dst->stages[i].operation == Alpha;i--)
						dst->stages.pop_back();

					Blendmap* bm = dynamic_cast<Blendmap*>(st.source);
					if (bm) {
						atr = bm->curAreaResult;
						dst->stages.push_back (st);
					}
					break;}
				case Add:
				case Mul:
					dst->stages.push_back(st);
					break;
			}
		}
		dst->hasLighting = hasLighting;
	}

	void ShaderDef::Output()
	{
		const char *opstr[] = { "add", "mul", "alpha" ,"blend" };
		d_trace ("Shader: %d stages.\n", stages.size());
		for (int a=0;a<stages.size();a++){
			d_trace ("%d\toperation=%s\n",a, opstr[(int)stages[a].operation]);
			d_trace ("\tsource=%s\n", stages[a].sourceName.c_str());
		}
	}

	void ShaderDef::GetTextureUsage(TextureUsage* tu)
	{
		set<BaseTexture*> texUnits;
		set<BaseTexture*> texCoordUnits;

		for(int a=0;a<stages.size();a++) {
			texUnits.insert(stages[a].source);

			if (stages[a].source->ShareTexCoordUnit()) {
				set<BaseTexture*>::iterator i=texCoordUnits.begin();
				for (; i != texCoordUnits.end(); i++)
					if ((*i)->tilesize.x == stages[a].source->tilesize.x && (*i)->tilesize.y == stages[a].source->tilesize.y)
						break;

				if (i == texCoordUnits.end())
					texCoordUnits.insert(stages[a].source);

			} else texCoordUnits.insert(stages[a].source);
		}
		tu->texUnits.resize(texUnits.size());
		copy(texUnits.begin(), texUnits.end(), tu->texUnits.begin());
		tu->coordUnits.resize(texCoordUnits.size());
		copy(texCoordUnits.begin(), texCoordUnits.end(), tu->coordUnits.begin());
	}
};
