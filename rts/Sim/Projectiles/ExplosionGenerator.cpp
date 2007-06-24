#include "StdAfx.h"
#include <fstream>
#include <stdexcept>
#include <SDL_types.h>
#include "ExplosionGenerator.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Projectiles/HeatCloudProjectile.h"
#include "Sim/Projectiles/SmokeProjectile2.h"
#include "Sim/Projectiles/WreckProjectile.h"
#include "Rendering/GroundFlash.h"
#include "Sim/Projectiles/SpherePartProjectile.h"
#include "Sim/Projectiles/BubbleProjectile.h"
#include "Sim/Projectiles/WakeProjectile.h"
#include "Game/Camera.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/DirtProjectile.h"
#include "Sim/Projectiles/ExploSpikeProjectile.h"
#include "LogOutput.h"
#include "FileSystem/FileHandler.h"
#include "creg/VarTypes.h"
#include "Rendering/Textures/ColorMap.h"
#include "mmgr.h"
#include "Platform/ConfigHandler.h"

using namespace std;

CR_BIND_DERIVED_INTERFACE(CExpGenSpawnable, CWorldObject);

CR_REG_METADATA(CExpGenSpawnable, 
				);

CExplosionGeneratorHandler* explGenHandler=0;
// -------------------------------------------------------------------------------
// ClassAliasList: Finds C++ classes with class aliases
// -------------------------------------------------------------------------------


ClassAliasList::ClassAliasList() {}

void ClassAliasList::Load(TdfParser& parser, const string& location)
{
	const map<string,string>& lst = parser.GetAllValues (location);

	aliases.insert(lst.begin(), lst.end());
}

creg::Class* ClassAliasList::GetClass (const std::string& name)
{
	std::string n = name;
	for (;;) {
		map<string,string>::iterator i = aliases.find(n);
		if (i == aliases.end())
			break;

		n = i->second;
	}
	creg::Class *cls = creg::System::GetClass (n);
	if (!cls)
		throw content_error("Unknown class: " + name);
	return cls;
}

std::string ClassAliasList::FindAlias (const std::string& className)
{
	for (map<string,string>::iterator i = aliases.begin(); i != aliases.end(); ++i)
		if (i->second == className) return i->first;
	return className;
}


// -------------------------------------------------------------------------------
// Explosion generator handler: loads and stores a list of explosion generators
// -------------------------------------------------------------------------------

CExplosionGeneratorHandler::CExplosionGeneratorHandler()
{
 	TdfParser aliasParser("gamedata/explosion_alias.tdf");
	projectileClasses.Load(aliasParser, "projectiles");
	generatorClasses.Load(aliasParser, "generators");

	vector<string> files = CFileHandler::FindFiles("gamedata/explosions/", "*.tdf");

	for(unsigned int i=0; i<files.size(); i++)
	{
		try {
			parser.LoadFile(files[i]);
		}catch( TdfParser::parse_error const& e) {
			logOutput.Print ("Exception: %s\n",e.what());
		} // non-parse exceptions are handled the normal way
	}
}

CExplosionGenerator* CExplosionGeneratorHandler::LoadGenerator (const std::string& tag)
{
	std::string klass;
	std::string::size_type seppos = tag.find(':');
	if (seppos == std::string::npos)
		klass = tag;
	else
		klass = tag.substr(0, seppos);

	creg::Class *cls = generatorClasses.GetClass (klass);
	if (!cls->IsSubclassOf (CExplosionGenerator::StaticClass()))
		throw content_error(klass + " is not a subclass of CExplosionGenerator");

	CExplosionGenerator* eg = (CExplosionGenerator *)cls->CreateInstance();

	if (seppos != std::string::npos)
		eg->Load (this, tag.substr(seppos+1));

	return eg;
}

// -------------------------------------------------------------------------------
// Base explosion generator class
// -------------------------------------------------------------------------------
CR_BIND_INTERFACE(CExplosionGenerator);

