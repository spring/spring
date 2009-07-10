#include "UnitDefHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"

// ------------------------------------------------------------------------------------------------

sRAIUnitDefBL::sRAIUnitDefBL(sRAIUnitDef* RAIud, sRAIBuildList* BuildList, float Efficiency, int Task)
{
	RUD=RAIud;
	RUD->List[RUD->ListSize++]=this;
	RBL=BuildList;
	RBL->UDef[RBL->UDefSize++]=this;
	if( Efficiency > 0 )
		efficiency = Efficiency;
	else
		efficiency = -1;
	if( Task > 0 )
		task = Task;
	else
		task = -1;
};
sRAIUnitDefBL::~sRAIUnitDefBL()
{
	for(int iUD=0; iUD<RBL->UDefSize; iUD++ )
	{
		if( RBL->UDef[iUD] == this )
		{
			RBL->UDefSize--;
			if( iUD < RBL->UDefSize )
				RBL->UDef[iUD]=RBL->UDef[RBL->UDefSize];
			iUD=RBL->UDefSize; // end loop
		}
	}
	for(int iBL=0; iBL<RUD->ListSize; iBL++ )
	{
		if( RUD->List[iBL] == this )
		{
			RUD->ListSize--;
			if( iBL < RUD->ListSize )
				RUD->List[iBL]=RUD->List[RUD->ListSize];
			iBL=RUD->ListSize; // end loop
		}
	}
};

// ------------------------------------------------------------------------------------------------

sRAIUnitDef::sRAIUnitDef(const UnitDef *unitdef, IAICallback* cb, GlobalResourceMap* RM, GlobalTerrainMap* TM, float EnergyToMetalRatio, cLogFile *l, float MaxFiringRange)
{
	ud=unitdef;
//*l<<"\n   "+ud->humanName+"("+ud->name+")("<<ud->id<<")";
//	if( ud->canAssist ) *l<<" (ud->canAssist)";
//	if( ud->canBuild ) *l<<" (ud->canBuild)";
//	if( ud->canReclaim ) *l<<" (ud->canReclaim)";
//	if( ud->canResurrect ) *l<<" (ud->canResurrect)";
//	if( ud->canRepair ) *l<<" (ud->canRepair)";
//	if( ud->buildSpeed > 0 ) { *l<<" ud->buildSpeed="<<ud->buildSpeed; }
//	if( ud->buildDistance > 0 ) { *l<<" ud->buildDistance="<<ud->buildDistance; }
//	if( ud->buildOptions.size() > 0) { *l<<" ud->buildOptions.size()="<<ud->buildOptions.size(); }
//	if( ud->stealth ) *l<<" (ud->stealth)";
//	if( ud->canKamikaze ) *l<<" (ud->canKamikaze)";
//	if( ud->turnRate > 0 ) *l<<" ud->turnRate="<<ud->turnRate;
//	*l<<" ud->armorType="<<ud->armorType;
//	if( ud->extractsMetal > 0) { *l<<" extractsMetal="<<ud->extractsMetal; }
//	if( ud->extractRange > 0) { *l<<" extractRange="<<ud->extractRange; }
//	*l<<" ud->windGenerator="<<ud->windGenerator;
//	*l<<" ud->tidalGenerator="<<ud->tidalGenerator;
//	*l<<" ud->energyUpkeep="<<ud->energyUpkeep;
//	*l<<" ud->ysize="<<ud->ysize;
//	*l<<" ud->buildingDecalDecaySpeed="<<ud->buildingDecalDecaySpeed;
//	*l<<" ud->buildingDecalSizeX="<<ud->buildingDecalSizeX;
//	*l<<" ud->buildingDecalSizeY="<<ud->buildingDecalSizeY;
//	*l<<" ud->buildingDecalType="<<ud->buildingDecalType;
//	*l<<" ud->aihint="<<ud->aihint;
//	*l<<" ud->category="<<ud->category;
//	*l<<" ud->categoryString="<<ud->categoryString;
//	*l<<" ud->wantedHeight="<<ud->wantedHeight;
//	*l<<" ud->speed="<<ud->speed;
//	*l<<" ud->mass="<<ud->mass;
//	*l<<" ud->transportMass="<<ud->transportMass;
// What is ud->transportSize?
//	*l<<" xsize="<<ud->xsize;
//	*l<<" ysize="<<ud->ysize;
//	*l<<" ud->canhover="<<ud->canhover;
//	*l<<" ud->canfly="<<ud->canfly;
//	*l<<" cb->GetUnitDefHeight(ud->id)="<<cb->GetUnitDefHeight(ud->id);
//	if( ud->canSubmerge ) *l<<" (canSubmerge)";
//	*l<<" maxSlope="<<ud->maxSlope;
//if( ud->movedata != 0 ) *l<<" pathType="<<ud->movedata->pathType<<" maxSlope="<<ud->movedata->maxSlope<<" depth="<<ud->movedata->depth<<" depthMod="<<ud->movedata->depthMod;
//	*l<<" maxWaterDepth="<<ud->maxWaterDepth;
//	*l<<" minWaterDepth="<<ud->minWaterDepth;
//	*l<<" deathExplosion="<<ud->deathExplosion;
//	*l<<" energyStorage="<<ud->energyStorage;
//	*l<<" ud->maxThisUnit="<<ud->maxThisUnit;
//	*l<<" ud->losRadius="<<ud->losRadius;
//	*l<<" ud->health="<<ud->health;
//	*l<<" ud->energyCost="<<ud->energyCost;
//	*l<<" ud->metalCost="<<ud->metalCost;
//	*l<<" ud->buildTime="<<ud->buildTime;
//	*l<<" ud->maxThisUnit="<<ud->maxThisUnit;
	ListSize=0;
	SetUnitLimit(ud->maxThisUnit);
	CanBuild=false;
	CanBeBuilt=false;
	HasPrerequisite=false;
	Disabled=false;
	RBUnitLimit=false;
	RBCost=true;
	RBPrereq=true;
	DGun=0;
	SWeapon=0;
	UnitConstructs=0;
	IsBomber=false;
	OnOffMetalDifference= -ud->metalUpkeep+ud->makesMetal;
	OnOffEnergyDifference= -ud->energyUpkeep;
	if( ud->extractsMetal > 0 )
		OnOffMetalDifference += RM->averageMetalSite*ud->extractsMetal;
	if( ud->tidalGenerator > 0 )
		OnOffEnergyDifference += ud->tidalGenerator*cb->GetTidalStrength();
	if( ud->windGenerator > 0 && (cb->GetMaxWind()-cb->GetMinWind())/2 + cb->GetMinWind() >= 11 )
		OnOffEnergyDifference += (cb->GetMaxWind()-cb->GetMinWind())/2 + cb->GetMinWind();
	MetalDifference= OnOffMetalDifference +ud->metalMake;
	EnergyDifference=OnOffEnergyDifference+ud->energyMake;
	if( EnergyDifference < 0 && EnergyToMetalRatio*MetalDifference+EnergyDifference < 0 )
		HighEnergyDemand=true;
	else
		HighEnergyDemand=false;
//	*l<<" EnergyDifference="<<EnergyDifference;
//	*l<<" MetalDifference="<<MetalDifference;

	MetalPCost  =  0.0080f*ud->metalCost  + 0.00080f*ud->metalCost *0.01f*float(rand()%101);
	if( MetalPCost < MetalDifference && MetalPCost > 0 )
		MetalPCost = MetalDifference;
	EnergyPCost = (0.0022f*ud->energyCost + 0.00022f*ud->energyCost*0.01f*float(rand()%101))*EnergyToMetalRatio;
	if( EnergyPCost < EnergyDifference && ud->energyCost > EnergyDifference )
		EnergyPCost = EnergyDifference;
	if( EnergyPCost < 1.5*-EnergyDifference )
		EnergyPCost = 1.5*-EnergyDifference;
//	*l<<" EnergyPCost="<<EnergyPCost;
//	*l<<" MetalPCost="<<MetalPCost;

	if( ud->speed > 0 )
		CloakMaxEnergyDifference = -ud->cloakCostMoving;
	else
		CloakMaxEnergyDifference = -ud->cloakCost;

	WeaponGuardRange = 0;
	WeaponEnergyDifference = 0;
	WeaponMaxEnergyCost = 0;
	for(std::vector<UnitDef::UnitDefWeapon>::const_iterator iW=ud->weapons.begin(); iW!=ud->weapons.end(); iW++)
	{
		if( iW->def->reload!=0 )
		{
			WeaponEnergyDifference -= int(iW->def->energycost/iW->def->reload);
			if( WeaponMaxEnergyCost < iW->def->energycost )
				WeaponMaxEnergyCost = iW->def->energycost;
		}
//		*l<<" W="<<iW->def->name;
//		*l<<" def->onlyTarget="<<iW->def->onlyTargetCategory;
//		*l<<" onlyTarget="<<iW->onlyTargetCat;
//		*l<<" badTarget="<<iW->badTargetCat;
//		if( iW->def->waterweapon ) *l<<"(Water)";
//		if( iW->def->stockpile ) *l<<"(Stock)";
//		if( iW->def->manualfire ) *l<<"(Manual)";
//		if( iW->def->coverageRange > 0 ) *l<<"(c="<<iW->def->coverageRange<<")";
//		if( iW->def->shieldRadius > 0 ) *l<<"(sh="<<iW->def->shieldRadius<<")";
//		if( *iW->def->damages.GetDefaultDamage() > 0 ) *l<<"(d="<<*iW->def->damages.GetDefaultDamage()<<")";
//		if( iW->def->salvosize > 0 ) *l<<"(sa="<<iW->def->salvosize<<")";
//		if( iW->def->reload > 0 ) *l<<"(re="<<iW->def->reload<<")";
//		if( iW->def->range > 0 ) *l<<"(ra="<<iW->def->range<<")";
//		*l<<" s="<<iW->def->salvosize;
//		*l<<" selfExplode="<<iW->def->selfExplode;
//		*l<<" areaOfEffect="<<iW->def->areaOfEffect;
		if( ud->canDGun && iW->def->manualfire )
		{
			DGun=iW->def;
//			*l<<"(DGun)";
		}
		if( iW->def->stockpile )
		{
			SWeapon=iW->def;
//			*l<<"(SWeapon)";
		}
		if( iW->def->dropped )
			IsBomber = true;

		if( ud->speed == 0 && iW->def->range > 250 && iW->def->coverageRange == 0 && (!iW->def->manualfire || iW->def->range < MaxFiringRange ) )
		{
			float NewGuardRange = iW->def->range;
			if( iW->def->range > 550 )
			{
				NewGuardRange = 550.0f + 0.25*(iW->def->range-550.0f);
				if( NewGuardRange > 1100.0f )
					NewGuardRange = 1100.0f;
			}
			if( WeaponGuardRange < NewGuardRange )
				WeaponGuardRange = NewGuardRange;
		}
//		if( iW->def->selfExplode )
//			Destruct=iW->def;
	}

	SetBestWeaponEff(&WeaponLandEff,1,MaxFiringRange);
	SetBestWeaponEff(&WeaponAirEff,2,MaxFiringRange);
	SetBestWeaponEff(&WeaponSeaEff,3,MaxFiringRange);
	if( ud->kamikazeDist > 0 )
	{
		if( ud->minWaterDepth < 0 && WeaponLandEff.BestRange < ud->kamikazeDist )
			WeaponLandEff.BestRange = ud->kamikazeDist;
		if( ud->movedata != 0 && -ud->movedata->depth < 0 && WeaponSeaEff.BestRange < ud->kamikazeDist )
			WeaponSeaEff.BestRange = ud->kamikazeDist;
	}

//	*l<<"(");
//	if( WeaponLandEff.BestRange > 0 ) *l<<"L";
//	if( WeaponAirEff.BestRange > 0 ) *l<<"A";
//	if( WeaponSeaEff.BestRange > 0 ) *l<<"S";
//	*l<<" WeaponGuardRange="<<WeaponGuardRange;
//	*l<<" BestWeaponRange="<<BestWeaponRange<<" ";
//	*l<<")";

	mobileType = 0;
	immobileType = 0;

	if( ud->canfly ) {}
	else if( ud->movedata == 0 ) // for immobile units
	{
		immobileType = TM->udImmobileType.find(ud->id)->second;
		if( ud->needGeo )
		{
			int NewUnitLimit=0;
			for( int i=0; i<RM->RSize[1]; i++ )
				if( RM->R[1][i]->options.find(ud->id) != RM->R[1][i]->options.end() )
					NewUnitLimit++;
			if( NewUnitLimit == 0 )
			{
				Disabled = true;
				CheckBuildOptions();
			}
		}
		else if( ud->extractsMetal > 0 && !RM->isMetalMap )
		{
			int NewUnitLimit=0;
			for( int i=0; i<RM->RSize[0]; i++ )
				if( RM->R[0][i]->options.find(ud->id) != RM->R[0][i]->options.end() )
					NewUnitLimit++;
			if( NewUnitLimit == 0 )
			{
				Disabled = true;
				CheckBuildOptions();
			}
		}
		else
		{
			// If a unit can build mobile units then it will inherit mobileType from it's options
			map<TerrainMapMobileType*,int> MTcount;
			typedef pair<TerrainMapMobileType*,int> MTpair;
			for( map<int,string>::const_iterator iB=ud->buildOptions.begin(); iB!=ud->buildOptions.end(); iB++ )
			{
				if( cb->GetUnitDef(iB->second.c_str()) == 0 ) // Work-Around(Mod: ? Don't Remember):  Spring-Version(?)
				{
					cb->SendTextMsg("WARNING: Mod Unit Definition Missing.",0);
					*l<<"\nWARNING: No Unit Definition was found for '"<<iB->second.c_str()<<"'. ";
				}
				else
				{
					const UnitDef* ud = cb->GetUnitDef(iB->second.c_str());
					if( ud->movedata != 0 && TM->udMobileType.find(ud->id)->second->areaSize > 0 )
					{
						TerrainMapMobileType *MT = TM->udMobileType.find(ud->id)->second;
						if( MTcount.find(MT) != MTcount.end() )
							MTcount.find(MT)->second++;
						else
							MTcount.insert(MTpair(MT,1));
					}
				}
			}
			int iMost = 0;
			for( map<TerrainMapMobileType*,int>::iterator iM=MTcount.begin(); iM!=MTcount.end(); iM++ )
				if( mobileType == 0 || iM->second > iMost )
				{
					mobileType = iM->first;
					iMost = iM->second;
				}
			if( (mobileType!=0 && !mobileType->typeUsable) ||
				(!immobileType->typeUsable && (mobileType==0 || immobileType->sector.size()<100)) )
			{
				Disabled = true;
				CheckBuildOptions();
			}
		}
	}
	else // for mobile units
	{
		mobileType = TM->udMobileType.find(ud->id)->second;
		if( !mobileType->typeUsable )
		{
			Disabled = true;
			CheckBuildOptions();
		}
	}
}

