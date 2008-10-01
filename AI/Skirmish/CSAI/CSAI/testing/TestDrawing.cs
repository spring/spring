using System;

using CSharpAI;

namespace Testing
{
    public class TestDrawing
    {
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;
        
        public void Go()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
            
            csai.RegisterVoiceCommand( "dumppoints", new CSAI.VoiceCommandHandler( this.DumpPoints ) );
            csai.RegisterVoiceCommand( "drawradii", new CSAI.VoiceCommandHandler( this.DrawRadii ) );
        }
        
        void DumpPoints( string cmd, string[]splitcmd, int player )
        {
            MapPoint[] mappoints = aicallback.GetMapPoints();
            foreach( MapPoint mappoint in mappoints )
            {
                aicallback.DrawUnit( "ARMSOLAR", mappoint.Pos, 0.0f, 100, aicallback.GetMyAllyTeam(), true, true);
                logfile.WriteLine( "mappoint: " + mappoint.Pos.ToString() + " " + mappoint.Label );
            }
            logfile.Flush();
        }
        
        void DrawRadii( string cmd, string[]splitcmd, int player )
        {
            Float3 commanderpos = CommanderController.GetInstance().GetCommanderPos();
            for( int i = 100; i <= 1000; i+= 100 )
            {
                DrawingUtils.DrawCircle( commanderpos, i );
            }
        }
    }
}