CExplosionGenerator::CExplosionGenerator()
{}

CExplosionGenerator::~CExplosionGenerator()
{}


// -------------------------------------------------------------------------------
// Default explosion generator: everything is calculated from damage and radius
// -------------------------------------------------------------------------------

CR_BIND_DERIVED(CStdExplosionGenerator, CExplosionGenerator, );

CStdExplosionGenerator::CStdExplosionGenerator()
{}

CStdExplosionGenerator::~CStdExplosionGenerator()
{}

void CStdExplosionGenerator::Load (CExplosionGeneratorHandler *h, const std::string& tag)
{}

void CStdExplosionGenerator::Explosion(const float3 &pos, float damage, float radius, CUnit *owner,float gfxMod, CUnit *hit, const float3 &dir)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	float h2=ground->GetHeight2(pos.x,pos.z);

	float height=pos.y-h2;
	if(height<0)
		height=0;

	bool waterExplosion=h2<-3;
	bool uwExplosion=pos.y<-15;
	bool airExplosion=pos.y-max((float)0,h2)>20;

	damage=damage/20;
	if(damage>radius*1.5f) //limit the visual effects based on the radius
		damage=radius*1.5f;
	damage*=gfxMod;
	for(int a=0;a<1;++a){
//		float3 speed((gs->randFloat()-0.5f)*(radius*0.04f),0.05f+(gs->randFloat())*(radius*0.007f),(gs->randFloat()-0.5f)*(radius*0.04f));
		float3 speed(0,0.3f,0);
		float3 camVect=camera->pos-pos;
		float camLength=camVect.Length();
		camVect/=camLength;
		float moveLength=radius*0.03f;
		if(camLength<moveLength+2)
			moveLength=camLength-2;
		float3 npos=pos+camVect*moveLength;

		SAFE_NEW CHeatCloudProjectile(npos,speed,8+sqrt(damage)*0.5f,7+damage*2.8f,owner);
	}
	if(ph->particleSaturation<1){		//turn off lots of graphic only particles when we have more particles than we want
		float smokeDamage=damage;
		if(uwExplosion)
			smokeDamage*=0.3f;
		if(airExplosion || waterExplosion)
			smokeDamage*=0.6f;
		float invSqrtsmokeDamage=1/(sqrt(smokeDamage)*0.35f);
		for(int a=0;a<smokeDamage*0.6f;++a){
			float3 speed(-0.1f+gu->usRandFloat()*0.2f,(0.1f+gu->usRandFloat()*0.3f)*invSqrtsmokeDamage,-0.1f+gu->usRandFloat()*0.2f);
			float3 npos(pos+gu->usRandVector()*(smokeDamage*1.0f));
			float h=ground->GetApproximateHeight(npos.x,npos.z);
			if(npos.y<h)
				npos.y=h;
			float time=(40+sqrt(smokeDamage)*15)*(0.8f+gu->usRandFloat()*0.7f);
			SAFE_NEW CSmokeProjectile2(pos,npos,speed,time,sqrt(smokeDamage)*4,0.4f,owner,0.6f);
		}
		if(!airExplosion && !uwExplosion && !waterExplosion){
			int numDirt=(int)min(20.f,damage*0.8f);
			float3 color(0.15f,0.1f,0.05f);
			for(int a=0;a<numDirt;++a){
				float3 speed((0.5f-gu->usRandFloat())*1.5f,1.7f+gu->usRandFloat()*1.6f,(0.5f-gu->usRandFloat())*1.5f);
				speed*=0.7f+min((float)30,damage)/30;
				float3 npos(pos.x-(0.5f-gu->usRandFloat())*(radius*0.6f),pos.y-2.0f-damage*0.2f,pos.z-(0.5f-gu->usRandFloat())*(radius*0.6f));
				SAFE_NEW CDirtProjectile(npos,speed,90+damage*2,2.0f+sqrt(damage)*1.5f,0.4f,0.999f,owner,color);
			}
		}
		if(!airExplosion && !uwExplosion && waterExplosion){
			int numDirt=(int)min(40.f,damage*0.8f);
			float3 color(1,1,1);
			for(int a=0;a<numDirt;++a){
				float3 speed((0.5f-gu->usRandFloat())*0.2f,a*0.1f+gu->usRandFloat()*0.8f,(0.5f-gu->usRandFloat())*0.2f);
				speed*=0.7f+min((float)30,damage)/30;
				float3 npos(pos.x-(0.5f-gu->usRandFloat())*(radius*0.2f),pos.y-2.0f-sqrt(damage)*2.0f,pos.z-(0.5f-gu->usRandFloat())*(radius*0.2f));
				SAFE_NEW CDirtProjectile(npos,speed,90+damage*2,2.0f+sqrt(damage)*2.0f,0.3f,0.99f,owner,color);
			}
		}
		if(damage>=20 && !uwExplosion && !airExplosion){
			int numDebris=gu->usRandInt()%6;
			if(numDebris>0)
				numDebris+=3+(int)(damage*0.04f);
			for(int a=0;a<numDebris;++a){
				float3 speed;
				if(height<4)
					speed=float3((0.5f-gu->usRandFloat())*2.0f,1.8f+gu->usRandFloat()*1.8f,(0.5f-gu->usRandFloat())*2.0f);
				else
					speed=float3(gu->usRandVector()*2);
				speed*=0.7f+min((float)30,damage)/23;
				float3 npos(pos.x-(0.5f-gu->usRandFloat())*(radius*1),pos.y,pos.z-(0.5f-gu->usRandFloat())*(radius*1));
				SAFE_NEW CWreckProjectile(npos,speed,90+damage*2,owner);
			}
		}
		if(uwExplosion){
			int numBubbles=(int)(damage*0.7f);
			for(int a=0;a<numBubbles;++a){
				SAFE_NEW CBubbleProjectile(pos+gu->usRandVector()*radius*0.5f,gu->usRandVector()*0.2f+float3(0,0.2f,0),damage*2+gu->usRandFloat()*damage,1+gu->usRandFloat()*2,0.02f,owner,0.5f+gu->usRandFloat()*0.3f);
			}
		}
		if(waterExplosion && !uwExplosion && !airExplosion){
			int numWake=(int)(damage*0.5f);
			for(int a=0;a<numWake;++a){
				SAFE_NEW CWakeProjectile(pos+gu->usRandVector()*radius*0.2f,gu->usRandVector()*radius*0.003f,sqrt(damage)*4,damage*0.03f,owner,0.3f+gu->usRandFloat()*0.2f,0.8f/(sqrt(damage)*3+50+gu->usRandFloat()*90),1);
			}
		}
		if(radius>10 && damage>4){
			int numSpike=(int)sqrt(damage)+8;
			for(int a=0;a<numSpike;++a){
				float3 speed=gu->usRandVector();
				speed.Normalize();
				speed*=(8+damage*3.0f)/(9+sqrt(damage)*0.7f)*0.35f;
				if(!airExplosion && !waterExplosion && speed.y<0)
					speed.y=-speed.y;
				SAFE_NEW CExploSpikeProjectile(pos+speed,speed*(0.9f+gu->usRandFloat()*0.4f),radius*0.1f,radius*0.1f,0.6f,0.8f/(8+sqrt(damage)),owner);
			}
		}
	}

	if(radius>20 && damage>6 && height<radius*0.7f){
		float modSize=max(radius,damage*2);
		float circleAlpha=0;
		float circleGrowth=0;
		float ttl=8+sqrt(damage)*0.8f;
		if(radius>40 && damage>12){
			circleAlpha=min(0.5f,damage*0.01f);
			circleGrowth=(8+damage*2.5f)/(9+sqrt(damage)*0.7f)*0.55f;
		}
		float flashSize=modSize;
		float flashAlpha=min(0.8f,damage*0.01f);
		SAFE_NEW CStandardGroundFlash(pos,circleAlpha,flashAlpha,flashSize,circleGrowth,ttl);
	}

	if(radius>40 && damage>12){
		CSpherePartProjectile::CreateSphere(pos,min(0.7f,damage*0.02f),5+(int)(sqrt(damage)*0.7f),(8+damage*2.5f)/(9+sqrt(damage)*0.7f)*0.5f,owner);
	}
	POP_CODE_MODE;
}