int sRAIUnitDef::GetPrerequisite()
{
	int iBest=-1;
	vector<int> vTempIDList; // Unit ID
	set<int> sTemp; // searchable record of vTempIDList contents
	for( map<int,sRAIPrerequisite>::iterator iP=AllPrerequisiteOptions.begin(); iP!=AllPrerequisiteOptions.end(); iP++ )
		if( int(iP->second.udr->UnitsActive.size()) > 0 )
		{
			if( iBest == -1 || iBest > iP->second.buildLine ) // New or Better buildline was found
			{
				iBest = iP->second.buildLine;
				vTempIDList.clear();
				sTemp.clear();
			}
			if( iBest == iP->second.buildLine )
			{
				for( map<int,sRAIUnitDef*>::iterator iB=iP->second.udr->BuildOptions.begin(); iB!=iP->second.udr->BuildOptions.end(); iB++ )
					if( iB->second->CanBeBuilt && AllPrerequisiteOptions.find(iB->first) != AllPrerequisiteOptions.end() && AllPrerequisiteOptions.find(iB->first)->second.buildLine == iBest-1 && sTemp.find(iB->first)==sTemp.end() && iB->second->GetBuildList("Constructor") != 0 && iB->second->GetBuildList("Constructor")->udIndex < iB->second->GetBuildList("Constructor")->RBL->UDefActiveTemp )
					{
						vTempIDList.push_back(iB->first);
						sTemp.insert(iB->first);
					}
			}
		}

	if( int(vTempIDList.size()) > 0 )
	{
		int i=rand()%int(vTempIDList.size());
		return AllPrerequisiteOptions.find(vTempIDList.at(i))->first;
	}

	return ud->id;
};

int sRAIUnitDef::GetPrerequisiteNewBuilder()
{
	vector<int> vTempIDList;
	for( map<int,sRAIUnitDef*>::iterator iP=PrerequisiteOptions.begin(); iP!=PrerequisiteOptions.end(); iP++ )
	{
		if( iP->second->CanBeBuilt && (iP->second->ListSize > 1 || iP->second->List[0]->efficiency >= 0.5 || PrerequisiteOptions.size() == 1 ) )
		{
			for( map<int,sRAIUnitDef*>::iterator iP2=iP->second->PrerequisiteOptions.begin(); iP2!=iP->second->PrerequisiteOptions.end(); iP2++ )
			{
				if( int(iP2->second->UnitsActive.size()) > 0 )
				{
					vTempIDList.push_back(iP->first);
					break;
				}
			}
		}
	}
	if( int(vTempIDList.size()) > 0 )
	{
		int i=rand()%int(vTempIDList.size());
		return PrerequisiteOptions.find(vTempIDList.at(i))->first;
	}

	return GetPrerequisite();
}

void sRAIUnitDef::SetUnitLimit(int num)
{
	UnitLimit[0]=num;
	if( UnitLimit[0] > ud->maxThisUnit )
		UnitLimit[0] = ud->maxThisUnit;
	CheckUnitLimit();
}

void sRAIUnitDef::SetULConstructs(int num)
{
	UnitLimit[1] = num;
	UnitLimit[0] = UnitLimit[1] + int(UnitsActive.size()) + int(UnitConstructsActive.size());
	SetUnitLimit(UnitLimit[0]);
}

sRAIUnitDefBL* sRAIUnitDef::GetBuildList(string Name)
{
	for( int iBL=0; iBL<ListSize; iBL++ )
		if( List[iBL]->RBL->Name == Name )
			return List[iBL];
	return 0;
}

void sRAIUnitDef::CheckUnitLimit()
{
	if( int(UnitsActive.size())+UnitConstructs >= UnitLimit[0] )
	{
		if( !RBUnitLimit )
		{
			RBUnitLimit = true;
			CheckBuildOptions();
		}
	}
	else
	{
		if( RBUnitLimit )
		{
			RBUnitLimit = false;
			CheckBuildOptions();
		}
	}
}

