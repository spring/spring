// Copyright Hugh Perkins 2006
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURVector3E. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//
// ======================================================================================
//

using System;
using System.IO;
using System.Collections;
using System.Xml;

namespace CSharpAI
{
    public class Metal
    {
        public const string MetalClassVersion = "0.4"; // used for detecting cache validity; if cache was built with older version we rebuild it
        
        public int MinMetalForSpot = 30; // from 0-255, the minimum percentage of metal a spot needs to have
                                //from the maximum to be saved. Prevents crappier spots in between taken spaces.
                                //They are still perfectly valid and will generate metal mind you!
        public int MaxSpots = 5000; //If more spots than that are found the map is considered a metalmap, tweak this as needed        
        //int MaxSpots = 3;
        
        public MetalSpot[] MetalSpots;
        public ArrayList MetalSpotsUsed = new ArrayList(); // arraylist of MetalSpots
        public bool isMetalMap = false;
            
        public Hashtable Extractors = new Hashtable();
        
        double ExtractorRadius;
        
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        UnitController unitcontroller;
        UnitDefHelp unitdefhelp;
        
        static Metal instance = new Metal();
        public static Metal GetInstance(){ return instance; } // Singleton pattern
                    
        Metal() // protected constructor to force Singleton instantiation
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            unitcontroller = UnitController.GetInstance();
            
            unitdefhelp = new UnitDefHelp( aicallback );

            ExtractorRadius = aicallback.GetExtractorRadius();

            csai.UnitCreatedEvent += new CSAI.UnitCreatedHandler(UnitAdded);
            unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler( UnitAdded );
            unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler( UnitRemoved );
            
            csai.RegisterVoiceCommand( "showmetalspots", new CSAI.VoiceCommandHandler( this.DrawMetalSpotsCommand ) );            
        }
        
        public void Init()
        {
            if( !LoadCache() )
            {
                aicallback.SendTextMsg( "Metal analyzer rebuilding cachefile", 0 );
                SearchMetalSpots();
            }
            if( !isMetalMap )
            {
                ReserveMetalExtractorSpaces();
            }            
        }
        
        void ReserveMetalExtractorSpaces()
        {
            foreach( object metalspotobject in MetalSpots )
            {
                MetalSpot metalspot = metalspotobject as MetalSpot;
                logfile.WriteLine("reserving space for " + metalspot.Pos.ToString() );
                BuildMap.GetInstance().ReserveSpace( this, (int)( metalspot.Pos.x / 8 ), (int)( metalspot.Pos.z / 8 ), 6, 6 );
            }
        }
        
        public void UnitAdded( int deployedid , IUnitDef unitdef )
        {
            if( !isMetalMap )
            {
                if( !Extractors.Contains( deployedid ) && unitdefhelp.IsMex( unitdef ) )
                {
                    Float3 mexpos = aicallback.GetUnitPos( deployedid );
                    logfile.WriteLine( "Metal.UnitAdded, pos " + mexpos.ToString() );
                    Extractors.Add( deployedid, mexpos );
                    double squareextractorradius = ExtractorRadius * ExtractorRadius;
                    foreach( MetalSpot metalspot in MetalSpots )
                    {
                        double thisdistancesquared = Float3Helper.GetSquaredDistance( metalspot.Pos, mexpos );
                     //   logfile.WriteLine( "squareextractorradius: " + squareextractorradius + " thisdistancesquared: " + thisdistancesquared );
                        if( thisdistancesquared <= squareextractorradius )
                        {
                            MetalSpotsUsed.Add( metalspot );
                            logfile.WriteLine( "Marking metal spot used: " + metalspot.Pos.ToString() );
                        }
                    }
                }
            }
        }
        
