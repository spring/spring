using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

public class TdfParser
{
    public TdfParser()
    {
        this.rawtdf = "";
    }

    public TdfParser(string tdfcontents)
    {
        this.rawtdf = "";
        this.rawtdf = tdfcontents;
        this.Parse();
    }

    public static TdfParser FromBuffer(byte[] data, int size)
    {
        return new TdfParser(Encoding.UTF8.GetString(data, 0, size));
    }

    public static TdfParser FromFile(string filename)
    {
        string text1 = "";
        StreamReader reader1 = new StreamReader(filename, Encoding.UTF8);
        text1 = reader1.ReadToEnd();
        reader1.Close();
        return new TdfParser(text1);
    }

    private void GenerateSplitArray()
    {
        this.splitrawtdf = this.rawtdf.Split("\n".ToCharArray());
        for (int num1 = 0; num1 < this.splitrawtdf.GetLength(0); num1++)
        {
            this.splitrawtdf[num1] = this.splitrawtdf[num1].Trim();
        }
    }

    private void Parse()
    {
        this.GenerateSplitArray();
        this.RootSection = new Section();
        this.level = 0;
        this.currentsection = this.RootSection;
        this.currentstate = State.NormalParse;
        for (int num1 = 0; num1 < this.splitrawtdf.GetLength(0); num1++)
        {
            this.ParseLine(ref num1, this.splitrawtdf[num1]);
        }
    }

    private void ParseLine(ref int linenum, string line)
    {
        switch (this.currentstate)
        {
            case State.InSectionHeader:
                if (line.IndexOf("{") == 0)
                {
                    this.currentstate = State.NormalParse;
                    this.level++;
                }
                return;

            case State.NormalParse:
                if (line.IndexOf("[") == 0)
                {
                    this.currentstate = State.InSectionHeader;
                    string text1 = (line.Substring(1) + "]").Split("]".ToCharArray())[0].ToLower();
                    Section section1 = new Section(text1);
                    section1.Parent = this.currentsection;
                    if (!this.currentsection.SubSections.ContainsKey(text1))
                    {
                        this.currentsection.SubSections.Add(text1, section1);
                    }
                    this.currentsection = section1;
                    return;
                }
                if (line.IndexOf("}") == 0)
                {
                    this.level--;
                    if (this.currentsection.Parent != null)
                    {
                        this.currentsection = this.currentsection.Parent;
                        return;
                    }
                    return;
                }
                if (((line != "") && (line.IndexOf("//") != 0)) && (line.IndexOf("/*") != 0))
                {
                    int num1 = line.IndexOf("=");
                    if (num1 < 0)
                    {
                        return;
                    }
                    string text2 = line.Substring(0, num1).ToLower();
                    string text3 = line.Substring(num1 + 1);
                    text3 = (text3 + ";").Split(";".ToCharArray())[0];
                    if (this.currentsection.Values.ContainsKey(text2))
                    {
                        return;
                    }
                    this.currentsection.Values.Add(text2, text3);
                }
                return;
        }
    }


    private Section currentsection;
    private State currentstate;
    private int level;
    public string rawtdf;
    public Section RootSection;
    private string[] splitrawtdf;


    public class Section
    {
        public Section()
        {
            this.SubSections = new Dictionary<string, TdfParser.Section>();
            this.Values = new Dictionary<string, string>();
        }

        public Section(string name)
        {
            this.SubSections = new Dictionary<string, TdfParser.Section>();
            this.Values = new Dictionary<string, string>();
            this.Name = name;
        }

        public double[] GetDoubleArray(string name)
        {
            return this.GetDoubleArray(new double[0], name);
        }

        public double[] GetDoubleArray(double[] defaultvalue, string name)
        {
            try
            {
                string text1 = this.GetValueByPath(name);
                string[] textArray1 = text1.Trim().Split(" ".ToCharArray());
                int num1 = textArray1.Length;
                double[] numArray1 = new double[num1];
                for (int num2 = 0; num2 < num1; num2++)
                {
                    numArray1[num2] = Convert.ToDouble(textArray1[num2]);
                }
                return numArray1;
            }
            catch
            {
                return defaultvalue;
            }
        }

        public double GetDoubleValue(string name)
        {
            return this.GetDoubleValue(0, name);
        }

        public double GetDoubleValue(double defaultvalue, string name)
        {
            try
            {
                string text1 = this.GetValueByPath(name);
                return Convert.ToDouble(text1);
            }
            catch
            {
                return defaultvalue;
            }
        }

        public int GetIntValue(string name)
        {
            return this.GetIntValue(0, name);
        }

        public int GetIntValue(int defaultvalue, string name)
        {
            try
            {
                string text1 = this.GetValueByPath(name);
                return Convert.ToInt32(text1);
            }
            catch
            {
                return defaultvalue;
            }
        }

        private List<string> GetPathParts(string path)
        {
            string[] textArray1 = path.Split("/".ToCharArray());
            List<string> list1 = new List<string>();
            foreach (string text1 in textArray1)
            {
                string[] textArray2 = text1.Trim().Split(@"\".ToCharArray());
                foreach (string text2 in textArray2)
                {
                    list1.Add(text2.Trim().ToLower());
                }
            }
            return list1;
        }

        private TdfParser.Section GetSectionByPath(string path)
        {
            List<string> list1 = this.GetPathParts(path);
            TdfParser.Section section1 = this;
            for (int num1 = 0; num1 < list1.Count; num1++)
            {
                section1 = section1.SubSections[list1[num1]];
            }
            return section1;
        }

        public string GetStringValue(string name)
        {
            return this.GetStringValue("", name);
        }

        public string GetStringValue(string defaultvalue, string name)
        {
            try
            {
                return this.GetValueByPath(name);
            }
            catch
            {
                return defaultvalue;
            }
        }

        private string GetValueByPath(string path)
        {
            List<string> list1 = this.GetPathParts(path);
            TdfParser.Section section1 = this;
            for (int num1 = 0; num1 < (list1.Count - 1); num1++)
            {
                section1 = section1.SubSections[list1[num1]];
            }
            return section1.Values[list1[list1.Count - 1]];
        }

        public TdfParser.Section SubSection(string name)
        {
            try
            {
                return this.GetSectionByPath(name);
            }
            catch
            {
                return null;
            }
        }


        public string Name;
        public TdfParser.Section Parent;
        public Dictionary<string, TdfParser.Section> SubSections;
        public Dictionary<string, string> Values;
    }

    private enum State
    {
        InSectionHeader,
        NormalParse
    }
}

