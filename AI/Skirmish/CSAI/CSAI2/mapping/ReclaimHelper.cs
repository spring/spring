using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class ReclaimHelper
    {
        static ReclaimHelper instance = new ReclaimHelper();
        public static ReclaimHelper GetInstance() { return instance; }

        static int maxreclaimradius = 500;
        static int reclaimradiusperonehundredmetal = 150;
        
        public ReclaimHelper()
        {
        }

        public static Float3 GetNearestReclaim(Float3 mypos, int constructorid)
        {
            if( CSAI.GetInstance().aicallback.GetCurrentFrame() == 0 )// check ticks first, beacuse metal shows as zero at start
            {
                return null;
            }
            LogFile logfile = LogFile.GetInstance();
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            IUnitDef unitdef = UnitController.GetInstance().UnitDefByDeployedId[ constructorid ];
            if (!new UnitDefHelp(aicallback).IsMobile(unitdef))
            {
                return null;
            }
            //Float3 mypos = aicallback.GetUnitPos( constructorid );
            MovementMaps movementmaps = MovementMaps.GetInstance();
            int currentarea = movementmaps.GetArea( unitdef, mypos );
            //double nearestreclaimdistancesquared = 1000000;
            //Float3 nearestreclaimpos = null;
            double bestmetaldistanceratio = 0;
            int bestreclaimid = 0;
            int metalspace = (int)( aicallback.GetMetalStorage() - aicallback.GetMetal() );
            logfile.WriteLine( "available space in metal storage: " + metalspace );
            int[] nearbyfeatures = aicallback.GetFeatures( mypos, maxreclaimradius );
            bool reclaimfound = false;
            foreach( int feature in nearbyfeatures )
            {
                IFeatureDef featuredef = aicallback.GetFeatureDef( feature );
                if( featuredef.metal > 0 && featuredef.metal <= metalspace  )
                {
                    Float3 thisfeaturepos = aicallback.GetFeaturePos( feature );
                    double thisdistance = Math.Sqrt( Float3Helper.GetSquaredDistance( thisfeaturepos, mypos ) );
                    double thismetaldistanceratio = featuredef.metal / thisdistance;
                    if( thismetaldistanceratio > bestmetaldistanceratio && movementmaps.GetArea( unitdef, thisfeaturepos ) == currentarea )
                    {
                        logfile.WriteLine( "Potential reclaim, distance = " + thisdistance + " metal = " + featuredef.metal + " ratio = " + thismetaldistanceratio );
                        bestmetaldistanceratio = thismetaldistanceratio;
                        bestreclaimid = feature;
               //         nearestreclaimpo
                        reclaimfound = true;
                    }
                }
            }
            if( reclaimfound && ( bestmetaldistanceratio > ( 1.0 / ( 100 * reclaimradiusperonehundredmetal ) ) ) )
            {
                Float3 reclaimpos = aicallback.GetFeaturePos( bestreclaimid );
                logfile.WriteLine( "Reclaim found, pos " + reclaimpos.ToString() );
                if( CSAI.GetInstance().DebugOn )
                {
                    aicallback.DrawUnit( "ARMMEX", reclaimpos, 0.0f, 200, aicallback.GetMyAllyTeam(), true, true);
                }
                return reclaimpos;
                //aicallback.GiveOrder( constructorid, new Command( Command.CMD_RECLAIM, 
                //    new double[]{ reclaimpos.x, reclaimpos.y, reclaimpos.z, 10 } ) );
            }
            else
            {
                //logfile.WriteLine( "No reclaim within parameters" );
                return null;
            }
        }
    }
}
