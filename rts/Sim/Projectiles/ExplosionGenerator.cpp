#include "StdAfx.h"
#include <stdexcept>
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
#include "Game/UI/InfoConsole.h"
#include "FileSystem/FileHandler.h"
#include "creg/VarTypes.h"
#include <SDL_types.h>
#include <fstream>

using namespace std;

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
	creg::Class *cls = creg::ClassBinder::GetClass (n);
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

	vector<string> files = CFileHandler::FindFiles("gamedata/explosions/*.tdf");

	for(unsigned int i=0; i<files.size(); i++)
	{
		try {
			parser.LoadFile(files[i]);
		}catch( TdfParser::parse_error const& e) {
			info->AddLine ("Exception: %s\n",e.what());
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

CR_BIND_DERIVED(CStdExplosionGenerator, CExplosionGenerator);

CStdExplosionGenerator::CStdExplosionGenerator()
{}

CStdExplosionGenerator::~CStdExplosionGenerator()
{}

void CStdExplosionGenerator::Load (CExplosionGeneratorHandler *h, const std::string& tag)
{}

void CStdExplosionGenerator::Explosion(const float3 &pos, const DamageArray& damages, float radius, CUnit *owner,float gfxMod)
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

	float damage=damages[0]/20;
	if(damage>radius*1.5) //limit the visual effects based on the radius
		damage=radius*1.5;
	damage*=gfxMod;
	for(int a=0;a<1;++a){
//		float3 speed((gs->randFloat()-0.5f)*(radius*0.04),0.05+(gs->randFloat())*(radius*0.007),(gs->randFloat()-0.5)*(radius*0.04));
		float3 speed(0,0.3f,0);
		float3 camVect=camera->pos-pos;
		float camLength=camVect.Length();
		camVect/=camLength;
		float moveLength=radius*0.03;
		if(camLength<moveLength+2)
			moveLength=camLength-2;
		float3 npos=pos+camVect*moveLength;

		CHeatCloudProjectile* p=new CHeatCloudProjectile(npos,speed,8+sqrt(damage)*0.5,7+damage*2.8,owner);

		//p->Update();
		//p->maxheat=p->heat;
	}
	if(ph->particleSaturation<1){		//turn off lots of graphic only particles when we have more particles than we want
		float smokeDamage=damage;
		if(uwExplosion)
			smokeDamage*=0.3;
		if(airExplosion || waterExplosion)
			smokeDamage*=0.6;
		float invSqrtsmokeDamage=1/(sqrt(smokeDamage)*0.35);
		for(int a=0;a<smokeDamage*0.6;++a){
			float3 speed(-0.1f+gu->usRandFloat()*0.2,(0.1f+gu->usRandFloat()*0.3)*invSqrtsmokeDamage,-0.1f+gu->usRandFloat()*0.2);
			float3 npos(pos+gu->usRandVector()*(smokeDamage*1.0));
			float h=ground->GetApproximateHeight(npos.x,npos.z);
			if(npos.y<h)
				npos.y=h;
			float time=(40+sqrt(smokeDamage)*15)*(0.8+gu->usRandFloat()*0.7);
			new CSmokeProjectile2(pos,npos,speed,time,sqrt(smokeDamage)*4,0.4,owner,0.6);
		}
		if(!airExplosion && !uwExplosion && !waterExplosion){
			int numDirt=(int)min(20.,damage*0.8);
			float3 color(0.15,0.1,0.05);
			for(int a=0;a<numDirt;++a){
				float3 speed((0.5-gu->usRandFloat())*1.5,1.7f+gu->usRandFloat()*1.6,(0.5-gu->usRandFloat())*1.5);
				speed*=0.7+min((float)30,damage)/30;
				float3 npos(pos.x-(0.5-gu->usRandFloat())*(radius*0.6),pos.y-2.0-damage*0.2,pos.z-(0.5-gu->usRandFloat())*(radius*0.6));
				CDirtProjectile* dp=new CDirtProjectile(npos,speed,90+damage*2,2.0+sqrt(damage)*1.5,0.4,0.999f,owner,color);
			}
		}
		if(!airExplosion && !uwExplosion && waterExplosion){
			int numDirt=(int)min(40.,damage*0.8);
			float3 color(1,1,1);
			for(int a=0;a<numDirt;++a){
				float3 speed((0.5-gu->usRandFloat())*0.2,a*0.1+gu->usRandFloat()*0.8,(0.5-gu->usRandFloat())*0.2);
				speed*=0.7+min((float)30,damage)/30;
				float3 npos(pos.x-(0.5-gu->usRandFloat())*(radius*0.2),pos.y-2.0-sqrt(damage)*2.0,pos.z-(0.5-gu->usRandFloat())*(radius*0.2));
				CDirtProjectile* dp=new CDirtProjectile(npos,speed,90+damage*2,2.0+sqrt(damage)*2.0,0.3f,0.99f,owner,color);
			}
		}
		if(damage>=20 && !uwExplosion && !airExplosion){
			int numDebris=gu->usRandInt()%6;
			if(numDebris>0)
				numDebris+=3+(int)(damage*0.04);
			for(int a=0;a<numDebris;++a){
				float3 speed;
				if(height<4)
					speed=float3((0.5f-gu->usRandFloat())*2.0,1.8f+gu->usRandFloat()*1.8,(0.5f-gu->usRandFloat())*2.0);
				else
					speed=float3(gu->usRandVector()*2);
				speed*=0.7+min((float)30,damage)/23;
				float3 npos(pos.x-(0.5-gu->usRandFloat())*(radius*1),pos.y,pos.z-(0.5-gu->usRandFloat())*(radius*1));
				new CWreckProjectile(npos,speed,90+damage*2,owner);
			}
		}
		if(uwExplosion){
			int numBubbles=(int)(damage*0.7);
			for(int a=0;a<numBubbles;++a){
				new CBubbleProjectile(pos+gu->usRandVector()*radius*0.5,gu->usRandVector()*0.2+float3(0,0.2,0),damage*2+gu->usRandFloat()*damage,1+gu->usRandFloat()*2,0.02,owner,0.5+gu->usRandFloat()*0.3);
			}
		}
		if(waterExplosion && !uwExplosion && !airExplosion){
			int numWake=(int)(damage*0.5);
			for(int a=0;a<numWake;++a){
				new CWakeProjectile(pos+gu->usRandVector()*radius*0.2,gu->usRandVector()*radius*0.003,sqrt(damage)*4,damage*0.03,owner,0.3+gu->usRandFloat()*0.2,0.8/(sqrt(damage)*3+50+gu->usRandFloat()*90),1);
			}
		}
		if(radius>10 && damage>4){
			int numSpike=(int)sqrt(damage)+8;
			for(int a=0;a<numSpike;++a){
				float3 speed=gu->usRandVector();
				speed.Normalize();
				speed*=(8+damage*3.0)/(9+sqrt(damage)*0.7)*0.35;
				if(!airExplosion && !waterExplosion && speed.y<0)
					speed.y=-speed.y;
				new CExploSpikeProjectile(pos+speed,speed*(0.9f+gu->usRandFloat()*0.4f),radius*0.1,radius*0.1,0.6f,0.8/(8+sqrt(damage)),owner);
			}
		}
	}

	if(radius>20 && damage>6 && height<radius*0.7){
		float modSize=max(radius,damage*2);
		float circleAlpha=0;
		float circleGrowth=0;
		float ttl=8+sqrt(damage)*0.8;
		if(radius>40 && damage>12){
			circleAlpha=min(0.5,damage*0.01);
			circleGrowth=(8+damage*2.5)/(9+sqrt(damage)*0.7)*0.55;
		}
		float flashSize=modSize;
		float flashAlpha=min(0.8,damage*0.01);
		new CStandarGroundFlash(pos,circleAlpha,flashAlpha,flashSize,circleGrowth,ttl);
	}

	if(radius>40 && damage>12){
		CSpherePartProjectile::CreateSphere(pos,min(0.7,damage*0.02),5+(int)(sqrt(damage)*0.7),(8+damage*2.5)/(9+sqrt(damage)*0.7)*0.5,owner);	
	}
	POP_CODE_MODE;
}

// -------------------------------------------------------------------------------
// CCustomExplosionGenerator: Uses explosion info from a TDF file
// -------------------------------------------------------------------------------

CR_BIND_DERIVED(CCustomExplosionGenerator, CStdExplosionGenerator);

#define SPW_WATER 1
#define SPW_GROUND 2
#define SPW_AIR 4
#define SPW_UNDERWATER 8

CCustomExplosionGenerator::CCustomExplosionGenerator()
{
	groundFlash = 0;
}

CCustomExplosionGenerator::~CCustomExplosionGenerator()
{
	if (groundFlash)
		delete groundFlash;
}

#define OP_END 0
#define OP_STOREI 1
#define OP_STOREF 2
#define OP_STOREC 3
#define OP_ADD 4
#define OP_RAND 5
#define OP_DAMAGE 6
#define OP_INDEX 7

void CCustomExplosionGenerator::ExecuteExplosionCode (const char *code, float damage, char *instance, int spawnIndex)
{
	float val=0.0f;

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
		}
	}
}

