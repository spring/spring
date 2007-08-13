//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "BaseAIDef.h"
#include "BuildTable.h"
#include "Sim/Weapons/WeaponDefHandler.h"

#define BT_CACHE_VERSION 10

BuildTable buildTable;

// ----------------------------------------------------------------------------------------
// Build table construction
// ----------------------------------------------------------------------------------------

BuildTable::BuildTable()
{
	deflist=0;
	numDefs=0;
	buildby=0;
}

BuildTable::~BuildTable()
{
	if (deflist)
		delete[] deflist;

	if (buildby)
		delete[] buildby;
}


void BuildTable::CalcBuildTable (int cur, int endbuild, int depth, ResInfo res)
{
	const UnitDef* d = deflist [cur].def;
	res.buildtime += d->buildTime;
	res.energy += d->energyCost;
	res.metal += d->metalCost;

	float score = CalcScore(depth,res);

	std::vector<int> &bb=buildby[cur];
	for (std::vector<int>::iterator a = bb.begin(); a!=bb.end(); ++a) // for every unit that can build cur
	{
		BuildTable::Table::ent& ent = table.get (*a, endbuild);

		float entscore = CalcScore (ent.depth, ent.res);

		if (ent.id < 0 || entscore > score)
		{
			ent.id = cur;
			ent.depth = depth;
			ent.res = res;

			CalcBuildTable(*a, endbuild, depth+1, res);
		}
	}
}

static float CalcAverageDamage (const DamageArray *da)
{
	//float s = 0.0f;

	//for (int a=0;a<DamageArray::numTypes;a++)
	//	s += da->damages [a];

	/*numTypes isn't accessible from the dll, so only one damage value can be used */
	return da->GetDefaultDamage();//s/DamageArray::numTypes;
}

void BuildTable::Init (IAICallback *callback, bool cache)
{
	cb = callback;

	char cachefn[80];
	ReplaceExtension (cb->GetModName (), cachefn, sizeof(cachefn), "modcache");
	if (cache) {
		if (LoadCache (cachefn))
			return;
	}

	// this is what makes loading times long, because spring will load unit defs only when they're needed.
	// meaning, GetUnitDefs() will make spring load all the unit defs at once.
	numDefs = cb->GetNumUnitDefs();
	deflist = new UDef[numDefs];

	const UnitDef** templist=new const UnitDef*[numDefs];
	cb->GetUnitDefList (templist);
	for (int a=0;a<numDefs;a++)
	{
		UDef& d = deflist[a];
		d.def = templist[a];
		d.cost.energy = d.def->energyCost;
		d.cost.metal = d.def->metalCost;
		d.make.energy = d.def->energyMake;
		d.make.metal = d.def->metalMake;
		if (d.def->activateWhenBuilt)
		{ // Solars have negative energy upkeep so they can be disabled
			d.make.energy -= d.def->energyUpkeep;
			d.make.metal += d.def->makesMetal;
			d.make.metal -= d.def->metalUpkeep;
		}
		d.storage.energy = d.def->energyStorage;
		d.storage.metal = d.def->metalStorage;
		d.metalExtractDepth = d.def->extractsMetal;
		d.buildTime = d.def->buildTime;
		d.energyUse = d.def->energyUpkeep;
		d.buildSpeed = d.def->buildSpeed;
		d.numBuildOptions = d.def->buildOptions.size();

		d.flags = 0;
		if (!d.def->buildOptions.empty ()) d.flags |= CUD_Builder;
		if (d.def->type == "Building" || d.def->type == "Factory" || d.def->type == "MetalExtractor")
			d.flags |= CUD_Building;
		if (d.def->windGenerator) d.flags |= CUD_WindGen;
		if (d.def->movedata)  {
			d.moveType = d.def->movedata->moveType;
			d.flags |= CUD_MoveType;
		}
		
		deflist[a].def = templist[a];
		deflist[a].name = templist[a]->name;

		d.weaponDamage = 0.0f;
		d.weaponRange = 0.0f;

		for (int b=0;b<d.def->weapons.size();b++)
		{
			const WeaponDef* w = d.def->weapons[b].def;

			if (d.weaponRange < w->range)
				d.weaponRange = w->range;

			d.weaponDamage += CalcAverageDamage (&w->damages);
		}
	}
	delete[] templist;

    table.alloc(numDefs);

	logPrintf ("Calculating build table for %d build types...\n", numDefs);

	// convert all string'ed identifiers to id's, which can be used for indexing directly
	buildby = new std::vector<int>[numDefs]; 

	for (int a=0;a<numDefs;a++)
	{
		const UnitDef *def=deflist[a].def;
		
		for (map<int,string>::const_iterator i=def->buildOptions.begin();i!=def->buildOptions.end();++i)
		{
			const UnitDef *bdef = cb->GetUnitDef (i->second.c_str());
			if (bdef)
				buildby[bdef->id-1].push_back(a);
		}

		deflist[a].buildby = &buildby [a];
	}

	for (int a=0;a<numDefs;a++)
		CalcBuildTable (a, a, 0, ResInfo());

	SaveCache (cachefn);

/*	for (int a=0;a<numDefs;a++)
	{
		logPrintf ("%s builds:", deflist[a].name.c_str());

		for (int b=0;b<numDefs;b++)
		{
			BuildTable::Table::ent& e=table.get(a,b);
			if (e.id>=0)
				logPrintf ("%s by building %s (%d), ", deflist[b].name.c_str(), deflist[e.id].name.c_str(), e.depth);
		}

		logPrintf ("\n");
	}*/
}


