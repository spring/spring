using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    // wraps the Spring Command with an OOP version

    // allows adding ToString etc

    // something that can be attacked, moved to etc
    public class Target
    {
        public Target() { }
        public virtual double[] ToDoubleArray()
        {
            return null;
        }
    }

    public class PositionTarget : Target
    {
        public Float3 targetpos;
        public PositionTarget(Float3 targetpos)
        {
            this.targetpos = targetpos;
        }
        public override double[] ToDoubleArray()
        {
            return targetpos.ToDoubleArray();
        }
        public override string ToString()
        {
            return "PositionTarget: " + targetpos.ToString();
        }
    }

    // targeting a specific unit
    public class UnitTarget : Target
    {
        public int targetid;
        public UnitTarget(int targetid)
        {
            this.targetid = targetid;
        }
        public override double[] ToDoubleArray()
        {
            return new double[] { targetid };
        }
    }

    public class OOPCommand
    {
        public int UnitToReceiveOrder;
        public OOPCommand()
        {
        }
        public OOPCommand(int unittoreceiveorder)
        {
            this.UnitToReceiveOrder = unittoreceiveorder;
        }
        public virtual Command ToSpringCommand()
        {
            return null;
        }
        public override string ToString()
        {
            return this.GetType().ToString() + " unit " + UnitToReceiveOrder;
        }
    }

    public class BuildCommand : OOPCommand
    {
        public int idtobuild;
        public Float3 pos = null;

        public BuildCommand(int builder, int idtobuild)
        {
            this.UnitToReceiveOrder = builder;
            this.idtobuild = idtobuild;
        }

        public BuildCommand(int builder, int idtobuild, Float3 pos )
        {
            this.UnitToReceiveOrder = builder;
            this.idtobuild = idtobuild;
            this.pos = pos;
        }
        public override Command ToSpringCommand()
        {
            if (pos == null)
            {
                return new Command( - idtobuild, new double[] { });
            }
            else
            {
                return new Command( -idtobuild, pos.ToDoubleArray());
            }
        }
        public override string ToString()
        {
            if (pos != null)
            {
                return "BuildCommand " + UnitToReceiveOrder + " building " + BuildTable.GetInstance().UnitDefById[idtobuild].humanName + " at " + pos.ToString();
            }
            else
            {
                return "BuildCommand " + UnitToReceiveOrder + " building " + BuildTable.GetInstance().UnitDefById[idtobuild].humanName;
            }
        }
    }

    public class AttackCommand : OOPCommand
    {
        public Target target;
        public AttackCommand(int attacker, Target target)
        {
            this.UnitToReceiveOrder = attacker;
            this.target = target;
        }
        public override Command ToSpringCommand()
        {
            return new Command(Command.CMD_ATTACK, target.ToDoubleArray());
        }
        public override string ToString()
        {
            return "AttackCommand " + UnitToReceiveOrder + " attacking " + target.ToString();
        }
    }

    public class MoveToCommand : OOPCommand
    {
        public Float3 targetpos;
        public MoveToCommand(int unit, Float3 pos)
        {
            this.UnitToReceiveOrder = unit;
            this.targetpos = pos;
        }
        public override Command ToSpringCommand()
        {
            return new Command(Command.CMD_MOVE, targetpos.ToDoubleArray());
        }
        public override string ToString()
        {
            return "MoveToCommand " + UnitToReceiveOrder + " moving to " + targetpos.ToString();
        }
    }

    public class GuardCommand : OOPCommand
    {
        public int targetid;
        public GuardCommand(int unit, int unittobeguarded)
        {
            this.UnitToReceiveOrder = unit;
            this.targetid = unittobeguarded;
        }
        public override Command ToSpringCommand()
        {
            return new Command(Command.CMD_GUARD, new double[] { targetid } );
        }
    }

    public class ReclaimCommand : OOPCommand
    {
        public Float3 pos;
        public double radius;
        public ReclaimCommand(int unit, Float3 pos, double radius)
        {
            this.UnitToReceiveOrder = unit;
            this.pos = pos;
            this.radius = radius;
        }
        public override Command ToSpringCommand()
        {
            return new Command(Command.CMD_RECLAIM, new double[] { pos.x, pos.y, pos.z, radius });
        }
    }

    public class SelfDestructCommand : OOPCommand
    {
        public SelfDestructCommand(int unit )
        {
            this.UnitToReceiveOrder = unit;
        }
        public override Command ToSpringCommand()
        {
            return new Command(Command.CMD_SELFD, new double[] {});
        }
    }

    public class StopCommand : OOPCommand
    {
        public StopCommand(int unit)
        {
            this.UnitToReceiveOrder = unit;
        }
        public override Command ToSpringCommand()
        {
            return new Command(Command.CMD_STOP, new double[] { });
        }
    }
}
