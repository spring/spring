using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class Level1ConstructorList
    {
        static Level1ConstructorList instance = new Level1ConstructorList();
        public static Level1ConstructorList GetInstance() { return instance; }

        List<string> constructorunitnames = new List<string>(new string[] { "armcv", "corcv" });

        public Dictionary<int, IUnitDef> defbyid = new Dictionary<int, IUnitDef>();

        Level1ConstructorList()
        {
            UnitController.GetInstance().UnitAddedEvent += new UnitController.UnitAddedHandler(Level1ConstructorList_UnitAddedEvent);
            UnitController.GetInstance().UnitRemovedEvent += new UnitController.UnitRemovedHandler(Level1ConstructorList_UnitRemovedEvent);
        }

        void Level1ConstructorList_UnitRemovedEvent(int deployedid)
        {
            if (defbyid.ContainsKey(deployedid))
            {
                defbyid.Remove(deployedid);
            }
        }

        void Level1ConstructorList_UnitAddedEvent(int deployedid, IUnitDef unitdef)
        {
            if (!defbyid.ContainsKey(deployedid))
            {
                string name = unitdef.name.ToLower();
                if (constructorunitnames.Contains(name))
                {
                    defbyid.Add(deployedid, unitdef);
                }
            }
        }
    }
}
