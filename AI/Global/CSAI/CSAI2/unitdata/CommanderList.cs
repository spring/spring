using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class CommanderList
    {
        static CommanderList instance = new CommanderList();
        public static CommanderList GetInstance() { return instance; }

        List<string> commandernames = new List<string>(new string[] { "arm_commander", "core_commander" });

        public Dictionary<int, IUnitDef> defbyid = new Dictionary<int, IUnitDef>();

        CommanderList()
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
                if (commandernames.Contains(name))
                {
                    LogFile.GetInstance().WriteLine("commanderlist unitaddedevent " + deployedid + " " + unitdef.humanName);
                    defbyid.Add(deployedid, unitdef);
                }
            }
        }
    }
}
