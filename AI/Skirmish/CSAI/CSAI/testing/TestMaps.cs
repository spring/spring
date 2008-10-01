using System;

using CSharpAI;

namespace Testing
{
    public class TestMaps
    {
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        // note: sector size for scout probably should be 4 times larger than radar map squaresize, ie should be 256 * 256 mappos
        
        public void Go()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            csai.TickEvent+= new CSAI.TickHandler( Tick );
            
            csai.RegisterVoiceCommand( "dumplos", new CSAI.VoiceCommandHandler( this.DumpLos ) );
            csai.RegisterVoiceCommand( "dumpradar", new CSAI.VoiceCommandHandler( this.DumpRadar ) );
            csai.RegisterVoiceCommand( "dumpcentreheights", new CSAI.VoiceCommandHandler( this.DumpCentreHeights ) );
            csai.RegisterVoiceCommand( "dumpfeatures", new CSAI.VoiceCommandHandler( this.DumpFeaturesMap ) );
            csai.RegisterVoiceCommand( "dumpslopes", new CSAI.VoiceCommandHandler( this.DumpSlopes ) );
            csai.RegisterVoiceCommand( "dumpbuildmap", new CSAI.VoiceCommandHandler( this.DumpBuildMap ) );
            csai.RegisterVoiceCommand( "dumpmovementareas", new CSAI.VoiceCommandHandler( this.DumpMovementAreas ) );
            csai.RegisterVoiceCommand( "dumplosmap", new CSAI.VoiceCommandHandler( this.DumpLosMap ) );
        }
            
        public void DumpLos( string cmd, string[]cmdsplit, int player )
        {
            logfile.WriteLine( "calling getlosmap..." );
            bool[] LosMap = aicallback.GetLosMap();
            logfile.WriteLine( "...done" );
            int loswidth = aicallback.GetMapWidth() / 2;
            int losheight = aicallback.GetMapHeight() / 2;
            logfile.WriteLine( "losmap width: " + loswidth + " losheight: " + losheight );
            ArrayIndexer arrayindexer = new ArrayIndexer( loswidth, losheight );
            for( int y = 0; y < losheight; y++ )
            {
                string line = "";
                for( int x = 0; x < loswidth; x++ )
                {
                    if( LosMap[ arrayindexer.GetIndex( x, y ) ] )
                    {
                        line += "*";
                      //  aicallback.DrawUnit( "ARMCOM", new Float3( x * 16, 0, y * 16 ), 0.0f, 100, aicallback.GetMyAllyTeam(), true, true);
                    }
                    else
                    {
                        line += "-";
                    }
                }
                logfile.WriteLine( line );
            }
            aicallback.SendTextMsg("los dumped to logfile", 0 );
        }
        
        public void DumpRadar( string cmd, string[]cmdsplit, int player )
        {
            logfile.WriteLine( "calling GetRadarMap..." );
            bool[] LosMap = aicallback.GetRadarMap();
            logfile.WriteLine( "...done" );
            int loswidth = aicallback.GetMapWidth() / 8;
            int losheight = aicallback.GetMapHeight() / 8;
            logfile.WriteLine( "losmap width: " + loswidth + " losheight: " + losheight );
            ArrayIndexer arrayindexer = new ArrayIndexer( loswidth, losheight );
            for( int y = 0; y < losheight; y++ )
            {
                string line = "";
                for( int x = 0; x < loswidth; x++ )
                {
                    if( LosMap[ arrayindexer.GetIndex( x, y ) ] )
                    {
                        aicallback.DrawUnit( "ARMRAD", new Float3( x * 64, 0, y * 64 ), 0.0f, 100, aicallback.GetMyAllyTeam(), true, true);
                        line += "*";
                    }
                    else
                    {
                        line += "-";
                    }
                }
                logfile.WriteLine( line );
            }
            aicallback.SendTextMsg("radar dumped to logfile", 0 );
        }

        public void DumpCentreHeights( string cmd, string[]cmdsplit, int player )
        {
            logfile.WriteLine( "calling GetCentreHeightMap..." );
            double[] thismap = aicallback.GetCentreHeightMap();
            logfile.WriteLine( "...done" );
            int width = aicallback.GetMapWidth();
            int height = aicallback.GetMapHeight();
            logfile.WriteLine( "CentreHeightmap width: " + width + " losheight: " + height );
            ArrayIndexer arrayindexer = new ArrayIndexer( width, height );
            for( int y = 0; y < height; y+= 64 )
            {
                string line = "";
                for( int x = 0; x < width; x+= 64 )
                {
                    if( thismap[ arrayindexer.GetIndex( x, y ) ] > 0 )
                    {
                       // aicallback.DrawUnit( "ARMRAD", new Float3( x * 64, 0, y * 64 ), 0.0f, 100, aicallback.GetMyAllyTeam(), true, true);
                        line += "*";
                    }
                    else
                    {
                        line += "-";
                    }
                }
                logfile.WriteLine( line );
            }
            aicallback.SendTextMsg("Height dumped to logfile", 0 );
        }
        