// -------------------------------------------------------------------------------
// CCustomExplosionGenerator: Uses explosion info from a TDF file
// -------------------------------------------------------------------------------

CR_BIND_DERIVED(CCustomExplosionGenerator, CStdExplosionGenerator, );

#define SPW_WATER 1
#define SPW_GROUND 2
#define SPW_AIR 4
#define SPW_UNDERWATER 8
#define SPW_UNIT 16 // only execute when the explosion hits a unit
#define SPW_NO_UNIT 32 // only execute when the explosion doesn't hit a unit (environment)

CCustomExplosionGenerator::CCustomExplosionGenerator()
{
	groundFlash = 0;
}

CCustomExplosionGenerator::~CCustomExplosionGenerator()
{
	if (groundFlash)
		delete groundFlash;
}

#define OP_END    0
#define OP_STOREI 1 // int
#define OP_STOREF 2 // float
#define OP_STOREC 3 // char
#define OP_ADD    4
#define OP_RAND   5
#define OP_DAMAGE 6
#define OP_INDEX  7
#define OP_LOADP  8 // load a void* into the pointer register
#define OP_STOREP 9 // store the pointer register into a void*
#define OP_DIR	  10 //stor the float3 direction

void CCustomExplosionGenerator::ExecuteExplosionCode (const char *code, float damage, char *instance, int spawnIndex,const float3 &dir)
{
	float val = 0.0f;
	void* ptr = NULL;

	for (;;) {
		switch (*(code++)) {
		case OP_END:
			return;
		case OP_STOREI:{
			Uint16 offset = *(Uint16*)code;
			code += 2;
			*(int*)(instance + offset) = (int) val;
			val = 0.0f;
			break;}
		case OP_STOREF:{
			Uint16 offset = *(Uint16*)code;
			code += 2;
			*(float*)(instance + offset) = val;
			val = 0.0f;
			break;}
		case OP_STOREC:{
			Uint16 offset = *(Uint16*)code;
			code += 2;
			*(unsigned char*)(instance + offset) = (int) val;
			val = 0.0f;
			break;}
		case OP_ADD:
			val += *(float*)code;
			code += 4;
			break;
		case OP_RAND:
			val += gu->usRandFloat () * *(float*)code;
			code += 4;
			break;
		case OP_DAMAGE:
			val += damage * *(float*)code;
			code += 4;
			break;
		case OP_INDEX:
			val += spawnIndex * *(float*)code;
			code += 4;
			break;
		case OP_LOADP:{
			ptr = *(void**)code;
			code += sizeof(void*);
			break;}
		case OP_STOREP:{
			Uint16 offset = *(Uint16*)code;
			code += 2;
			*(void**)(instance + offset) = ptr;
			ptr = NULL;
			break;}
		case OP_DIR:{
			Uint16 offset = *(Uint16*)code;
			code += 2;
			*(float3*)(instance + offset) = dir;
			break;}
		default:
			assert(false);
			break;
		}
	}
}

