using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    class TimeHelper
    {
        public static TimeSpan GetGameTime()
        {
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            int frames = aicallback.GetCurrentFrame();
            return TimeSpan.FromSeconds((double)frames / 30);
        }
        public static string GetGameTimeString()
        {
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            int frames = aicallback.GetCurrentFrame();
            TimeSpan gametime = TimeSpan.FromSeconds((double)frames / 30);
            string secondfractionstring = ( gametime.Milliseconds / 10 ).ToString().PadLeft( 2, '0' );
            return gametime.Hours + ":" + gametime.Minutes + ":" + gametime.Seconds + "." + secondfractionstring;
        }
        public static string FormatTimeSpan(TimeSpan timespan)
        {
            string secondfractionstring = (timespan.Milliseconds / 10).ToString().PadLeft(2, '0');
            return timespan.Hours + ":" + timespan.Minutes + ":" + timespan.Seconds + "." + secondfractionstring;
        }
    }
}