void sRAIUnitDef::CheckBuildOptions()
{
	bool CouldBuild = CanBuild;
	CanBuild = int(UnitsActive.size()) > 0;
	if( CouldBuild != CanBuild )
	{
		if( CanBuild )
		{
			for( map<int,sRAIUnitDef*>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); iB++ )
				if( !iB->second->HasPrerequisite )
					iB->second->HasPrerequisite = true;
		}
		else
		{
			for( map<int,sRAIUnitDef*>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); iB++ )
			{
				bool StillHasPrereq = false;
				for( map<int,sRAIUnitDef*>::iterator iP=iB->second->PrerequisiteOptions.begin(); iP!=iB->second->PrerequisiteOptions.end(); iP++ )
				{
					if( iP->second->CanBuild )
					{
						StillHasPrereq = true;
						break;
					}
				}
				if( !StillHasPrereq )
				{
					iB->second->HasPrerequisite = false;
				}
			}
		}
	}

	bool CouldBeBuilt = CanBeBuilt;
	CanBeBuilt = !(Disabled || RBUnitLimit || RBCost || RBPrereq);
	if( CouldBeBuilt != CanBeBuilt )
	{
		for( int i=0; i<ListSize; i++ )
		{
			if( CanBeBuilt )
			{
				// Enabling
				List[i]->RBL->Disable(List[i]->udIndex,false);
			}
			else
			{
				// Disabling
				List[i]->RBL->Disable(List[i]->udIndex);
			}
		}
	}

	if( CouldBuild || CouldBeBuilt != CanBuild || CanBeBuilt )
	{
		if( CanBuild || CanBeBuilt )
		{
			// Enabling
			for( map<int,sRAIUnitDef*>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); iB++ )
			{
				if( iB->second->RBPrereq )
				{
					iB->second->RBPrereq = false;
					iB->second->CheckBuildOptions();
				}
			}
		}
		else
		{
			// Disabling
			for( map<int,sRAIUnitDef*>::iterator iB=BuildOptions.begin(); iB!=BuildOptions.end(); iB++ )
			{
				if( !iB->second->RBPrereq )
				{
					bool prereq = false;
					for( map<int,sRAIUnitDef*>::iterator iP=iB->second->PrerequisiteOptions.begin(); iP!=iB->second->PrerequisiteOptions.end(); iP++ )
					{
						if( iP->second->CanBuild || iP->second->CanBeBuilt )
						{
							prereq = true;
							break;
						}
					}
					if( !prereq )
					{
						iB->second->RBPrereq = true;
						iB->second->CheckBuildOptions();
					}
				}
			}
		}
	}
}

bool sRAIUnitDef::IsCategory(string category)
{
	for( int i=0; i<=int(ud->categoryString.size()-category.size()); i++ )
	{
		bool found = true;
		for( int c = 0; c < int(category.size()); c++ )
			if( ud->categoryString.at(i+c) != category.at(c) )
			{
				found = false;
				i = int(ud->categoryString.size()); // end loop
				c = int(category.size()); // end loop
			}
		if( found )
			return true;
	}
	return false;
}

bool sRAIUnitDef::IsNano()
{
	if( int(BuildOptions.size()) == 0 && ud->buildDistance > 0 && ud->buildSpeed > 0 )
		return true;
	return false;
}

bool sRAIUnitDef::CheckWeaponType(vector<UnitDef::UnitDefWeapon>::const_iterator udw, int type)
{
	switch( type )
	{
	case 1: // Land
		{
			if( udw->def->waterweapon )
				return false;
		}
		break;
	case 2: // Air
		{
			if( udw->def->waterweapon )
				return false;
		}
		break;
	case 3: // Sea
		{
			if( !udw->def->waterweapon )
				return false;
		}
		break;
	}
	return true;
}

void sRAIUnitDef::SetBestWeaponEff(sWeaponEfficiency *we, int type, float MaxFiringRange)
{
	float fRange=-1;
	float fVal=0;
	for(vector<UnitDef::UnitDefWeapon>::const_iterator iW=ud->weapons.begin(); iW!=ud->weapons.end(); iW++)
	{
		if( CheckWeaponType(iW,type) )
		{
			float fRangeTemp=iW->def->range;
			if( fRangeTemp > MaxFiringRange )
				fRangeTemp = MaxFiringRange;

			float fValTemp=0;
			for(std::vector<UnitDef::UnitDefWeapon>::const_iterator iW2=ud->weapons.begin(); iW2!=ud->weapons.end(); iW2++)
			{
				if( CheckWeaponType(iW,type) )
				{
					float fRange=iW2->def->range;
					if( fRange > MaxFiringRange )
						fRange = MaxFiringRange;
					if( fRange >= fRangeTemp )
						fValTemp+=iW2->def->damages.GetDefaultDamage()*fRangeTemp/iW2->def->reload;
					if( fRangeTemp > ud->losRadius )
						fValTemp*=0.5;
				}
			}

			if( fRange == -1 || fValTemp>fVal )
			{
				fRange=fRangeTemp;
				fVal=fValTemp;
			}
		}
	}

	we->BestRange = fRange;
}

// ------------------------------------------------------------------------------------------------

sRAIBuildList::sRAIBuildList(int MaxDefSize, cRAIUnitDefHandler *UDRHandler)
{
	UDR = UDRHandler;
	index = UDR->BLSize;
	UDef = new sRAIUnitDefBL*[MaxDefSize];
	UDefSize=0;
	UDefActive=0;
	UDefActiveTemp=0;
	priority=-1;
	minUnits=0;
	minEfficiency=1.0;
	unitsActive=0;
	Name = "Undefined";
}

sRAIBuildList::~sRAIBuildList()
{
	for(int i=0; i<UDefSize; i++)
		delete UDef[i];
	delete [] UDef;
}

void sRAIBuildList::Disable(int udIndex, bool value)
{
	if( value == true )
	{
		UDefActive--;
		if( UDefActive == 0 )
		{
			UDR->BLActive--;
			UDR->BLSwitch(index,UDR->BLActive);
		}
	}
	UDefSwitch(udIndex,UDefActive);
	if( value == false )
	{
		UDefActive++;
		if( UDefActive == 1 )
		{
			UDR->BLSwitch(index,UDR->BLActive);
			UDR->BLActive++;
		}
	}
}

void sRAIBuildList::UDefSwitch(int index1, int index2)
{
	sRAIUnitDefBL *pRUD = UDef[index1];
	UDef[index1] = UDef[index2];
	UDef[index2] = pRUD;
	UDef[index1]->udIndex=index1;
	UDef[index2]->udIndex=index2;
}

// ------------------------------------------------------------------------------------------------

