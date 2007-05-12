using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class BuildEconomy
    {
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        public double energymetalratio = 10.0;

        MobileFusionController mobile;

        public BuildEconomy()
        {
            Workflow workflow = new Workflow();
            mobile = new MobileFusionController();

            //workflow.AddBuildUnitWeighting(0.2, 0.4, "armmex");
            //workflow.AddBuildUnitWeighting(0.2, 0.6, "armsolar");

            workflow.AddEnergyUnit("arm_solar_collector");
            //workflow.AddEnergyUnit("armmfus");

            workflow.AddMetalUnit("arm_metal_extractor");

            workflow.BuildUnit(2.0, "armvp", 1);

            workflow.BuildUnit(2.1, "armfav", 2);
            workflow.BuildUnit(2.0, "armstump", 10);
            workflow.BuildUnit(2.0, "armsam", 10);
            workflow.BuildUnit(1.95, "armmex", 4);
            workflow.BuildUnit(1.95, "armsolar", 4);
            workflow.BuildUnit(1.9, "armcv", 3);
            workflow.BuildUnit(1.8, "armmstor", 1);
            workflow.BuildUnit(1.8, "armavp", 1);
            workflow.BuildUnit(2.0, "armbull", 3);
            workflow.BuildUnit(2.0, "armmart", 2);
            workflow.BuildUnit(1.9, "armseer", 1); // experimental
            workflow.BuildUnit(1.7, "armyork", 3);
            workflow.BuildUnit(1.7, "armbull", 3);
            workflow.BuildUnit(1.7, "armmart", 2);
            workflow.BuildUnit(1.0, "armmfus", 1);
            workflow.BuildUnit(0.9, "armacv", 2);
            workflow.BuildUnit(0.8, "armmmkr", 4);
            workflow.BuildUnit(0.8, "armarad", 1);
            workflow.BuildUnit(0.8, "armestor", 1);
            workflow.BuildUnit(0.8, "armmfus", 8);
            workflow.BuildUnit(0.7, "armalab", 1);
            workflow.BuildUnit(0.7, "armfark", 2);

            workflow.BuildUnit(0.6, "armbull", 20);
            workflow.BuildUnit(0.6, "armyork", 20);
            workflow.BuildUnit(0.6, "armmart", 20);
            workflow.BuildUnit(0.5, "armseer", 1);
            workflow.BuildUnit(0.5, "armsjam", 1);

            workflow.BuildUnit(0.4, "armmav", 50); // experimental
            workflow.BuildUnit(0.3, "armfark", 4); // experimental
         //   workflow.BuildUnit(0.3, "armpeep", 3); // experimental
          //  workflow.BuildUnit(0.3, "armap", 1); // experimental
            //workflow.BuildUnit(0.2, "armbrawl", 50); // experimental
            //workflow.BuildUnit(0.2, "armaap", 1); // experimental
            
            workflow.Activate();
        }

}
}
