using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    // stores the start position
    // difficult job I know ;-)
    // actually, harder than it sounds, because of .reloadai
    // so we need to serialize it during reloads
    // or.... ????
    // add a point???
    public class StartPosition
    {
        static StartPosition instance = new StartPosition();
        public static StartPosition GetInstance() { return instance; }

        StartPosition()
        {
            CSAI.GetInstance().UnitCreatedEvent += new CSAI.UnitCreatedHandler(UnitCreatedEvent);
        }

        public Float3 startposition = null;

        void UnitCreatedEvent(int deployedunitid, IUnitDef unitdef)
        {
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            if (aicallback.GetCurrentFrame() <= 1)
            {
                if (unitdef.isCommander)
                {
                    startposition = aicallback.GetUnitPos(deployedunitid);
                    //aicallback.get
                }
            }
        }
    }
}