cRAIUnitDefHandler::cRAIUnitDefHandler(IAICallback* cb, GlobalResourceMap *RM, GlobalTerrainMap* TM, cLogFile *log)
{
	l=log;
	const UnitDef **udList = new const UnitDef*[cb->GetNumUnitDefs()];
	cb->GetUnitDefList(udList);
	int udSize=cb->GetNumUnitDefs();

	*l<<"\n Reading All Unit Definitions (Frame="<<cb->GetCurrentFrame()<<") ...";
	for( int iud=cb->GetNumUnitDefs()-1; iud>=0; iud-- )
	{
		if( udList[iud] == 0 ) // Work-around: War Alien VS Human v1.0 (as well as other possible mods)
		{
			*l<<"\n  WARNING: (unitdef->id="<<iud+1<<") Mod UnitDefList["<<iud<<"] = 0";
			udSize--;
			udList[iud] = udList[udSize];
		}
	}

	AverageConstructSpeed=0;
	float fMetalCostTotal=0;
	float fEnergyCostTotal=0;
	int Constructs=0;
	for( int iud=0; iud<udSize; iud++ )
	{
		const UnitDef* ud=udList[iud];
		fMetalCostTotal+=ud->metalCost;
		fEnergyCostTotal+=ud->energyCost;
		if (ud->builder)
		{
			AverageConstructSpeed+=ud->buildSpeed;
			Constructs++;
		}
	}
	if( fMetalCostTotal == 0 )
	{
		fMetalCostTotal = 1;
		*l<<"\n  No Metal Usage.";
	}
	if( fEnergyCostTotal == 0 )
	{
		fEnergyCostTotal = 1;
		*l<<"\n  No Energy Usage.";
	}
	EnergyToMetalRatio = fEnergyCostTotal/fMetalCostTotal;
	if( EnergyToMetalRatio > 40.0f ) // having this too high only causes problems with the cost limits
		EnergyToMetalRatio = 40.0f;

	*l<<"\n  Average Metal Cost = "<<fMetalCostTotal/udSize;
	*l<<"\n  Average Energy Cost = "<<fEnergyCostTotal/udSize;
	*l<<"\n  Energy to Metal Ratio = "<<EnergyToMetalRatio;

	if( Constructs == 0 )
	{
		AverageConstructSpeed = 1;
		*l<<"\n  No Constructors Detected ...";
	}
	else
	{
		AverageConstructSpeed/=Constructs;
		*l<<"\n  Average Construction Speed = "<<AverageConstructSpeed;
	}

	float MaxFiringRange=8.0*(cb->GetMapWidth()-1.0+cb->GetMapHeight()-1.0)/2.0;
	typedef pair<int,sRAIUnitDef> iuPair;
	for( int iud=0; iud<udSize; iud++ )
	{
		const UnitDef* ud=udList[iud];
		UDR.insert(iuPair(ud->id,*new sRAIUnitDef(ud,cb,RM,TM,EnergyToMetalRatio,l,MaxFiringRange)));
	}
	delete [] udList;

	*l<<"\n Reading UnitDef Build Options ...";
	typedef pair<int,sRAIUnitDef*> iupPair; // used to access UDR->BuildOptions & UDR->PrerequisiteOptions
	typedef pair<int,sRAIPrerequisite> ipPair; // used to access UDR->AllPrerequisiteOptions
	for( map<int,sRAIUnitDef>::iterator iU=UDR.begin(); iU!=UDR.end(); iU++ )
	{
		const UnitDef* ud = iU->second.ud;
		for(map<int,string>::const_iterator iBO=ud->buildOptions.begin(); iBO!=ud->buildOptions.end(); iBO++ )
		{
			const UnitDef* bd = cb->GetUnitDef(iBO->second.c_str());
			if( bd == 0 )
			{
				cb->SendTextMsg("WARNING: Mod Unit Definition Invalid.",0);
				*l<<"\n  WARNING: (unitdef->id="<<ud->id<<")"<<ud->humanName<<" has an invalid unitdef->buildOption '"<<iBO->second.c_str()<<"'";
			}
			else
			{
				sRAIUnitDef *U=&UDR.find(bd->id)->second;
				iU->second.BuildOptions.insert(iupPair(bd->id,U));
				U->PrerequisiteOptions.insert(iupPair(ud->id,&iU->second));
			}
		}
	}

	*l<<"\n Determining All Build Options and Prerequisites ...";
	for( map<int,sRAIUnitDef>::iterator iU=UDR.begin(); iU!=UDR.end(); iU++ )
	{
		vector<sBuildLine> vTemp;
		vTemp.clear();
		for( map<int,sRAIUnitDef*>::iterator iB=iU->second.PrerequisiteOptions.begin(); iB!=iU->second.PrerequisiteOptions.end(); iB++ )
			vTemp.push_back(sBuildLine(iB->first,1));
		for(int iT=0; iT<int(vTemp.size()); iT++ )
		{
			if( iU->second.AllPrerequisiteOptions.find(vTemp[iT].ID) == iU->second.AllPrerequisiteOptions.end() )
			{
				for( map<int,sRAIUnitDef*>::iterator iB=UDR.find(vTemp[iT].ID)->second.PrerequisiteOptions.begin(); iB!=UDR.find(vTemp[iT].ID)->second.PrerequisiteOptions.end(); iB++ )
					vTemp.push_back(sBuildLine(iB->first,vTemp[iT].BL+1));
				iU->second.AllPrerequisiteOptions.insert(ipPair(vTemp[iT].ID,sRAIPrerequisite(&UDR.find(vTemp[iT].ID)->second)));
				iU->second.AllPrerequisiteOptions.find(vTemp[iT].ID)->second.buildLine=vTemp[iT].BL;
			}
			else if( iU->second.AllPrerequisiteOptions.find(vTemp[iT].ID)->second.buildLine > vTemp[iT].BL )
				iU->second.AllPrerequisiteOptions.find(vTemp[iT].ID)->second.buildLine=vTemp[iT].BL;
		}
		if( !iU->second.ud->isCommander && int(iU->second.AllPrerequisiteOptions.size()) == 0 )
			*l<<"\n  WARNING: ("<<iU->first<<")"<<iU->second.ud->humanName<<" is a non-commander unit that can not be built.";
	}

	for( list<TerrainMapMobileType>::iterator i=TM->mobileType.begin(); i!=TM->mobileType.end(); i++)
		if( i->typeUsable )
			RBMobile.insert(&*i);
	for( list<TerrainMapImmobileType>::iterator i=TM->immobileType.begin(); i!=TM->immobileType.end(); i++)
		if( i->typeUsable )
			RBImmobile.insert(&*i);
/*
*l<<"\n\nDisplaying All Build Options & All Prerequisite Options for All Units ...";
for(map<int,sRAIUnitDef>::iterator iU=UDR.begin(); iU!=UDR.end(); iU++ )
{
	*l<<"\n("<<iU->second.ud->id<<")"+iU->second.ud->humanName+" (size="<<iU->second.BuildOptions.size()<<"):";
	for( map<int,sRAIUnitDef*>::iterator iB=UDR.find(iU->second.ud->id)->second.BuildOptions.begin(); iB!=UDR.find(iU->second.ud->id)->second.BuildOptions.end(); iB++ )
		*l<<"  "+iB->second->ud->humanName;
	*l<<"\n("<<iU->second.ud->id<<")"+iU->second.ud->humanName+" (size="<<iU->second.AllPrerequisiteOptions.size()<<"):";
	for( map<int,sRAIPrerequisite>::iterator iP=UDR.find(iU->second.ud->id)->second.AllPrerequisiteOptions.begin(); iP!=UDR.find(iU->second.ud->id)->second.AllPrerequisiteOptions.end(); iP++ )
		*l<<" "+iP->second.udr->ud->humanName<<"("<<iP->second.buildLine<<")";
}
*l<<"\n";
*/
	*l<<"\n Determining Unit Efficiencies ...";
	const int ARMOR		=0;
	const int SPEED		=1;
	const int MANUVER	=2;
	const int BUILD		=3;
	const int BUILDOPT	=4;
	const int ENERGYP	=5;
	const int ENERGYS	=6;
	const int METALP	=7;
	const int METALS	=8;
	const int AIRBASE	=9;
	const int RADAR		=10;
	const int RADARJAM	=11;
	const int SONAR		=12;
	const int SONARJAM	=13;
	const int TRANSPORT	=14;
	const int TARGETING	=15;
	const int BOMB		=16;
	const int STOCKW	=17;
	const int ANTIMIS	=18;
	const int SHIELD	=19;
	const int WEAPON	=20;
	const int WEAPONSEA	=21;
	const int UPGRADE	=22;
	const int COST		=23;
	const int ESIZE		=24;

//	double UE[cb->GetNumUnitDefs()][ESIZE]; // Unit Efficency
	double UE[5000][ESIZE]; // required for Visual Studios
	for( map<int,sRAIUnitDef>::iterator iUD=UDR.begin(); iUD!=UDR.end(); iUD++ )
	{
		sRAIUnitDef *udr=&iUD->second;
		const UnitDef* ud=iUD->second.ud;
		int i=iUD->first-1;
		float cost=ud->energyCost+(ud->metalCost*EnergyToMetalRatio);
		if( cost == 0 ) cost=1;

		UE[i][COST]=cost;
		UE[i][ARMOR]=ud->health/cost;
		UE[i][SPEED]=ud->speed;

		UE[i][BUILD]=(ud->buildSpeed*ud->buildDistance)/cost;
		UE[i][BUILDOPT]=int(ud->buildOptions.size()) + 8.0*ud->canResurrect;
		for( map<int,sRAIUnitDef*>::iterator iB=udr->BuildOptions.begin(); iB!=udr->BuildOptions.end(); iB++ )
			if( (iB->second->PrerequisiteOptions.size()) == 1 )
			{
				UE[i][BUILDOPT] += 2;
				if( UE[i][BUILDOPT] < 15 )
					UE[i][BUILDOPT] = 15;
			}
		if( UE[i][BUILDOPT] == 0 )
		{
			if( !ud->canRepair || ud->isAirBase )
				UE[i][BUILD]=0;
		}
		else
		{
			if( ud->buildSpeed == 0 ) // Work-Around for the mod: fibre v13.1
				UE[i][BUILD]=(AverageConstructSpeed*ud->buildDistance)/cost;
			if( ud->isCommander )
				UE[i][BUILD]*=2;
		}

		UE[i][MANUVER]=1.5*ud->canfly + 1.0*ud->canCloak + 0.5*ud->canhover + 0.5*ud->stealth + 0.5*(ud->minWaterDepth<0 && ud->maxWaterDepth > TM->minElevation);

		UE[i][ENERGYS]=ud->energyStorage/cost;
		if( 10*ud->energyStorage < fEnergyCostTotal/udSize || ud->energyStorage < 500 )
			UE[i][ENERGYS] = 0;

		UE[i][ENERGYP]=udr->EnergyDifference/cost;
		if( 1000*udr->EnergyDifference < ud->energyCost )
			UE[i][ENERGYP]=0;

		if( fEnergyCostTotal == 1 )
		{
			UE[i][ENERGYP]=0;
			UE[i][ENERGYS]=0;
		}

		UE[i][METALS]=ud->metalStorage/cost;
		if( 2.5*ud->metalStorage < fMetalCostTotal/udSize || ud->metalStorage < 500 )
			UE[i][METALS]=0;

		UE[i][METALP]=udr->MetalDifference/cost;
		if( 1000*udr->MetalDifference < ud->metalCost )
			UE[i][METALP]=0;

		UE[i][RADAR]=ud->radarRadius + 0.20*ud->losRadius;
		UE[i][RADARJAM]=ud->jammerRadius;
		UE[i][SONAR]=ud->sonarRadius + 0.20*ud->losRadius;
		UE[i][SONARJAM]=ud->sonarJamRadius;
		UE[i][AIRBASE]=(ud->buildSpeed*ud->isAirBase);

		UE[i][TRANSPORT]=ud->transportMass+(0.1*ud->transportCapacity*ud->transportMass);
		if( ud->transportCapacity == 0 )
			UE[i][TRANSPORT] = 0;
		UE[i][TARGETING]=float(ud->targfac)/cost;
		UE[i][BOMB]=ud->canKamikaze + ud->kamikazeDist;
		UE[i][STOCKW]=0;
		UE[i][ANTIMIS]=0;
		UE[i][SHIELD]=0;
		UE[i][WEAPON]=0;
		UE[i][WEAPONSEA]=0;
		for(std::vector<UnitDef::UnitDefWeapon>::const_iterator iW=ud->weapons.begin(); iW!=ud->weapons.end(); iW++)
		{
			float reload=iW->def->reload;
			if( reload == 0 )
			{
				*l<<"\n  WARNING: the weapon '"<<iW->name<<"' belonging to ("<<iUD->first<<")"<<iUD->second.ud->humanName<<" has a 0 reload time.";
				reload = 10;
			}
			float range=iW->def->range;
			if( range > MaxFiringRange )
				range = MaxFiringRange;

			if( iW->def->stockpile )
			{
				if( iW->def->manualfire )
				{
					UE[i][STOCKW] = iW->def->damages.GetDefaultDamage()*range/reload;
					float fWCost=(iW->def->metalcost*EnergyToMetalRatio)+iW->def->energycost;
					if(fWCost==0)
						fWCost=1;
					UE[i][STOCKW]/=fWCost;
				}
				else if( iW->def->coverageRange > 0 )
				{
					float rangeC=iW->def->coverageRange;
					if( rangeC > MaxFiringRange )
						rangeC = MaxFiringRange;

					UE[i][ANTIMIS] = rangeC/reload;
					float fWCost=(iW->def->metalcost*EnergyToMetalRatio)+iW->def->energycost;
					if(fWCost==0)
						fWCost=1;
					UE[i][ANTIMIS]/=fWCost;
				}
			}
			else if( iW->def->shieldRadius > 0 )
			{
				UE[i][SHIELD]=iW->def->shieldPower*iW->def->shieldRadius*iW->def->shieldPowerRegen;
			}
			else if( iW->def->damages.GetDefaultDamage() > 0 )
			{
				if( iW->def->waterweapon )
					UE[i][WEAPONSEA]=iW->def->salvosize*iW->def->damages.GetDefaultDamage()*range/reload;
				else
				{
					UE[i][WEAPON]=iW->def->salvosize*iW->def->damages.GetDefaultDamage()*range/reload;
				}
			}
		}

		if( udr->IsCategory("UPGRADE") ) // mod(CvC 0.4), change this if a more standard upgrade system is added
		{
			*l<<"\n  Upgrade Category Tag found: "<<ud->humanName;
			UE[i][BUILD]=0;
			UE[i][RADAR]=0;
			UE[i][TARGETING]=0;
			UE[i][WEAPON]=0;
			UE[i][UPGRADE]=1.0f;
		}
		else
			UE[i][UPGRADE]=0;

		if( udr->IsCategory("NOTARGET") ) // Just used in mod(Kernal Panic), Hopefully...
		{
			*l<<"\n  NoTarget Category Tag found: "<<ud->humanName;
			UE[i][WEAPON]=0;
		}

		if( int(udr->PrerequisiteOptions.size()) == 0 || ud->isFeature )
		{
			UE[i][ENERGYP]=0;
			UE[i][ENERGYS]=0;
			UE[i][METALP]=0;
			UE[i][METALS]=0;
			UE[i][RADAR]=0;
			UE[i][RADARJAM]=0;
			UE[i][SONAR]=0;
			UE[i][SONARJAM]=0;
			UE[i][UPGRADE]=0;
		}
	}

	for( int e=0; e<ESIZE; e++ )
	{
		sRAIUnitDef *udr;
		double Max=0;
		for( int i=0; i<cb->GetNumUnitDefs(); i++ )
			if( UDR.find(i+1) != UDR.end() )
			{
				udr = &UDR.find(i+1)->second;
				if( Max<UE[i][e] && udr->ud->maxThisUnit > 1 && int(udr->PrerequisiteOptions.size()) > 0 && ( int(udr->PrerequisiteOptions.size()) > 1 || udr->PrerequisiteOptions.begin()->first != i+1 ) )
					Max=UE[i][e];
			}
		if( Max == 0 )
		{
			for( int i=0; i<cb->GetNumUnitDefs(); i++ )
				if( UDR.find(i+1) != UDR.end() && UE[i][e]>Max )
					Max=UE[i][e];
		}
		if( Max > 0 )
			for( int i=0; i<cb->GetNumUnitDefs(); i++ )
			{
				if( UDR.find(i+1) != UDR.end() )
				{
					udr = &UDR.find(i+1)->second;
					UE[i][e]/=Max;
					if( int(udr->PrerequisiteOptions.size()) == 0 || udr->ud->maxThisUnit == 1 || (udr->PrerequisiteOptions.size() == 1 && udr->PrerequisiteOptions.begin()->first == i+1) )
					{
						if( UE[i][e] > 1.01f )
							UE[i][e] = 1.01f;
						else if( UE[i][e] > 0.0f && UE[i][e] < 0.1f )
							UE[i][e] = 0.1f;
					}
				}
			}
	}
/*
	*l<<"\n\nDisplaying Unit Efficency Table ...";
	for( int i=0; i<cb->GetNumUnitDefs(); i++ ) // Unit Efficency Debug
		if( UDR.find(i+1) != UDR.end() )
		{
			*l<<"\n("<<i+1<<") "+UDR.find(i+1)->second.ud->humanName;
			for( int e=0; e<ESIZE; e++ )
				if( UE[i][e] != 0 )
					*l<<" ["<<e<<"]:"<<100*UE[i][e]<<"%";
		}
	*l<<"\n";
*/
	*l<<"\n Creating the Build-Lists ...";
	BLSize=0;
	BLActive=0;
	for(int iBL=0; iBL<35; iBL++)
	{
		BL[BLSize] = new sRAIBuildList(cb->GetNumUnitDefs(),this);
		float fAverageTaskEfficiency=0;
		for( map<int,sRAIUnitDef>::iterator iUD=UDR.begin(); iUD!=UDR.end(); iUD++ )
		{
			int i=iUD->first-1;
			sRAIUnitDef *udr=&iUD->second;
			const UnitDef* ud=iUD->second.ud;
			float fCost=ud->energyCost+(ud->metalCost*EnergyToMetalRatio);
			float fValue=0;
			int iTask=-1;
			switch(iBL)
			{
			case 0:
				BLBuilder = BL[BLSize];
				BL[BLSize]->Name="Constructor";
				if( UE[i][BUILD] == 0 || ( UE[i][BUILDOPT] == 0 && ud->speed == 0) ) break;

				// BuildOption Var
				fValue+=0.7*UE[i][BUILD] + 0.7*UE[i][BUILDOPT] + 0.2*UE[i][MANUVER] + 0.1*UE[i][SPEED] + 0.1*UE[i][ARMOR] + 0.1*UE[i][ENERGYP] + 0.1*UE[i][METALP];
				iTask=TASK_CONSTRUCT;

				// BuildList Var.
				BL[BLSize]->priority=10;
				BL[BLSize]->minUnits=2;
				BL[BLSize]->minEfficiency=0.5f;

				break;
			case 1:
				BL[BLSize]->Name= "Immobile Assist Constructor";
				if( ud->speed > 0 || UE[i][BUILDOPT] > 0 || UE[i][BUILD] == 0 ) break;

				fValue=UE[i][BUILD];
				iTask=TASK_CONSTRUCT;

				BL[BLSize]->priority=5;
				BL[BLSize]->minEfficiency=2.0f;
				break;
			case 2:
				BLEnergyL = BL[BLSize];
				BL[BLSize]->Name= "Energy Production (Geothermal)";
				if( UE[i][ENERGYP] <= 0 || !ud->needGeo ) break;

				fValue=2.0*UE[i][ENERGYP] + 0.2*UE[i][ARMOR];

				BL[BLSize]->minEfficiency=0.5f;
				break;
			case 3:
				BLEnergy = BL[BLSize];
				BL[BLSize]->Name="Energy Production";
				if( UE[i][ENERGYP] <= 0.0 || ud->needGeo || ud->extractsMetal > 0 ) break;

				fValue=2.0*UE[i][ENERGYP] + 0.2*UE[i][ARMOR];
				if( ud->speed > 0 )
					iTask=TASK_NONE;

				BL[BLSize]->minUnits=2;
				BL[BLSize]->minEfficiency=1.01f;
				break;
			case 4:
				BLMetalL = BL[BLSize];
				BL[BLSize]->Name="Metal Production (Extractor)";
				if( ud->extractsMetal <= 0 || RM->isMetalMap ) break;

				fValue=2.0*UE[i][METALP] + 0.1*UE[i][ARMOR]*UE[i][METALP];

				BL[BLSize]->minUnits=2;
				BL[BLSize]->minEfficiency=0.5f;
				break;
			case 5:
				BLMetal = BL[BLSize];
				BL[BLSize]->Name="Metal Production";
				if( UE[i][METALP] <= 0.0 || ud->needGeo || (ud->extractsMetal > 0.0 && !RM->isMetalMap) ) break;

				fValue=2.0*UE[i][METALP] + 0.1*UE[i][ARMOR]*UE[i][METALP];
				if( UE[i][ENERGYP] < 0.0f )
					fValue+=0.1*UE[i][ENERGYP];

				if( ud->speed > 0 )
					iTask=TASK_NONE;

				if( ud->extractsMetal > 0 )
					BL[BLSize]->minUnits=2;
				BL[BLSize]->minEfficiency=1.01f;
				break;
			case 6:
				BLEnergyStorage = BL[BLSize];
				BL[BLSize]->Name="Energy Storage";
				if( UE[i][ENERGYS] <= 0.0 ) break;

				fValue=2.0*UE[i][ENERGYS] + 0.2*UE[i][ARMOR];
				if( ud->speed > 0 )
					iTask=TASK_NONE;

				BL[BLSize]->minEfficiency=2.0f;
				break;
			case 7:
				BLMetalStorage = BL[BLSize];
				BL[BLSize]->Name="Metal Storage";
				if( UE[i][METALS] <= 0.0 ) break;

				fValue=2.0*UE[i][METALS] + 0.2*UE[i][ARMOR];
				if( ud->speed > 0 )
					iTask=TASK_NONE;

				BL[BLSize]->minEfficiency=2.0f;
				break;
			case 8:
				BLMobileRadar = BL[BLSize];
				BL[BLSize]->Name="Mobile Radar";
				if( ud->radarRadius <= 0 || ud->speed == 0.0 ) break;

				fValue=UE[i][RADAR] / UE[i][COST];
				iTask=TASK_SCOUT;

				BL[BLSize]->priority=2+(TM->percentLand/100.0)*3.0;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 9:
				BL[BLSize]->Name="Immobile Radar";
				if( ud->radarRadius <= 0 || ud->speed > 0.0 ) break;

				fValue=UE[i][RADAR] / UE[i][COST];

				BL[BLSize]->priority=3+(TM->percentLand/100.0)*5.0;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 10:
				BL[BLSize]->Name="Mobile Radar Jammer";
				if( ud->jammerRadius <= 0 || ud->speed == 0.0 ) break;

				fValue=UE[i][RADARJAM] / UE[i][COST];
				iTask=TASK_SUPPORT;

				BL[BLSize]->priority=1+(TM->percentLand/100.0)*1.0;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 11:
				BL[BLSize]->Name="Immobile Radar Jammer";
				if( ud->jammerRadius <= 0 || ud->speed > 0.0 ) break;

				fValue=UE[i][RADARJAM] / UE[i][COST];
				fValue/=fCost;

				BL[BLSize]->priority=2+(TM->percentLand/100.0)*5.0;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 12:
				BL[BLSize]->Name="Mobile Sonar";
				if( ud->sonarRadius <= 0 || ud->speed == 0.0 ) break;

				fValue=UE[i][SONAR] / UE[i][COST];
				iTask=TASK_SCOUT;

				if( TM->percentLand < 97.0 )
					BL[BLSize]->priority=1.0+((100.0-TM->percentLand)/100.0)*3.0;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 13:
				BL[BLSize]->Name="Immobile Sonar";
				if( ud->sonarRadius <= 0 || ud->speed > 0.0 ) break;

				fValue=UE[i][SONAR] / UE[i][COST];

				BL[BLSize]->priority=((100.0-TM->percentLand)/100.0)*5.0;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 14:
				BL[BLSize]->Name="Mobile Sonar Jammer";
				if( ud->sonarJamRadius <= 0 || ud->speed == 0.0 ) break;

				fValue=UE[i][SONARJAM] / UE[i][COST];
				iTask=TASK_SUPPORT;

				BL[BLSize]->priority=((100.0-TM->percentLand)/100.0)*2.0;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 15:
				BL[BLSize]->Name="Immobile Sonar Jammer";
				if( ud->sonarJamRadius <= 0 || ud->speed > 0.0 ) break;

				fValue=UE[i][SONARJAM] / UE[i][COST];

				BL[BLSize]->priority=((100.0-TM->percentLand)/100.0)*5.0;
				BL[BLSize]->minEfficiency=3.0f;
				break;
/*
			case 16: // Check for immobile scouts
				BL[BLSize]->Name="Immobile Recon";
				break;
			case 17: // Check for scouts
				BL[BLSize]->Name="Mobile Recon";
				break;
*/
			case 18:
				BL[BLSize]->Name="Mobile Bomb";
				if( !ud->canKamikaze || ud->speed == 0.0 ) break;

				fValue=UE[i][BOMB];
				iTask=TASK_SUICIDE;

				BL[BLSize]->minEfficiency=0.99f;
				BL[BLSize]->priority=2;
				break;
			case 19:
				BL[BLSize]->Name="Immobile Bomb";
				if( !ud->canKamikaze || ud->speed > 0.0 ) break;

				fValue=UE[i][BOMB];

//				BL[BLSize]->priority=2;
				break;
			case 20:
				BL[BLSize]->Name="Transport";
				if( ud->transportCapacity <= 0 ) break;

				fValue=UE[i][TRANSPORT] + UE[i][MANUVER];
				iTask=TASK_TRANSPORT;

				BL[BLSize]->minEfficiency=0.15f;
				break;
			case 21:
				BL[BLSize]->Name="Targeting Facility";
				if( !ud->targfac ) break;

				fValue=UE[i][TARGETING];

				BL[BLSize]->priority=1;
				BL[BLSize]->minEfficiency=3.0f;
				break;
/*
			case 22:
				BL[BLSize]->Name="Immobile Anti-Air";
				break;
			case 23:
				BL[BLSize]->Name="Mobile Anti-Air";
				break;
			case 24:
				BL[BLSize]->Name="Immobile Anti-Land";
				break;
			case 25:
				BL[BLSize]->Name="Mobile Anti-Land";
				break;
*/
			case 24:
				BL[BLSize]->Name="Immobile Anti-Land/Air";
				if( UE[i][WEAPON] <= 0 || ud->canKamikaze || ud->speed > 0.0 || int(udr->BuildOptions.size()) > 0 ) break;

				fValue=1.3*UE[i][WEAPON] + 0.7*UE[i][ARMOR];

				BL[BLSize]->priority=int(15.0+(TM->percentLand/100.0)*15.0);
				BL[BLSize]->minEfficiency=0.99f;
				break;
			case 25:
				BL[BLSize]->Name="Mobile Anti-Land/Air";
				if( UE[i][WEAPON] <= 0 || ud->canKamikaze || ud->speed == 0.0 || int(udr->BuildOptions.size()) > 0 ) break;

				fValue=1.3*UE[i][WEAPON] + 0.5*UE[i][ARMOR] + 0.1*UE[i][MANUVER] + 0.1*UE[i][SPEED];
				iTask=TASK_ASSAULT;

				BL[BLSize]->priority=int(18.0+(TM->percentLand/100.0)*18.0);
				BL[BLSize]->minEfficiency=0.99f;
				break;
			case 26:
				BL[BLSize]->Name="Immobile Anti-Naval";
				if( UE[i][WEAPONSEA] <= 0 || ud->canKamikaze || ud->speed > 0.0 ) break;

				fValue=1.3*UE[i][WEAPONSEA] + 0.7*UE[i][ARMOR];

				if( !TM->waterIsHarmful )
					BL[BLSize]->priority=int(((100.0-TM->percentLand)/100.0)*12.0);
				BL[BLSize]->minEfficiency=0.99f;
				break;
			case 27:
				BL[BLSize]->Name="Mobile Anti-Naval";
				if( UE[i][WEAPONSEA] <= 0 || ud->speed == 0.0 ) break;

				fValue=1.3*UE[i][WEAPONSEA] + 0.5*UE[i][ARMOR] + 0.1*UE[i][MANUVER] + 0.1*UE[i][SPEED];
				iTask=TASK_ASSAULT;

				if( !TM->waterIsHarmful )
					BL[BLSize]->priority=int(((100.0-TM->percentLand)/100.0)*18.0);
				BL[BLSize]->minEfficiency=0.99f;
				break;
			case 28:
				BL[BLSize]->Name="Anti-Missile";
				if( UE[i][ANTIMIS] <= 0 ) break;

				fValue=UE[i][ANTIMIS];

				BL[BLSize]->priority=1;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 29:
				BL[BLSize]->Name="Stockpile Weapon";
				if( UE[i][STOCKW] <= 0 ) break;

				fValue=UE[i][STOCKW];

				BL[BLSize]->priority=1;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 30:
				BL[BLSize]->Name="Anti-Weapon(Shields)";
				if( UE[i][SHIELD] <= 0 ) break;
			
				fValue=UE[i][SHIELD];

				BL[BLSize]->priority=1;
				BL[BLSize]->minEfficiency=3.0f;
				break;
			case 31:
				BLAirBase = BL[BLSize];
				BL[BLSize]->Name="Air Base";
				if( !ud->isAirBase || UE[i][BUILDOPT] > 0 ) break;

				fValue=UE[i][AIRBASE];

				BL[BLSize]->priority=2;
				BL[BLSize]->minEfficiency=0.75f;
				break;
			case 32:
				BL[BLSize]->Name="Wall";
				if( int(udr->PrerequisiteOptions.size()) == 0 || !ud->isFeature ) break;

				fValue=1;

				break;
			case 33:
				BL[BLSize]->Name="Upgrade";
				if( UE[i][UPGRADE] == 0 ) break;

				fValue = 1;

				BL[BLSize]->priority=10;
				break;
			}

			if( fValue > 0.0 )
			{
				fAverageTaskEfficiency=((fAverageTaskEfficiency*BL[BLSize]->UDefSize) + fValue)/(BL[BLSize]->UDefSize+1);
				new sRAIUnitDefBL(&iUD->second,BL[BLSize],fValue,iTask);
			}
		}

		if( BL[BLSize]->Name == "Undefined" )
		{
			delete BL[BLSize];
			BLSize--;
		}
		else if( BL[BLSize]->UDefSize == 0 )
		{
			BL[BLSize]->minUnits=0;
			BL[BLSize]->priority=-1;
		}
		else
		{
			if( BL[BLSize]->priority == 0 ) // SafeGuard
				BL[BLSize]->priority = -1;

			for(int iud=0; iud<BL[BLSize]->UDefSize; iud++)
			{
				if( BL[BLSize]->UDef[iud]->efficiency/fAverageTaskEfficiency >= 8.0 ) // Check for absurdly unbalanced units, needed for Gundam v1.1
				{
					float fEffDecrease=BL[BLSize]->UDef[iud]->efficiency-(7.75*fAverageTaskEfficiency);
					BL[BLSize]->UDef[iud]->efficiency-=fEffDecrease;
					fAverageTaskEfficiency-=fEffDecrease/BL[BLSize]->UDefSize;
					iud=-1; // Start Over
				}
			}
			float max=0;
			for(int iud=0; iud<BL[BLSize]->UDefSize; iud++)
			{
				BL[BLSize]->UDef[iud]->efficiency/=fAverageTaskEfficiency;
				if( max == 0 || max < BL[BLSize]->UDef[iud]->efficiency )
					max = BL[BLSize]->UDef[iud]->efficiency;
			}
			if( BL[BLSize]->minEfficiency > 1 && BL[BLSize]->minEfficiency <= max/3.0f )
			{
				BL[BLSize]->minEfficiency = max/2.5f;
				*l<<"\n  Setting "<<BL[BLSize]->Name<<" Min. Efficiency to '"<<BL[BLSize]->minEfficiency<<"'.";
			}
		}
		BLSize++;
	}
/*
*l<<"\n\nDEBUGGING: Build-List";
for(int iBL=0; iBL<BLSize; iBL++ )
{
	*l<<"\n"+BL[iBL]->Name+" Build-List #"<<iBL+1<<" (size="<<BL[iBL]->UDefSize<<"):";
	for(int iud=0; iud<BL[iBL]->UDefSize; iud++)
		*l<<"  ("<<BL[iBL]->UDef[iud]->RUD->ud->id<<")"+BL[iBL]->UDef[iud]->RUD->ud->name+" "<<BL[iBL]->UDef[iud]->efficiency;
}
*l<<"\nUndefined Build-List:";
for(map<int,sRAIUnitDef>::iterator iud=UDR.begin(); iud!=UDR.end(); iud++ )
	if( iud->second.ListSize == 0 )
		*l<<"  ("<<iud->second.ud->id<<")"+iud->second.ud->name;
*l<<"\n";
*/
/*
*l<<"\n\nDEBUGGING: Build Options ...";
	for(map<int,sRAIUnitDef>::iterator iud=UDR.begin(); iud!=UDR.end(); iud++ )
	{
		*l<<"\n("<<iud->second.ud->id<<") Name="+iud->second.ud->humanName+" ";
		*l<<" LS="<<iud->second.ListSize;
		for(int iBL=0; iBL<iud->second.ListSize; iBL++)
		{
			sRAIUnitDefBL *udrBL=iud->second.List[iBL];
			*l<<"\n  LN="<< udrBL->RBL->Name;
			*l<<" iBL="<<iBL;
			*l<<" UDefSize="<< udrBL->RBL->UDefSize;
			*l<<" priority="<< udrBL->RBL->priority;
			*l<<" fEfficiency="<<udrBL->efficiency;
			*l<<" iTask="<<udrBL->task;
		}
		*l<<"\n";
	}
*l<<"\n";
*/
	*l<<"\n Removing inappropriate build options ...";
	for(int iud=0; iud<BLBuilder->UDefSize; iud++)
	{
		sRAIUnitDef *udr=BLBuilder->UDef[iud]->RUD;
		if( int(udr->BuildOptions.size()) == 0 && udr->ListSize > 1 )
		{
			if( UE[udr->ud->id-1][BUILDOPT] == 0 ) // BUILDOPT includes 'resurrect' ability
			{
//				*l<<"\n  (Special Rule) "<<BLBuilder->Name<<" Removed ... ("<<udr->ud->id<<")"<<udr->ud->humanName;
				delete BLBuilder->UDef[iud--];
			}
			else
			{
//				*l<<"\n  (Special Rule) "<<BLBuilder->Name<<" Reduced ... ("<<udr->ud->id<<")"<<udr->ud->humanName;
				BLBuilder->UDef[iud]->efficiency /= 5.0f; // This will discourage using certain units as constructors, was needed for MOD: Blox
			}
		}
		else if( udr->ud->isCommander )
		{
			sRAIUnitDefBL* bestEP = 0;
			for( map<int,sRAIUnitDef*>::iterator iB = udr->BuildOptions.begin(); iB != udr->BuildOptions.end(); iB++ )
				for(int iL=0; iL<iB->second->ListSize; iL++)
					if( iB->second->List[iL]->RBL == BLEnergy && (bestEP == 0 || bestEP->efficiency < iB->second->List[iL]->efficiency) )
						bestEP = iB->second->List[iL];
			if( bestEP != 0 && bestEP->RUD->ListSize > 1 && bestEP->efficiency < BLEnergy->minEfficiency )
			{
//				*l<<"\n  (Special Rule) "<<BLEnergy->Name<<" Increased ... ("<<bestEP->RUD->ud->id<<")"<<bestEP->RUD->ud->humanName;
				bestEP->efficiency *= 3.0f; // This will encourage using certain units as Energy Production, was needed for MOD: Epic Legions
			}
		}
	}

	// Adjusts the buildlist for useage on a metalmap
	if( RM->isMetalMap )
	{
		*l<<"\n Check '"+BLMetal->Name+" Build-List' for usage on a metal map.";
		float Low=-1.0;
		for(int i=0; i<BLMetal->UDefSize; i++ ) // Find lowest extractor efficiency
			if( (Low == -1.0 || Low > BLMetal->UDef[i]->efficiency) && BLMetal->UDef[i]->RUD->ud->extractsMetal > 0.0 )
				Low = BLMetal->UDef[i]->efficiency;
		if( Low > 0 )
		{
			// Disable metal makers below this efficiency, most likely all of them
			for(int i=0; i<BLMetal->UDefSize; i++ )
				if( BLMetal->UDef[i]->RUD->ud->extractsMetal <= 0.0 ) // Low > BLMetal->UDef[i]->efficiency &&
				{
					if( BLMetal->UDef[i]->RUD->ListSize == 1 )
					{
						BLMetal->UDef[i]->RUD->Disabled = true;
						BLMetal->UDef[i]->RUD->CheckBuildOptions();
//						*l<<"\n  (Special Rule) "<<BLMetal->Name<<" Disabled ... ("<<BLMetal->UDef[i]->RUD->ud->id<<")"<<BLMetal->UDef[i]->RUD->ud->humanName;
					}
					else
					{
//						*l<<"\n  (Special Rule) "<<BLMetal->Name<<" Removed ... ("<<BLMetal->UDef[i]->RUD->ud->id<<")"<<BLMetal->UDef[i]->RUD->ud->humanName;
						delete BLMetal->UDef[i--];
					}
				}
		}
	}

	// If an "extractor" can be used for some other task, remove it from "Metal Production" and discourage its useage as anything but an extractor, was needed for MOD: FF
	for(int iud=0; iud<BLMetalL->UDefSize; iud++)
	{
		sRAIUnitDef *udr = BLMetalL->UDef[iud]->RUD;
		if( udr->ListSize > 1 )
		{
			for( int i=0; i<udr->ListSize; i++ )
				if( udr->List[i]->RBL == BLMetal )
				{
//					*l<<"\n  (Special Rule) "<<BLMetalL->Name<<" Removed ... ("<<udr->ud->id<<")"<<udr->ud->humanName;
					delete BLMetalL->UDef[iud--];
					break;
				}
				else if( udr->List[i]->RBL != BLMetalL )
				{
//					*l<<"\n  (Special Rule) "<<udr->List[i]->RBL->Name<<" Reduced ... ("<<udr->ud->id<<")"<<udr->ud->humanName;
					udr->List[i]->efficiency /= 2.0f;
				}
		}
	}

	// If a "geo-energy plant" can be used for some other task, discourage its useage as anything but an "geo-energy plant", was needed for MOD: FF
	for(int iud=0; iud<BLEnergyL->UDefSize; iud++)
	{
		sRAIUnitDef *udr = BLEnergyL->UDef[iud]->RUD;
		if( udr->ListSize > 1 )
		{
			for( int i=0; i<udr->ListSize; i++ )
				if( udr->List[i]->RBL != BLEnergyL )
				{
//					*l<<"\n  (Special Rule) "<<udr->List[i]->RBL->Name<<" Reduced ... ("<<udr->ud->id<<")"<<udr->ud->humanName;
					udr->List[i]->efficiency /= 2.0f;
				}
		}
	}

	// If a low efficiency "metal producer" can be used for some other task, discourage its use as a "metal producer", was needed for MOD: CvC
	for(int iud=0; iud<BLMetal->UDefSize; iud++)
	{
		sRAIUnitDef *udr = BLMetal->UDef[iud]->RUD;
		if( UE[udr->ud->id-1][METALP] < 0.15 && udr->ListSize > 1 )
		{
//			*l<<"\n  (Special Rule) "<<BLMetal->Name<<" Reduced ... ("<<udr->ud->id<<")"<<udr->ud->humanName;
			BLMetal->UDef[iud]->efficiency /= 5.0f;
		}
	}

	// If a low efficiency "energy producer" can be used for some other task, discourage its use as a "energy producer", was needed for MOD: FF
	for(int iud=0; iud<BLEnergy->UDefSize; iud++)
	{
		sRAIUnitDef *udr = BLEnergy->UDef[iud]->RUD;
		if( UE[udr->ud->id-1][ENERGYP] < 0.15 && udr->ListSize > 1 )
		{
//			*l<<"\n  (Special Rule) "<<BLEnergy->Name<<" Reduced ... ("<<udr->ud->id<<")"<<udr->ud->humanName;
			BLEnergy->UDef[iud]->efficiency /= 5.0f;
		}
	}

	for(map<int,sRAIUnitDef>::iterator iud=UDR.begin(); iud!=UDR.end(); iud++ )
		if( iud->second.ListSize > 1 )
		{
			int bestTIndex=0; // best task-index
			for(int iT=0; iT<iud->second.ListSize; iT++) // Finds the task this unit is best at
				if( iud->second.List[iT]->efficiency/iud->second.List[iT]->RBL->minEfficiency > iud->second.List[bestTIndex]->efficiency/iud->second.List[bestTIndex]->RBL->minEfficiency )
					bestTIndex=iT;
			// Remove inappropriate tasks based off of the estimated efficiency
			for(int iT=0; iT<iud->second.ListSize; iT++)
			{
				sRAIBuildList *RBL = iud->second.List[iT]->RBL;
				if( iT != bestTIndex && iud->second.List[iT]->efficiency < RBL->minEfficiency )
				{
					int bestLIndex = -1; // best list-index
					for(int i=0; i<RBL->UDefSize; i++)
					{
						if( RBL->UDef[i]->RUD->Disabled ) {}
						else if( bestLIndex == -1 || RBL->UDef[bestLIndex]->efficiency < RBL->UDef[i]->efficiency )
							bestLIndex = i;
					}
					if( iT != bestLIndex || 3*iud->second.List[iT]->efficiency < RBL->minEfficiency )
					{
//						*l<<"\nRemoving "+iud->second.ud->humanName+" from buildlist: "+iud->second.List[iT]->RBL->Name<<" Eff="<<iud->second.List[iT]->efficiency;
						delete iud->second.List[iT];
						if( bestTIndex == iud->second.ListSize )
							bestTIndex = iT;
						iT--;
					}
				}
			}
		}

	for( int iL=0; iL<BLSize; iL++)
	{
		int bestLIndex = -1; // best list-index
		for(int iU=0; iU<BL[iL]->UDefSize; iU++)
		{
			if( BL[iL]->UDef[iU]->RUD->Disabled ) {}
			else if( bestLIndex == -1 || BL[iL]->UDef[bestLIndex]->efficiency < BL[iL]->UDef[iU]->efficiency )
				bestLIndex = iU;
		}
		if( bestLIndex >= 0 )
		{
			if( BL[iL]->UDef[bestLIndex]->efficiency < 0.25 && BL[iL]->minEfficiency < 0.25 )
			{
				*l<<"\n  Setting the unit limit to one for the '"<<BL[iL]->Name<<"' Build-List units. (due to all of it's units having low efficiencies)";
				BL[iL]->priority = -1;
				for(int iU=0; iU<BL[iL]->UDefSize; iU++)
					BL[iL]->UDef[iU]->RUD->SetUnitLimit(1);
			}
			else
				for(int iU=0; iU<BL[iL]->UDefSize; iU++)
					if( BL[iL]->UDef[iU]->efficiency <= 0.05 )
					{
						BL[iL]->UDef[iU]->RUD->SetUnitLimit(1);
						*l<<"\n  Setting the unit limit to one for ("<<BL[iL]->UDef[iU]->RUD->ud->id<<")"<<BL[iL]->UDef[iU]->RUD->ud->humanName<<", this unit had a very low efficiency rating for all known tasks.";
					}
		}
	}

	// Restrict all HoverCraft from land or water maps but only if they are not needed for unique build options or tasks
	if( TM->percentLand > 85 || TM->percentLand < 15 )
	{
		bool UseHoverCraft=false;
		set<int> HoverCraftFactory;
		for(map<int,sRAIUnitDef>::iterator iud=UDR.begin(); iud!=UDR.end() && !UseHoverCraft; iud++ )
		{
			if( iud->second.immobileType != 0 && iud->second.mobileType != 0 && iud->second.mobileType->canHover )
				HoverCraftFactory.insert(iud->first);
			if( !iud->second.Disabled && iud->second.ud->canhover )
			{
				// check if it is needed for a unique task
				for(int iT=0; iT<iud->second.ListSize; iT++)
				{
					sRAIBuildList *BL = iud->second.List[iT]->RBL;
					bool alternative=false;
					for(int iud2=0; iud2<BL->UDefSize; iud2++)
						if( !BL->UDef[iud2]->RUD->Disabled && !BL->UDef[iud2]->RUD->ud->canhover && !BL->UDef[iud2]->RUD->ud->canfly )
						{
							alternative=true;
							break;
						}
					if( !alternative )
					{
						UseHoverCraft = true;
						break;
					}
				}
			}
		}
		if( !UseHoverCraft )
		{
			for(map<int,sRAIUnitDef>::iterator iud=UDR.begin(); iud!=UDR.end() && !UseHoverCraft; iud++ )
				if( !iud->second.ud->canhover && !iud->second.Disabled && HoverCraftFactory.find(iud->first) == HoverCraftFactory.end() )
					// known defect (unlikely, has yet to occur):
					// for every iud, all prerequisites are capable of building other prerequisites but at the same time a buildline doesn't exist
					for(map<int,sRAIPrerequisite>::iterator iP=iud->second.AllPrerequisiteOptions.begin(); iP!=iud->second.AllPrerequisiteOptions.end(); iP++)
						if( !iP->second.udr->ud->canhover && !iP->second.udr->Disabled && HoverCraftFactory.find(iP->first) == HoverCraftFactory.end() )
						{
							bool alternative=false;
							for(map<int,sRAIUnitDef*>::iterator iB=iP->second.udr->BuildOptions.begin(); iB!=iP->second.udr->BuildOptions.end(); iB++)
								if( !iB->second->ud->canhover && !iB->second->Disabled && HoverCraftFactory.find(iB->first) == HoverCraftFactory.end() )
									if( iB->first == iud->first || iud->second.AllPrerequisiteOptions.find(iB->first) != iud->second.AllPrerequisiteOptions.end() )
									{
										alternative = true;
										break;
									}
							if( !alternative )
							{
//								*l<<"\n  No alternative found: "<<iP->second.udr->ud->humanName<<" -> "<<iud->second.ud->humanName;
								UseHoverCraft = true;
								break;
							}
						}
		}

		if( !UseHoverCraft )
		{
			*l<<"\n Disabling all hovercraft & their factories ...";
			for(map<int,sRAIUnitDef>::iterator iud=UDR.begin(); iud!=UDR.end() && !UseHoverCraft; iud++ )
			{
				if( iud->second.ud->canhover || HoverCraftFactory.find(iud->first) != HoverCraftFactory.end() )
				{
//					*l<<"\n  Disabling '"<<iud->second.ud->humanName<<"'";
					iud->second.Disabled = true;
					iud->second.CheckBuildOptions();
				}
			}
		}
	}

	for(int iud=0; iud<BLBuilder->UDefSize; iud++)
	{
		sRAIUnitDef *udr = BLBuilder->UDef[iud]->RUD;

		// TEMPORARY, limits hubs from building extractors/geos/metalmakers - but only if something else can build it
		if( udr->ud->speed == 0 )
		{
			set<int> deletion;
			for( map<int,sRAIUnitDef*>::iterator i=udr->BuildOptions.begin(); i!=udr->BuildOptions.end(); i++ )
			{
				if( i->second->ud->speed == 0 )
					if( i->second->ud->needGeo || i->second->ud->extractsMetal > 0 || i->second->HighEnergyDemand )
					{
						bool alternative=false;
						for( map<int,sRAIUnitDef*>::iterator iP=i->second->PrerequisiteOptions.begin(); iP!=i->second->PrerequisiteOptions.end(); iP++)
							if( iP->second->ud->speed > 0 && !iP->second->Disabled )
								alternative = true;
						if( alternative )
							deletion.insert(i->first);
					}
			}
			for( set<int>::iterator i=deletion.begin(); i!=deletion.end(); i++ )
			{
				sRAIUnitDef *udr2 = udr->BuildOptions.find(*i)->second;
//				*l<<"\n  (Special Rule) Resource Build Option Removed ... ("<<udr->ud->id<<")"<<udr->ud->humanName<<" -> ("<<udr2->ud->id<<")"<<udr2->ud->humanName;
				udr->BuildOptions.erase(udr2->ud->id);
				udr2->PrerequisiteOptions.erase(udr->ud->id);
				udr2->AllPrerequisiteOptions.erase(udr->ud->id);
			}
		}

		// Unnecessary?: limit boat constructors from building land defences - but only if something else can build it
		if( udr->ud->speed > 0 && udr->ud->minWaterDepth >= 0 )
		{
			set<int> deletion;
			for( map<int,sRAIUnitDef*>::iterator i=udr->BuildOptions.begin(); i!=udr->BuildOptions.end(); i++ )
			{
				if( i->second->ud->speed == 0 && i->second->WeaponGuardRange > 0 && i->second->ud->minWaterDepth < 0 )
				{
					bool alternative=false;
					for( map<int,sRAIUnitDef*>::iterator iP=i->second->PrerequisiteOptions.begin(); iP!=i->second->PrerequisiteOptions.end(); iP++)
						if( iP->second->ud->speed > 0 && !iP->second->Disabled && iP->second->ud->minWaterDepth < 0 )
							alternative = true;
					if( alternative )
						deletion.insert(i->first);
				}
			}
			for( set<int>::iterator i=deletion.begin(); i!=deletion.end(); i++ )
			{
				sRAIUnitDef *udr2 = udr->BuildOptions.find(*i)->second;
//				*l<<"\n  (Special Rule) Defence Build Option Removed ... ("<<udr->ud->id<<")"<<udr->ud->humanName<<" -> ("<<udr2->ud->id<<")"<<udr2->ud->humanName;
				udr->BuildOptions.erase(udr2->ud->id);
				udr2->PrerequisiteOptions.erase(udr->ud->id);
				udr2->AllPrerequisiteOptions.erase(udr->ud->id);
			}
		}

		// Unnecessary?: limit mobile land constructors from build underwater extractors
		if( udr->ud->movedata != 0 && !udr->ud->canfly && !udr->ud->canhover && udr->ud->minWaterDepth < 0 && -udr->ud->movedata->depth > TM->minElevation )
		{
			set<int> deletion;
			for( map<int,sRAIUnitDef*>::iterator i=udr->BuildOptions.begin(); i!=udr->BuildOptions.end(); i++ )
				if( -i->second->ud->minWaterDepth < 0 && i->second->ud->extractsMetal > 0 )
					deletion.insert(i->first);
			for( set<int>::iterator i=deletion.begin(); i!=deletion.end(); i++ )
			{
				sRAIUnitDef *udr2 = udr->BuildOptions.find(*i)->second;
//				*l<<"\n  (Special Rule) Extractor Build Option Removed ... ("<<udr->ud->id<<")"<<udr->ud->humanName<<" -> ("<<udr2->ud->id<<")"<<udr2->ud->humanName;
				udr->BuildOptions.erase(udr2->ud->id);
				udr2->PrerequisiteOptions.erase(udr->ud->id);
				udr2->AllPrerequisiteOptions.erase(udr->ud->id);
			}
		}
	}

	*l<<"\n Updating Unit Limits for Metal/Geo build options ...";
	for( int i=0; i<BLMetalL->UDefSize; i++ )
		BLMetalL->UDef[i]->RUD->SetULConstructs(0);
	if( BLMetalL->UDefSize == 0 && BLMetal->UDefSize > 0 )
		BLMetal->minUnits = 1;
	if( BLMetalL->UDefSize == 0 && BLEnergyL->minUnits == 0 && RM->RSize[1] > 0 )
		BLEnergyL->minUnits = 1;
	for( int i=0; i<BLEnergyL->UDefSize; i++ )
		BLEnergyL->UDef[i]->RUD->SetULConstructs(0);

	for(int ib=0; ib<BLSize; ib++ )
		for(int iu=0; iu<BL[ib]->UDefSize; iu++)
			BL[ib]->UDef[iu]->udIndex = iu;

	*l<<"\n Build-List Result ...";
	for(int iBL=0; iBL<BLSize; iBL++ )
	{
		*l<<"\n  "+BL[iBL]->Name+" Build-List";
		if( BL[iBL]->UDefSize==0 ||
			(BL[iBL]->priority <= 0 && BL[iBL]!=BLBuilder && BL[iBL]!=BLEnergyL && BL[iBL]!=BLEnergy && BL[iBL]!=BLMetalL
			 && BL[iBL]!=BLMetal && BL[iBL]!=BLEnergyStorage && BL[iBL]!=BLMetalStorage ) )
			 *l<<" (disabled)";
		*l<<":  ";
		for(int iud=0; iud<BL[iBL]->UDefSize; iud++)
		{
			*l<<" "+BL[iBL]->UDef[iud]->RUD->ud->humanName+"(";
			if( BL[iBL]->UDef[iud]->RUD->Disabled )
				*l<<"disabled";
			else
				*l<<BL[iBL]->UDef[iud]->efficiency;
			*l<<")";
		}
	}
	*l<<"\n  List of Undefined Units:";
	for(map<int,sRAIUnitDef>::iterator iud=UDR.begin(); iud!=UDR.end(); iud++ )
		if( iud->second.ListSize == 0 )
			*l<<"  "+iud->second.ud->humanName;
}

cRAIUnitDefHandler::~cRAIUnitDefHandler()
{
	for(int i=0; i<BLSize; i++)
		delete BL[i];
}

void cRAIUnitDefHandler::BLSwitch(int index1, int index2)
{
	sRAIBuildList *pRBL = BL[index1];
	BL[index1] = BL[index2];
	BL[index2] = pRBL;
	BL[index1]->index=index1;
	BL[index2]->index=index2;
}
