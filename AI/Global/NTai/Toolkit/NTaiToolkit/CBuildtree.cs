namespace NTaiToolkit
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Windows.Forms;

    public class CBuildtree
    {
        public CBuildtree(Form1 f1)
        {
            this._buildtreefilename = "";
            this.use_mod_default_if_absent = 1;
            this._author = "author";
            this._version = "0.1";
            this._message = "";
            this.interpolate_tag = "b_rule_extreme_nofact";
            this.antistall = 3;
            this.MaxStallTimeMobile = 0x7d0;
            this.MaxStallTimeImmobile = -40;
            this.f = f1;
            this.f.debugsave.Text = this.f.debugsave.Text + "cbuildtree created\n";
            this.keywords = new CKeywords();
            this.movement = new Dictionary<string, int>();
            this.firingstate = new Dictionary<string, int>();
            this.ConstructionRepairRanges = new Dictionary<string, decimal>();
            this.ConstructionExclusionRange = new Dictionary<string, decimal>();
            this.MaxEnergy = new Dictionary<string, decimal>();
            this.MinEnergy = new Dictionary<string, decimal>();
            this.attackers = new ArrayList();
            this.scouters = new ArrayList();
            this.kamikaze = new ArrayList();
            this.solobuild = new ArrayList();
            this.singlebuild = new ArrayList();
            this.alwaysantistall = new ArrayList();
            this.neverantistall = new ArrayList();
            this.tasklists = new Dictionary<string, ArrayList>();
            this.unittasklists = new Dictionary<string, ArrayList>();
            this.unitcheattasklists = new Dictionary<string, ArrayList>();
        }

        public void AddTaskList(string name)
        {
            if (!this.tasklists.ContainsKey(name))
            {
                ArrayList list1 = new ArrayList();
                this.tasklists[name] = list1;
            }
        }

        public int booltoint(bool value)
        {
            if (value)
            {
                return 1;
            }
            return 0;
        }

        public void Dump(TdfParser.Section section)
        {
            this.f.debugsave.Text = this.f.debugsave.Text + section.Name + "\n";
            foreach (TdfParser.Section section1 in section.SubSections.Values)
            {
                this.Dump(section1);
            }
        }

        public ArrayList GetTaskList(string name)
        {
            if (!this.tasklists.ContainsKey(name))
            {
                return new ArrayList();
            }
            return this.tasklists[name];
        }

        public bool inttobool(int value)
        {
            if (value == 0)
            {
                return false;
            }
            return true;
        }

        public bool Load(string TDFfile)
        {
            this.buildtreefilename = TDFfile;
            Stream stream1 = File.OpenRead(TDFfile);
            StreamReader reader1 = new StreamReader(stream1);
            string text1 = reader1.ReadToEnd();
            reader1.Close();
            TdfParser parser1 = new TdfParser(text1);
            this.author = parser1.RootSection.GetStringValue("", @"AI\author");
            this.f.debugsave.Text = this.f.debugsave.Text + this.author + "\n";
            this.version = parser1.RootSection.GetStringValue("0.1", @"AI\version");
            this.message = parser1.RootSection.GetStringValue("", @"AI\message");

            this.use_mod_default = parser1.RootSection.GetIntValue(0, @"AI\use_mod_default");
            this.use_mod_default_if_absent = parser1.RootSection.GetIntValue(1, @"AI\use_mod_default_if_absent");
            this.defence_spacing = (decimal) parser1.RootSection.GetIntValue(7, @"AI\defence_spacing");
            this.power_spacing = (decimal) parser1.RootSection.GetIntValue(6, @"AI\power_spacing");
            this.factory_spacing = (decimal) parser1.RootSection.GetIntValue(4, @"AI\factory_spacing");
            this.default_spacing = (decimal) parser1.RootSection.GetIntValue(5, @"AI\default_spacing");
            this.spacemod = this.inttobool(parser1.RootSection.GetIntValue(0, @"AI\spacemod"));
            this.Antistall = this.inttobool(parser1.RootSection.GetIntValue(1, @"AI\antistall"));
            this.StallTimeImMobile = (decimal) parser1.RootSection.GetIntValue(0, @"AI\MaxStallTimeImmobile");
            this.StallTimeMobile = (decimal) parser1.RootSection.GetIntValue(0, @"AI\MaxStallTimeMobile");
            this.powerRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\power"));
            this.mexRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\mex"));
            this.powerRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\power"));
            this.mexRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\mex"));
            this.factorymetalRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\factorymetal"));
            this.factorymetalRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\factorymetal"));
            this.factoryenergyRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\factoryenergy"));
            this.factoryenergyRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\factoryenergy"));
            this.factorymetalgapRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\factorymetalgap"));
            this.factorymetalgapRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\factorymetalgap"));
            this.energystorageRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\energystorage"));
            this.energystorageRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\energystorage"));
            this.metalstorageRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\metalstorage"));
            this.metalstorageRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\metalstorage"));
            this.makermetalRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\makermetal"));
            this.makermetalRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\makermetal"));
            this.makerenergyRule = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\makerenergy"));
            this.makerenergyRuleEx = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0.7, @"ECONOMY\RULES\EXTREME\makerenergy"));
            this.normal_handicap = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(0, @"AI\normal_handicap"));
            this.antistallwindow = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(43, @"AI\antistallwindow"));

            this.initialAttackSize = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(5, @"AI\initial_threat_value"));
            this.interpolate = parser1.RootSection.GetStringValue(@"AI\interpolate_tag").Equals("b_rule_extreme_nofact");

            this.AttackIncrementValue = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(1, @"AI\increase_threshold_value"));
            this.maxAttackSize = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(1, @"AI\maximum_attack_group_size"));
            this.AttackIncrementPercentage = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(1, @"AI\increase_threshold_percentage"));

            this.mexnoweaponradius = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(900, @"AI\mexnoweaponradius"));

            this.GeoSearchRadius = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(3000, @"AI\geotherm\searchdistance"));
            this.NoEnemyGeo = Convert.ToDecimal(parser1.RootSection.GetDoubleValue(600, @"AI\geotherm\noenemiesdistance"));

            string[] textArray1 = parser1.RootSection.GetStringValue(" ", @"AI\hold_pos").Split(new char[] { ',' });
            foreach (string text3 in textArray1)
            {
                string text4 = text3;
                text4.Trim();
                text4 = text4.ToLower();
                this.movement[text4] = 0;
            }
            textArray1 = parser1.RootSection.GetStringValue(" ", @"AI\maneouvre").Split(new char[] { ',' });
            foreach (string text5 in textArray1)
            {
                string text6 = text5;
                text6.Trim();
                text6 = text6.ToLower();
                this.movement[text6] = 1;
            }
            textArray1 = parser1.RootSection.GetStringValue(" ", @"AI\roam").Split(new char[] { ',' });
            foreach (string text7 in textArray1)
            {
                string text8 = text7;
                text8.Trim();
                text8 = text8.ToLower();
                this.movement[text8] = 2;
            }
            textArray1 = parser1.RootSection.GetStringValue(" ", @"AI\hold_fire").Split(new char[] { ',' });
            foreach (string text9 in textArray1)
            {
                string text10 = text9;
                text10 = text10.Trim();
                text10 = text10.ToLower();
                this.firingstate[text10] = 0;
            }
            textArray1 = parser1.RootSection.GetStringValue(" ", @"AI\return_fire").Split(new char[] { ',' });
            foreach (string text11 in textArray1)
            {
                string text12 = text11;
                text12.Trim();
                text12 = text12.ToLower();
                this.firingstate[text12] = 1;
            }
            textArray1 = parser1.RootSection.GetStringValue(" ", @"AI\fire_at_will").Split(new char[] { ',' });
            foreach (string text13 in textArray1)
            {
                string text14 = text13;
                text14.Trim();
                text14 = text14.ToLower();
                this.firingstate[text14] = 2;
            }
            foreach (string text15 in parser1.RootSection.GetStringValue(" ", @"AI\attackers").Split(new char[] { ',' }))
            {
                string text16 = text15;
                text16 = text16.Trim();
                text16 = text16.ToLower();
                this.attackers.Add(text16);
            }
            foreach (string text17 in parser1.RootSection.GetStringValue(" ", @"AI\scouters").Split(new char[] { ',' }))
            {
                string text18 = text17;
                text18 = text18.Trim();
                text18 = text18.ToLower();
                this.scouters.Add(text18);
            }
            foreach (string text19 in parser1.RootSection.GetStringValue(" ", @"AI\kamikaze").Split(new char[] { ',' }))
            {
                string text20 = text19;
                text20 = text20.Trim();
                text20 = text20.ToLower();
                this.kamikaze.Add(text20);
            }
            foreach (string text21 in parser1.RootSection.GetStringValue(" ", @"AI\solobuild").Split(new char[] { ',' }))
            {
                string text22 = text21;
                text22 = text22.Trim();
                text22 = text22.ToLower();
                this.solobuild.Add(text22);
            }
            foreach (string text23 in parser1.RootSection.GetStringValue(" ", @"AI\singlebuild").Split(new char[] { ',' }))
            {
                string text24 = text23;
                text24 = text24.Trim();
                text24 = text24.ToLower();
                this.singlebuild.Add(text24);
            }
            foreach (string text25 in parser1.RootSection.GetStringValue(" ", @"AI\alwaysantistall").Split(new char[] { ',' }))
            {
                string text26 = text25;
                text26 = text26.Trim();
                text26 = text26.ToLower();
                this.alwaysantistall.Add(text26);
            }
            foreach (string text27 in parser1.RootSection.GetStringValue(" ", @"AI\neverantistall").Split(new char[] { ',' }))
            {
                string text28 = text27;
                text28 = text28.Trim();
                text28 = text28.ToLower();
                this.neverantistall.Add(text28);
            }
            TdfParser.Section section1 = parser1.RootSection.SubSection(@"Resource\ConstructionRepairRanges");
            if (section1 != null)
            {
                foreach (string text29 in section1.Values.Keys)
                {
                    this.ConstructionRepairRanges[text29] = decimal.Parse(section1.Values[text29]);
                }
            }
            section1 = parser1.RootSection.SubSection(@"Resource\ConstructionExclusionRange");
            if (section1 != null)
            {
                foreach (string text30 in section1.Values.Keys)
                {
                    this.ConstructionExclusionRange[text30] = decimal.Parse(section1.Values[text30]);
                }
            }
            section1 = parser1.RootSection.SubSection(@"Resource\MaxEnergy");
            if (section1 != null)
            {
                foreach (string text31 in section1.Values.Keys)
                {
                    this.MaxEnergy[text31] = decimal.Parse(section1.Values[text31]);
                }
            }
            section1 = parser1.RootSection.SubSection(@"Resource\MinEnergy");
            if (section1 != null)
            {
                foreach (string text32 in section1.Values.Keys)
                {
                    this.MinEnergy[text32] = decimal.Parse(section1.Values[text32]);
                }
            }
            section1 = parser1.RootSection.SubSection(@"TASKLISTS\NORMAL");
            if (section1 != null)
            {
                foreach (string text33 in section1.Values.Keys)
                {
                    ArrayList list1 = new ArrayList();
                    foreach (string text34 in section1.Values[text33].Split(new char[] { ',' }))
                    {
                        string text35 = text34;
                        text35 = text35.Trim();
                        text35 = text35.ToLower();
                        list1.Add(text35);
                    }
                    this.unittasklists[text33] = list1;
                }
            }
            section1 = parser1.RootSection.SubSection(@"TASKLISTS\CHEAT");
            if (section1 != null)
            {
                foreach (string text36 in section1.Values.Keys)
                {
                    ArrayList list2 = new ArrayList();
                    foreach (string text37 in section1.Values[text36].Split(new char[] { ',' }))
                    {
                        string text38 = text37;
                        text38 = text38.Trim();
                        text38 = text38.ToLower();
                        list2.Add(text38);
                    }
                    this.unitcheattasklists[text36] = list2;
                }
            }
            section1 = parser1.RootSection.SubSection(@"TASKLISTS\LISTS");
            if (section1 != null)
            {
                foreach (string text39 in section1.Values.Keys)
                {
                    ArrayList list3 = new ArrayList();
                    foreach (string text40 in section1.Values[text39].Split(new char[] { ',' }))
                    {
                        string text41 = text40;
                        text41 = text41.Trim();
                        text41 = text41.ToLower();
                        list3.Add(text41);
                    }
                    this.tasklists[text39] = list3;
                }
            }
            return true;
        }

        public void RemoveTaskList(string name)
        {
            this.tasklists.Remove(name);
            foreach (string text1 in this.unittasklists.Keys)
            {
                while (this.unittasklists[text1].Contains(name))
                {
                    this.unittasklists[text1].Remove(name);
                }
            }
        }

        public string Save(bool tofile)
        {
            if (tofile)
            {
                if (buildtreefilename == "")
                {
                    //
                    MessageBox.Show("you cant save! You havent opened a config yet!");
                    return "";
                }
            }
            string text1 = "";
            text1 += "[AI]\n{\n";
            text1 = text1 + "\tauthor=" + this.author + ";\n";
            text1 = text1 + "\tversion=" + this.version + ";\n";
            text1 = text1 + "\tmessage=" + this.message + ";\n\n";
            text1 = text1 + "\tabstract=0;\n";
            text1 = text1 + "\tuse_mod_default_if_absent=0;\n\n";
            object obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\tdefence_spacing=", this.defence_spacing, ";\n" });
            object obj2 = text1;
            text1 = string.Concat(new object[] { obj2, "\tpower_spacing=", this.power_spacing, ";\n" });
            object obj3 = text1;
            text1 = string.Concat(new object[] { obj3, "\tfactory_spacing=", this.factory_spacing, ";\n" });
            text1 += "\tdefault_spacing="+ default_spacing + ";\n\n";
            text1 += "\tdynamic_selection=0;\n";
            text1 += "\tspacemod=" + booltoint(spacemod) + ";\n";
            text1 +="\thard_target=0;\n\n";
            if (this.Antistall){
                text1 = text1 + "\tantistall=3;\n";
            } else {
                text1 = text1 + "\tantistall=0;\n";
            }
            if (this.interpolate){
                text1 = text1 + "\tinterpolate_tag=b_rule_extreme_nofact;\n";
            }else{
                text1 = text1 + "\tinterpolate_tag=b_na;\n";
            }
            object obj8 = text1;
            text1 = string.Concat(new object[] { obj8, "\tMaxStallTimeMobile=", this.StallTimeMobile, ";\n" });
            text1 +="\tMaxStallTimeImmobile="+this.StallTimeImMobile+";\n\n";
            text1 +="\tnormal_handicap="+normal_handicap+";\n";
            text1 += "\tantistallwindow="+ antistallwindow+";\n";
            text1 += "\tinitial_threat_value="+ initialAttackSize+ ";\n\n";
            text1 += "\tmaximum_attack_group_size="+maxAttackSize+";\n\n";
            text1 += "\tincrease_threshold_percentage=" + AttackIncrementPercentage + ";\n\n";
            text1 += "\tincrease_threshold_value=" + AttackIncrementValue + ";\n\n";
            text1 += "\tmexnoweaponradius=" + mexnoweaponradius + ";\n\n";

            ArrayList list1 = new ArrayList();
            foreach (string text2 in this.movement.Keys)
            {
                if (this.movement[text2] == 0)
                {
                    list1.Add(text2);
                }
            }
            text1 = text1 + "\thold_pos=";
            for (int num1 = 0; num1 < list1.Count; num1++)
            {
                text1 = text1 + list1[num1];
                if ((num1 + 1) < list1.Count)
                {
                    text1 = text1 + ",";
                }
            }
            text1 = text1 + ";\n";
            list1.Clear();
            foreach (string text3 in this.movement.Keys)
            {
                if (this.movement[text3] == 1)
                {
                    list1.Add(text3);
                }
            }
            text1 = text1 + "\tmaneouvre=";
            for (int num2 = 0; num2 < list1.Count; num2++)
            {
                text1 = text1 + list1[num2];
                if ((num2 + 1) < list1.Count)
                {
                    text1 = text1 + ",";
                }
            }
            text1 = text1 + ";\n";
            list1.Clear();
            foreach (string text4 in this.movement.Keys)
            {
                if (this.movement[text4] == 2)
                {
                    list1.Add(text4);
                }
            }
            text1 = text1 + "\troam=";
            for (int num3 = 0; num3 < list1.Count; num3++)
            {
                text1 = text1 + list1[num3];
                if ((num3 + 1) < list1.Count)
                {
                    text1 = text1 + ",";
                }
            }
            text1 = text1 + ";\n";
            list1.Clear();
            foreach (string text5 in this.firingstate.Keys)
            {
                if (this.firingstate[text5] == 0)
                {
                    list1.Add(text5);
                }
            }
            text1 = text1 + "\thold_fire=";
            for (int num4 = 0; num4 < list1.Count; num4++)
            {
                text1 = text1 + list1[num4];
                if ((num4 + 1) < list1.Count)
                {
                    text1 = text1 + ",";
                }
            }
            text1 = text1 + ";\n";
            list1.Clear();
            foreach (string text6 in this.firingstate.Keys)
            {
                if (this.firingstate[text6] == 1)
                {
                    list1.Add(text6);
                }
            }
            text1 = text1 + "\treturn_fire=";
            for (int num5 = 0; num5 < list1.Count; num5++)
            {
                text1 = text1 + list1[num5];
                if ((num5 + 1) < list1.Count)
                {
                    text1 = text1 + ",";
                }
            }
            text1 = text1 + ";\n";
            list1.Clear();
            foreach (string text7 in this.firingstate.Keys)
            {
                if (this.firingstate[text7] == 2)
                {
                    list1.Add(text7);
                }
            }
            text1 = text1 + "\tfire_at_will=";
            for (int num6 = 0; num6 < list1.Count; num6++)
            {
                text1 = text1 + list1[num6];
                if ((num6 + 1) < list1.Count)
                {
                    text1 = text1 + ",";
                }
            }
            text1 = text1 + ";\n\n";
            list1.Clear();
            text1 = text1 + "\tNoAntiStall=";
            for (int num7 = 0; num7 < this.neverantistall.Count; num7++)
            {
                if (!this.neverantistall[num7].Equals(""))
                {
                    text1 = text1 + this.neverantistall[num7];
                    if ((num7 + 1) < this.neverantistall.Count)
                    {
                        text1 = text1 + ",";
                    }
                }
            }
            text1 = text1 + ";\n";
            text1 = text1 + "\tAlwaysAntiStall=";
            for (int num8 = 0; num8 < this.alwaysantistall.Count; num8++)
            {
                if (!this.alwaysantistall[num8].Equals(""))
                {
                    text1 = text1 + this.alwaysantistall[num8];
                    if ((num8 + 1) < this.alwaysantistall.Count)
                    {
                        text1 = text1 + ",";
                    }
                }
            }
            text1 = text1 + ";\n";
            text1 = text1 + "\tsinglebuild=";
            for (int num9 = 0; num9 < this.singlebuild.Count; num9++)
            {
                if (!this.singlebuild[num9].Equals(""))
                {
                    text1 = text1 + this.singlebuild[num9];
                    if ((num9 + 1) < this.singlebuild.Count)
                    {
                        text1 = text1 + ",";
                    }
                }
            }
            text1 = text1 + ";\n";
            text1 = text1 + "\tsolobuild=";
            for (int num10 = 0; num10 < this.solobuild.Count; num10++)
            {
                if (!this.solobuild[num10].Equals(""))
                {
                    text1 = text1 + this.solobuild[num10];
                    if ((num10 + 1) < this.solobuild.Count)
                    {
                        text1 = text1 + ",";
                    }
                }
            }
            text1 = text1 + ";\n";
            text1 = text1 + "\tscouters=";
            for (int num11 = 0; num11 < this.scouters.Count; num11++)
            {
                if (!this.scouters[num11].Equals(""))
                {
                    text1 = text1 + this.scouters[num11];
                    if ((num11 + 1) < this.scouters.Count)
                    {
                        text1 = text1 + ",";
                    }
                }
            }
            text1 += ";\n";
            text1 = text1 + "\tattackers=";
            for (int num12 = 0; num12 < this.attackers.Count; num12++)
            {
                if (!this.attackers[num12].Equals(""))
                {
                    text1 = text1 + this.attackers[num12];
                    if ((num12 + 1) < this.attackers.Count)
                    {
                        text1 = text1 + ",";
                    }
                }
            }
            text1 += ";\n";
            text1 += "\t[geotherm]\n";
            text1 += "\t{\n";
            text1 += "\t\tsearchdistance="+GeoSearchRadius+";\n";
            text1 += "\t\tnoenemiesdistance=" + NoEnemyGeo + ";\n";
            text1 += "\t}\n";
            text1 = text1 + ";\n";
            text1 = text1 + "\n}\n\n";
            text1 = text1 + "[TASKLISTS]\n";
            text1 = text1 + "{\n";
            text1 = text1 + "\t[NORMAL]\n";
            text1 = text1 + "\t{\n";
            foreach (string text8 in this.unittasklists.Keys)
            {
                ArrayList list2 = this.unittasklists[text8];
                if (list2.Count > 0)
                {
                    text1 = text1 + "\t\t" + text8 + "=";
                    for (int num13 = 0; num13 < list2.Count; num13++)
                    {
                        text1 = text1 + list2[num13];
                        if ((num13 + 1) < list2.Count)
                        {
                            text1 = text1 + ",";
                        }
                    }
                    text1 = text1 + ";\n";
                }
            }
            text1 = text1 + "\t}\n";
            text1 = text1 + "\t[CHEAT]\n";
            text1 = text1 + "\t{\n";
            foreach (string text9 in this.unitcheattasklists.Keys)
            {
                ArrayList list3 = this.unitcheattasklists[text9];
                if (list3.Count > 0)
                {
                    text1 = text1 + "\t\t" + text9 + "=";
                    for (int num14 = 0; num14 < list3.Count; num14++)
                    {
                        text1 = text1 + list3[num14];
                        if ((num14 + 1) < list3.Count)
                        {
                            text1 = text1 + ",";
                        }
                    }
                    text1 = text1 + ";\n";
                }
            }
            text1 = text1 + "\t}\n";
            text1 = text1 + "\t[LISTS]\n";
            text1 = text1 + "\t{\n";
            foreach (string text10 in this.tasklists.Keys)
            {
                ArrayList list4 = this.tasklists[text10];
                if (list4.Count > 0)
                {
                    text1 = text1 + "\t\t" + text10 + "=";
                    for (int num15 = 0; num15 < list4.Count; num15++)
                    {
                        text1 = text1 + list4[num15];
                        if ((num15 + 1) < list4.Count)
                        {
                            text1 = text1 + ",";
                        }
                    }
                    text1 = text1 + ";\n";
                }
            }
            text1 = text1 + "\t}\n";
            text1 = text1 + "}\n\n";
            text1 = text1 + "[ECONOMY]\n";
            text1 = text1 + "{\n";
            text1 = text1 + "\t[RULES]\n";
            text1 = text1 + "\t{\n";
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tpower=", this.powerRule, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tmex=", this.mexRule, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tfactorymetal=", this.factorymetalRule, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tfactoryenergy=", this.factoryenergyRule, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tfactorymetalgap=", this.factorymetalgapRule, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tenergystorage=", this.energystorageRule, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tmetalstorage=", this.metalstorageRule, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tmakermetal=", this.makermetalRule, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\tmakerenergy=", this.makerenergyRule, ";\n" });
            text1 = text1 + "\t\t[EXTREME]\n";
            text1 = text1 + "\t\t{\n";
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tpower=", this.powerRuleEx, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tmex=", this.mexRuleEx, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tfactorymetal=", this.factorymetalRuleEx, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tfactoryenergy=", this.factoryenergyRuleEx, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tfactorymetalgap=", this.factorymetalgapRuleEx, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tenergystorage=", this.energystorageRuleEx, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tmetalstorage=", this.metalstorageRuleEx, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tmakermetal=", this.makermetalRuleEx, ";\n" });
            obj1 = text1;
            text1 = string.Concat(new object[] { obj1, "\t\t\tmakerenergy=", this.makerenergyRuleEx, ";\n" });
            text1 = text1 + "\t\t}\n";
            text1 = text1 + "\t}\n";
            text1 = text1 + "}\n";
            text1 = text1 + "[Resource]\n";
            text1 = text1 + "{\n";
            text1 = text1 + "\t[ConstructionRepairRanges]\n";
            text1 = text1 + "\t{\n";
            foreach (string text11 in this.ConstructionRepairRanges.Keys)
            {
                obj1 = text1;
                text1 = string.Concat(new object[] { obj1, "\t\t", text11, "=", this.ConstructionRepairRanges[text11], ";\n" });
            }
            text1 = text1 + "\t}\n";
            text1 = text1 + "\t[ConstructionExclusionRange]\n";
            text1 = text1 + "\t{\n";
            foreach (string text12 in this.ConstructionExclusionRange.Keys)
            {
                obj1 = text1;
                text1 = string.Concat(new object[] { obj1, "\t\t", text12, "=", this.ConstructionExclusionRange[text12], ";\n" });
            }
            text1 = text1 + "\t}\n";
            text1 = text1 + "\t[MaxEnergy]\n";
            text1 = text1 + "\t{\n";
            foreach (string text13 in this.MaxEnergy.Keys)
            {
                obj1 = text1;
                text1 = string.Concat(new object[] { obj1, "\t\t", text13, "=", this.MaxEnergy[text13], ";\n" });
            }
            text1 = text1 + "\t}\n";
            text1 = text1 + "\t[MinEnergy]\n";
            text1 = text1 + "\t{\n";
            foreach (string text14 in this.MinEnergy.Keys)
            {
                obj1 = text1;
                text1 = string.Concat(new object[] { obj1, "\t\t", text14, "=", this.MinEnergy[text14], ";\n" });
            }
            text1 = text1 + "\t}\n";
            text1 = text1 + "}\n";
            if (tofile)
            {
                Stream stream1 = File.Create(this.buildtreefilename);
                StreamWriter writer1 = new StreamWriter(stream1);
                writer1.Write(text1);
                writer1.Flush();
                writer1.Close();
            }
            return text1;
        }

        public void SetTaskList(string name, ArrayList newdata)
        {
            this.tasklists[name] = newdata;
        }


        public bool Antistall
        {
            get
            {
                return this.f.Antistall.Checked;
            }
            set
            {
                this.f.Antistall.Checked = value;
            }
        }

        public decimal antistallwindow
        {
            get
            {
                return this.f.antistallwindow.Value;
            }
            set
            {
                this.f.antistallwindow.Value = value;
            }
        }

        public decimal AttackIncrementValue
        {
            get
            {
                return this.f.AttackIncrementValue.Value;
            }
            set
            {
                this.f.AttackIncrementValue.Value = value;
            }
        }

        public decimal maxAttackSize
        {
            get
            {
                return this.f.maxAttackSize.Value;
            }
            set
            {
                this.f.maxAttackSize.Value = value;
            }
        }//

        public decimal AttackIncrementPercentage
        {
            get
            {
                return this.f.AttackIncrementPercentage.Value;
            }
            set
            {
                this.f.AttackIncrementPercentage.Value = value;
            }
        }

        public decimal mexnoweaponradius
        {
            get
            {
                return this.f.mexnoweaponradius.Value;
            }
            set
            {
                this.f.mexnoweaponradius.Value = value;
            }
        }

        public decimal GeoSearchRadius
        {
            get
            {
                return this.f.GeoSearchRadius.Value;
            }
            set
            {
                this.f.GeoSearchRadius.Value = value;
            }
        }

        public decimal NoEnemyGeo
        {
            get
            {
                return this.f.NoEnemyGeo.Value;
            }
            set
            {
                this.f.NoEnemyGeo.Value = value;
            }
        }
        public string author
        {
            get
            {
                return this._author;
            }
            set
            {
                this._author = value;
                this.f.Author.Text = this._author;
            }
        }

        public string buildtreefilename
        {
            get
            {
                return this._buildtreefilename;
            }
            set
            {
                this._buildtreefilename = value;
            }
        }

        public decimal default_spacing
        {
            get
            {
                return this.f.DefaultSpacing.Value;
            }
            set
            {
                this.f.DefaultSpacing.Value = value;
            }
        }

        public decimal defence_spacing
        {
            get
            {
                return this.f.DefenceSpacing.Value;
            }
            set
            {
                this.f.DefenceSpacing.Value = value;
            }
        }

        public decimal energystorageRule
        {
            get
            {
                return this.f.energystorageRule.Value;
            }
            set
            {
                this.f.energystorageRule.Value = value;
            }
        }

        public decimal energystorageRuleEx
        {
            get
            {
                return this.f.energystorageRuleEx.Value;
            }
            set
            {
                this.f.energystorageRuleEx.Value = value;
            }
        }

        public decimal factory_spacing
        {
            get
            {
                return this.f.FactorySpacing.Value;
            }
            set
            {
                this.f.FactorySpacing.Value = value;
            }
        }

        public decimal factoryenergyRule
        {
            get
            {
                return this.f.factoryenergyRule.Value;
            }
            set
            {
                this.f.factoryenergyRule.Value = value;
            }
        }

        public decimal factoryenergyRuleEx
        {
            get
            {
                return this.f.factoryenergyRuleEx.Value;
            }
            set
            {
                this.f.factoryenergyRuleEx.Value = value;
            }
        }

        public decimal factorymetalgapRule
        {
            get
            {
                return this.f.factorymetalgapRule.Value;
            }
            set
            {
                this.f.factorymetalgapRule.Value = value;
            }
        }

        public decimal factorymetalgapRuleEx
        {
            get
            {
                return this.f.factorymetalgapRuleEx.Value;
            }
            set
            {
                this.f.factorymetalgapRuleEx.Value = value;
            }
        }

        public decimal factorymetalRule
        {
            get
            {
                return this.f.factorymetalRule.Value;
            }
            set
            {
                this.f.factorymetalRule.Value = value;
            }
        }

        public decimal factorymetalRuleEx
        {
            get
            {
                return this.f.factorymetalRuleEx.Value;
            }
            set
            {
                this.f.factorymetalRuleEx.Value = value;
            }
        }

        public decimal initialAttackSize
        {
            get
            {
                return this.f.initialAttackSize.Value;
            }
            set
            {
                this.f.initialAttackSize.Value = value;
            }
        }

        public bool interpolate
        {
            get
            {
                return this.f.interpolate.Checked;
            }
            set
            {
                this.f.interpolate.Checked = value;
            }
        }

        public decimal makerenergyRule
        {
            get
            {
                return this.f.makerenergyRule.Value;
            }
            set
            {
                this.f.makerenergyRule.Value = value;
            }
        }

        public decimal makerenergyRuleEx
        {
            get
            {
                return this.f.makerenergyRuleEx.Value;
            }
            set
            {
                this.f.makerenergyRuleEx.Value = value;
            }
        }

        public decimal makermetalRule
        {
            get
            {
                return this.f.makermetalRule.Value;
            }
            set
            {
                this.f.makermetalRule.Value = value;
            }
        }

        public decimal makermetalRuleEx
        {
            get
            {
                return this.f.makermetalRuleEx.Value;
            }
            set
            {
                this.f.makermetalRuleEx.Value = value;
            }
        }

        public string message
        {
            get
            {
                return this._message;
            }
            set
            {
                this._message = value;
                this.f.Message.Text = this._message;
            }
        }

        public decimal metalstorageRule
        {
            get
            {
                return this.f.metalstorageRule.Value;
            }
            set
            {
                this.f.metalstorageRule.Value = value;
            }
        }

        public decimal metalstorageRuleEx
        {
            get
            {
                return this.f.metalstorageRuleEx.Value;
            }
            set
            {
                this.f.metalstorageRuleEx.Value = value;
            }
        }

        public decimal mexRule
        {
            get
            {
                return this.f.mexRule.Value;
            }
            set
            {
                this.f.mexRule.Value = value;
            }
        }

        public decimal mexRuleEx
        {
            get
            {
                return this.f.mexRuleEx.Value;
            }
            set
            {
                this.f.mexRuleEx.Value = value;
            }
        }

        public decimal normal_handicap
        {
            get
            {
                return this.f.normal_handicap.Value;
            }
            set
            {
                this.f.normal_handicap.Value = value;
            }
        }

        public decimal power_spacing
        {
            get
            {
                return this.f.PowerSpacing.Value;
            }
            set
            {
                this.f.PowerSpacing.Value = value;
            }
        }

        public decimal powerRule
        {
            get
            {
                return this.f.powerRule.Value;
            }
            set
            {
                this.f.powerRule.Value = value;
            }
        }

        public decimal powerRuleEx
        {
            get
            {
                return this.f.powerRuleEx.Value;
            }
            set
            {
                this.f.powerRuleEx.Value = value;
            }
        }

        public bool spacemod
        {
            get
            {
                return this.f.spacemod.Checked;
            }
            set
            {
                this.f.spacemod.Checked = value;
            }
        }

        public decimal StallTimeImMobile
        {
            get
            {
                return this.f.StallTimeImMobile.Value;
            }
            set
            {
                this.f.StallTimeImMobile.Value = value;
            }
        }

        public decimal StallTimeMobile
        {
            get
            {
                return this.f.StallTimeMobile.Value;
            }
            set
            {
                this.f.StallTimeMobile.Value = value;
            }
        }

        public string version
        {
            get
            {
                return this._version;
            }
            set
            {
                this._version = value;
                this.f.Version.Text = this._version;
            }
        }


        private string _author;
        public string _buildtreefilename;
        private string _message;
        private string _version;
        public ArrayList alwaysantistall;
        public int antistall;
        public ArrayList attackers;
        public Dictionary<string, decimal> ConstructionExclusionRange;
        public Dictionary<string, decimal> ConstructionRepairRanges;
        public Form1 f;
        public Dictionary<string, int> firingstate;
        public string interpolate_tag;
        public ArrayList kamikaze;
        public CKeywords keywords;
        public Dictionary<string, decimal> MaxEnergy;
        public int MaxStallTimeImmobile;
        public int MaxStallTimeMobile;
        public Dictionary<string, decimal> MinEnergy;
        public Dictionary<string, int> movement;
        public ArrayList neverantistall;
        public ArrayList scouters;
        public ArrayList singlebuild;
        public ArrayList solobuild;
        public Dictionary<string, ArrayList> tasklists;
        public Dictionary<string, ArrayList> unitcheattasklists;
        public Dictionary<string, ArrayList> unittasklists;
        public int use_mod_default;
        public int use_mod_default_if_absent;
    }
}