void CCustomExplosionGenerator::ParseExplosionCode(
	CCustomExplosionGenerator::ProjectileSpawnInfo *psi,
	int offset, creg::IType *type, const std::string& script, std::string& code)
{

	string::size_type end = script.find(';', 0);
	std::string vastr = script.substr(0, end);


	if(vastr == "dir") //first see if we can match any keywords
	{
		//if the user uses a keyword assume he knows that it is put on the right datatype for now
		code += OP_DIR;
		Uint16 ofs = offset;
		code.append ((char*)&ofs, (char*)&ofs + 2);
	}
	else if (dynamic_cast<creg::BasicType*>(type)) {
		creg::BasicType *bt = (creg::BasicType*)type;

		if (bt->id != creg::crInt && bt->id != creg::crFloat && bt->id != creg::crUChar && bt->id != creg::crBool)
		{
			throw content_error("Projectile properties other than int, float and uchar, are not supported (" + script + ")");
			return;
		}

		int p = 0;
		while (p < script.length())
		{
			char opcode;
			char c;
			do { c = script[p++]; } while(c == ' ');
			if (c=='i')	opcode = OP_INDEX;
			else if (c=='r') opcode = OP_RAND;
			else if (c=='d') opcode = OP_DAMAGE;
			else if (isdigit(c)||c=='.'||c=='-') { opcode = OP_ADD; p--; }
			else throw content_error ("Explosion script error: \"" + script + "\"  : \'" + string(1, c) + "\' is unknown opcode.");

			char *endp;
			float v = (float)strtod(&script[p], &endp);
			p += endp - &script[p];
			code += opcode;
			code.append ((char*)&v, ((char*)&v) + 4);
		}

		switch (bt->id) {
			case creg::crInt: code.push_back (OP_STOREI); break;
			case creg::crBool: code.push_back (OP_STOREI); break;
			case creg::crFloat: code.push_back (OP_STOREF); break;
			case creg::crUChar: code.push_back (OP_STOREC); break;
			default:
				throw content_error ("Explosion script variable is of unsupported type. "
					"Contact the Spring team to fix this.");
					break;
		}
		Uint16 ofs = offset;
		code.append ((char*)&ofs, (char*)&ofs + 2);
	}
	else if(dynamic_cast<creg::ObjectInstanceType*>(type))
	{
		creg::ObjectInstanceType *oit = (creg::ObjectInstanceType *)type;

		string::size_type start = 0;
		for(creg::Class* c = oit->objectClass; c; c=c->base) {
			for(int a=0;a<c->members.size();a++) {
				string::size_type end = script.find(',', start+1);
				ParseExplosionCode(psi, offset + c->members [a]->offset, c->members[a]->type, script.substr(start,end-start), code);
				start = end+1;
				if (start >= script.length()) break;
			}
			if (start >= script.length()) break;
		}
	}
	else if(dynamic_cast<creg::StaticArrayBaseType*>(type))
	{
		creg::StaticArrayBaseType *sat = (creg::StaticArrayBaseType*)type;

		string::size_type start = 0;
		for (unsigned int i=0; i < sat->size; i++) {
			string::size_type end = script.find(',', start+1);
			ParseExplosionCode(psi, offset + sat->elemSize * i, sat->elemType, script.substr(start,end-start), code);
			start = end+1;
			if (start >= script.length()) break;
		}
	}
	else
	{
		if(type->GetName()=="AtlasedTexture*")
		{
			string::size_type end = script.find(';', 0);
			std::string texname = script.substr(0, end);
			void* tex = ph->textureAtlas->GetTexturePtr(texname);
			code += OP_LOADP;
			code.append((char*)(&tex), ((char*)(&tex)) + sizeof(void*));
			code += OP_STOREP;
			Uint16 ofs = offset;
			code.append ((char*)&ofs, (char*)&ofs + 2);
		}
		else if(type->GetName()=="GroundFXTexture*")
		{
			string::size_type end = script.find(';', 0);
			std::string texname = script.substr(0, end);
			void* tex = ph->groundFXAtlas->GetTexturePtr(texname);
			code += OP_LOADP;
			code.append((char*)(&tex), ((char*)(&tex)) + sizeof(void*));
			code += OP_STOREP;
			Uint16 ofs = offset;
			code.append ((char*)&ofs, (char*)&ofs + 2);
		}
		else if(type->GetName()=="CColorMap*")
		{
			string::size_type end = script.find(';', 0);
			std::string colorstring = script.substr(0, end);
			void* colormap = CColorMap::LoadFromDefString(colorstring);
			code += OP_LOADP;
			code.append((char*)(&colormap), ((char*)(&colormap)) + sizeof(void*));
			code += OP_STOREP;
			Uint16 ofs = offset;
			code.append ((char*)&ofs, (char*)&ofs + 2);
		}
		else if(type->GetName()=="CExplosionGenerator*")
		{
			string::size_type end = script.find(';', 0);
			std::string name = script.substr(0, end);
			void* explgen = explGenHandler->LoadGenerator(name);
			code += OP_LOADP;
			code.append((char*)(&explgen), ((char*)(&explgen)) + sizeof(void*));
			code += OP_STOREP;
			Uint16 ofs = offset;
			code.append ((char*)&ofs, (char*)&ofs + 2);
		}
	}
}