        public void UnitRemoved( int deployedid )
        {
            if( !isMetalMap )
            {
                if( Extractors.Contains( deployedid ) )
                {
                    Float3 mexpos = Extractors[ deployedid ] as Float3;
                    logfile.WriteLine( "Metal.UnitRemoved, pos " + mexpos.ToString() );
                    double squareextractorradius = ExtractorRadius * ExtractorRadius;
                    foreach( MetalSpot metalspot in MetalSpots )
                    {
                        if( Float3Helper.GetSquaredDistance( metalspot.Pos, mexpos ) < squareextractorradius )
                        {
                            if( MetalSpotsUsed.Contains( metalspot ) )
                            {
                                logfile.WriteLine( "Marking metal spot free: " + metalspot.Pos.ToString() );
                                MetalSpotsUsed.Remove( metalspot );
                            }
                        }
                    }
                    Extractors.Remove( deployedid );
                }
            }
        }
        
        public Float3 GetNearestMetalSpot( Float3 mypos )
        {
            if( !isMetalMap )
            {
                double closestdistancesquared = 1000000000000;
                Float3 bestpos = null;
                MetalSpot bestspot = null;
                foreach( MetalSpot metalspot in MetalSpots )
                {
                    if( !MetalSpotsUsed.Contains( metalspot ) )
                    {
                        if( bestpos == null )
                        {
                            bestpos = metalspot.Pos;
                        }
                        double thisdistancesquared = Float3Helper.GetSquaredDistance( mypos, metalspot.Pos );
                        //logfile.WriteLine( "thisdistancesquared = " + thisdistancesquared + " closestdistancesquared= " + closestdistancesquared );
                        if( thisdistancesquared < closestdistancesquared )
                        {
                            closestdistancesquared = thisdistancesquared;
                            bestpos = metalspot.Pos;
                            bestspot = metalspot;
                        }
                    }
                }
                return bestspot.Pos;
            }
            else
            {
                return mypos; // if metal map just return passed-in pos
            }
        }
        
        public void DrawMetalSpotsCommand( string chatstring, string[] splitchatstring, int player )
        {
            DrawMetalSpots();
        }
        
        public class MetalSpot
        {
            public int Amount;
            public Float3 Pos = new Float3();
            public bool IsOccupied = false;
                
            public MetalSpot(){}
            public MetalSpot( int amount, Float3 pos )
            {
                Amount = amount;
                Pos = pos;
            }
            public MetalSpot( int amount, Float3 pos, bool isoccupied )
            {
                Amount = amount;
                IsOccupied = isoccupied;
                Pos = pos;
            }
            public override string ToString()
            {
                return "MetalSpot( Pos=" + Pos.ToString() + ", Amount=" + Amount + ", IsOccupied=" + IsOccupied;
            }
        }
        
        // for debugging / convincing oneself spots are in right place
        void DrawMetalSpots()
        {
            if( !isMetalMap )
            {
                foreach( object metalspotobject in MetalSpots )
                {
                    MetalSpot metalspot = metalspotobject as MetalSpot;
                    logfile.WriteLine("drawing spot at " + metalspot.Pos );
                    aicallback.DrawUnit("arm_metal_extractor", metalspot.Pos, 0.0f, 500, aicallback.GetMyAllyTeam(), true, true);
                }
                foreach( object metalspotobject in MetalSpotsUsed )
                {
                    MetalSpot metalspot = metalspotobject as MetalSpot;
                    logfile.WriteLine("drawing usedspot at " + metalspot.Pos );
                    aicallback.DrawUnit("ARMFORT", metalspot.Pos, 0.0f, 500, aicallback.GetMyAllyTeam(), true, true);
                }
            }
            else
            {
                aicallback.SendTextMsg( "Metal analyzer reports this is a metal map", 0 );
            }
        }
        
