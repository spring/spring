using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class HelicopterList
    {
        static HelicopterList instance = new HelicopterList();
        public static HelicopterList GetInstance() { return instance; }

        List<string> unitnames = new List<string>(new string[] { "armbrawl" });

        public Dictionary<int, IUnitDef> defbyid = new Dictionary<int, IUnitDef>();

        HelicopterList()
        {
            UnitController.GetInstance().UnitAddedEvent += new UnitController.UnitAddedHandler(UnitAddedEvent);
            UnitController.GetInstance().UnitRemovedEvent += new UnitController.UnitRemovedHandler(UnitRemovedEvent);
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
            if (!defbyid.ContainsKey(deployedid))
            {
                string name = unitdef.name.ToLower();
                if (unitnames.Contains(name))
                {
                    defbyid.Add(deployedid, unitdef);
                }
            }
        }
    }
}
