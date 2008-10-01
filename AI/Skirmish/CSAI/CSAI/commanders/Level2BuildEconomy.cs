using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    class BuildEconomyController : IController, IUnitRequester
    {
        IPlayStyle playstyle;
        UnitController unitcontroller;

        public BuildEconomyController( IPlayStyle playstyle )
        {
            this.playstyle = playstyle;
            UnitController.GetInstance();
        }

        bool Active = false;    
        public void Activate()
        {
            if (!Active)
            {
                unitcontroller.UnitAddedEvent += new UnitController.UnitAddedHandler(unitcontroller_UnitAddedEvent);
                unitcontroller.UnitRemovedEvent += new UnitController.UnitRemovedHandler(unitcontroller_UnitRemovedEvent);
                foreach( IFactoryController factorycontroller in playstyle.GetControllersOfType( typeof( IFactoryController ) ) )
                {
                    factorycontroller.RegisterRequester( this );
                }
                Active = true;
            }
        }

        public void Disactivate()
        {
            if (Active)
            {
                unitcontroller.UnitAddedEvent -= new UnitController.UnitAddedHandler(unitcontroller_UnitAddedEvent);
                unitcontroller.UnitRemovedEvent -= new UnitController.UnitRemovedHandler(unitcontroller_UnitRemovedEvent);
                Active = false;
            }
        }

        void unitcontroller_UnitRemovedEvent(int deployedid)
        {
        }

        void unitcontroller_UnitAddedEvent(int deployedid, IUnitDef unitdef)
        {
        }

        public void AssignUnits(System.Collections.ArrayList unitdefs)
        {
        }

        public void RevokeUnits(System.Collections.ArrayList unitdefs)
        {
        }

        public void AssignEnergy(int energy)
        {
        }

        public void AssignMetal(int metal)
        {
        }

        public void AssignPower(double power)
        {
        }

        public void AssignMetalStream(double metalstream)
        {
        }

        double DoYouNeedSomeUnits(IFactory factory)
        {
            return 1.0;
        }

        IUnitDef WhatUnitDoYouNeed(IFactory factory)
        {

        }

        void WeAreBuildingYouA(IUnitDef unitwearebuilding)
        {
        }

        void YourRequestWasDestroyedDuringBuilding_Sorry(IUnitDef unitwearebuilding)
        {
        }
    }
}