void CCustomExplosionGenerator::Load (CExplosionGeneratorHandler *h, const std::string& tag)
{
	TdfParser& parser = h->GetParser ();

	if (!parser.SectionExist (tag))
		throw content_error ("Explosion info for " + tag + " not found.");

	vector<string> spawns = parser.GetSectionList (tag);
	ProjectileSpawnInfo *psi;
	for (vector<string>::iterator si = spawns.begin(); si != spawns.end(); ++si)
	{
		if (*si == "groundflash")
			continue;

		psi = SAFE_NEW ProjectileSpawnInfo;
		projectileSpawn.push_back (psi);

		std::string location = tag + "\\" + *si + "\\";

		std::string className = parser.SGetValueDef(*si, location + "class");
		psi->projectileClass = h->projectileClasses.GetClass (className);
		unsigned int flags = 0;
		if(!!atoi(parser.SGetValueDef ("0", location + "ground").c_str()))     flags |= SPW_GROUND;
		if(!!atoi(parser.SGetValueDef ("0", location + "water").c_str()))      flags |= SPW_WATER;
		if(!!atoi(parser.SGetValueDef ("0", location + "air").c_str()))        flags |= SPW_AIR;
		if(!!atoi(parser.SGetValueDef ("0", location + "underwater").c_str())) flags |= SPW_UNDERWATER;
		if(!!atoi(parser.SGetValueDef ("0", location + "unit").c_str())) flags |= SPW_UNIT;
		if(!!atoi(parser.SGetValueDef ("0", location + "nounit").c_str())) flags |= SPW_NO_UNIT;
		psi->flags = flags;
		psi->count = atoi(parser.SGetValueDef("1", location + "count").c_str());

		string code;
		const map<string,string>& props=parser.GetAllValues(location + "properties");
		for(map<string,string>::const_iterator i=props.begin();i!=props.end();++i) {
			creg::Class::Member *m = psi->projectileClass->FindMember(i->first.c_str());
			if (m && (m->flags & creg::CM_Config)) ParseExplosionCode(psi, m->offset, m->type, i->second, code);
		}

		code += (char)OP_END;
		psi->code.resize(code.size());
		copy(code.begin(), code.end(), psi->code.begin());
	}

	string location = tag + "\\groundflash\\";
	int ttl = atoi(parser.SGetValueDef ("0", location + "ttl").c_str());
	if (ttl) {
		groundFlash = SAFE_NEW GroundFlashInfo;
		groundFlash->circleAlpha = atof(parser.SGetValueDef ("0",location + "circleAlpha" ).c_str());
		groundFlash->flashSize = atof(parser.SGetValueDef ("0",location + "flashSize" ).c_str());
		groundFlash->flashAlpha = atof(parser.SGetValueDef ("0",location + "flashAlpha" ).c_str());
		groundFlash->circleGrowth = atof(parser.SGetValueDef ("0",location + "circleGrowth" ).c_str());
		groundFlash->color = parser.GetFloat3 (float3(1.0f,1.0f,1.0f), location + "color");

		unsigned int flags = SPW_GROUND;
		if(!!atoi(parser.SGetValueDef ("0", location + "ground").c_str()))     flags |= SPW_GROUND;
		if(!!atoi(parser.SGetValueDef ("0", location + "water").c_str()))      flags |= SPW_WATER;
		if(!!atoi(parser.SGetValueDef ("0", location + "air").c_str()))        flags |= SPW_AIR;
		if(!!atoi(parser.SGetValueDef ("0", location + "underwater").c_str())) flags |= SPW_UNDERWATER;
		if(!!atoi(parser.SGetValueDef ("0", location + "unit").c_str())) flags |= SPW_UNIT;
		if(!!atoi(parser.SGetValueDef ("0", location + "nounit").c_str())) flags |= SPW_NO_UNIT;
		groundFlash->flags = flags;

		string color = parser.SGetValueDef("1,1,0.8",location + "color");
		string::size_type start = 0;
		for (unsigned int i=0; i < 3; i++) {
			string::size_type end = color.find(',', start+1);
			groundFlash->color[i]=atof(color.substr(start,end-start).c_str());
			start = end+1;
			if (start >= color.length()) break;
		}
		groundFlash->ttl = ttl;
	}

	useDefaultExplosions = !!atoi(parser.SGetValueDef("0", tag + "\\UseDefaultExplosions").c_str());
}


