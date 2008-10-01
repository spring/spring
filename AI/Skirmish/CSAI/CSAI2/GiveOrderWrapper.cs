using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{

    // wraps giveorder, registers command in UnitCommandCache,
    // carries out any statistical analysis for debugging, performance analysis
    // abstract order interface somewhat
    public class GiveOrderWrapper
    {
        static GiveOrderWrapper instance = new GiveOrderWrapper();
        public static GiveOrderWrapper GetInstance() { return instance; }

        CSAI csai;
        IAICallback aicallback;
        LogFile logfile;

        class CommandInfo
        {
            public TimeSpan datetime;
            public OOPCommand command;
            public CommandInfo(TimeSpan datetime, OOPCommand command)
            {
                this.datetime = datetime;
                this.command = command;
            }
        }

        int lastcommandssentresetframe = 0;
        List<CommandInfo> recentcommands = new List<CommandInfo>(); // for debugging, logging
        List<CommandInfo> allcommands = new List<CommandInfo>();

        GiveOrderWrapper()
        {
            csai = CSAI.GetInstance();
            aicallback = csai.aicallback;
            logfile = LogFile.GetInstance();

            csai.TickEvent += new CSAI.TickHandler(csai_TickEvent);
            if (csai.DebugOn)
            {
                csai.RegisterVoiceCommand("dumporders", new CSAI.VoiceCommandHandler(DumpOrders));
            }
        }

        void csai_TickEvent()
        {
            if (aicallback.GetCurrentFrame() - lastcommandssentresetframe >= 30)
            {
                lastcommandssentresetframe = aicallback.GetCurrentFrame();
                if (csai.DebugOn)
                {
                  //  DumpCommandsSentStats();
                }
            }
        }

        public void DumpOrders(string cmd, string[] split, int player)
        {
            logfile.WriteLine("Command history:");
            lock (allcommands)
            {
                Dictionary<Type, int> CountByType = new Dictionary<Type, int>();
                foreach (CommandInfo commandinfo in allcommands)
                {
                    logfile.WriteLine( TimeHelper.FormatTimeSpan( commandinfo.datetime ) + ": " + commandinfo.command.ToString());
                    if (!CountByType.ContainsKey(commandinfo.command.GetType()))
                    {
                        CountByType.Add(commandinfo.command.GetType(), 1);
                    }
                    else
                    {
                        CountByType[ commandinfo.command.GetType() ]++;
                    }
                }

                logfile.WriteLine("Command stats");
                foreach (KeyValuePair<Type, int> kvp in CountByType)
                {
                    logfile.WriteLine(kvp.Key.ToString() + ": " + kvp.Value);
                }
            }
        }

        void DumpCommandsSentStats()
        {
            logfile.WriteLine("recent commands:");
            lock (recentcommands)
            {
                foreach (CommandInfo commandinfo in recentcommands)
                {
                    logfile.WriteLine(commandinfo.datetime.ToString() + ": " + commandinfo.command.ToString());
                }
                recentcommands.Clear();
            }
        }

        public void BuildUnit(int builderid, string targetunitname )
        {
            int targetunittypeid = BuildTable.GetInstance().UnitDefByName[ targetunitname.ToLower() ].id;
            GiveOrder( new BuildCommand( builderid, targetunittypeid ) );
        }

        public void BuildUnit(int builderid, string targetunitname, Float3 pos)
        {
            int targetunittypeid = BuildTable.GetInstance().UnitDefByName[targetunitname.ToLower()].id;
            GiveOrder( new BuildCommand( builderid, targetunittypeid, pos ) );
        }

        public void MoveTo(int unitid, Float3 pos)
        {
            GiveOrder( new MoveToCommand( unitid, pos ) );
        }

        public void Guard(int unitid, int targetunitid)
        {
            GiveOrder(new GuardCommand(unitid, targetunitid));
        }

        public void Attack(int unitid, int targetunitid)
        {
            GiveOrder( new AttackCommand( unitid, new UnitTarget( targetunitid ) ) );
        }

        public void Attack(int unitid, Float3 pos)
        {
            GiveOrder( new AttackCommand( unitid, new PositionTarget( pos ) ) );
        }

        public void SelfDestruct(int unitid)
        {
            GiveOrder(new SelfDestructCommand(unitid));
        }

        public void Stop(int unitid)
        {
            GiveOrder(new StopCommand(unitid));
        }

        public void Reclaim(int unitid, Float3 pos, double radius)
        {
            GiveOrder(new ReclaimCommand(unitid, pos, radius));
        }

        void GiveOrder( OOPCommand command)
        {
            UnitCommandCache.GetInstance().RegisterCommand( command.UnitToReceiveOrder, command);
            logfile.WriteLine("GiveOrder " + command.ToString());
            lock (recentcommands)
            {
                TimeSpan gametime = TimeHelper.GetGameTime();
                recentcommands.Add( new CommandInfo( gametime, command) );
                allcommands.Add(new CommandInfo(gametime, command));
            }
            if (aicallback.GiveOrder(command.UnitToReceiveOrder, command.ToSpringCommand()) == -1)
            {
                throw new Exception( "GiveOrder failed");
            }
        }
    }
}
