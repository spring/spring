using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    class MobileFusionController
    {
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        public MobileFusionController()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();

            UnitController.GetInstance().UnitAddedEvent += new UnitController.UnitAddedHandler(MobileFusionController_UnitAddedEvent);
            UnitController.GetInstance().AllUnitsLoaded += new UnitController.AllUnitsLoadedHandler(MobileFusionController_AllUnitsLoaded);
        }

        void MobileFusionController_AllUnitsLoaded()
        {
            foreach (int mob in mobileunits)
            {
                Float3 targetpos = aicallback.GetUnitPos(UnitController.GetInstance().UnitDefsByName["armmex"][0].id);
                //aicallback.GiveOrder(mob, new Command(Command.CMD_PATROL, targetpos.ToDoubleArray()));
                GiveOrderWrapper.GetInstance().MoveTo(mob, targetpos);
            }
        }

        List<int> mobileunits = new List<int>();

        void MobileFusionController_UnitAddedEvent(int deployedid, IUnitDef unitdef)
        {
            if (unitdef.name.ToLower() == "armmfus")
            {
                mobileunits.Add(deployedid);
            }
        }

    }
}
