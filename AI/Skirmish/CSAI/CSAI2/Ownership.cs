using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    // handles assigning ownership of newly created units
    // a unit taht builds something tells Ownership what it built and roughly where
    // this class assigns the first unit that matches to that unit
    // this class follows units until they are finished or destroyed
    public class Ownership
    {
        static Ownership instance = new Ownership();
        public static Ownership GetInstance() { return instance; }

        int maxdistanceconsideredsame = 500;

        public interface IBuilder
        {
            void UnitCreated(IOrder order,int deployedunitid, IUnitDef unitdef);
            void UnitFinished(IOrder order, int deployedunitid, IUnitDef unitdef);
            void UnitDestroyed(IOrder order, int deployedunitid, IUnitDef unitdef );
        }

        public interface IOrder
        {
        }

        class OwnershipOrder : IOrder
        {
            public IBuilder builder;
            public int constructorid;
            public IUnitDef orderedunit;
            public Float3 pos;
            public int unitdeployedid = 0;
            public OwnershipOrder(){}
            public OwnershipOrder( IBuilder builder, int constructorid, IUnitDef orderedunit, Float3 pos )
            {
                this.builder = builder;
                this.constructorid = constructorid;
                this.orderedunit = orderedunit;
                this.pos = pos;
            }
            public override string ToString()
            {
                return "OwnershipOrder orderedunit: " + orderedunit.humanName + " pos " + pos.ToString() + " deployedid " + unitdeployedid;
            }
        }

        List<OwnershipOrder> orders = new List<OwnershipOrder>();
        Dictionary<int, OwnershipOrder> ordersbyconstructorid = new Dictionary<int, OwnershipOrder>(); // index of orders by constructorid, to allow removal later

        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        Ownership()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();

            csai.UnitCreatedEvent += new CSAI.UnitCreatedHandler(csai_UnitCreatedEvent);
            csai.UnitDestroyedEvent += new CSAI.UnitDestroyedHandler(csai_UnitDestroyedEvent);
            csai.UnitFinishedEvent += new CSAI.UnitFinishedHandler(csai_UnitFinishedEvent);

            if (csai.DebugOn)
            {
                csai.RegisterVoiceCommand("dumpownership", new CSAI.VoiceCommandHandler(DumpOwnership));
            }
        }

        // if constructor is idle, this constructor is no longer building
        public void SignalConstructorIsIdle(int constructorid)
        {
            RemovePreviousOrdersFromThisConstructor(constructorid);
        }

        void RemovePreviousOrdersFromThisConstructor(int constructorid)
        {
            lock (orders)
            {
                if (ordersbyconstructorid.ContainsKey(constructorid))
                {
                    OwnershipOrder order = ordersbyconstructorid[constructorid];
                    logfile.WriteLine("Removing order by " + constructorid + " for " + order.orderedunit.humanName);
                    order.builder.UnitDestroyed(order, order.unitdeployedid, order.orderedunit);
                    orders.Remove(order);
                    ordersbyconstructorid.Remove(constructorid);
                }
            }
        }

        public void DumpOwnership(string cmd, string[] split, int player)
        {
            lock (orders)
            {
                logfile.WriteLine("number orders: " + orders.Count);
                foreach (OwnershipOrder order in orders)
                {
                    logfile.WriteLine(order.ToString());
                }
            }
        }

        void csai_UnitDestroyedEvent(int deployedunitid, int enemyid)
        {
            lock (orders)
            {
                List<OwnershipOrder> orderstoremove = new List<OwnershipOrder>();
                foreach (OwnershipOrder order in orders)
                {
                    if (order.unitdeployedid == deployedunitid)
                    {
                        order.builder.UnitDestroyed(order, deployedunitid, order.orderedunit);
                        orderstoremove.Add(order);
                    }
                }
                foreach (OwnershipOrder order in orderstoremove)
                {
                    orders.Remove(order);
                }
                ordersbyconstructorid.Remove(deployedunitid);
            }
        }

        void csai_UnitCreatedEvent(int deployedunitid, IUnitDef unitdef)
        {
            lock (orders)
            {
                foreach (OwnershipOrder order in orders)
                {
                    if (order.orderedunit.id == unitdef.id)
                    {
                        Float3 createdunitpos = aicallback.GetUnitPos(deployedunitid);
                        if (Float3Helper.GetSquaredDistance(createdunitpos, order.pos) < maxdistanceconsideredsame * maxdistanceconsideredsame)
                        {
                            order.unitdeployedid = deployedunitid;
                            order.builder.UnitCreated(order, deployedunitid, order.orderedunit);
                        }
                    }
                }
            }
        }

        void csai_UnitFinishedEvent(int deployedunitid, IUnitDef unitdef)
        {
            lock (orders)
            {
                Random random = new Random();
                int refnum = random.Next(0, 10000);
                List<OwnershipOrder> orderstoremove = new List<OwnershipOrder>();
                logfile.WriteLine(refnum + " orders is null: " + (orders == null).ToString());
                logfile.WriteLine(refnum + " orders type is : " + orders.GetType().ToString());
                logfile.WriteLine(refnum + " orders count is : " + orders.Count.ToString());
                try
                {
                    foreach (OwnershipOrder order in orders)
                    {
                        if (order.unitdeployedid == deployedunitid)
                        {
                            order.builder.UnitFinished(order, deployedunitid, order.orderedunit);
                            orderstoremove.Add(order);
                        }
                    }
                    foreach (OwnershipOrder order in orderstoremove)
                    {
                        orders.Remove(order);
                    }
                    ordersbyconstructorid.Remove(deployedunitid);
                }
                catch (Exception e)
                {
                    logfile.WriteLine(refnum + " " + e.ToString());
                    throw e;
                }
            }
        }

        public IOrder RegisterBuildingOrder(IBuilder builder, int constructorid, IUnitDef orderedunittype, Float3 orderedpos)
        {
            RemovePreviousOrdersFromThisConstructor(constructorid);

            OwnershipOrder order = new OwnershipOrder(builder, constructorid, orderedunittype, orderedpos);
            lock (orders)
            {
                orders.Add(order);
                ordersbyconstructorid.Add(constructorid, order);
            }
            return order;
        }
    }
}