static inline string ReadZStr(FILE *f)
{
	string s;
	int c,i=0;
	while((c = fgetc(f)) != EOF && c != '\0') 
		s += c;
	return s;
}


static inline bool WriteZStr(FILE *f, const string& s)
{
	int c;
	c = s.length ();
	return fwrite(&s[0],c+1,1,f)==1;
}

void BuildTable::SaveCache (const char *fn)
{
	FILE *f;

	f = fopen (fn, "wb");
	if (!f) {
		logPrintf ("BuildTable::SaveCache(): Can't open %s for saving\n", fn);
		return;
	}

	fputc (BT_CACHE_VERSION, f);

	bool err=false;
	do {
		if (fwrite (&numDefs, sizeof(int), 1, f) != 1) {
			err=true;
			break;
		}

		for (int a=0;a<numDefs;a++)
		{
			if (!WriteZStr (f, deflist[a].name))
			{
				err=true;
				break;
			}

			UDef *d = &deflist[a];
			fwrite (&d->cost, sizeof(ResourceInfo), 1, f);
			fwrite (&d->make, sizeof(ResourceInfo), 1, f);
			fwrite (&d->storage, sizeof(ResourceInfo), 1, f);
			fwrite (&d->buildTime, sizeof(float),1 , f);
			fwrite (&d->metalExtractDepth, sizeof(float), 1, f);
			fwrite (&d->energyUse, sizeof(float), 1,f);
			fwrite (&d->buildSpeed, sizeof(float), 1,f);
			fwrite (&d->weaponDamage, sizeof(float),1,f);
			fwrite (&d->weaponRange, sizeof(float),1,f);
			fwrite (&d->flags, sizeof(ulong),1,f);
			fwrite (&d->numBuildOptions, sizeof(int),1,f);
		}
		if (err) break;

		for (int a=0;a<numDefs;a++)
			if (deflist[a].IsBuilder()) {
				if (fwrite (&table.data[numDefs*a], sizeof(Table::ent), numDefs, f) != numDefs)
				{
					err=true;
					break;
				}
			}

		// store the buildby table
		for (int a=0;a<numDefs;a++)
		{
			vector<int>& v=buildby[a];
			short size = v.size();
			if (fwrite (&size, sizeof(short), 1, f) != 1 ||
				fwrite (&v[0], sizeof(int), v.size(), f) != v.size()) 
			{
				err=true;
				break;
			}
		}

		if(fwrite (buildAssistTypes, sizeof(int), NUM_BUILD_ASSIST, f) != NUM_BUILD_ASSIST)
			err=true;
	} while(0);

	if (err)
		logPrintf ("ERROR: Failed to write to file %s\n", fn);

	fclose (f);
}

