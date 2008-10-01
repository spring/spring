// from AF

namespace CSharpAI
{
    public class UnitDefHelp
    {
        IAICallback aicallback;
        
        public UnitDefHelp( IAICallback aicallback )
        {
            this.aicallback = aicallback;
        }
        
        public bool IsEnergy(IUnitDef ud){
            if(ud.needGeo){
                return false;
            }
            if(ud.energyUpkeep<-1) return true;
            if((ud.windGenerator>0)&&(aicallback.GetMaxWind()>9)) return true;
            if(ud.tidalGenerator>0) return true;
            if(ud.energyMake>5) return true;
            return false;
        }
        
        public bool IsMobile(IUnitDef ud){
            if(ud.speed < 1) return false;
            if(ud.movedata != null) return true;
            if(ud.canfly) return true;
            return false;
        }
        
        public bool IsBoat( IUnitDef unitdef )
        {
            if( IsMobile( unitdef ) && unitdef.minWaterDepth > 0 )
            {
                return true;
            }
            return false;
        }
        
        public bool IsConstructor( IUnitDef unitdef )
        {
            if(unitdef.GetNumBuildOptions() == 0) return false;
            return( unitdef.builder && IsMobile( unitdef ) );
        }
        
        public bool IsFactory(IUnitDef ud){
            if (ud.GetNumBuildOptions() == 0) return false;
            if(ud.type.ToLower() == "factory" ) return true;
            return ud.builder && !IsMobile(ud);
        }
        
        public bool IsGroundMelee( IUnitDef ud )
        {
            return IsMobile( ud ) && ud.canAttack;
        }
        
        public bool IsMex(IUnitDef ud){
            if(ud.type.ToLower() == "metalextractor"){
                return true;
            }
            return false;
        }
        
        public bool IsAirCraft(IUnitDef ud){
            if(ud.type.ToLower() == "fighter")	return true;
            if(ud.type.ToLower() == "bomber")	return true;
            if(ud.canfly&&(ud.movedata == null)) return true;
            return false;
        }
        
        public bool IsGunship(IUnitDef ud){
            if(IsAirCraft(ud)&&ud.hoverAttack) return true;
            return false;
        }
        
        public bool IsFighter(IUnitDef ud){
            if(IsAirCraft(ud)&& !ud.hoverAttack) return true;
            return false;
        }
        
        public bool IsBomber(IUnitDef ud){
            if(IsAirCraft(ud)&&(ud.type.ToLower() == "bomber" )) return true;
            return false;
        }        
    }
}

