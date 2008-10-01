// Copyright Hugh Perkins 2006
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURVector3E. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//
// ======================================================================================
//

// This class ported from Command class in TASpring project

using System;

namespace CSharpAI
{
    // command object, for giving orders, via aicallback.GiveOrder()
    [Serializable]
    public class Command
    {
        public const int CMD_STOP                 =  0;
        public const int CMD_WAIT              =     5;
        public const int CMD_MOVE               =   10;
        public const int CMD_PATROL            =    15;
        public const int CMD_FIGHT               =  16;
        public const int CMD_ATTACK             =   20;
        public const int CMD_AREA_ATTACK      =     21;
        public const int CMD_GUARD              =   25;
        public const int CMD_AISELECT           =   30;
        public const int CMD_GROUPSELECT      =     35;
        public const int CMD_GROUPADD            =  36;
        public const int CMD_GROUPCLEAR          =  37;
        public const int CMD_REPAIR         =       40;
        public const int CMD_FIRE_STATE    =        45;
        public const int CMD_MOVE_STATE    =        50;
        public const int CMD_SETBASE        =       55;
        public const int CMD_INTERNAL       =       60;
        public const int CMD_SELFD            =     65;
        public const int CMD_SET_WANTED_MAX_SPEED = 70;
        public const int CMD_LOAD_UNITS         =   75;
        public const int CMD_UNLOAD_UNITS     =     80;
        public const int CMD_UNLOAD_UNIT       =    81;
        public const int CMD_ONOFF             =    85;
        public const int CMD_RECLAIM          =     90;
        public const int CMD_CLOAK         =        95;
        public const int CMD_STOCKPILE    =        100;
        public const int CMD_DGUN            =     105;
        public const int CMD_RESTORE       =       110;
        public const int CMD_REPEAT             =  115;
        public const int CMD_TRAJECTORY   =        120;
        public const int CMD_RESURRECT      =      125;
        public const int CMD_CAPTURE          =    130;
        public const int CMD_AUTOREPAIRLEVEL   =   135;
        public const int CMD_LOOPBACKATTACK     = 140;

        public int id;
        public double[] parameters = new double[]{};
        public Command()
        {
        }
        public Command( int id )
        {
            this.id = id;
        }
        public Command( int id, double[] parameters )
        {
            this.id = id;
            this.parameters = parameters;
        }
    }
}