        // loads cache file
        // returns true if cache loaded ok, otherwise false if not found, out-of-date, etc
        // we check the version and return false if out-of-date
        bool LoadCache()
        {
            string MapName = aicallback.GetMapName();
            string cachefilepath = Path.Combine( csai.CacheDirectoryPath, MapName + "_metal.xml" );
            
            if( !File.Exists( cachefilepath ) )
            {
                logfile.WriteLine( "cache file doesnt exist -> building" );
                return false;
            }
            
            XmlDocument cachedom = XmlHelper.OpenDom( cachefilepath );
            XmlElement metadata = cachedom.SelectSingleNode( "/root/metadata" ) as XmlElement;
            string cachemetalclassversion = metadata.GetAttribute( "version" );
            if( cachemetalclassversion != MetalClassVersion )
            {
                logfile.WriteLine( "cache file out of date ( " + cachemetalclassversion + " vs " + MetalClassVersion + " ) -> rebuilding" );
                return false;
            }
            
            logfile.WriteLine( cachedom.InnerXml );
            
            isMetalMap = Convert.ToBoolean( metadata.GetAttribute( "ismetalmap" ) );
            
            if( isMetalMap )
            {
                logfile.WriteLine( "metal map" );
                return true;
            }
            
            
            XmlElement metalspots = cachedom.SelectSingleNode( "/root/metalspots" ) as XmlElement;
            ArrayList metalspotsal = new ArrayList();
            foreach( XmlElement metalspot in metalspots.SelectNodes( "metalspot" ) )
            {
                int amount = Convert.ToInt32( metalspot.GetAttribute("amount") );
                Float3 pos = new Float3();
                Float3Helper.WriteXmlElementToFloat3( metalspot, pos );
                //pos.LoadCsv( metalspot.GetAttribute("pos") );
                MetalSpot newmetalspot = new MetalSpot( amount, pos );
                metalspotsal.Add( newmetalspot );
                // logfile.WriteLine( "metalspot xml: " + metalspot.InnerXml );
                logfile.WriteLine( "metalspot: " + newmetalspot.ToString() );
            }
            MetalSpots = (MetalSpot[])metalspotsal.ToArray( typeof( MetalSpot ) );
            
            logfile.WriteLine( "cache file loaded" );
            return true;
        }
        
        // save cache to speed up load times
        // we store the MetalClassVersion to enable cache rebuild if algo changed
        void SaveCache()
        {
            string MapName = aicallback.GetMapName();
            string cachefilepath = Path.Combine( csai.CacheDirectoryPath, MapName + "_metal.xml" );
            
            XmlDocument cachedom = XmlHelper.CreateDom();
            XmlElement metadata = XmlHelper.AddChild( cachedom.DocumentElement, "metadata" );
            metadata.SetAttribute( "type", "MetalCache" );
            metadata.SetAttribute( "map", MapName );
            metadata.SetAttribute( "version", MetalClassVersion );
            metadata.SetAttribute( "datecreated", DateTime.Now.ToString() );
            metadata.SetAttribute( "ismetalmap", isMetalMap.ToString() );
            metadata.SetAttribute( "extractorradius", aicallback.GetExtractorRadius().ToString() );
            metadata.SetAttribute( "mapheight", aicallback.GetMapHeight().ToString() );
            metadata.SetAttribute( "mapwidth", aicallback.GetMapWidth().ToString() );
            
            XmlElement metalspots = XmlHelper.AddChild( cachedom.DocumentElement, "metalspots" );
            foreach( MetalSpot metalspot in MetalSpots )
            {
                XmlElement metalspotnode = XmlHelper.AddChild( metalspots, "metalspot" );
                //metalspotnode.SetAttribute( "pos", metalspot.Pos.ToCsv() );
                Float3Helper.WriteFloat3ToXmlElement( metalspot.Pos, metalspotnode );
                metalspotnode.SetAttribute( "amount", metalspot.Amount.ToString() );
            }
            
            cachedom.Save( cachefilepath );
        }
        
