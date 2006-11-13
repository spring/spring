using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    // handles assigning ownership of newly created units
    // a unit taht builds something tells Ownership what it built and roughly where
    // this class assigns the first unit that matches to that unit
    public class Ownership
    {
        static Ownership instance = new Ownership();
        public static Ownership GetInstance() { return instance; }

        int maxdistanceconsideredsame = 50;

        class Order : IUnitOrder
        {
            public IFactoryController factorycontroller;
            public IUnitDef orderedunit;
            public Float3 pos;
            public int unitdeployedid = 0;
            public Order(){}
            public Order( IFactoryController factorycontroller, IUnitDef orderedunit, Float3 pos )
            {
                this.factorycontroller = factorycontroller;
                this.orderedunit = orderedunit;
                this.pos = pos;
            }
        }

        List<Order> orders = new List<Order>();

        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        Ownership()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = logfile.GetInstance();

            csai.UnitCreatedEvent += new CSAI.UnitCreatedHandler(csai_UnitCreatedEvent);
        }

        void csai_UnitDestroyedEvent(int deployedunitid, int enemyid)
        {
        }

        void csai_UnitCreatedEvent(int deployedunitid, IUnitDef unitdef)
        {
            foreach (Order order in orders)
            {
                if (order.orderedunit.id == unitdef.id)
                {
                    Float3 createdunitpos = aicallback.GetUnitPos( deployedunitid );
                    if (Float3Helper.GetSquaredDistance(createdunitpos, order.pos) < maxdistanceconsideredsame * maxdistanceconsideredsame)
                    {
                        order.unitdeployedid = deployedunitid;
                        order.factorycontroller.UnitCreated(order, deployedunitid);
                    }
                }
            }
        }

        void csai_UnitFinishedEvent(int deployedunitid, IUnitDef unitdef)
        {
        }

        public IUnitOrder RegisterBuildingOrder( IFactoryController factorycontroller, IUnitDef orderedunittype, Float3 orderedpos)
        {
            Order order = new Order(factorycontroller, orderedunittype, orderedpos);
            orders.Add(order);
            return order;
        }
    }
}