bool BuildTable::LoadCache (const char *fn)
{
	FILE *f;

	f = fopen(fn, "rb");
	if (!f) {
		logPrintf ("BuildTable: Can't open cache file %s\n", fn);
		return false;
	}

	if (fgetc (f) != BT_CACHE_VERSION) {
		logPrintf ("BuildTable: File %s has wrong version.\n", fn);
		return false;
	}

	fread (&numDefs, sizeof(int), 1, f);

	if (numDefs != cb->GetNumUnitDefs ())
	{
		logPrintf ("Cache isn't generated for current mod version.\n");
		fclose (f);
		return false;
	}

	deflist = new UDef[numDefs];
	for (int a=0;a<numDefs;a++)
	{
		deflist[a].name = ReadZStr (f);
		fread (&deflist[a].cost, sizeof(ResourceInfo), 1, f);
		fread (&deflist[a].make, sizeof(ResourceInfo), 1, f);
		fread (&deflist[a].storage, sizeof(ResourceInfo), 1, f);
		fread (&deflist[a].buildTime, sizeof(float),1,f);
		fread (&deflist[a].metalExtractDepth, sizeof(float), 1, f);
		fread (&deflist[a].energyUse, sizeof(float), 1,f);
		fread (&deflist[a].buildSpeed, sizeof(float), 1,f);
		fread (&deflist[a].weaponDamage, sizeof(float),1,f);
		fread (&deflist[a].weaponRange, sizeof(float),1,f);
		fread (&deflist[a].flags, sizeof(ulong),1,f);
		fread (&deflist[a].numBuildOptions, sizeof(int), 1,f);
	}

	table.alloc (numDefs);

	for (int a=0;a<numDefs;a++) 
		if (deflist[a].IsBuilder())
			fread (&table.data[a*numDefs], sizeof(Table::ent), numDefs, f);

	buildby = new vector<int>[numDefs];
	for (int a=0;a<numDefs;a++)
	{
		vector<int>& v = buildby[a];
		short size;

		fread (&size, sizeof(short), 1, f);
		v.resize (size);

		fread (&v[0], sizeof(int), size, f);
		deflist[a].buildby = &v;
	}

	fread (buildAssistTypes, sizeof(int), NUM_BUILD_ASSIST, f);

	if (ferror(f)) {
		logPrintf ("Error reading AI mod cache file %s\n", fn);
		fclose(f);
		return false;
	}

	fclose (f);

	return true;
}

bool BuildTable::UnitCanBuild (const UnitDef *builder, const UnitDef *constr)
{
	BuildPathNode& bpn = GetBuildPath (builder, constr);
	return bpn.id == constr->id-1;
}

BuildTable::BuilderList* BuildTable::GetBuilderList (const UnitDef *b)
{
	return &buildby[b->id-1];
}

const UnitDef* BuildTable::GetDef (BuildPathNode *ent)
{
	return GetDef (ent->id+1);
}

const UnitDef* BuildTable::GetDef (int i)
{
	if (!deflist[i-1].def)
		deflist[i-1].def = cb->GetUnitDef (deflist[i-1].name.c_str());

	return deflist[i-1].def;
}

int BuildTable::GetDefID (const char *name)
{
	for (int a=0;a<numDefs;a++) 
	{
		if (!STRCASECMP (deflist[a].name.c_str(), name))
			return a+1;
	}

	logPrintf ("Error: No unit called %s was found\n", name);
	return 0;
}