        // algorithm more or less by krogothe
        // ported from Submarine's original C++ version
        int [,]CalculateAvailableMetalForEachSpot( int[,] metalremaining, int ExtractorRadius, out int maxmetalspotamount )
        {
            int mapwidth = metalremaining.GetUpperBound(0) + 1;
            int mapheight = metalremaining.GetUpperBound(1) + 1;
            int SquareExtractorRadius = (int)( ExtractorRadius * ExtractorRadius );
            int [,]SpotAvailableMetal = new int[ mapwidth, mapheight ];
            
            logfile.WriteLine( "mapwidth: " + mapwidth + " mapheight: " + mapheight + " ExtractorRadius: " + ExtractorRadius + " SquareExtractorRadius: " + SquareExtractorRadius );
            
            // Now work out how much metal each spot can make by adding up the metal from nearby spots
            // we scan each point on map, and for each point, we scan from + to - ExtractorRadius, eliminating anything where
            // the actual straight line distance is more than the ExtractorRadius
            maxmetalspotamount = 0;
            for (int spoty = 0; spoty < mapheight; spoty++)
            {
                for (int spotx = 0; spotx < mapwidth; spotx++)
                {
                    int metalthisspot = 0;
                    //get the metal from all pixels around the extractor radius 
                    for (int deltax = - ExtractorRadius; deltax <= ExtractorRadius; deltax++)
                    {
                        int thisx = spotx + deltax;
                        if ( thisx >= 0 && thisx < mapwidth )
                        {
                            for (int deltay = - ExtractorRadius; deltay <= ExtractorRadius; deltay++)
                            {
                                int thisy = spoty + deltay;
                                if ( thisy >= 0 && thisy < mapheight )
                                {
                                   if( ( deltax * deltax + deltay * deltay ) <= SquareExtractorRadius )
                                    {
                                        metalthisspot += metalremaining[ thisx, thisy ]; 
                                    }
                                }
                            }
                        }
                    }
                    SpotAvailableMetal[ spotx, spoty ] = metalthisspot; //set that spots metal making ability (divide by cells to values are small)
                    maxmetalspotamount = Math.Max( metalthisspot, maxmetalspotamount ); //find the spot with the highest metal to set as the map's max
                }
            }
            
          //  logfile.WriteLine ("*******************************************");
            return SpotAvailableMetal;
        }
        