        public void DumpFeaturesMap( string cmd, string[]cmdsplit, int player )
        {
            int featuremappospersquare = 64;
            int width = aicallback.GetMapWidth() * 8 / featuremappospersquare;
            int height = aicallback.GetMapHeight() * 8 / featuremappospersquare;
            bool[,] featuremap = new bool[ width, height ];
            logfile.WriteLine( "requesting features" );
            int[] featureids = aicallback.GetFeatures();
            logfile.WriteLine( "got features" );
            foreach( int featureid in featureids )
            {
                Float3 pos = aicallback.GetFeaturePos( featureid );
                int featuremapposx = (int)( pos.x / featuremappospersquare ); // implies Floor
                int featuremapposy = (int)( pos.z / featuremappospersquare ); // implies Floor
                IFeatureDef featuredef = aicallback.GetFeatureDef( featureid );
                if( featuredef.metal > 0 )
                {
                    featuremap[ featuremapposx, featuremapposy ] = true;
                }
            }
            for( int y = 0; y < height; y+= 1 )
            {
                string line = "";
                for( int x = 0; x < width; x+= 1 )
                {
                    if( featuremap[ x, y ] )
                    {
                       // aicallback.DrawUnit( "ARMRAD", new Float3( x * 64, 0, y * 64 ), 0.0f, 100, aicallback.GetMyAllyTeam(), true, true);
                        line += "*";
                    }
                    else
                    {
                        line += "-";
                    }
                }
                logfile.WriteLine( line );
            }
            aicallback.SendTextMsg("features dumped to logfile", 0 );        
        }
        
        public void DumpSlopes( string cmd, string[]cmdsplit, int player )
        {
            int sectorpospersquare = 64;            
            int width = aicallback.GetMapWidth() / 2;
            int height = aicallback.GetMapHeight() / 2;
            double[,] slopemap = new SlopeMap().GetSlopeMap();            
            double maxslope = 0;
            int sectoredmapwidth = width * 16 / sectorpospersquare;
            int sectoredmapheight = height * 16 / sectorpospersquare;
            bool[,]sectoredslopemap = new bool[ sectoredmapwidth, sectoredmapheight ];
            for( int y = 0; y < height; y+= 1 )
            {
                string line = "";
                for( int x = 0; x < width; x+= 1 )
                {
                    maxslope = Math.Max( maxslope, slopemap[ x, y ] );
                    if( slopemap[ x, y ] > 0.08 )
                    //if( slopemap[ x, y ] > 0.33 )
                    {
                       // aicallback.DrawUnit( "ARMRAD", new Float3( x * 64, 0, y * 64 ), 0.0f, 100, aicallback.GetMyAllyTeam(), true, true);
                        line += "*";
                        sectoredslopemap[ x / 4, y / 4 ] = true;
                    }
                    else
                    {
                        line += "-";
                    }
                }
                logfile.WriteLine( line );
            }
            
            for( int y = 0; y < sectoredmapheight; y++ )
            {
                string line = "";
                for( int x = 0; x < sectoredmapwidth; x++ )
                {
                    if( sectoredslopemap[ x, y ] )
                    {
                        aicallback.DrawUnit( "ARMFORT", new Float3( x * 64, 0, y * 64 ), 0.0f, 100, aicallback.GetMyAllyTeam(), true, true);
                        line += "*";
                    }
                    else
                    {
                        line += "-";
                    }
                }
                logfile.WriteLine( line );
            }
            
            aicallback.SendTextMsg("slopes dumped to logfile", 0 );                 
            logfile.WriteLine("maxslope: " + maxslope );
        }
        
        public void DumpBuildMap( string cmd, string[]cmdsplit, int player )
        {
            for( int y = 0; y < aicallback.GetMapHeight(); y++ )
            {
                string line = "";
                for( int x = 0; x < aicallback.GetMapWidth(); x++ )
                {
                    if( BuildMap.GetInstance().SquareAvailable[ x, y ] )
                    {
                        line += "-";
                    }
                    else
                    {
                        aicallback.DrawUnit( "ARMMINE1", new Float3( x * 8, 0, y * 8 ), 0.0f, 400, aicallback.GetMyAllyTeam(), true, true);
                        line += "*";
                    }
                }
                logfile.WriteLine( line );
            }
        }
        
        public void DumpMovementAreas( string cmd, string[]cmdsplit, int player )
        {
            int figuregroup = DrawingUtils.DrawMap(MovementMaps.GetInstance().infantryareas);
            aicallback.SetFigureColor(figuregroup, 1, 0, 0,1);
            figuregroup = DrawingUtils.DrawMap(MovementMaps.GetInstance().vehicleareas);

            aicallback.SetFigureColor(figuregroup, 0, 0, 1, 1);

            figuregroup = DrawingUtils.DrawMap(MovementMaps.GetInstance().boatareas);
            aicallback.SetFigureColor(figuregroup, 0, 1, 0, 1);
        }
        
        void DumpLosMap( string cmd, string[] split, int player )
        {
            bool[,] losmapfordisplay = new bool[aicallback.GetMapWidth() / 8, aicallback.GetMapHeight() / 8];
            for (int y = 0; y < aicallback.GetMapHeight() / 2; y += 4)
            {
                for (int x = 0; x < aicallback.GetMapWidth() / 2; x += 4)
                {
                    losmapfordisplay[x / 4, y / 4] = (aicallback.GetCurrentFrame() - LosMap.GetInstance().LastSeenFrameCount[x, y]) < 6000;
                }
            }
            int figuregroup = DrawingUtils.DrawMap(losmapfordisplay);
            aicallback.SetFigureColor(figuregroup, 0, 1, 1, 1);
        }
        
        int tick = 0;
        void Tick()
        {
            tick++;
            if( tick == 30 )
            {
                DumpLosMap( "", null, 0 );
                tick = 0;
            }
        }
    }
}

