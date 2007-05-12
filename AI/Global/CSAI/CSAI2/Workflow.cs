using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    public class Workflow : Ownership.IBuilder
    {
        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        Random random = new Random();

        public double energymetalratio = 10.0;

        List<int> ActiveConstructors = new List<int>();
        Dictionary<int, int> AssistingConstructors = new Dictionary<int, int>(); // id of assisted keyed by id of assistor

        public Workflow()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();

            if (csai.DebugOn)
            {
                csai.RegisterVoiceCommand("dumpworkflow", new CSAI.VoiceCommandHandler(DumpWorkFlow));
            }
        }

        public void DumpWorkFlow(string cmd, string[] split, int playe)
        {
            logfile.WriteLine("Workflow:");
            foreach (Order order in orders)
            {
                logfile.WriteLine(order.ToString());
            }
        }

        TankController tankcontroller;
        TankController helicoptercontroller;
        ScoutControllerRaider scoutcontroller;

        public void Activate()
        {
            UnitController.GetInstance();
            EnemyController.GetInstance();
            FriendlyUnitPositionObserver.GetInstance();

            MovementMaps.GetInstance();
            BuildMap.GetInstance();
            Metal.GetInstance().Init();
            LosMap.GetInstance();
            BuildPlanner.GetInstance();

            BuildTable.GetInstance();

            CommanderList.GetInstance();
            Level1ConstructorList.GetInstance();
            Level1FactoryList.GetInstance();

            tankcontroller = new TankController(TankList.GetInstance().defbyid, BuildTable.GetInstance().UnitDefByName["arm_stumpy"]);
            tankcontroller.Activate();
            helicoptercontroller = new TankController(HelicopterList.GetInstance().defbyid, BuildTable.GetInstance().UnitDefByName["arm_brawler"]);
            helicoptercontroller.Activate();
            scoutcontroller = new ScoutControllerRaider();
            scoutcontroller.Activate();

            UnitController.GetInstance().LoadExistingUnits();
            EnemyController.GetInstance().LoadExistingUnits();

            CheckIdleUnits();

            //HeightMapPersistence.GetInstance();

            //BuildSolarCell(CommanderList.GetInstance().defbyid.Keys.GetEnumerator().Current);

            csai.UnitIdleEvent += new CSAI.UnitIdleHandler(csai_UnitIdleEvent);
            csai.TickEvent += new CSAI.TickHandler(csai_TickEvent);
        }

        int lastcheckidleframe = -1000;
        void csai_TickEvent()
        {
            if (aicallback.GetCurrentFrame() - lastcheckidleframe > 30)
            {
                CheckIdleUnits();
                lastcheckidleframe = aicallback.GetCurrentFrame();
            }
        }

        void CheckIdleUnits()
        {
            logfile.WriteLine("CheckIdleUnits()");
            foreach (KeyValuePair<int, IUnitDef> kvp in UnitController.GetInstance().UnitDefByDeployedId)
            {
               // logfile.WriteLine("Checking " + kvp.Key);
                if (kvp.Value.canBuild)
                {
                    if (!aicallback.UnitIsBusy(kvp.Key))
                    {
                        logfile.WriteLine("Unit " + kvp.Value.humanName + " not busy, calling idle");
                        if( AssistingConstructors.ContainsKey( kvp.Key ) )
                        {
                            logfile.WriteLine("removing from assistingconstructors");
                            AssistingConstructors.Remove(kvp.Key);
                        }
                        csai_UnitIdleEvent(kvp.Key);
                    }
                    if (!ActiveConstructors.Contains(kvp.Key))
                    {
                        logfile.WriteLine("Unit " + kvp.Value.humanName + " not in active commanders, calling idle");
                        csai_UnitIdleEvent(kvp.Key);
                    }
                }
            }
        }

        class Order
        {
            public double priority;
            public string unitname;
            public int quantity;
            public List<Ownership.IOrder> unitsunderconstruction = new List<Ownership.IOrder>();
            public Order(double priority, string unitname, int quantity)
            {
                this.priority = priority;
                this.unitname = unitname;
                this.quantity = quantity;
            }
            public override string ToString()
            {
                int underconstruction = unitsunderconstruction.Count;
                int currentunits = 0;
                if (UnitController.GetInstance().UnitDefsByName.ContainsKey(unitname))
                {
                    currentunits += UnitController.GetInstance().UnitDefsByName[unitname].Count;
                }
                return "Order priority: " + priority + " unitname: " + unitname + " quantity: " + currentunits + "(" + 
                    underconstruction + ") / " + quantity;
            }
        }

        List<Order> orders = new List<Order>();

        // note to self: need to add weighting probably
        // need to add filters/position requireemtns for radars, metal extractors
        public void BuildUnit(double priority, string name, int quantity)
        {
            orders.Add( new Order(priority, name, quantity));
        }

        List<string> energyunits = new List<string>();

        public void AddEnergyUnit(string energyunitname)
        {
            energyunits.Add(energyunitname);
        }

        List<string> metalunits = new List<string>();

        public void AddMetalUnit(string metalunitname)
        {
            metalunits.Add(metalunitname);
        }

        void csai_UnitIdleEvent(int deployedunitid)
        {
            IUnitDef unitdef = UnitController.GetInstance().UnitDefByDeployedId[deployedunitid];
            if (!unitdef.canBuild)
            {
                //logfile.WriteLine("cantbuild");
                return;
            }
            logfile.WriteLine("unitidleevent " + deployedunitid + " " + unitdef.name + " " + unitdef.humanName);
            if (ActiveConstructors.Contains(deployedunitid))
            {
                ActiveConstructors.Remove(deployedunitid);
            }
            Ownership.GetInstance().SignalConstructorIsIdle(deployedunitid);
            double highestpriority = 0;
            List<Order> bestorders = new List<Order>();
            foreach (Order order in orders)
            {
                double thispriority = order.priority;
                if (thispriority >= highestpriority)
                {
                    int currentunits = order.unitsunderconstruction.Count;
                    if (UnitController.GetInstance().UnitDefsByName.ContainsKey(order.unitname))
                    {
                        currentunits += UnitController.GetInstance().UnitDefsByName[order.unitname].Count;
                    }
                    if( currentunits < order.quantity)
                    {
                        if (BuildTree.GetInstance().CanBuild(unitdef.name.ToLower(), order.unitname))
                        {
                            //if( CanBuild(deployedunitid, 
                            if (thispriority > highestpriority)
                            {
                                highestpriority = thispriority;
                                bestorders = new List<Order>();
                                bestorders.Add(order);
                                csai.DebugSay("Possible order: " + order.ToString());
                            }
                            else if (thispriority == highestpriority)
                            {
                                bestorders.Add(order);
                                csai.DebugSay("Possible order: " + order.ToString());
                            }
                        }
                    }
                }
            }
            //if (bestorders.Count == 0)
          //  {
          //      csai.DebugSay("No orders found");
          //      return;
          //  }
            List<Order>possibleorders = new List<Order>(); // get orders this unit can build
            bool metalneeded = false;
            bool energyneeded = false;
            IUnitDef deftobuild = null;
            foreach (Order order in bestorders)
            {
                csai.DebugSay("bestorder " + order.unitname);
                //if( BuildTree.GetInstance().CanBuild( unitdef.name.ToLower(), order.unitname ) )
                //{
                    deftobuild = BuildTable.GetInstance().UnitDefByName[order.unitname];
                    if (MetalController.GetInstance().CanBuild(deftobuild))
                    {
                        if (EnergyController.GetInstance().CanBuild(deftobuild))
                        {
                            possibleorders.Add(order);
                            csai.DebugSay("possible: " + order.unitname);
                        }
                        else
                        {
                            csai.DebugSay("needs energy");
                            energyneeded = true;
                        }
                    }
                    else
                    {
                        csai.DebugSay("needs metal");
                        metalneeded = true;
                    }
                //}
            }
            if( possibleorders.Count == 0 )
            {
                if ( Level1ConstructorList.GetInstance().defbyid.Count < 1 &&
                   !UnitController.GetInstance().UnitDefsByName.ContainsKey("arm_commander") )
                {
                    if (BuildConstructionVehicle(deployedunitid, unitdef))
                    {
                        return;
                    }
                }

                if (energyneeded || aicallback.GetEnergy() < aicallback.GetEnergyStorage() / 5 )
                {
                    if (BuildSolarCell(deployedunitid))
                    {
                        logfile.WriteLine("building solarcell");
                        if (AssistingConstructors.ContainsKey(deployedunitid))
                        {
                            AssistingConstructors.Remove(deployedunitid);
                        }
                        return;
                    }
                }
                if (metalneeded || aicallback.GetMetal() < aicallback.GetMetalStorage() / 5 )
                {
                    Float3 reclaimpos = ReclaimHelper.GetNearestReclaim( aicallback.GetUnitPos( deployedunitid ), deployedunitid );
                    if( reclaimpos != null )
                    {
                        GiveOrderWrapper.GetInstance().Reclaim( deployedunitid, reclaimpos, 100 );
                        return;
                    }
                    if (BuildMex(deployedunitid))
                    {
                        logfile.WriteLine("building mex");
                        if (AssistingConstructors.ContainsKey(deployedunitid))
                        {
                            AssistingConstructors.Remove(deployedunitid);
                        }
                        return;
                    }
                }


                logfile.WriteLine("offering assistance");
                OfferAssistance(deployedunitid);
                return;
            }
            Order ordertodo = possibleorders[random.Next(0, possibleorders.Count)];
            if (ordertodo.unitname == "arm_metal_extractor")
            {
                BuildMex(deployedunitid);
                if (AssistingConstructors.ContainsKey(deployedunitid))
                {
                    AssistingConstructors.Remove(deployedunitid);
                }
            }
            else
            {
                //ordertodo.unitsunderconstruction += 1;
                deftobuild = BuildTable.GetInstance().UnitDefByName[ordertodo.unitname];
                Float3 pos = BuildUnit(deployedunitid, ordertodo.unitname);
                Ownership.IOrder ownershiporder = Ownership.GetInstance().RegisterBuildingOrder(this,
                    deployedunitid,
                   deftobuild, pos);
                logfile.WriteLine("building: " + ordertodo.unitname);
                ordertodo.unitsunderconstruction.Add(ownershiporder);
                if (AssistingConstructors.ContainsKey(deployedunitid))
                {
                    AssistingConstructors.Remove(deployedunitid);
                }
            }
        }

        void OfferAssistance(int constructorid)
        {
            int bestconstructor = 0;
            double bestsquareddistance = 1000000000;
            Float3 ourpos = aicallback.GetUnitPos( constructorid );
            foreach (int activeconstructor in ActiveConstructors)
            {
                Float3 constructorpos = aicallback.GetUnitPos(activeconstructor);
                if (constructorpos != null)
                {
                    double thissquareddistance = Float3Helper.GetSquaredDistance(ourpos, constructorpos);
                    if (thissquareddistance < bestsquareddistance)
                    {
                        bestconstructor = activeconstructor;
                        bestsquareddistance = thissquareddistance;
                    }
                }
            }
            if (bestconstructor != 0)
            {
                logfile.WriteLine("unit to assist should be " + bestconstructor);
                if( !AssistingConstructors.ContainsKey( constructorid ) )
                {
                    logfile.WriteLine("assisting " + bestconstructor);
                    GiveOrderWrapper.GetInstance().Guard(constructorid, bestconstructor);
                    AssistingConstructors.Add(constructorid, bestconstructor);
                }
                else if( AssistingConstructors[ constructorid ] != bestconstructor )
                {
                    logfile.WriteLine("assisting " + bestconstructor);
                    GiveOrderWrapper.GetInstance().Guard(constructorid, bestconstructor);
                    AssistingConstructors[ constructorid ] = bestconstructor;
                }
            }
        }

        bool BuildConstructionVehicle( int constructorid, IUnitDef constructordef)
        {
            IUnitDef deftobuild = null;
            if( BuildTree.GetInstance().CanBuild( constructordef.name.ToLower(), "arm_construction_vehicle" ) )
            {
                deftobuild = BuildTable.GetInstance().UnitDefByName["arm_construction_vehicle"];
            }
            else if (BuildTree.GetInstance().CanBuild(constructordef.name.ToLower(), "arm_construction_vehicle"))
            {
                deftobuild = BuildTable.GetInstance().UnitDefByName["arm_construction_vehicle"];
            }
            else
            {
                return false;
            }
            Float3 pos = BuildUnit(constructorid, deftobuild.name.ToLower());
            Ownership.IOrder ownershiporder = Ownership.GetInstance().RegisterBuildingOrder(this,
                constructorid,
               deftobuild, pos);
            logfile.WriteLine("building: " + deftobuild.name.ToLower());
            //ordertodo.unitsunderconstruction.Add(ownershiporder);
            if (AssistingConstructors.ContainsKey(constructorid))
            {
                AssistingConstructors.Remove(constructorid);
            }
            return true;
        }

        Float3 BuildUnit(int constructorid, string targetunitname)
        {
            csai.DebugSay("workflow, building " + targetunitname);
            IUnitDef targetunitdef = BuildTable.GetInstance().UnitDefByName[targetunitname];
            IUnitDef constructordef = UnitController.GetInstance().UnitDefByDeployedId[ constructorid ];
            if (new UnitDefHelp(aicallback).IsMobile(constructordef))
            {
                logfile.WriteLine("constructor is mobile");
                Float3 constructorpos = aicallback.GetUnitPos(constructorid);
                Float3 buildsite = BuildPlanner.GetInstance().ClosestBuildSite(targetunitdef,
                       constructorpos,
                       3000, 2);
                buildsite = aicallback.ClosestBuildSite(targetunitdef, buildsite, 1400, 0);
                logfile.WriteLine("constructor location: " + constructorpos.ToString() + " Buildsite: " + buildsite.ToString() + " target item: " + targetunitdef.humanName);
                if (!ActiveConstructors.Contains(constructorid))
                {
                    ActiveConstructors.Add(constructorid);
                }
                aicallback.DrawUnit(targetunitname.ToUpper(), buildsite, 0.0, 200, aicallback.GetMyAllyTeam(), true, true);
                GiveOrderWrapper.GetInstance().BuildUnit(constructorid, targetunitname, buildsite);
                return buildsite;
            }
            else
            {
                Float3 factorypos = aicallback.GetUnitPos(constructorid);
                logfile.WriteLine("factory location: " + factorypos.ToString() + " target item: " + targetunitdef.humanName);
                if (!ActiveConstructors.Contains(constructorid))
                {
                    ActiveConstructors.Add(constructorid);
                }
                aicallback.DrawUnit(targetunitdef.name.ToUpper(), factorypos, 0.0, 200, aicallback.GetMyAllyTeam(), true, true);
                GiveOrderWrapper.GetInstance().BuildUnit(constructorid, targetunitname );
                return factorypos;
            }
        }

        bool BuildMex(int constructorid)
        {
            if( !BuildTree.GetInstance().CanBuild( UnitController.GetInstance().UnitDefByDeployedId[ constructorid ].name.ToLower(),
                "armmex" ) )
            {
                return false;
            }
            IUnitDef unitdef = BuildTable.GetInstance().UnitDefByName["arm_metal_extractor"];
            Float3 buildsite = Metal.GetInstance().GetNearestMetalSpot(aicallback.GetUnitPos(constructorid));
            buildsite = aicallback.ClosestBuildSite(unitdef, buildsite, 100, 0);
            if (!ActiveConstructors.Contains(constructorid))
            {
                ActiveConstructors.Add(constructorid);
            }
            //aicallback.GiveOrder(constructorid, new Command(-unitdef.id, buildsite.ToDoubleArray()));
            GiveOrderWrapper.GetInstance().BuildUnit(constructorid, unitdef.name, buildsite);
            return true;
        }

        bool BuildSolarCell(int constructorid)
        {
            if (!BuildTree.GetInstance().CanBuild(UnitController.GetInstance().UnitDefByDeployedId[constructorid].name.ToLower(),
                "armsolar"))
            {
                return false;
            }
            IUnitDef unitdef = BuildTable.GetInstance().UnitDefByName["arm_solar_collector"];
            Float3 buildsite = BuildPlanner.GetInstance().ClosestBuildSite(unitdef,
                aicallback.GetUnitPos(constructorid),
                3000, 2);
            buildsite = aicallback.ClosestBuildSite(unitdef, buildsite, 100, 0);
            if (!ActiveConstructors.Contains(constructorid))
            {
                ActiveConstructors.Add(constructorid);
            }
            //aicallback.GiveOrder(constructorid, new Command(-unitdef.id, buildsite.ToDoubleArray()));
            GiveOrderWrapper.GetInstance().BuildUnit(constructorid, unitdef.name, buildsite);
            return true;
        }


        public void UnitCreated(Ownership.IOrder order,int deployedunitid, IUnitDef unitdef)
        {
        }

        public void UnitDestroyed(Ownership.IOrder ownershiporder, int deployedunitid, IUnitDef unitdef)
        {
            foreach (Order order in orders)
            {
                if (order.unitsunderconstruction.Contains(ownershiporder))
                {
                    order.unitsunderconstruction.Remove(ownershiporder);
                }
            }
        }

        public void UnitFinished(Ownership.IOrder ownershiporder, int deployedunitid, IUnitDef unitdef)
        {
            foreach (Order order in orders)
            {
                if (order.unitsunderconstruction.Contains(ownershiporder))
                {
                    order.unitsunderconstruction.Remove(ownershiporder);
                }
            }
            csai_UnitIdleEvent(deployedunitid);
        }
    }
}