        // algorithm more or less by krogothe
        // ported from Submarine's original C++ version
        public void SearchMetalSpots()
        {	
            logfile.WriteLine( "SearchMetalSpots() >>>");
            
            isMetalMap = false;
        
            ArrayList metalspotsal = new ArrayList();
            
            int mapheight = aicallback.GetMapHeight() / 2; //metal map has 1/2 resolution of normal map
            int mapwidth = aicallback.GetMapWidth() / 2;
            double mapmaxmetal = aicallback.GetMaxMetal();
            int totalcells = mapheight * mapwidth;
            
            logfile.WriteLine( "mapwidth: " + mapwidth + " mapheight " + mapheight + " maxmetal:" + mapmaxmetal );
            
            byte[] metalmap = aicallback.GetMetalMap(); // original metal map
            int[,] metalremaining = new int[ mapwidth, mapheight ];  // actual metal available at that point. we remove metal from this as we add spots to MetalSpots
            int[,] SpotAvailableMetal = new int [ mapwidth, mapheight ]; // amount of metal an extractor on this spot could make
            int[,] NormalizedSpotAvailableMetal = new int [ mapwidth, mapheight ]; // SpotAvailableMetal, normalized to 0-255 range
        
            int totalmetal = 0;
            ArrayIndexer arrayindexer = new ArrayIndexer( mapwidth, mapheight );
            //Load up the metal Values in each pixel
            logfile.WriteLine( "width: " + mapwidth + " height: " + mapheight );
            for (int y = 0; y < mapheight; y++)
            {
                //string logline = "";
                for( int x = 0; x < mapwidth; x++ )
                {
                    metalremaining[ x, y ] = (int)metalmap[ arrayindexer.GetIndex( x, y ) ];
                    totalmetal += metalremaining[ x, y ];		// Count the total metal so you can work out an average of the whole map
                    //logline += metalremaining[ x, y ].ToString() + " ";
                  //  logline += metalremaining[ x, y ] + " ";
                }
               // logfile.WriteLine( logline );
            }
            logfile.WriteLine ("*******************************************");
        
            double averagemetal = ((double)totalmetal) / ((double)totalcells);  //do the average
           // int maxmetal = 0;

            int ExtractorRadius = (int)( aicallback.GetExtractorRadius() / 16.0 );
            int DoubleExtractorRadius = ExtractorRadius * 2;
            int SquareExtractorRadius = ExtractorRadius * ExtractorRadius; //used to speed up loops so no recalculation needed
            int FourSquareExtractorRadius = 4 * SquareExtractorRadius; // same as above 
            double CellsInRadius = Math.PI * ExtractorRadius * ExtractorRadius;
            
            int maxmetalspotamount = 0;
            logfile.WriteLine( "Calculating available metal for each spot..." );
            SpotAvailableMetal = CalculateAvailableMetalForEachSpot( metalremaining, ExtractorRadius, out maxmetalspotamount );
            
            logfile.WriteLine( "Normalizing..." );
            // normalize the metal so any map will have values 0-255, no matter how much metal it has
            int[,] NormalizedMetalRemaining = new int[ mapwidth, mapheight ];
            for (int y = 0; y < mapheight; y++)
            {
                for (int x = 0; x < mapwidth; x++)
                {
                    NormalizedSpotAvailableMetal[ x, y ] = ( SpotAvailableMetal[ x, y ] * 255 ) / maxmetalspotamount;
                }
            }
            
            logfile.WriteLine( "maxmetalspotamount: " + maxmetalspotamount );
            
            bool Stopme = false;
            int SpotsFound = 0;
            //logfile.WriteLine( BuildTable.GetInstance().GetBiggestMexUnit().ToString() );
           // IUnitDef biggestmex = BuildTable.GetInstance().GetBiggestMexUnit();
           // logfile.WriteLine( "biggestmex is " + biggestmex.name + " " + biggestmex.humanName );
            for (int spotindex = 0; spotindex < MaxSpots && !Stopme; spotindex++)
            {	                
                logfile.WriteLine( "spotindex: " + spotindex );
                int bestspotx = 0, bestspoty = 0;
                int actualmetalatbestspot = 0; // use to try to put extractors over spot itself
                //finds the best spot on the map and gets its coords
                int BestNormalizedAvailableSpotAmount = 0;
                for (int y = 0; y < mapheight; y++)
                {
                    for (int x = 0; x < mapwidth; x++)
                    {
                        if( NormalizedSpotAvailableMetal[ x, y ] > BestNormalizedAvailableSpotAmount ||
                            ( NormalizedSpotAvailableMetal[ x, y ] == BestNormalizedAvailableSpotAmount && 
                            metalremaining[ x, y ] > actualmetalatbestspot ) )
                        {
                            BestNormalizedAvailableSpotAmount = NormalizedSpotAvailableMetal[ x, y ];
                            bestspotx = x;
                            bestspoty = y;
                            actualmetalatbestspot = metalremaining[ x, y ];
                        }
                    }
                }		
                logfile.WriteLine( "BestNormalizedAvailableSpotAmount: " + BestNormalizedAvailableSpotAmount );                
                if( BestNormalizedAvailableSpotAmount < MinMetalForSpot )
                {
                    Stopme = true; // if the spots get too crappy it will stop running the loops to speed it all up
                    logfile.WriteLine( "Remaining spots too small; stopping search" );
                }
        
                if( !Stopme )
                {
                    Float3 pos = new Float3();
                    pos.x = bestspotx * 2 * MovementMaps.SQUARE_SIZE;
                    pos.z = bestspoty * 2 * MovementMaps.SQUARE_SIZE;
                    pos.y = aicallback.GetElevation( pos.x, pos.z );
                    
                    //pos = Map.PosToFinalBuildPos( pos, biggestmex );
                
                    logfile.WriteLine( "Metal spot: " + pos + " " + BestNormalizedAvailableSpotAmount );
                    MetalSpot thismetalspot = new MetalSpot( (int)( ( BestNormalizedAvailableSpotAmount  * mapmaxmetal * maxmetalspotamount ) / 255 ), pos );
                    
                //    if (aicallback.CanBuildAt(biggestmex, pos) )
                  //  {
                       // pos = Map.PosToBuildMapPos( pos, biggestmex );
                       // logfile.WriteLine( "Metal spot: " + pos + " " + BestNormalizedAvailableSpotAmount );
                        
                   //     if(pos.z >= 2 && pos.x >= 2 && pos.x < mapwidth -2 && pos.z < mapheight -2)
                  //      {
                          //  if(CanBuildAt(pos.x, pos.z, biggestmex.xsize, biggestmex.ysize))
                           // {
                                metalspotsal.Add( thismetalspot );			
                                SpotsFound++;
        
                                //if(pos.y >= 0)
                                //{
                                   // SetBuildMap(pos.x-2, pos.z-2, biggestmex.xsize+4, biggestmex.ysize+4, 1);
                                //}
                                //else
                                //{
                                    //SetBuildMap(pos.x-2, pos.z-2, biggestmex.xsize+4, biggestmex.ysize+4, 5);
                                //}
                          //  }
                     //   }
                 //   }
                        
                    for (int myx = bestspotx - (int)ExtractorRadius; myx < bestspotx + (int)ExtractorRadius; myx++)
                    {
                        if (myx >= 0 && myx < mapwidth )
                        {
                            for (int myy = bestspoty - (int)ExtractorRadius; myy < bestspoty + (int)ExtractorRadius; myy++)
                            {
                                if ( myy >= 0 && myy < mapheight &&
                                    ( ( bestspotx - myx ) * ( bestspotx - myx ) + ( bestspoty - myy ) * ( bestspoty - myy ) ) <= (int)SquareExtractorRadius )
                                {
                                    metalremaining[ myx, myy ] = 0; //wipes the metal around the spot so its not counted twice
                                    NormalizedSpotAvailableMetal[ myx, myy ] = 0;
                                }
                            }
                        }
                    }
        
                    // Redo the whole averaging process around the picked spot so other spots can be found around it
                    for (int y = bestspoty - (int)DoubleExtractorRadius; y < bestspoty + (int)DoubleExtractorRadius; y++)
                    {
                        if(y >=0 && y < mapheight)
                        {
                            for (int x = bestspotx - (int)DoubleExtractorRadius; x < bestspotx + (int)DoubleExtractorRadius; x++)
                            {
                                //funcion below is optimized so it will only update spots between r and 2r, greatly speeding it up
                                if((bestspotx - x)*(bestspotx - x) + (bestspoty - y)*(bestspoty - y) <= (int)FourSquareExtractorRadius && 
                                    x >=0 && x < mapwidth && 
                                    NormalizedSpotAvailableMetal[ x, y ] > 0 )
                                {
                                    totalmetal = 0;
                                    for (int myx = x - (int)ExtractorRadius; myx < x + (int)ExtractorRadius; myx++)
                                    {
                                        if (myx >= 0 && myx < mapwidth )
                                        {
                                            for (int myy = y - (int)ExtractorRadius; myy < y + (int)ExtractorRadius; myy++)
                                            { 
                                                if (myy >= 0 && myy < mapheight && 
                                                    ((x - myx)*(x - myx) + (y - myy)*(y - myy)) <= (int)SquareExtractorRadius )
                                                {
                                                    totalmetal += metalremaining[ myx, myy ]; //recalculate nearby spots to account for deleted metal from chosen spot
                                                }
                                            }
                                        }
                                    }
                                    NormalizedSpotAvailableMetal[ x, y ] = totalmetal * 255 / maxmetalspotamount; //set that spots metal amount 
                                }
                            }
                        }
                    }
                }
            }
        
            if(SpotsFound > 500)
            {
                isMetalMap = true;
                metalspotsal.Clear();
                logfile.WriteLine( "Map is considered to be a metal map" );
            }
            else
            {
                isMetalMap = false;
        
                // debug
                //for(list<AAIMetalSpot>::iterator spot = metal_spots.begin(); spot != metal_spots.end(); spot++)
            }
            
            MetalSpots = ( MetalSpot[] )metalspotsal.ToArray( typeof( MetalSpot ) );
            
            SaveCache();
            logfile.WriteLine( "SearchMetalSpots() <<<");
        }
    }
}