void CCustomExplosionGenerator::ParseExplosionCode(
	CCustomExplosionGenerator::ProjectileSpawnInfo *psi, 
	int offset, creg::IType *type, const std::string& script, std::string& code)
{
	if (dynamic_cast<creg::BasicType*>(type)) {
		creg::BasicType *bt = (creg::BasicType*)type;

		if (bt->id != creg::crInt && bt->id != creg::crFloat && bt->id != creg::crUChar)
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
			double dv = strtod(&script[p], &endp);
			p += endp - &script[p];
			float v=dv;
			code += opcode;
			code.append ((char*)&v, ((char*)&v) + 4);
		}

		switch (bt->id) {
			case creg::crInt: code.push_back (OP_STOREI); break;
			case creg::crFloat: code.push_back (OP_STOREF); break;
			case creg::crUChar: code.push_back (OP_STOREC); break;
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

		psi = new ProjectileSpawnInfo;
		projectileSpawn.push_back (psi);

		std::string location = tag + "\\" + *si + "\\";

		std::string className = parser.SGetValueDef(*si, location + "class");
		psi->projectileClass = h->projectileClasses.GetClass (className);
		unsigned int flags = 0;
        if(!!atoi(parser.SGetValueDef ("0", location + "ground").c_str()))	flags |= SPW_GROUND;
        if(!!atoi(parser.SGetValueDef ("0", location + "water").c_str()))	flags |= SPW_WATER;
        if(!!atoi(parser.SGetValueDef ("0", location + "air").c_str()))	flags |= SPW_AIR;
        if(!!atoi(parser.SGetValueDef ("0", location + "underwater").c_str()))	flags |= SPW_UNDERWATER;
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
		groundFlash = new GroundFlashInfo;
		groundFlash->circleAlpha = atof(parser.SGetValueDef ("0",location + "circleAlpha" ).c_str());
		groundFlash->flashSize = atof(parser.SGetValueDef ("0",location + "flashSize" ).c_str());
		groundFlash->flashAlpha = atof(parser.SGetValueDef ("0",location + "flashAlpha" ).c_str());
		groundFlash->circleGrowth = atof(parser.SGetValueDef ("0",location + "circleGrowth" ).c_str());
		groundFlash->color = parser.GetFloat3 (float3(1.0f,1.0f,1.0f), location + "color");

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


void CCustomExplosionGenerator::Explosion(const float3 &pos, const DamageArray& damages, float radius, CUnit *owner,float gfxMod)
{
	float h2=ground->GetHeight2(pos.x,pos.z);

	unsigned int flags = 0;
	if (h2<-3) flags = SPW_WATER;
	else if (pos.y<-15) flags = SPW_UNDERWATER;
	else if (pos.y-max((float)0,h2)>20) flags = SPW_AIR;
	else flags = SPW_GROUND;
    
	for (int a=0;a<projectileSpawn.size();a++)
	{
		ProjectileSpawnInfo *psi = projectileSpawn[a];

		if (!(psi->flags & flags))
			continue;

		for (int c=0;c<psi->count;c++)
		{
			CProjectile *projectile = (CProjectile*)psi->projectileClass->CreateInstance ();

			ExecuteExplosionCode (&psi->code[0], damages.damages[0], (char*)projectile, c);
			projectile->Init(pos, owner);
		}
	}

	if ((flags & SPW_GROUND) && groundFlash)
		new CStandarGroundFlash(pos, groundFlash->circleAlpha, groundFlash->flashAlpha, groundFlash->flashSize, groundFlash->circleGrowth, groundFlash->ttl, groundFlash->color);

	if (useDefaultExplosions)
		CStdExplosionGenerator::Explosion(pos, damages, radius, owner, gfxMod);
}

void CCustomExplosionGenerator::OutputProjectileClassInfo()
{
	const vector<creg::Class*>& classes = creg::ClassBinder::GetClasses();
	std::ofstream fs;
	fs.open ("projectiles.txt", ios_base::out);
	CExplosionGeneratorHandler egh;

	for (vector<creg::Class*>::const_iterator ci = classes.begin(); ci != classes.end(); ++ci)
	{
		if (!(*ci)->IsSubclassOf (CProjectile::StaticClass()) || (*ci) == CProjectile::StaticClass())
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
