namespace NTaiToolkit
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    public class CMod
    {
        public CMod()
        {
            this.modfilename = "";
            this.modname = "";
            this.units = new ArrayList();
            this.values = new Dictionary<string, double>();
            this.human_names = new Dictionary<string, string>();
            this.descriptions = new Dictionary<string, string>();
        }

        public bool LoadMod(string modfile)
        {
            this.modfilename = modfile;
            TdfParser parser1 = TdfParser.FromFile(modfile);
            if (parser1 != null)
            {
                TdfParser.Section section1 = parser1.RootSection.SubSection(@"AI\VALUES");
                if (section1 != null)
                {
                    foreach (string text1 in section1.Values.Keys)
                    {
                        string text2 = text1;
                        text2 = text2.ToLower();
                        text2 = text2.Trim();
                        this.units.Add(text2);
                        this.values[text2] = section1.GetDoubleValue(text1);
                    }
                }
                section1 = parser1.RootSection.SubSection(@"AI\NAMES");
                if (section1 != null)
                {
                    foreach (string text3 in section1.Values.Keys)
                    {
                        string text4 = text3;
                        text4 = text4.ToLower();
                        text4 = text4.Trim();
                        this.human_names[text4] = section1.GetStringValue(text3);
                    }
                }
                section1 = parser1.RootSection.SubSection(@"AI\DESCRIPTIONS");
                if (section1 != null)
                {
                    foreach (string text5 in section1.Values.Keys)
                    {
                        string text6 = text5;
                        text6 = text6.ToLower();
                        text6 = text6.Trim();
                        this.descriptions[text6] = section1.GetStringValue(text5);
                    }
                }
                this.units.Sort();
            }
            return false;
        }

        public bool SaveMod()
        {
            return true;
        }


        public Dictionary<string, string> descriptions;
        public Form1 f;
        public Dictionary<string, string> human_names;
        public string modfilename;
        public string modname;
        public ArrayList units;
        public Dictionary<string, double> values;
    }
}

