using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    // caches last instruction given to a unit
    public class UnitCommandCache
    {
        static UnitCommandCache instance = new UnitCommandCache();
        public static UnitCommandCache GetInstance() { return instance; }

        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        UnitCommandCache()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();
        }

        public Dictionary<int,OOPCommand> LastCommandByUnit = new Dictionary<int, OOPCommand>();

        public void RegisterCommand(int unitid, OOPCommand command)
        {
            if (!LastCommandByUnit.ContainsKey(unitid))
            {
                LastCommandByUnit.Add(unitid, command);
            }
            else
            {
                LastCommandByUnit[unitid] = command;
            }
        }
    }
}
