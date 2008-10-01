    int GetCurrentFrame(  )
    {
        return callbacktorts->GetCurrentFrame(  );
    }

    int GetMyTeam(  )
    {
        return callbacktorts->GetMyTeam(  );
    }

    int GetMyAllyTeam(  )
    {
        return callbacktorts->GetMyAllyTeam(  );
    }

    int GetPlayerTeam( int player )
    {
        return callbacktorts->GetPlayerTeam( player );
    }

    bool AddUnitToGroup( int unitid, int groupid )
    {
        return callbacktorts->AddUnitToGroup( unitid, groupid );
    }

    bool RemoveUnitFromGroup( int unitid )
    {
        return callbacktorts->RemoveUnitFromGroup( unitid );
    }

    int GetUnitGroup( int unitid )
    {
        return callbacktorts->GetUnitGroup( unitid );
    }

    int GetUnitAiHint( int unitid )
    {
        return callbacktorts->GetUnitAiHint( unitid );
    }

    int GetUnitTeam( int unitid )
    {
        return callbacktorts->GetUnitTeam( unitid );
    }

    int GetUnitAllyTeam( int unitid )
    {
        return callbacktorts->GetUnitAllyTeam( unitid );
    }

    double GetUnitHealth( int unitid )
    {
        return callbacktorts->GetUnitHealth( unitid );
    }

    double GetUnitMaxHealth( int unitid )
    {
        return callbacktorts->GetUnitMaxHealth( unitid );
    }

    double GetUnitSpeed( int unitid )
    {
        return callbacktorts->GetUnitSpeed( unitid );
    }

    double GetUnitPower( int unitid )
    {
        return callbacktorts->GetUnitPower( unitid );
    }

    double GetUnitExperience( int unitid )
    {
        return callbacktorts->GetUnitExperience( unitid );
    }

    double GetUnitMaxRange( int unitid )
    {
        return callbacktorts->GetUnitMaxRange( unitid );
    }

    bool IsUnitActivated( int unitid )
    {
        return callbacktorts->IsUnitActivated( unitid );
    }

    bool UnitBeingBuilt( int unitid )
    {
        return callbacktorts->UnitBeingBuilt( unitid );
    }

    int GetBuildingFacing( int unitid )
    {
        return callbacktorts->GetBuildingFacing( unitid );
    }

    bool IsUnitCloaked( int unitid )
    {
        return callbacktorts->IsUnitCloaked( unitid );
    }

    bool IsUnitParalyzed( int unitid )
    {
        return callbacktorts->IsUnitParalyzed( unitid );
    }

    int GetMapWidth(  )
    {
        return callbacktorts->GetMapWidth(  );
    }

    int GetMapHeight(  )
    {
        return callbacktorts->GetMapHeight(  );
    }

    double GetMinHeight(  )
    {
        return callbacktorts->GetMinHeight(  );
    }

    double GetMaxHeight(  )
    {
        return callbacktorts->GetMaxHeight(  );
    }

    double GetMaxMetal(  )
    {
        return callbacktorts->GetMaxMetal(  );
    }

    double GetExtractorRadius(  )
    {
        return callbacktorts->GetExtractorRadius(  );
    }

    double GetMinWind(  )
    {
        return callbacktorts->GetMinWind(  );
    }

    double GetMaxWind(  )
    {
        return callbacktorts->GetMaxWind(  );
    }

    double GetTidalStrength(  )
    {
        return callbacktorts->GetTidalStrength(  );
    }

    double GetGravity(  )
    {
        return callbacktorts->GetGravity(  );
    }

    double GetElevation( double x, double z )
    {
        return callbacktorts->GetElevation( x, z );
    }

    double GetMetal(  )
    {
        return callbacktorts->GetMetal(  );
    }

    double GetMetalIncome(  )
    {
        return callbacktorts->GetMetalIncome(  );
    }

    double GetMetalUsage(  )
    {
        return callbacktorts->GetMetalUsage(  );
    }

    double GetMetalStorage(  )
    {
        return callbacktorts->GetMetalStorage(  );
    }

    double GetEnergy(  )
    {
        return callbacktorts->GetEnergy(  );
    }

    double GetEnergyIncome(  )
    {
        return callbacktorts->GetEnergyIncome(  );
    }

    double GetEnergyUsage(  )
    {
        return callbacktorts->GetEnergyUsage(  );
    }

    double GetEnergyStorage(  )
    {
        return callbacktorts->GetEnergyStorage(  );
    }

    double GetFeatureHealth( int feature )
    {
        return callbacktorts->GetFeatureHealth( feature );
    }

    double GetFeatureReclaimLeft( int feature )
    {
        return callbacktorts->GetFeatureReclaimLeft( feature );
    }

    int GetNumUnitDefs(  )
    {
        return callbacktorts->GetNumUnitDefs(  );
    }

    double GetUnitDefRadius( int def )
    {
        return callbacktorts->GetUnitDefRadius( def );
    }

    double GetUnitDefHeight( int def )
    {
        return callbacktorts->GetUnitDefHeight( def );
    }