void CCustomExplosionGenerator::Explosion(const float3 &pos, float damage, float radius, CUnit *owner,float gfxMod, CUnit *hit,const float3 &dir)
{
	float h2=ground->GetHeight2(pos.x,pos.z);

	unsigned int flags = 0;
	if (h2<-3) flags = SPW_WATER;
	else if (pos.y<-15) flags = SPW_UNDERWATER;
	else if (pos.y-max((float)0,h2)>20) flags = SPW_AIR;
	else flags = SPW_GROUND;
	if (hit) flags |= SPW_UNIT;
	else flags |= SPW_NO_UNIT;

	for (int a=0;a<projectileSpawn.size();a++)
	{
		ProjectileSpawnInfo *psi = projectileSpawn[a];

		if (!(psi->flags & flags))
			continue;

		for (int c=0;c<psi->count;c++)
		{
			CExpGenSpawnable *projectile = (CExpGenSpawnable*)psi->projectileClass->CreateInstance ();

			ExecuteExplosionCode (&psi->code[0], damage, (char*)projectile, c, dir);
			projectile->Init(pos, owner);
		}
	}

	if ((flags & SPW_GROUND) && groundFlash)
		SAFE_NEW CStandardGroundFlash(pos, groundFlash->circleAlpha, groundFlash->flashAlpha, groundFlash->flashSize, groundFlash->circleGrowth, groundFlash->ttl, groundFlash->color);

	if (useDefaultExplosions)
		CStdExplosionGenerator::Explosion(pos, damage, radius, owner, gfxMod, hit, dir);
}

void CCustomExplosionGenerator::OutputProjectileClassInfo()
{
	const vector<creg::Class*>& classes = creg::System::GetClasses();
	std::ofstream fs("projectiles.txt");
	CExplosionGeneratorHandler egh;

	if (fs.bad() || !fs.is_open())
		return;

	for (vector<creg::Class*>::const_iterator ci = classes.begin(); ci != classes.end(); ++ci)
	{
		if (!(*ci)->IsSubclassOf (CExpGenSpawnable::StaticClass()) || (*ci) == CExpGenSpawnable::StaticClass())
			continue;

		creg::Class *klass = *ci;
		fs << "Class: " << klass->name << ".  Scriptname: " << egh.projectileClasses.FindAlias (klass->name) << endl;
		for (;klass;klass=klass->base) {
			for (unsigned int a=0;a<klass->members.size();a++)
			{
				if (klass->members[a]->flags & creg::CM_Config)
					fs << "\t" << klass->members[a]->name << ": " << klass->members[a]->type->GetName() << "\n";
			}
		}
		fs << "\n\n";
	}
}
