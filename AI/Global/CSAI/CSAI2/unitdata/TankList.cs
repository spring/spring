using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class TankList
    {
        static TankList instance = new TankList();
        public static TankList GetInstance() { return instance; }

        List<string> unitnames = new List<string>(new string[] { "armsam", "armstump", "armrock", "armjeth", "armkam", "armanac", "armsfig", "armmh", "armah", 
            "armbull", "armmart", "armmav", "armyork", "corak" });

        public Dictionary<int, IUnitDef> defbyid = new Dictionary<int, IUnitDef>();

        TankList()
        {
            LogFile.GetInstance().WriteLine("TankList() >>>");
            UnitController.GetInstance().UnitAddedEvent += new UnitController.UnitAddedHandler(UnitAddedEvent);
            UnitController.GetInstance().UnitRemovedEvent += new UnitController.UnitRemovedHandler(UnitRemovedEvent);
            LogFile.GetInstance().WriteLine("TankList() <<<");
        }

        void UnitRemovedEvent(int deployedid)
        {
            if (defbyid.ContainsKey(deployedid))
            {
                defbyid.Remove(deployedid);
            }
        }

        void UnitAddedEvent(int deployedid, IUnitDef unitdef)
        {
            LogFile.GetInstance().WriteLine("TankList.UnitAddedEvent " + deployedid + " " + unitdef.humanName);
            if (!defbyid.ContainsKey(deployedid))
            {
                string name = unitdef.name.ToLower();
                if (unitnames.Contains(name))
                {
                    LogFile.GetInstance().WriteLine("TankList.UnitAddedEvent " + deployedid + " " + unitdef.humanName + " ACCEPTED");
                    defbyid.Add(deployedid, unitdef);
                }
            }
        }
    }
}
