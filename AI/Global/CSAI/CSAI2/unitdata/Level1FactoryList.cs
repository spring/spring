using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class Level1FactoryList
    {
        static Level1FactoryList instance = new Level1FactoryList();
        public static Level1FactoryList GetInstance() { return instance; }

        List<string> factorynames = new List<string>( new string[] { "armvp", "armlab", "corvp", "corlab", "armap", "corap" } );

        public Dictionary<int, IUnitDef> defbyid = new Dictionary<int, IUnitDef>();

        Level1FactoryList()
        {
            UnitController.GetInstance().UnitAddedEvent += new UnitController.UnitAddedHandler(Level1FactoryList_UnitAddedEvent);
            UnitController.GetInstance().UnitRemovedEvent += new UnitController.UnitRemovedHandler(Level1FactoryList_UnitRemovedEvent);
        }

        void Level1FactoryList_UnitRemovedEvent(int deployedid)
        {
            if (defbyid.ContainsKey(deployedid))
            {
                defbyid.Remove(deployedid);
            }
        }

        void Level1FactoryList_UnitAddedEvent(int deployedid, IUnitDef unitdef)
        {
            if (!defbyid.ContainsKey(deployedid))
            {
                string name = unitdef.name.ToLower();
                if (factorynames.Contains(name))
                {
                    defbyid.Add(deployedid, unitdef);
                }
            }
        }
    }
}
