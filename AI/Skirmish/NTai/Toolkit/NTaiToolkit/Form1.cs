namespace NTaiToolkit
{
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;
    using System.Resources;
    using System.Reflection;
    using ICSharpCode.SharpZipLib.Zip;

    public class Form1 : Form
    {
        public Form1()
        {
            this.VERSION = 12;
            this.currentselected = "";
            Thread.CurrentThread.CurrentCulture = new CultureInfo("en-GB");
            this.InitializeComponent();
            Assembly _assembly = Assembly.GetExecutingAssembly();
            StreamReader _textStreamReader = new StreamReader(_assembly.GetManifestResourceStream("NTaiToolkit.TextFile1.txt"));
            string helpText = _textStreamReader.ReadToEnd();
            this.richTextBox1.Text = helpText;
            this.mod = new CMod();
            this.buildtree = new CBuildtree(this);
            this.mod.f = this;
        }

        private void AlwaysantistallCheck_CheckedChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                if (!this.buildtree.alwaysantistall.Contains(text1))
                {
                    this.buildtree.alwaysantistall.Add(text1);
                }else{
                    this.buildtree.alwaysantistall.Remove(text1);
                }
            }
        }



        private void AttackCheckChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                if (!this.buildtree.attackers.Contains(text1))
                {
                    this.buildtree.attackers.Add(text1);
                }
                else
                {
                    this.buildtree.attackers.Remove(text1);
                }
            }
        }

        private void Author_TextChanged(object sender, EventArgs e)
        {
            this.buildtree.author = this.Author.Text;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            string text1 = this.newtasklistbox.Text;
            if (text1.Length > 0)
            {

                this.buildtree.AddTaskList(text1);
                if (!this.currentbuildqueue.Items.Contains(text1))
                {
                    this.currentbuildqueue.Items.Add(text1);
                    this.currentbuildqueue.SelectedItem = text1;
                    this.currentqueue.ClearSelected();
                }
                else
                {
                    MessageBox.Show("A tasklist with this name already exists");
                    this.currentbuildqueue.SelectedItem = text1;
                }
                
            }
            else
            {
                //fix for blank tasklist name bug
                MessageBox.Show("You must specify a name for the tasklist");
            }
        }

        private void button10_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                int num1 = this.currentqueue.SelectedIndex;
                string text1 = this.GetSelectedKeyWord();
                if (this.currentqueue.SelectedIndex > 0)
                {
                    this.currentqueue.Items.Insert(this.keywords.SelectedIndex, text1);
                    this.currentqueue.SelectedIndex--;
                }
                else
                {
                    this.currentqueue.Items.Insert(0, text1);
                    this.currentqueue.SelectedIndex = 0;
                }
                this.SaveQueue();
            }
        }

        private void button11_Click(object sender, EventArgs e)
        {
            string text1 = this.newtasklistbox.Text;
            if (text1 != "")
            {
                string text2 = "";
                this.openFileDialog1.ShowDialog();
                try
                {
                    if (this.openFileDialog1.CheckFileExists.Equals(true))
                    {
                        Stream stream1 = this.openFileDialog1.OpenFile();
                        StreamReader reader1 = new StreamReader(stream1);
                        text2 = reader1.ReadToEnd();
                        reader1.Close();
                        string[] textArray1 = text2.Split(new char[] { ',' });
                        for (int num1 = 0; num1 < textArray1.Length; num1++)
                        {
                            string text3 = textArray1[num1];
                            text3 = text3.Trim();
                            text3 = text3.ToLower();
                            textArray1[num1] = text3;
                        }
                        this.buildtree.AddTaskList(text1);
                        if (!this.currentbuildqueue.Items.Contains(text1))
                        {
                            this.currentbuildqueue.Items.Add(text1);
                        }
                        this.currentbuildqueue.SelectedItem = text1;
                        this.currentqueue.ClearSelected();
                        foreach (string text4 in textArray1)
                        {
                            this.currentqueue.Items.Add(text4);
                        }
                        this.SaveQueue();
                    }
                }
                catch
                {
                }
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                int num1 = this.currentqueue.SelectedIndex;
                if ((num1 > -1) && (this.currentqueue.SelectedIndex != 0))
                {
                    string text1 = this.currentqueue.SelectedItem as string;
                    this.currentqueue.Items.RemoveAt(num1);
                    this.currentqueue.Items.Insert(num1 - 1, text1);
                    this.currentqueue.SelectedIndex = num1 - 1;
                    this.SaveQueue();
                }
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                int num1 = this.currentqueue.SelectedIndex;
                if ((num1 > -1) && (this.currentqueue.SelectedIndex != (this.currentqueue.Items.Count - 1)))
                {
                    string text1 = this.currentqueue.SelectedItem as string;
                    this.currentqueue.Items.RemoveAt(num1);
                    this.currentqueue.Items.Insert(num1 + 1, text1);
                    this.currentqueue.SelectedIndex = num1 + 1;
                    this.SaveQueue();
                }
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                int num1 = this.currentqueue.SelectedIndex;
                if (num1 > -1)
                {
                    this.currentqueue.Items.RemoveAt(num1);
                    this.SaveQueue();
                }
            }
        }

        private void button5_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                this.currentqueue.Items.Clear();
                this.SaveQueue();
            }
        }

        private void button6_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                int num1 = this.currentqueue.SelectedIndex;
                if ((num1 > -1) && (this.currentqueue.SelectedIndex != 0))
                {
                    string text1 = this.currentqueue.SelectedItem as string;
                    this.currentqueue.Items.RemoveAt(num1);
                    this.currentqueue.Items.Insert(0, text1);
                    this.currentqueue.SelectedIndex = 0;
                    this.SaveQueue();
                }
            }
        }

        private void button7_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                int num1 = this.currentqueue.SelectedIndex;
                if ((num1 > -1) && (this.currentqueue.SelectedIndex != (this.currentqueue.Items.Count - 1)))
                {
                    string text1 = this.currentqueue.SelectedItem as string;
                    this.currentqueue.Items.RemoveAt(num1);
                    this.currentqueue.Items.Add(text1);
                    this.currentqueue.SelectedIndex = this.currentqueue.Items.Count - 1;
                    this.SaveQueue();
                }
            }
        }

        private void button8_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                this.currentqueue.Items.Insert(0, this.GetSelectedKeyWord());
                this.SaveQueue();
            }
        }

        private void button9_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                int num1 = this.currentqueue.SelectedIndex;
                string text1 = this.GetSelectedKeyWord();
                if ((num1 > -1) && (this.currentqueue.SelectedIndex != (this.currentqueue.Items.Count - 1)))
                {
                    this.currentqueue.Items.Insert(this.keywords.SelectedIndex + 1, text1);
                    this.currentqueue.SelectedIndex++;
                }
                else
                {
                    this.currentqueue.Items.Insert(this.currentqueue.Items.Count - 1, text1);
                    this.currentqueue.SelectedIndex = this.currentqueue.Items.Count - 1;
                }
                this.SaveQueue();
            }
        }

        private void cheattaskqueue_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.SaveListAssignments();
        }

        public bool CheckSelectedList()
        {
            return (this.currentbuildqueue.SelectedIndex > -1);
        }

        public void CheckUpdate()
        {
        }

        private void Conrepairrange_ValueChanged(object sender, EventArgs e)
        {
        }

        private void currentbuildqueue_SelectedIndexChanged(object sender, EventArgs e)
        {
            string text1 = this.currentbuildqueue.SelectedItem as string;
            this.LoadCurrentQueue(text1);
        }

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
        }

        private void energystorageRule_ValueChanged(object sender, EventArgs e)
        {
        }

        private void ExclusionZone_ValueChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                this.buildtree.ConstructionExclusionRange[text1] = this.ExclusionZone.Value;
            }
        }

        public void FillQuickSetPage()
        {
            string text1 = this.QuicksetCombo.SelectedItem as string;
            ArrayList list1 = new ArrayList();
            ArrayList list2 = this.mod.units;
            switch (text1)
            {
                case "Scouters":
                    foreach (string text2 in list2)
                    {
                        if (this.buildtree.scouters.Contains(text2))
                        {
                            list1.Add(text2);
                        }
                    }
                    goto Label_0496;

                case "Attackers":
                    foreach (string text3 in list2)
                    {
                        if (this.buildtree.attackers.Contains(text3))
                        {
                            list1.Add(text3);
                        }
                    }
                    goto Label_0496;

                case "Kamikaze":
                    foreach (string text4 in list2)
                    {
                        if (this.buildtree.kamikaze.Contains(text4))
                        {
                            list1.Add(text4);
                        }
                    }
                    goto Label_0496;

                case "NeverAntistall":
                    foreach (string text5 in list2)
                    {
                        if (this.buildtree.neverantistall.Contains(text5))
                        {
                            list1.Add(text5);
                        }
                    }
                    goto Label_0496;

                case "Fire at will":
                    foreach (string text6 in list2)
                    {
                        if (this.buildtree.firingstate.ContainsKey(text6))
                        {
                            if (this.buildtree.firingstate[text6] == 2)
                            {
                                list1.Add(text6);
                            }
                        }
                        else
                        {
                            list1.Add(text6);
                        }
                    }
                    goto Label_0496;

                case "Return Fire":
                    foreach (string text7 in list2)
                    {
                        if (this.buildtree.firingstate.ContainsKey(text7) && (this.buildtree.firingstate[text7] == 1))
                        {
                            list1.Add(text7);
                        }
                    }
                    goto Label_0496;

                case "Hold Fire":
                    foreach (string text8 in list2)
                    {
                        if (this.buildtree.firingstate.ContainsKey(text8) && (this.buildtree.firingstate[text8] == 0))
                        {
                            list1.Add(text8);
                        }
                    }
                    goto Label_0496;

                case "Roam":
                    foreach (string text9 in list2)
                    {
                        if (this.buildtree.movement.ContainsKey(text9) && (this.buildtree.movement[text9] == 2))
                        {
                            list1.Add(text9);
                        }
                    }
                    goto Label_0496;

                case "Maneouvre":
                    foreach (string text10 in list2)
                    {
                        if (this.buildtree.movement.ContainsKey(text10) && (this.buildtree.movement[text10] == 1))
                        {
                            list1.Add(text10);
                        }
                    }
                    goto Label_0496;

                case "Hold Position":
                    foreach (string text11 in list2)
                    {
                        if (this.buildtree.movement.ContainsKey(text11) && (this.buildtree.movement[text11] == 0))
                        {
                            list1.Add(text11);
                        }
                    }
                    break;
            }
        Label_0496:
            this.quicksetchecks.Items.Clear();
            foreach (string text12 in list2)
            {
                this.quicksetchecks.Items.Add(text12, list1.Contains(text12));
            }
        }

        private void Firingstatecombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                this.buildtree.firingstate[text1] = this.Firingstatecombo.SelectedIndex;
            }
        }

        private void Firingstatecombo_SelectedValueChanged(object sender, EventArgs e)
        {
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            this.CheckUpdate();
        }

        private string GetSelectedKeyWord()
        {
            string text1 = "";
            if (this.keywords.SelectedIndex == 0)
            {
                text1 = this.universalkeywordsList.SelectedItem as string;
            }
            else
            {
                text1 = this.unitkeywords.SelectedItem as string;
            }
            if (text1 == "")
            {
                return text1;
            }
            return text1.Split(new char[] { ' ' })[0];
        }

        public string GetSelectedUnit()
        {
            string text1 = this.Units.SelectedItem as string;
            if (text1 == null)
            {
                return "";
            }
            text1 = text1.Split(new char[] { ' ' })[0];
            return text1.Trim();
        }

        public string GetWebPage(string url)
        {
            StringBuilder builder1 = new StringBuilder();
            byte[] buffer1 = new byte[0x2000];
            HttpWebRequest request1 = (HttpWebRequest) WebRequest.Create(url);
            Stream stream1 = ((HttpWebResponse) request1.GetResponse()).GetResponseStream();
            string text1 = null;
            int num1 = 0;
            while (true)
            {
                num1 = stream1.Read(buffer1, 0, buffer1.Length);
                if (num1 != 0)
                {
                    text1 = Encoding.ASCII.GetString(buffer1, 0, num1);
                    builder1.Append(text1);
                }
                if (num1 <= 0)
                {
                    return builder1.ToString();
                }
            }
        }

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.Units = new System.Windows.Forms.ListBox();
            this.mainTabs = new System.Windows.Forms.TabControl();
            this.StartTab = new System.Windows.Forms.TabPage();
            this.label67 = new System.Windows.Forms.Label();
            this.linkLabel3 = new System.Windows.Forms.LinkLabel();
            this.label66 = new System.Windows.Forms.Label();
            this.label63 = new System.Windows.Forms.Label();
            this.label62 = new System.Windows.Forms.Label();
            this.linkLabel1 = new System.Windows.Forms.LinkLabel();
            this.label29 = new System.Windows.Forms.Label();
            this.modlabel = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.linkLabel2 = new System.Windows.Forms.LinkLabel();
            this.ll_OpenConfigFile = new System.Windows.Forms.LinkLabel();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.richTextBox1 = new System.Windows.Forms.RichTextBox();
            this.label54 = new System.Windows.Forms.Label();
            this.UnitsTab = new System.Windows.Forms.TabPage();
            this.label61 = new System.Windows.Forms.Label();
            this.label60 = new System.Windows.Forms.Label();
            this.label59 = new System.Windows.Forms.Label();
            this.label58 = new System.Windows.Forms.Label();
            this.label57 = new System.Windows.Forms.Label();
            this.label56 = new System.Windows.Forms.Label();
            this.label55 = new System.Windows.Forms.Label();
            this.label47 = new System.Windows.Forms.Label();
            this.label31 = new System.Windows.Forms.Label();
            this.label45 = new System.Windows.Forms.Label();
            this.label44 = new System.Windows.Forms.Label();
            this.label34 = new System.Windows.Forms.Label();
            this.label33 = new System.Windows.Forms.Label();
            this.label30 = new System.Windows.Forms.Label();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage6 = new System.Windows.Forms.TabPage();
            this.TaskQueues = new System.Windows.Forms.CheckedListBox();
            this.tabPage7 = new System.Windows.Forms.TabPage();
            this.cheattaskqueue = new System.Windows.Forms.CheckedListBox();
            this.label21 = new System.Windows.Forms.Label();
            this.label20 = new System.Windows.Forms.Label();
            this.label19 = new System.Windows.Forms.Label();
            this.label18 = new System.Windows.Forms.Label();
            this.ExclusionZone = new System.Windows.Forms.NumericUpDown();
            this.RepairRange = new System.Windows.Forms.NumericUpDown();
            this.MaxEnergy = new System.Windows.Forms.NumericUpDown();
            this.MinEnergy = new System.Windows.Forms.NumericUpDown();
            this.KamikazeCheck = new System.Windows.Forms.CheckBox();
            this.AttackCheck = new System.Windows.Forms.CheckBox();
            this.SolobuildCheck = new System.Windows.Forms.CheckBox();
            this.SinglebuildCheck = new System.Windows.Forms.CheckBox();
            this.AlwaysantistallCheck = new System.Windows.Forms.CheckBox();
            this.NeverantistallCheck = new System.Windows.Forms.CheckBox();
            this.Firingstatecombo = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.Movestatecombo = new System.Windows.Forms.ComboBox();
            this.modTDFtab = new System.Windows.Forms.TabPage();
            this.groupBox12 = new System.Windows.Forms.GroupBox();
            this.label73 = new System.Windows.Forms.Label();
            this.label69 = new System.Windows.Forms.Label();
            this.maxAttackSize = new System.Windows.Forms.NumericUpDown();
            this.label43 = new System.Windows.Forms.Label();
            this.AttackIncrementPercentage = new System.Windows.Forms.NumericUpDown();
            this.label68 = new System.Windows.Forms.Label();
            this.AttackIncrementValue = new System.Windows.Forms.NumericUpDown();
            this.label28 = new System.Windows.Forms.Label();
            this.initialAttackSize = new System.Windows.Forms.NumericUpDown();
            this.groupBox11 = new System.Windows.Forms.GroupBox();
            this.label38 = new System.Windows.Forms.Label();
            this.label53 = new System.Windows.Forms.Label();
            this.normal_handicap = new System.Windows.Forms.NumericUpDown();
            this.groupBox7 = new System.Windows.Forms.GroupBox();
            this.label71 = new System.Windows.Forms.Label();
            this.label70 = new System.Windows.Forms.Label();
            this.mexnoweaponradius = new System.Windows.Forms.NumericUpDown();
            this.NoEnemyGeo = new System.Windows.Forms.NumericUpDown();
            this.label40 = new System.Windows.Forms.Label();
            this.GeoSearchRadius = new System.Windows.Forms.NumericUpDown();
            this.label39 = new System.Windows.Forms.Label();
            this.label37 = new System.Windows.Forms.Label();
            this.label32 = new System.Windows.Forms.Label();
            this.label16 = new System.Windows.Forms.Label();
            this.DefaultSpacing = new System.Windows.Forms.NumericUpDown();
            this.DefenceSpacing = new System.Windows.Forms.NumericUpDown();
            this.FactorySpacing = new System.Windows.Forms.NumericUpDown();
            this.PowerSpacing = new System.Windows.Forms.NumericUpDown();
            this.label12 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.label14 = new System.Windows.Forms.Label();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.spacemod = new System.Windows.Forms.CheckBox();
            this.Antistall = new System.Windows.Forms.CheckBox();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.label72 = new System.Windows.Forms.Label();
            this.label64 = new System.Windows.Forms.Label();
            this.Author = new System.Windows.Forms.TextBox();
            this.label9 = new System.Windows.Forms.Label();
            this.Message = new System.Windows.Forms.TextBox();
            this.label10 = new System.Windows.Forms.Label();
            this.label11 = new System.Windows.Forms.Label();
            this.Version = new System.Windows.Forms.TextBox();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.groupBox13 = new System.Windows.Forms.GroupBox();
            this.interpolate = new System.Windows.Forms.CheckBox();
            this.MaxAntiStallBox = new System.Windows.Forms.GroupBox();
            this.label41 = new System.Windows.Forms.Label();
            this.StallTimeMobile = new System.Windows.Forms.NumericUpDown();
            this.StallTimeImMobile = new System.Windows.Forms.NumericUpDown();
            this.label15 = new System.Windows.Forms.Label();
            this.label17 = new System.Windows.Forms.Label();
            this.antistallwindow = new System.Windows.Forms.NumericUpDown();
            this.label52 = new System.Windows.Forms.Label();
            this.groupBox9 = new System.Windows.Forms.GroupBox();
            this.groupBox10 = new System.Windows.Forms.GroupBox();
            this.powerRuleEx = new System.Windows.Forms.NumericUpDown();
            this.factorymetalRuleEx = new System.Windows.Forms.NumericUpDown();
            this.factoryenergyRuleEx = new System.Windows.Forms.NumericUpDown();
            this.factorymetalgapRuleEx = new System.Windows.Forms.NumericUpDown();
            this.metalstorageRuleEx = new System.Windows.Forms.NumericUpDown();
            this.energystorageRuleEx = new System.Windows.Forms.NumericUpDown();
            this.mexRuleEx = new System.Windows.Forms.NumericUpDown();
            this.makermetalRuleEx = new System.Windows.Forms.NumericUpDown();
            this.makerenergyRuleEx = new System.Windows.Forms.NumericUpDown();
            this.label42 = new System.Windows.Forms.Label();
            this.groupBox8 = new System.Windows.Forms.GroupBox();
            this.powerRule = new System.Windows.Forms.NumericUpDown();
            this.factorymetalRule = new System.Windows.Forms.NumericUpDown();
            this.metalstorageRule = new System.Windows.Forms.NumericUpDown();
            this.makermetalRule = new System.Windows.Forms.NumericUpDown();
            this.mexRule = new System.Windows.Forms.NumericUpDown();
            this.makerenergyRule = new System.Windows.Forms.NumericUpDown();
            this.energystorageRule = new System.Windows.Forms.NumericUpDown();
            this.factorymetalgapRule = new System.Windows.Forms.NumericUpDown();
            this.factoryenergyRule = new System.Windows.Forms.NumericUpDown();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label26 = new System.Windows.Forms.Label();
            this.label22 = new System.Windows.Forms.Label();
            this.label25 = new System.Windows.Forms.Label();
            this.label23 = new System.Windows.Forms.Label();
            this.label24 = new System.Windows.Forms.Label();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.button8 = new System.Windows.Forms.Button();
            this.InsertWord = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.button6 = new System.Windows.Forms.Button();
            this.button7 = new System.Windows.Forms.Button();
            this.button4 = new System.Windows.Forms.Button();
            this.button5 = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.currentbuildqueue = new System.Windows.Forms.ComboBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.newtasklistbox = new System.Windows.Forms.TextBox();
            this.button11 = new System.Windows.Forms.Button();
            this.btn_CreateNewTaskList = new System.Windows.Forms.Button();
            this.label51 = new System.Windows.Forms.Label();
            this.label50 = new System.Windows.Forms.Label();
            this.label49 = new System.Windows.Forms.Label();
            this.label36 = new System.Windows.Forms.Label();
            this.label35 = new System.Windows.Forms.Label();
            this.currentqueue = new System.Windows.Forms.ListBox();
            this.keywords = new System.Windows.Forms.TabControl();
            this.tabPage8 = new System.Windows.Forms.TabPage();
            this.universalkeywordsList = new System.Windows.Forms.ListBox();
            this.tabPage9 = new System.Windows.Forms.TabPage();
            this.unitkeywords = new System.Windows.Forms.ListBox();
            this.tabPage5 = new System.Windows.Forms.TabPage();
            this.label27 = new System.Windows.Forms.Label();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.toolStripLabel1 = new System.Windows.Forms.ToolStripLabel();
            this.QuicksetCombo = new System.Windows.Forms.ToolStripComboBox();
            this.toolStripButton1 = new System.Windows.Forms.ToolStripButton();
            this.quicksetchecks = new System.Windows.Forms.CheckedListBox();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.label65 = new System.Windows.Forms.Label();
            this.refresh = new System.Windows.Forms.Button();
            this.debugsave = new System.Windows.Forms.RichTextBox();
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.mainTabs.SuspendLayout();
            this.StartTab.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.UnitsTab.SuspendLayout();
            this.tabControl1.SuspendLayout();
            this.tabPage6.SuspendLayout();
            this.tabPage7.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ExclusionZone)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.RepairRange)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.MaxEnergy)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.MinEnergy)).BeginInit();
            this.modTDFtab.SuspendLayout();
            this.groupBox12.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.maxAttackSize)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.AttackIncrementPercentage)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.AttackIncrementValue)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.initialAttackSize)).BeginInit();
            this.groupBox11.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.normal_handicap)).BeginInit();
            this.groupBox7.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mexnoweaponradius)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.NoEnemyGeo)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.GeoSearchRadius)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.DefaultSpacing)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.DefenceSpacing)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.FactorySpacing)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.PowerSpacing)).BeginInit();
            this.groupBox5.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.groupBox13.SuspendLayout();
            this.MaxAntiStallBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.StallTimeMobile)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.StallTimeImMobile)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.antistallwindow)).BeginInit();
            this.groupBox9.SuspendLayout();
            this.groupBox10.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.powerRuleEx)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.factorymetalRuleEx)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.factoryenergyRuleEx)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.factorymetalgapRuleEx)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.metalstorageRuleEx)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.energystorageRuleEx)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.mexRuleEx)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.makermetalRuleEx)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.makerenergyRuleEx)).BeginInit();
            this.groupBox8.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.powerRule)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.factorymetalRule)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.metalstorageRule)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.makermetalRule)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.mexRule)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.makerenergyRule)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.energystorageRule)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.factorymetalgapRule)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.factoryenergyRule)).BeginInit();
            this.tabPage3.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.keywords.SuspendLayout();
            this.tabPage8.SuspendLayout();
            this.tabPage9.SuspendLayout();
            this.tabPage5.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            this.tabPage4.SuspendLayout();
            this.SuspendLayout();
            // 
            // Units
            // 
            this.Units.BackColor = System.Drawing.Color.White;
            this.Units.ForeColor = System.Drawing.Color.Gray;
            this.Units.HorizontalScrollbar = true;
            this.Units.Location = new System.Drawing.Point(6, 42);
            this.Units.Name = "Units";
            this.Units.Size = new System.Drawing.Size(413, 615);
            this.Units.TabIndex = 1;
            this.Units.SelectedIndexChanged += new System.EventHandler(this.Units_SelectedIndexChanged);
            this.Units.SelectedValueChanged += new System.EventHandler(this.Units_SelectedValueChanged);
            // 
            // mainTabs
            // 
            this.mainTabs.Controls.Add(this.StartTab);
            this.mainTabs.Controls.Add(this.tabPage1);
            this.mainTabs.Controls.Add(this.UnitsTab);
            this.mainTabs.Controls.Add(this.modTDFtab);
            this.mainTabs.Controls.Add(this.tabPage2);
            this.mainTabs.Controls.Add(this.tabPage3);
            this.mainTabs.Controls.Add(this.tabPage5);
            this.mainTabs.Controls.Add(this.tabPage4);
            this.mainTabs.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainTabs.Location = new System.Drawing.Point(0, 0);
            this.mainTabs.Multiline = true;
            this.mainTabs.Name = "mainTabs";
            this.mainTabs.SelectedIndex = 0;
            this.mainTabs.Size = new System.Drawing.Size(975, 692);
            this.mainTabs.TabIndex = 2;
            // 
            // StartTab
            // 
            this.StartTab.BackColor = System.Drawing.Color.DarkGray;
            this.StartTab.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.StartTab.Controls.Add(this.label67);
            this.StartTab.Controls.Add(this.linkLabel3);
            this.StartTab.Controls.Add(this.label66);
            this.StartTab.Controls.Add(this.label63);
            this.StartTab.Controls.Add(this.label62);
            this.StartTab.Controls.Add(this.linkLabel1);
            this.StartTab.Controls.Add(this.label29);
            this.StartTab.Controls.Add(this.modlabel);
            this.StartTab.Controls.Add(this.label8);
            this.StartTab.Controls.Add(this.label7);
            this.StartTab.Controls.Add(this.linkLabel2);
            this.StartTab.Controls.Add(this.ll_OpenConfigFile);
            this.StartTab.Location = new System.Drawing.Point(4, 22);
            this.StartTab.Name = "StartTab";
            this.StartTab.Padding = new System.Windows.Forms.Padding(3);
            this.StartTab.Size = new System.Drawing.Size(967, 666);
            this.StartTab.TabIndex = 2;
            this.StartTab.Text = "Welcome";
            this.StartTab.UseVisualStyleBackColor = true;
            this.StartTab.Click += new System.EventHandler(this.StartTab_Click);
            // 
            // label67
            // 
            this.label67.AutoSize = true;
            this.label67.BackColor = System.Drawing.Color.Transparent;
            this.label67.ForeColor = System.Drawing.Color.LightGray;
            this.label67.Location = new System.Drawing.Point(58, 226);
            this.label67.Name = "label67";
            this.label67.Size = new System.Drawing.Size(231, 13);
            this.label67.TabIndex = 15;
            this.label67.Text = "Generate a new config by running NTai for now";
            // 
            // linkLabel3
            // 
            this.linkLabel3.AutoSize = true;
            this.linkLabel3.BackColor = System.Drawing.Color.Transparent;
            this.linkLabel3.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F);
            this.linkLabel3.ForeColor = System.Drawing.Color.LightGray;
            this.linkLabel3.LinkBehavior = System.Windows.Forms.LinkBehavior.HoverUnderline;
            this.linkLabel3.LinkColor = System.Drawing.Color.LightGray;
            this.linkLabel3.Location = new System.Drawing.Point(58, 210);
            this.linkLabel3.Name = "linkLabel3";
            this.linkLabel3.Size = new System.Drawing.Size(76, 16);
            this.linkLabel3.TabIndex = 14;
            this.linkLabel3.TabStop = true;
            this.linkLabel3.Text = "New Config";
            // 
            // label66
            // 
            this.label66.AutoSize = true;
            this.label66.BackColor = System.Drawing.Color.Transparent;
            this.label66.ForeColor = System.Drawing.Color.Black;
            this.label66.Location = new System.Drawing.Point(58, 280);
            this.label66.Name = "label66";
            this.label66.Size = new System.Drawing.Size(180, 13);
            this.label66.TabIndex = 13;
            this.label66.Text = "Open a tdf config generated by NTai";
            // 
            // label63
            // 
            this.label63.AutoSize = true;
            this.label63.BackColor = System.Drawing.Color.Transparent;
            this.label63.ForeColor = System.Drawing.Color.Black;
            this.label63.Location = new System.Drawing.Point(58, 367);
            this.label63.Name = "label63";
            this.label63.Size = new System.Drawing.Size(303, 26);
            this.label63.TabIndex = 12;
            this.label63.Text = "Makes a zip archive in the /AI/NTai/ folder that can be shared\r\nUse this to distr" +
                "ibute/send configs or issue error reports";
            // 
            // label62
            // 
            this.label62.AutoSize = true;
            this.label62.BackColor = System.Drawing.Color.Transparent;
            this.label62.ForeColor = System.Drawing.Color.Black;
            this.label62.Location = new System.Drawing.Point(58, 324);
            this.label62.Name = "label62";
            this.label62.Size = new System.Drawing.Size(279, 13);
            this.label62.TabIndex = 11;
            this.label62.Text = "You must save a config before you can test your changes";
            // 
            // linkLabel1
            // 
            this.linkLabel1.AutoSize = true;
            this.linkLabel1.BackColor = System.Drawing.Color.Transparent;
            this.linkLabel1.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F);
            this.linkLabel1.ForeColor = System.Drawing.Color.Black;
            this.linkLabel1.LinkColor = System.Drawing.Color.Black;
            this.linkLabel1.Location = new System.Drawing.Point(58, 351);
            this.linkLabel1.Name = "linkLabel1";
            this.linkLabel1.Size = new System.Drawing.Size(217, 16);
            this.linkLabel1.TabIndex = 10;
            this.linkLabel1.TabStop = true;
            this.linkLabel1.Text = "Export config zip archive for sharing";
            this.linkLabel1.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel1_LinkClicked_1);
            // 
            // label29
            // 
            this.label29.AutoSize = true;
            this.label29.BackColor = System.Drawing.Color.Transparent;
            this.label29.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label29.ForeColor = System.Drawing.Color.Black;
            this.label29.Location = new System.Drawing.Point(56, 530);
            this.label29.Name = "label29";
            this.label29.Size = new System.Drawing.Size(854, 72);
            this.label29.TabIndex = 8;
            this.label29.Text = resources.GetString("label29.Text");
            // 
            // modlabel
            // 
            this.modlabel.AutoSize = true;
            this.modlabel.BackColor = System.Drawing.Color.Transparent;
            this.modlabel.ForeColor = System.Drawing.Color.Black;
            this.modlabel.Location = new System.Drawing.Point(8, 650);
            this.modlabel.Name = "modlabel";
            this.modlabel.Size = new System.Drawing.Size(572, 13);
            this.modlabel.TabIndex = 7;
            this.modlabel.Text = "Thanks go out to all who helped test and work on toolkit, DJ in particular for sa" +
                "ving the codebase from being lost forever";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.BackColor = System.Drawing.Color.Transparent;
            this.label8.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.ForeColor = System.Drawing.Color.Black;
            this.label8.Location = new System.Drawing.Point(364, 291);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(149, 16);
            this.label8.TabIndex = 6;
            this.label8.Text = "AF Darkstars 2004-2007";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.BackColor = System.Drawing.Color.Transparent;
            this.label7.Font = new System.Drawing.Font("Arial", 18F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(177)));
            this.label7.ForeColor = System.Drawing.Color.Black;
            this.label7.Location = new System.Drawing.Point(362, 264);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(206, 27);
            this.label7.TabIndex = 5;
            this.label7.Text = "NTai Toolkit V0.29";
            this.label7.Click += new System.EventHandler(this.label7_Click);
            // 
            // linkLabel2
            // 
            this.linkLabel2.AutoSize = true;
            this.linkLabel2.BackColor = System.Drawing.Color.Transparent;
            this.linkLabel2.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.linkLabel2.ForeColor = System.Drawing.Color.Black;
            this.linkLabel2.LinkColor = System.Drawing.Color.Black;
            this.linkLabel2.Location = new System.Drawing.Point(57, 308);
            this.linkLabel2.Name = "linkLabel2";
            this.linkLabel2.Size = new System.Drawing.Size(81, 16);
            this.linkLabel2.TabIndex = 2;
            this.linkLabel2.TabStop = true;
            this.linkLabel2.Text = "Save Config";
            this.linkLabel2.MouseClick += new System.Windows.Forms.MouseEventHandler(this.linkLabel2_MouseClick);
            this.linkLabel2.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel2_LinkClicked);
            // 
            // ll_OpenConfigFile
            // 
            this.ll_OpenConfigFile.AutoSize = true;
            this.ll_OpenConfigFile.BackColor = System.Drawing.Color.Transparent;
            this.ll_OpenConfigFile.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ll_OpenConfigFile.ForeColor = System.Drawing.Color.White;
            this.ll_OpenConfigFile.LinkColor = System.Drawing.Color.Black;
            this.ll_OpenConfigFile.Location = new System.Drawing.Point(57, 264);
            this.ll_OpenConfigFile.Name = "ll_OpenConfigFile";
            this.ll_OpenConfigFile.Size = new System.Drawing.Size(82, 16);
            this.ll_OpenConfigFile.TabIndex = 1;
            this.ll_OpenConfigFile.TabStop = true;
            this.ll_OpenConfigFile.Text = "Open Config";
            this.ll_OpenConfigFile.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel1_LinkClicked);
            // 
            // tabPage1
            // 
            this.tabPage1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(45)))), ((int)(((byte)(46)))), ((int)(((byte)(62)))));
            this.tabPage1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.tabPage1.Controls.Add(this.richTextBox1);
            this.tabPage1.Controls.Add(this.label54);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(967, 666);
            this.tabPage1.TabIndex = 8;
            this.tabPage1.Text = "Stuck?: Help & Troubleshooting";
            this.tabPage1.UseVisualStyleBackColor = true;
            this.tabPage1.Click += new System.EventHandler(this.tabPage1_Click);
            // 
            // richTextBox1
            // 
            this.richTextBox1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(45)))), ((int)(((byte)(46)))), ((int)(((byte)(62)))));
            this.richTextBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.richTextBox1.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.richTextBox1.ForeColor = System.Drawing.Color.White;
            this.richTextBox1.Location = new System.Drawing.Point(118, 10);
            this.richTextBox1.Name = "richTextBox1";
            this.richTextBox1.ReadOnly = true;
            this.richTextBox1.Size = new System.Drawing.Size(841, 648);
            this.richTextBox1.TabIndex = 9;
            this.richTextBox1.Text = "";
            // 
            // label54
            // 
            this.label54.AutoSize = true;
            this.label54.BackColor = System.Drawing.Color.Transparent;
            this.label54.Font = new System.Drawing.Font("Microsoft Sans Serif", 20.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label54.ForeColor = System.Drawing.Color.White;
            this.label54.Location = new System.Drawing.Point(8, 10);
            this.label54.Name = "label54";
            this.label54.Size = new System.Drawing.Size(104, 31);
            this.label54.TabIndex = 0;
            this.label54.Text = "Stuck?";
            // 
            // UnitsTab
            // 
            this.UnitsTab.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(251)))), ((int)(((byte)(252)))), ((int)(((byte)(255)))));
            this.UnitsTab.Controls.Add(this.label61);
            this.UnitsTab.Controls.Add(this.label60);
            this.UnitsTab.Controls.Add(this.label59);
            this.UnitsTab.Controls.Add(this.label58);
            this.UnitsTab.Controls.Add(this.label57);
            this.UnitsTab.Controls.Add(this.label56);
            this.UnitsTab.Controls.Add(this.label55);
            this.UnitsTab.Controls.Add(this.label47);
            this.UnitsTab.Controls.Add(this.label31);
            this.UnitsTab.Controls.Add(this.label45);
            this.UnitsTab.Controls.Add(this.label44);
            this.UnitsTab.Controls.Add(this.label34);
            this.UnitsTab.Controls.Add(this.label33);
            this.UnitsTab.Controls.Add(this.label30);
            this.UnitsTab.Controls.Add(this.tabControl1);
            this.UnitsTab.Controls.Add(this.label21);
            this.UnitsTab.Controls.Add(this.label20);
            this.UnitsTab.Controls.Add(this.label19);
            this.UnitsTab.Controls.Add(this.label18);
            this.UnitsTab.Controls.Add(this.ExclusionZone);
            this.UnitsTab.Controls.Add(this.RepairRange);
            this.UnitsTab.Controls.Add(this.MaxEnergy);
            this.UnitsTab.Controls.Add(this.MinEnergy);
            this.UnitsTab.Controls.Add(this.KamikazeCheck);
            this.UnitsTab.Controls.Add(this.Units);
            this.UnitsTab.Controls.Add(this.AttackCheck);
            this.UnitsTab.Controls.Add(this.SolobuildCheck);
            this.UnitsTab.Controls.Add(this.SinglebuildCheck);
            this.UnitsTab.Controls.Add(this.AlwaysantistallCheck);
            this.UnitsTab.Controls.Add(this.NeverantistallCheck);
            this.UnitsTab.Controls.Add(this.Firingstatecombo);
            this.UnitsTab.Controls.Add(this.label2);
            this.UnitsTab.Controls.Add(this.label1);
            this.UnitsTab.Controls.Add(this.Movestatecombo);
            this.UnitsTab.Location = new System.Drawing.Point(4, 22);
            this.UnitsTab.Name = "UnitsTab";
            this.UnitsTab.Padding = new System.Windows.Forms.Padding(3);
            this.UnitsTab.Size = new System.Drawing.Size(967, 666);
            this.UnitsTab.TabIndex = 0;
            this.UnitsTab.Text = "Edit Units and Assign Tasklists";
            this.UnitsTab.UseVisualStyleBackColor = true;
            this.UnitsTab.Enter += new System.EventHandler(this.UnitsTab_Enter);
            this.UnitsTab.Leave += new System.EventHandler(this.UnitsTab_Leave);
            // 
            // label61
            // 
            this.label61.AutoSize = true;
            this.label61.Location = new System.Drawing.Point(434, 358);
            this.label61.Name = "label61";
            this.label61.Size = new System.Drawing.Size(127, 13);
            this.label61.TabIndex = 43;
            this.label61.Text = "type being built too close.";
            // 
            // label60
            // 
            this.label60.AutoSize = true;
            this.label60.Location = new System.Drawing.Point(434, 345);
            this.label60.Name = "label60";
            this.label60.Size = new System.Drawing.Size(225, 13);
            this.label60.TabIndex = 42;
            this.label60.Text = "Exclusion zones prevent two units of the same";
            // 
            // label59
            // 
            this.label59.AutoSize = true;
            this.label59.Location = new System.Drawing.Point(434, 280);
            this.label59.Name = "label59";
            this.label59.Size = new System.Drawing.Size(222, 13);
            this.label59.TabIndex = 41;
            this.label59.Text = "radius rather than starting a new construction.";
            // 
            // label58
            // 
            this.label58.AutoSize = true;
            this.label58.Location = new System.Drawing.Point(434, 267);
            this.label58.Name = "label58";
            this.label58.Size = new System.Drawing.Size(217, 13);
            this.label58.TabIndex = 40;
            this.label58.Text = "Repair unfinished units of this type within this";
            // 
            // label57
            // 
            this.label57.AutoSize = true;
            this.label57.Location = new System.Drawing.Point(434, 181);
            this.label57.Name = "label57";
            this.label57.Size = new System.Drawing.Size(169, 13);
            this.label57.TabIndex = 39;
            this.label57.Text = "of energy needed to build this unit.";
            // 
            // label56
            // 
            this.label56.AutoSize = true;
            this.label56.Location = new System.Drawing.Point(434, 168);
            this.label56.Name = "label56";
            this.label56.Size = new System.Drawing.Size(226, 13);
            this.label56.TabIndex = 38;
            this.label56.Text = "These are the minimum and maximum amounts";
            this.label56.Click += new System.EventHandler(this.label56_Click);
            // 
            // label55
            // 
            this.label55.AutoSize = true;
            this.label55.Location = new System.Drawing.Point(434, 70);
            this.label55.Name = "label55";
            this.label55.Size = new System.Drawing.Size(99, 13);
            this.label55.TabIndex = 37;
            this.label55.Text = "factories or are built";
            // 
            // label47
            // 
            this.label47.AutoSize = true;
            this.label47.Location = new System.Drawing.Point(434, 56);
            this.label47.Name = "label47";
            this.label47.Size = new System.Drawing.Size(217, 13);
            this.label47.TabIndex = 36;
            this.label47.Text = "and movement options of units as they leave";
            this.label47.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // label31
            // 
            this.label31.AutoSize = true;
            this.label31.Location = new System.Drawing.Point(434, 42);
            this.label31.Name = "label31";
            this.label31.Size = new System.Drawing.Size(218, 13);
            this.label31.TabIndex = 35;
            this.label31.Text = "Initial behaviour states, NTai will set the firing";
            // 
            // label45
            // 
            this.label45.AutoSize = true;
            this.label45.Location = new System.Drawing.Point(688, 52);
            this.label45.Name = "label45";
            this.label45.Size = new System.Drawing.Size(208, 13);
            this.label45.TabIndex = 34;
            this.label45.Text = "assign it to the units you want to use it with";
            // 
            // label44
            // 
            this.label44.AutoSize = true;
            this.label44.Location = new System.Drawing.Point(688, 39);
            this.label44.Name = "label44";
            this.label44.Size = new System.Drawing.Size(204, 13);
            this.label44.TabIndex = 33;
            this.label44.Text = "Create a TaskList in the tasklists tab then ";
            // 
            // label34
            // 
            this.label34.AutoSize = true;
            this.label34.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label34.Location = new System.Drawing.Point(687, 7);
            this.label34.Name = "label34";
            this.label34.Size = new System.Drawing.Size(188, 20);
            this.label34.TabIndex = 32;
            this.label34.Text = "Assign This Unit Tasklists";
            // 
            // label33
            // 
            this.label33.AutoSize = true;
            this.label33.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label33.Location = new System.Drawing.Point(433, 7);
            this.label33.Name = "label33";
            this.label33.Size = new System.Drawing.Size(96, 20);
            this.label33.TabIndex = 31;
            this.label33.Text = "Edit Options";
            // 
            // label30
            // 
            this.label30.AutoSize = true;
            this.label30.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label30.Location = new System.Drawing.Point(8, 7);
            this.label30.Name = "label30";
            this.label30.Size = new System.Drawing.Size(195, 20);
            this.label30.TabIndex = 30;
            this.label30.Text = "Select a Unit to Edit below";
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage6);
            this.tabControl1.Controls.Add(this.tabPage7);
            this.tabControl1.Location = new System.Drawing.Point(684, 67);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(275, 590);
            this.tabControl1.TabIndex = 29;
            // 
            // tabPage6
            // 
            this.tabPage6.Controls.Add(this.TaskQueues);
            this.tabPage6.Location = new System.Drawing.Point(4, 22);
            this.tabPage6.Name = "tabPage6";
            this.tabPage6.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage6.Size = new System.Drawing.Size(267, 564);
            this.tabPage6.TabIndex = 0;
            this.tabPage6.Text = "Normal TaskLists";
            this.tabPage6.UseVisualStyleBackColor = true;
            // 
            // TaskQueues
            // 
            this.TaskQueues.BackColor = System.Drawing.SystemColors.Window;
            this.TaskQueues.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.TaskQueues.FormattingEnabled = true;
            this.TaskQueues.Location = new System.Drawing.Point(3, 0);
            this.TaskQueues.Name = "TaskQueues";
            this.TaskQueues.Size = new System.Drawing.Size(258, 525);
            this.TaskQueues.TabIndex = 20;
            this.TaskQueues.ThreeDCheckBoxes = true;
            this.TaskQueues.SelectedIndexChanged += new System.EventHandler(this.TaskQueues_SelectedIndexChanged);
            // 
            // tabPage7
            // 
            this.tabPage7.Controls.Add(this.cheattaskqueue);
            this.tabPage7.Location = new System.Drawing.Point(4, 22);
            this.tabPage7.Name = "tabPage7";
            this.tabPage7.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage7.Size = new System.Drawing.Size(267, 564);
            this.tabPage7.TabIndex = 1;
            this.tabPage7.Text = "Cheat mode TaskLists";
            this.tabPage7.UseVisualStyleBackColor = true;
            // 
            // cheattaskqueue
            // 
            this.cheattaskqueue.FormattingEnabled = true;
            this.cheattaskqueue.Location = new System.Drawing.Point(0, 0);
            this.cheattaskqueue.Name = "cheattaskqueue";
            this.cheattaskqueue.Size = new System.Drawing.Size(264, 559);
            this.cheattaskqueue.TabIndex = 0;
            this.cheattaskqueue.SelectedIndexChanged += new System.EventHandler(this.cheattaskqueue_SelectedIndexChanged);
            // 
            // label21
            // 
            this.label21.AutoSize = true;
            this.label21.Location = new System.Drawing.Point(434, 376);
            this.label21.Name = "label21";
            this.label21.Size = new System.Drawing.Size(109, 13);
            this.label21.TabIndex = 28;
            this.label21.Text = "Exclusion zone radius";
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.Location = new System.Drawing.Point(434, 308);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(68, 13);
            this.label20.TabIndex = 27;
            this.label20.Text = "Repair range";
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.Location = new System.Drawing.Point(434, 227);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(100, 13);
            this.label19.TabIndex = 26;
            this.label19.Text = "Max Energy to build";
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(434, 201);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(97, 13);
            this.label18.TabIndex = 25;
            this.label18.Text = "Min Energy to build";
            // 
            // ExclusionZone
            // 
            this.ExclusionZone.Location = new System.Drawing.Point(554, 374);
            this.ExclusionZone.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.ExclusionZone.Name = "ExclusionZone";
            this.ExclusionZone.Size = new System.Drawing.Size(105, 20);
            this.ExclusionZone.TabIndex = 24;
            this.ExclusionZone.ValueChanged += new System.EventHandler(this.ExclusionZone_ValueChanged);
            // 
            // RepairRange
            // 
            this.RepairRange.Location = new System.Drawing.Point(554, 306);
            this.RepairRange.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.RepairRange.Name = "RepairRange";
            this.RepairRange.Size = new System.Drawing.Size(105, 20);
            this.RepairRange.TabIndex = 23;
            this.RepairRange.ValueChanged += new System.EventHandler(this.RepairRange_ValueChanged);
            // 
            // MaxEnergy
            // 
            this.MaxEnergy.Location = new System.Drawing.Point(554, 225);
            this.MaxEnergy.Maximum = new decimal(new int[] {
            10000000,
            0,
            0,
            0});
            this.MaxEnergy.Name = "MaxEnergy";
            this.MaxEnergy.Size = new System.Drawing.Size(105, 20);
            this.MaxEnergy.TabIndex = 22;
            this.MaxEnergy.ValueChanged += new System.EventHandler(this.MaxEnergy_ValueChanged);
            // 
            // MinEnergy
            // 
            this.MinEnergy.Location = new System.Drawing.Point(554, 199);
            this.MinEnergy.Maximum = new decimal(new int[] {
            10000000,
            0,
            0,
            0});
            this.MinEnergy.Name = "MinEnergy";
            this.MinEnergy.Size = new System.Drawing.Size(105, 20);
            this.MinEnergy.TabIndex = 21;
            this.MinEnergy.ValueChanged += new System.EventHandler(this.MinEnergy_ValueChanged);
            // 
            // KamikazeCheck
            // 
            this.KamikazeCheck.AutoSize = true;
            this.KamikazeCheck.Location = new System.Drawing.Point(437, 641);
            this.KamikazeCheck.Name = "KamikazeCheck";
            this.KamikazeCheck.Size = new System.Drawing.Size(226, 17);
            this.KamikazeCheck.TabIndex = 19;
            this.KamikazeCheck.Text = "Kamikaze -  Self destruct near enemy units";
            this.KamikazeCheck.UseVisualStyleBackColor = true;
            this.KamikazeCheck.CheckedChanged += new System.EventHandler(this.KamikazeCheck_CheckedChanged);
            // 
            // AttackCheck
            // 
            this.AttackCheck.Location = new System.Drawing.Point(437, 444);
            this.AttackCheck.Name = "AttackCheck";
            this.AttackCheck.Size = new System.Drawing.Size(241, 18);
            this.AttackCheck.TabIndex = 0;
            this.AttackCheck.Text = "This unit is an attacker";
            this.AttackCheck.CheckedChanged += new System.EventHandler(this.AttackCheckChanged);
            // 
            // SolobuildCheck
            // 
            this.SolobuildCheck.Location = new System.Drawing.Point(437, 468);
            this.SolobuildCheck.Name = "SolobuildCheck";
            this.SolobuildCheck.Size = new System.Drawing.Size(241, 40);
            this.SolobuildCheck.TabIndex = 2;
            this.SolobuildCheck.Text = "Build as few of these at the same time\r\nas possible";
            this.SolobuildCheck.CheckedChanged += new System.EventHandler(this.SolobuildCheck_CheckedChanged);
            // 
            // SinglebuildCheck
            // 
            this.SinglebuildCheck.Location = new System.Drawing.Point(437, 514);
            this.SinglebuildCheck.Name = "SinglebuildCheck";
            this.SinglebuildCheck.Size = new System.Drawing.Size(241, 29);
            this.SinglebuildCheck.TabIndex = 3;
            this.SinglebuildCheck.Text = "Dont build multiple units of this type if possible";
            this.SinglebuildCheck.CheckedChanged += new System.EventHandler(this.SinglebuildCheck_CheckedChanged);
            // 
            // AlwaysantistallCheck
            // 
            this.AlwaysantistallCheck.Location = new System.Drawing.Point(437, 549);
            this.AlwaysantistallCheck.Name = "AlwaysantistallCheck";
            this.AlwaysantistallCheck.Size = new System.Drawing.Size(241, 32);
            this.AlwaysantistallCheck.TabIndex = 4;
            this.AlwaysantistallCheck.Text = "Always run the antistall algorithm on this if on";
            this.AlwaysantistallCheck.CheckedChanged += new System.EventHandler(this.AlwaysantistallCheck_CheckedChanged);
            // 
            // NeverantistallCheck
            // 
            this.NeverantistallCheck.Location = new System.Drawing.Point(437, 587);
            this.NeverantistallCheck.Name = "NeverantistallCheck";
            this.NeverantistallCheck.Size = new System.Drawing.Size(241, 48);
            this.NeverantistallCheck.TabIndex = 5;
            this.NeverantistallCheck.Text = "This unit is a basic resource unit, never run\r\nthe antistall algorithm on this";
            this.NeverantistallCheck.CheckedChanged += new System.EventHandler(this.NeverantistallCheck_CheckedChanged);
            // 
            // Firingstatecombo
            // 
            this.Firingstatecombo.FormattingEnabled = true;
            this.Firingstatecombo.Items.AddRange(new object[] {
            "Hold Fire",
            "Return Fire",
            "Fire at Will"});
            this.Firingstatecombo.Location = new System.Drawing.Point(525, 95);
            this.Firingstatecombo.Name = "Firingstatecombo";
            this.Firingstatecombo.Size = new System.Drawing.Size(134, 21);
            this.Firingstatecombo.TabIndex = 6;
            this.Firingstatecombo.SelectedIndexChanged += new System.EventHandler(this.Firingstatecombo_SelectedIndexChanged);
            this.Firingstatecombo.SelectedValueChanged += new System.EventHandler(this.Firingstatecombo_SelectedValueChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(434, 125);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(85, 13);
            this.label2.TabIndex = 9;
            this.label2.Text = "Movement State";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(434, 98);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(85, 13);
            this.label1.TabIndex = 7;
            this.label1.Text = "Initial Firing state";
            // 
            // Movestatecombo
            // 
            this.Movestatecombo.FormattingEnabled = true;
            this.Movestatecombo.Items.AddRange(new object[] {
            "Hold Position",
            "Maneouvre",
            "Roam"});
            this.Movestatecombo.Location = new System.Drawing.Point(525, 122);
            this.Movestatecombo.Name = "Movestatecombo";
            this.Movestatecombo.Size = new System.Drawing.Size(134, 21);
            this.Movestatecombo.TabIndex = 8;
            this.Movestatecombo.SelectedIndexChanged += new System.EventHandler(this.Movestatecombo_SelectedIndexChanged);
            this.Movestatecombo.SelectedValueChanged += new System.EventHandler(this.Movestatecombo_SelectedValueChanged);
            // 
            // modTDFtab
            // 
            this.modTDFtab.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(251)))), ((int)(((byte)(252)))), ((int)(((byte)(255)))));
            this.modTDFtab.Controls.Add(this.groupBox12);
            this.modTDFtab.Controls.Add(this.groupBox11);
            this.modTDFtab.Controls.Add(this.groupBox7);
            this.modTDFtab.Controls.Add(this.groupBox5);
            this.modTDFtab.Controls.Add(this.groupBox4);
            this.modTDFtab.Location = new System.Drawing.Point(4, 22);
            this.modTDFtab.Name = "modTDFtab";
            this.modTDFtab.Padding = new System.Windows.Forms.Padding(3);
            this.modTDFtab.Size = new System.Drawing.Size(967, 666);
            this.modTDFtab.TabIndex = 1;
            this.modTDFtab.Text = "Global Config Options";
            this.modTDFtab.UseVisualStyleBackColor = true;
            this.modTDFtab.Click += new System.EventHandler(this.modTDFtab_Click);
            // 
            // groupBox12
            // 
            this.groupBox12.Controls.Add(this.label73);
            this.groupBox12.Controls.Add(this.label69);
            this.groupBox12.Controls.Add(this.maxAttackSize);
            this.groupBox12.Controls.Add(this.label43);
            this.groupBox12.Controls.Add(this.AttackIncrementPercentage);
            this.groupBox12.Controls.Add(this.label68);
            this.groupBox12.Controls.Add(this.AttackIncrementValue);
            this.groupBox12.Controls.Add(this.label28);
            this.groupBox12.Controls.Add(this.initialAttackSize);
            this.groupBox12.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F);
            this.groupBox12.ForeColor = System.Drawing.Color.Black;
            this.groupBox12.Location = new System.Drawing.Point(406, 18);
            this.groupBox12.Name = "groupBox12";
            this.groupBox12.Size = new System.Drawing.Size(553, 194);
            this.groupBox12.TabIndex = 73;
            this.groupBox12.TabStop = false;
            this.groupBox12.Text = "Attack Groups";
            // 
            // label73
            // 
            this.label73.AutoSize = true;
            this.label73.Location = new System.Drawing.Point(8, 97);
            this.label73.Name = "label73";
            this.label73.Size = new System.Drawing.Size(143, 20);
            this.label73.TabIndex = 9;
            this.label73.Text = "When NTai attacks";
            // 
            // label69
            // 
            this.label69.AutoSize = true;
            this.label69.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label69.Location = new System.Drawing.Point(9, 54);
            this.label69.Name = "label69";
            this.label69.Size = new System.Drawing.Size(140, 13);
            this.label69.TabIndex = 8;
            this.label69.Text = "Maximum Attack Group Size";
            // 
            // maxAttackSize
            // 
            this.maxAttackSize.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.maxAttackSize.Location = new System.Drawing.Point(293, 52);
            this.maxAttackSize.Name = "maxAttackSize";
            this.maxAttackSize.Size = new System.Drawing.Size(95, 20);
            this.maxAttackSize.TabIndex = 7;
            // 
            // label43
            // 
            this.label43.AutoSize = true;
            this.label43.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label43.Location = new System.Drawing.Point(9, 152);
            this.label43.Name = "label43";
            this.label43.Size = new System.Drawing.Size(231, 13);
            this.label43.TabIndex = 6;
            this.label43.Text = "Increase Attack group size by this percentage%";
            // 
            // AttackIncrementPercentage
            // 
            this.AttackIncrementPercentage.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.AttackIncrementPercentage.Location = new System.Drawing.Point(293, 150);
            this.AttackIncrementPercentage.Name = "AttackIncrementPercentage";
            this.AttackIncrementPercentage.Size = new System.Drawing.Size(95, 20);
            this.AttackIncrementPercentage.TabIndex = 5;
            // 
            // label68
            // 
            this.label68.AutoSize = true;
            this.label68.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label68.Location = new System.Drawing.Point(9, 126);
            this.label68.Name = "label68";
            this.label68.Size = new System.Drawing.Size(147, 13);
            this.label68.TabIndex = 3;
            this.label68.Text = "Increase Attack group size by";
            // 
            // AttackIncrementValue
            // 
            this.AttackIncrementValue.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.AttackIncrementValue.Location = new System.Drawing.Point(293, 124);
            this.AttackIncrementValue.Maximum = new decimal(new int[] {
            1000,
            0,
            0,
            0});
            this.AttackIncrementValue.Minimum = new decimal(new int[] {
            1000,
            0,
            0,
            -2147483648});
            this.AttackIncrementValue.Name = "AttackIncrementValue";
            this.AttackIncrementValue.Size = new System.Drawing.Size(95, 20);
            this.AttackIncrementValue.TabIndex = 2;
            // 
            // label28
            // 
            this.label28.AutoSize = true;
            this.label28.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label28.Location = new System.Drawing.Point(9, 27);
            this.label28.Name = "label28";
            this.label28.Size = new System.Drawing.Size(195, 13);
            this.label28.TabIndex = 1;
            this.label28.Text = "Initial Attack Group Size (1 for fps mods)";
            // 
            // initialAttackSize
            // 
            this.initialAttackSize.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.initialAttackSize.Location = new System.Drawing.Point(293, 25);
            this.initialAttackSize.Maximum = new decimal(new int[] {
            1000,
            0,
            0,
            0});
            this.initialAttackSize.Name = "initialAttackSize";
            this.initialAttackSize.Size = new System.Drawing.Size(95, 20);
            this.initialAttackSize.TabIndex = 0;
            // 
            // groupBox11
            // 
            this.groupBox11.Controls.Add(this.label38);
            this.groupBox11.Controls.Add(this.label53);
            this.groupBox11.Controls.Add(this.normal_handicap);
            this.groupBox11.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox11.ForeColor = System.Drawing.Color.Black;
            this.groupBox11.Location = new System.Drawing.Point(406, 220);
            this.groupBox11.Name = "groupBox11";
            this.groupBox11.Size = new System.Drawing.Size(553, 75);
            this.groupBox11.TabIndex = 72;
            this.groupBox11.TabStop = false;
            this.groupBox11.Text = "Cheating";
            // 
            // label38
            // 
            this.label38.AutoSize = true;
            this.label38.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label38.Location = new System.Drawing.Point(11, 22);
            this.label38.Name = "label38";
            this.label38.Size = new System.Drawing.Size(173, 13);
            this.label38.TabIndex = 73;
            this.label38.Text = "30 handicap = 30% more resources";
            // 
            // label53
            // 
            this.label53.AutoSize = true;
            this.label53.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label53.Location = new System.Drawing.Point(12, 44);
            this.label53.Name = "label53";
            this.label53.Size = new System.Drawing.Size(157, 13);
            this.label53.TabIndex = 62;
            this.label53.Text = "Initial handicap (recommend 50)";
            // 
            // normal_handicap
            // 
            this.normal_handicap.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.normal_handicap.Location = new System.Drawing.Point(293, 42);
            this.normal_handicap.Name = "normal_handicap";
            this.normal_handicap.Size = new System.Drawing.Size(80, 20);
            this.normal_handicap.TabIndex = 63;
            // 
            // groupBox7
            // 
            this.groupBox7.Controls.Add(this.label71);
            this.groupBox7.Controls.Add(this.label70);
            this.groupBox7.Controls.Add(this.mexnoweaponradius);
            this.groupBox7.Controls.Add(this.NoEnemyGeo);
            this.groupBox7.Controls.Add(this.label40);
            this.groupBox7.Controls.Add(this.GeoSearchRadius);
            this.groupBox7.Controls.Add(this.label39);
            this.groupBox7.Controls.Add(this.label37);
            this.groupBox7.Controls.Add(this.label32);
            this.groupBox7.Controls.Add(this.label16);
            this.groupBox7.Controls.Add(this.DefaultSpacing);
            this.groupBox7.Controls.Add(this.DefenceSpacing);
            this.groupBox7.Controls.Add(this.FactorySpacing);
            this.groupBox7.Controls.Add(this.PowerSpacing);
            this.groupBox7.Controls.Add(this.label12);
            this.groupBox7.Controls.Add(this.label13);
            this.groupBox7.Controls.Add(this.label14);
            this.groupBox7.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox7.ForeColor = System.Drawing.Color.Black;
            this.groupBox7.Location = new System.Drawing.Point(16, 220);
            this.groupBox7.Name = "groupBox7";
            this.groupBox7.Size = new System.Drawing.Size(384, 356);
            this.groupBox7.TabIndex = 69;
            this.groupBox7.TabStop = false;
            this.groupBox7.Text = "Building Placement Spacing";
            // 
            // label71
            // 
            this.label71.AutoSize = true;
            this.label71.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label71.Location = new System.Drawing.Point(10, 210);
            this.label71.Name = "label71";
            this.label71.Size = new System.Drawing.Size(279, 13);
            this.label71.TabIndex = 80;
            this.label71.Text = "These values have the same scale as the co-ords ingame";
            // 
            // label70
            // 
            this.label70.AutoSize = true;
            this.label70.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label70.Location = new System.Drawing.Point(12, 297);
            this.label70.Name = "label70";
            this.label70.Size = new System.Drawing.Size(262, 13);
            this.label70.TabIndex = 79;
            this.label70.Text = "Dont build a mex closer than this to an enemy weapon";
            // 
            // mexnoweaponradius
            // 
            this.mexnoweaponradius.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.mexnoweaponradius.Location = new System.Drawing.Point(293, 295);
            this.mexnoweaponradius.Maximum = new decimal(new int[] {
            20000000,
            0,
            0,
            0});
            this.mexnoweaponradius.Name = "mexnoweaponradius";
            this.mexnoweaponradius.Size = new System.Drawing.Size(80, 20);
            this.mexnoweaponradius.TabIndex = 78;
            // 
            // NoEnemyGeo
            // 
            this.NoEnemyGeo.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.NoEnemyGeo.Location = new System.Drawing.Point(293, 269);
            this.NoEnemyGeo.Maximum = new decimal(new int[] {
            1000000,
            0,
            0,
            0});
            this.NoEnemyGeo.Name = "NoEnemyGeo";
            this.NoEnemyGeo.Size = new System.Drawing.Size(80, 20);
            this.NoEnemyGeo.TabIndex = 77;
            // 
            // label40
            // 
            this.label40.AutoSize = true;
            this.label40.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label40.Location = new System.Drawing.Point(12, 271);
            this.label40.Name = "label40";
            this.label40.Size = new System.Drawing.Size(180, 13);
            this.label40.TabIndex = 76;
            this.label40.Text = "No Enemies within geothermal radius";
            // 
            // GeoSearchRadius
            // 
            this.GeoSearchRadius.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.GeoSearchRadius.Location = new System.Drawing.Point(293, 243);
            this.GeoSearchRadius.Maximum = new decimal(new int[] {
            1000000,
            0,
            0,
            0});
            this.GeoSearchRadius.Name = "GeoSearchRadius";
            this.GeoSearchRadius.Size = new System.Drawing.Size(80, 20);
            this.GeoSearchRadius.TabIndex = 75;
            // 
            // label39
            // 
            this.label39.AutoSize = true;
            this.label39.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label39.Location = new System.Drawing.Point(12, 245);
            this.label39.Name = "label39";
            this.label39.Size = new System.Drawing.Size(129, 13);
            this.label39.TabIndex = 74;
            this.label39.Text = "Geothermal Search radius";
            // 
            // label37
            // 
            this.label37.AutoSize = true;
            this.label37.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label37.Location = new System.Drawing.Point(10, 41);
            this.label37.Name = "label37";
            this.label37.Size = new System.Drawing.Size(301, 13);
            this.label37.TabIndex = 73;
            this.label37.Text = "This value is in increments of 8, just like unit footprints in spring";
            // 
            // label32
            // 
            this.label32.AutoSize = true;
            this.label32.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label32.Location = new System.Drawing.Point(10, 28);
            this.label32.Name = "label32";
            this.label32.Size = new System.Drawing.Size(277, 13);
            this.label32.TabIndex = 73;
            this.label32.Text = "How much free space around a building should there be?";
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label16.Location = new System.Drawing.Point(11, 79);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(262, 13);
            this.label16.TabIndex = 15;
            this.label16.Text = "Default Spacing (doesn\'t affect mexes or radar towers)";
            // 
            // DefaultSpacing
            // 
            this.DefaultSpacing.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.DefaultSpacing.Location = new System.Drawing.Point(293, 77);
            this.DefaultSpacing.Name = "DefaultSpacing";
            this.DefaultSpacing.Size = new System.Drawing.Size(80, 20);
            this.DefaultSpacing.TabIndex = 6;
            // 
            // DefenceSpacing
            // 
            this.DefenceSpacing.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.DefenceSpacing.Location = new System.Drawing.Point(293, 155);
            this.DefenceSpacing.Name = "DefenceSpacing";
            this.DefenceSpacing.Size = new System.Drawing.Size(80, 20);
            this.DefenceSpacing.TabIndex = 7;
            // 
            // FactorySpacing
            // 
            this.FactorySpacing.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FactorySpacing.Location = new System.Drawing.Point(293, 129);
            this.FactorySpacing.Name = "FactorySpacing";
            this.FactorySpacing.Size = new System.Drawing.Size(80, 20);
            this.FactorySpacing.TabIndex = 8;
            // 
            // PowerSpacing
            // 
            this.PowerSpacing.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.PowerSpacing.Location = new System.Drawing.Point(293, 103);
            this.PowerSpacing.Name = "PowerSpacing";
            this.PowerSpacing.Size = new System.Drawing.Size(80, 20);
            this.PowerSpacing.TabIndex = 9;
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label12.Location = new System.Drawing.Point(11, 155);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(90, 13);
            this.label12.TabIndex = 11;
            this.label12.Text = "Defence Spacing";
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label13.Location = new System.Drawing.Point(11, 105);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(117, 13);
            this.label13.TabIndex = 12;
            this.label13.Text = "Power/Energy Spacing";
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label14.Location = new System.Drawing.Point(11, 129);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(109, 13);
            this.label14.TabIndex = 13;
            this.label14.Text = "Factory/Hub Spacing";
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.spacemod);
            this.groupBox5.Controls.Add(this.Antistall);
            this.groupBox5.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox5.ForeColor = System.Drawing.Color.Black;
            this.groupBox5.Location = new System.Drawing.Point(406, 301);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(553, 80);
            this.groupBox5.TabIndex = 67;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Mod Options";
            // 
            // spacemod
            // 
            this.spacemod.AutoSize = true;
            this.spacemod.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.spacemod.Location = new System.Drawing.Point(12, 25);
            this.spacemod.Name = "spacemod";
            this.spacemod.Size = new System.Drawing.Size(254, 17);
            this.spacemod.TabIndex = 17;
            this.spacemod.Text = "This mod is in Space (all mobile units are aircraft)";
            this.spacemod.UseVisualStyleBackColor = true;
            // 
            // Antistall
            // 
            this.Antistall.AutoSize = true;
            this.Antistall.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Antistall.Location = new System.Drawing.Point(12, 48);
            this.Antistall.Name = "Antistall";
            this.Antistall.Size = new System.Drawing.Size(467, 17);
            this.Antistall.TabIndex = 19;
            this.Antistall.Text = "Use Antistall algorithm (wont build if it\'ll cause nanostall/zero stored resource" +
                "s)(recommend off)";
            this.Antistall.UseVisualStyleBackColor = true;
            this.Antistall.CheckedChanged += new System.EventHandler(this.Antistall_CheckedChanged);
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.label72);
            this.groupBox4.Controls.Add(this.label64);
            this.groupBox4.Controls.Add(this.Author);
            this.groupBox4.Controls.Add(this.label9);
            this.groupBox4.Controls.Add(this.Message);
            this.groupBox4.Controls.Add(this.label10);
            this.groupBox4.Controls.Add(this.label11);
            this.groupBox4.Controls.Add(this.Version);
            this.groupBox4.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox4.ForeColor = System.Drawing.Color.Black;
            this.groupBox4.Location = new System.Drawing.Point(16, 8);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(384, 204);
            this.groupBox4.TabIndex = 66;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Version + Credits";
            // 
            // label72
            // 
            this.label72.AutoSize = true;
            this.label72.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label72.Location = new System.Drawing.Point(5, 136);
            this.label72.Name = "label72";
            this.label72.Size = new System.Drawing.Size(339, 13);
            this.label72.TabIndex = 66;
            this.label72.Text = "When NTai starts it will display the message above in the chat console";
            // 
            // label64
            // 
            this.label64.AutoSize = true;
            this.label64.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label64.Location = new System.Drawing.Point(6, 22);
            this.label64.Name = "label64";
            this.label64.Size = new System.Drawing.Size(358, 13);
            this.label64.TabIndex = 65;
            this.label64.Text = "The Message item is displayed by NTai everytime the user starts the game.";
            // 
            // Author
            // 
            this.Author.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Author.Location = new System.Drawing.Point(63, 42);
            this.Author.Name = "Author";
            this.Author.Size = new System.Drawing.Size(310, 20);
            this.Author.TabIndex = 0;
            this.Author.TextChanged += new System.EventHandler(this.Author_TextChanged);
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label9.Location = new System.Drawing.Point(5, 45);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(38, 13);
            this.label9.TabIndex = 2;
            this.label9.Text = "Author";
            // 
            // Message
            // 
            this.Message.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Message.Location = new System.Drawing.Point(63, 107);
            this.Message.Name = "Message";
            this.Message.Size = new System.Drawing.Size(310, 20);
            this.Message.TabIndex = 1;
            this.Message.TextChanged += new System.EventHandler(this.Message_TextChanged);
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label10.Location = new System.Drawing.Point(5, 110);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(50, 13);
            this.label10.TabIndex = 3;
            this.label10.Text = "Message";
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label11.Location = new System.Drawing.Point(6, 71);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(42, 13);
            this.label11.TabIndex = 4;
            this.label11.Text = "Version";
            // 
            // Version
            // 
            this.Version.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Version.Location = new System.Drawing.Point(63, 68);
            this.Version.Name = "Version";
            this.Version.Size = new System.Drawing.Size(310, 20);
            this.Version.TabIndex = 5;
            this.Version.TextChanged += new System.EventHandler(this.Version_TextChanged);
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.groupBox13);
            this.tabPage2.Controls.Add(this.MaxAntiStallBox);
            this.tabPage2.Controls.Add(this.groupBox9);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(967, 666);
            this.tabPage2.TabIndex = 10;
            this.tabPage2.Text = "Adv Config Options";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // groupBox13
            // 
            this.groupBox13.Controls.Add(this.interpolate);
            this.groupBox13.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox13.ForeColor = System.Drawing.Color.Black;
            this.groupBox13.Location = new System.Drawing.Point(8, 334);
            this.groupBox13.Name = "groupBox13";
            this.groupBox13.Size = new System.Drawing.Size(553, 74);
            this.groupBox13.TabIndex = 75;
            this.groupBox13.TabStop = false;
            this.groupBox13.Text = "Task List Options";
            // 
            // interpolate
            // 
            this.interpolate.AutoSize = true;
            this.interpolate.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.interpolate.Location = new System.Drawing.Point(13, 33);
            this.interpolate.Name = "interpolate";
            this.interpolate.Size = new System.Drawing.Size(281, 17);
            this.interpolate.TabIndex = 0;
            this.interpolate.Text = "Insert b_rule_extreme_nofact between every task item";
            this.interpolate.UseVisualStyleBackColor = true;
            // 
            // MaxAntiStallBox
            // 
            this.MaxAntiStallBox.Controls.Add(this.label41);
            this.MaxAntiStallBox.Controls.Add(this.StallTimeMobile);
            this.MaxAntiStallBox.Controls.Add(this.StallTimeImMobile);
            this.MaxAntiStallBox.Controls.Add(this.label15);
            this.MaxAntiStallBox.Controls.Add(this.label17);
            this.MaxAntiStallBox.Controls.Add(this.antistallwindow);
            this.MaxAntiStallBox.Controls.Add(this.label52);
            this.MaxAntiStallBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MaxAntiStallBox.ForeColor = System.Drawing.Color.Black;
            this.MaxAntiStallBox.Location = new System.Drawing.Point(8, 414);
            this.MaxAntiStallBox.Name = "MaxAntiStallBox";
            this.MaxAntiStallBox.Size = new System.Drawing.Size(553, 186);
            this.MaxAntiStallBox.TabIndex = 73;
            this.MaxAntiStallBox.TabStop = false;
            this.MaxAntiStallBox.Text = "Max Allowed NanoStall (secs)";
            // 
            // label41
            // 
            this.label41.AutoSize = true;
            this.label41.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
            this.label41.Location = new System.Drawing.Point(11, 22);
            this.label41.Name = "label41";
            this.label41.Size = new System.Drawing.Size(364, 26);
            this.label41.TabIndex = 62;
            this.label41.Text = "Stalling is when you have no more resources, these values dictate how long\r\nthe a" +
                "ntistaller algorithm can stall for. (you can use negative values too)";
            // 
            // StallTimeMobile
            // 
            this.StallTimeMobile.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.StallTimeMobile.Location = new System.Drawing.Point(295, 80);
            this.StallTimeMobile.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.StallTimeMobile.Minimum = new decimal(new int[] {
            10000,
            0,
            0,
            -2147483648});
            this.StallTimeMobile.Name = "StallTimeMobile";
            this.StallTimeMobile.Size = new System.Drawing.Size(80, 20);
            this.StallTimeMobile.TabIndex = 20;
            // 
            // StallTimeImMobile
            // 
            this.StallTimeImMobile.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.StallTimeImMobile.Location = new System.Drawing.Point(295, 106);
            this.StallTimeImMobile.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.StallTimeImMobile.Minimum = new decimal(new int[] {
            10000,
            0,
            0,
            -2147483648});
            this.StallTimeImMobile.Name = "StallTimeImMobile";
            this.StallTimeImMobile.Size = new System.Drawing.Size(80, 20);
            this.StallTimeImMobile.TabIndex = 21;
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label15.Location = new System.Drawing.Point(12, 80);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(183, 13);
            this.label15.TabIndex = 22;
            this.label15.Text = "Max allowed stall time for Mobile units";
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label17.Location = new System.Drawing.Point(12, 106);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(177, 13);
            this.label17.TabIndex = 23;
            this.label17.Text = "Max allowed stall time for static units";
            // 
            // antistallwindow
            // 
            this.antistallwindow.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.antistallwindow.Location = new System.Drawing.Point(295, 132);
            this.antistallwindow.Name = "antistallwindow";
            this.antistallwindow.Size = new System.Drawing.Size(80, 20);
            this.antistallwindow.TabIndex = 60;
            // 
            // label52
            // 
            this.label52.AutoSize = true;
            this.label52.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label52.Location = new System.Drawing.Point(12, 132);
            this.label52.Name = "label52";
            this.label52.Size = new System.Drawing.Size(157, 13);
            this.label52.TabIndex = 61;
            this.label52.Text = "Ignore Antistaller At Startup time";
            // 
            // groupBox9
            // 
            this.groupBox9.Controls.Add(this.groupBox10);
            this.groupBox9.Controls.Add(this.label42);
            this.groupBox9.Controls.Add(this.groupBox8);
            this.groupBox9.Controls.Add(this.label3);
            this.groupBox9.Controls.Add(this.label4);
            this.groupBox9.Controls.Add(this.label5);
            this.groupBox9.Controls.Add(this.label6);
            this.groupBox9.Controls.Add(this.label26);
            this.groupBox9.Controls.Add(this.label22);
            this.groupBox9.Controls.Add(this.label25);
            this.groupBox9.Controls.Add(this.label23);
            this.groupBox9.Controls.Add(this.label24);
            this.groupBox9.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox9.ForeColor = System.Drawing.Color.Black;
            this.groupBox9.Location = new System.Drawing.Point(8, 6);
            this.groupBox9.Name = "groupBox9";
            this.groupBox9.Size = new System.Drawing.Size(553, 312);
            this.groupBox9.TabIndex = 72;
            this.groupBox9.TabStop = false;
            this.groupBox9.Text = "Building Rule Modifiers";
            // 
            // groupBox10
            // 
            this.groupBox10.Controls.Add(this.powerRuleEx);
            this.groupBox10.Controls.Add(this.factorymetalRuleEx);
            this.groupBox10.Controls.Add(this.factoryenergyRuleEx);
            this.groupBox10.Controls.Add(this.factorymetalgapRuleEx);
            this.groupBox10.Controls.Add(this.metalstorageRuleEx);
            this.groupBox10.Controls.Add(this.energystorageRuleEx);
            this.groupBox10.Controls.Add(this.mexRuleEx);
            this.groupBox10.Controls.Add(this.makermetalRuleEx);
            this.groupBox10.Controls.Add(this.makerenergyRuleEx);
            this.groupBox10.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox10.ForeColor = System.Drawing.Color.Black;
            this.groupBox10.Location = new System.Drawing.Point(232, 25);
            this.groupBox10.Name = "groupBox10";
            this.groupBox10.Size = new System.Drawing.Size(110, 267);
            this.groupBox10.TabIndex = 72;
            this.groupBox10.TabStop = false;
            this.groupBox10.Text = "Extreme";
            // 
            // powerRuleEx
            // 
            this.powerRuleEx.DecimalPlaces = 3;
            this.powerRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.powerRuleEx.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.powerRuleEx.Location = new System.Drawing.Point(6, 32);
            this.powerRuleEx.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.powerRuleEx.Name = "powerRuleEx";
            this.powerRuleEx.Size = new System.Drawing.Size(95, 20);
            this.powerRuleEx.TabIndex = 30;
            // 
            // factorymetalRuleEx
            // 
            this.factorymetalRuleEx.DecimalPlaces = 3;
            this.factorymetalRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.factorymetalRuleEx.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.factorymetalRuleEx.Location = new System.Drawing.Point(6, 84);
            this.factorymetalRuleEx.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.factorymetalRuleEx.Name = "factorymetalRuleEx";
            this.factorymetalRuleEx.Size = new System.Drawing.Size(95, 20);
            this.factorymetalRuleEx.TabIndex = 24;
            // 
            // factoryenergyRuleEx
            // 
            this.factoryenergyRuleEx.DecimalPlaces = 3;
            this.factoryenergyRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.factoryenergyRuleEx.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.factoryenergyRuleEx.Location = new System.Drawing.Point(6, 110);
            this.factoryenergyRuleEx.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.factoryenergyRuleEx.Name = "factoryenergyRuleEx";
            this.factoryenergyRuleEx.Size = new System.Drawing.Size(95, 20);
            this.factoryenergyRuleEx.TabIndex = 25;
            // 
            // factorymetalgapRuleEx
            // 
            this.factorymetalgapRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.factorymetalgapRuleEx.Location = new System.Drawing.Point(6, 136);
            this.factorymetalgapRuleEx.Maximum = new decimal(new int[] {
            30000,
            0,
            0,
            0});
            this.factorymetalgapRuleEx.Name = "factorymetalgapRuleEx";
            this.factorymetalgapRuleEx.Size = new System.Drawing.Size(95, 20);
            this.factorymetalgapRuleEx.TabIndex = 26;
            // 
            // metalstorageRuleEx
            // 
            this.metalstorageRuleEx.DecimalPlaces = 3;
            this.metalstorageRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metalstorageRuleEx.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.metalstorageRuleEx.Location = new System.Drawing.Point(6, 188);
            this.metalstorageRuleEx.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.metalstorageRuleEx.Name = "metalstorageRuleEx";
            this.metalstorageRuleEx.Size = new System.Drawing.Size(95, 20);
            this.metalstorageRuleEx.TabIndex = 27;
            // 
            // energystorageRuleEx
            // 
            this.energystorageRuleEx.DecimalPlaces = 3;
            this.energystorageRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.energystorageRuleEx.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.energystorageRuleEx.Location = new System.Drawing.Point(6, 162);
            this.energystorageRuleEx.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.energystorageRuleEx.Name = "energystorageRuleEx";
            this.energystorageRuleEx.Size = new System.Drawing.Size(95, 20);
            this.energystorageRuleEx.TabIndex = 28;
            // 
            // mexRuleEx
            // 
            this.mexRuleEx.DecimalPlaces = 3;
            this.mexRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.mexRuleEx.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.mexRuleEx.Location = new System.Drawing.Point(6, 58);
            this.mexRuleEx.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.mexRuleEx.Name = "mexRuleEx";
            this.mexRuleEx.Size = new System.Drawing.Size(95, 20);
            this.mexRuleEx.TabIndex = 29;
            // 
            // makermetalRuleEx
            // 
            this.makermetalRuleEx.DecimalPlaces = 3;
            this.makermetalRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.makermetalRuleEx.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.makermetalRuleEx.Location = new System.Drawing.Point(6, 214);
            this.makermetalRuleEx.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.makermetalRuleEx.Name = "makermetalRuleEx";
            this.makermetalRuleEx.Size = new System.Drawing.Size(95, 20);
            this.makermetalRuleEx.TabIndex = 31;
            // 
            // makerenergyRuleEx
            // 
            this.makerenergyRuleEx.DecimalPlaces = 3;
            this.makerenergyRuleEx.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.makerenergyRuleEx.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.makerenergyRuleEx.Location = new System.Drawing.Point(6, 240);
            this.makerenergyRuleEx.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.makerenergyRuleEx.Name = "makerenergyRuleEx";
            this.makerenergyRuleEx.Size = new System.Drawing.Size(95, 20);
            this.makerenergyRuleEx.TabIndex = 41;
            // 
            // label42
            // 
            this.label42.AutoSize = true;
            this.label42.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label42.Location = new System.Drawing.Point(351, 22);
            this.label42.Name = "label42";
            this.label42.Size = new System.Drawing.Size(194, 117);
            this.label42.TabIndex = 56;
            this.label42.Text = resources.GetString("label42.Text");
            // 
            // groupBox8
            // 
            this.groupBox8.Controls.Add(this.powerRule);
            this.groupBox8.Controls.Add(this.factorymetalRule);
            this.groupBox8.Controls.Add(this.metalstorageRule);
            this.groupBox8.Controls.Add(this.makermetalRule);
            this.groupBox8.Controls.Add(this.mexRule);
            this.groupBox8.Controls.Add(this.makerenergyRule);
            this.groupBox8.Controls.Add(this.energystorageRule);
            this.groupBox8.Controls.Add(this.factorymetalgapRule);
            this.groupBox8.Controls.Add(this.factoryenergyRule);
            this.groupBox8.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox8.ForeColor = System.Drawing.Color.Black;
            this.groupBox8.Location = new System.Drawing.Point(112, 25);
            this.groupBox8.Name = "groupBox8";
            this.groupBox8.Size = new System.Drawing.Size(114, 267);
            this.groupBox8.TabIndex = 70;
            this.groupBox8.TabStop = false;
            this.groupBox8.Text = "Normal";
            // 
            // powerRule
            // 
            this.powerRule.DecimalPlaces = 3;
            this.powerRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.powerRule.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.powerRule.Location = new System.Drawing.Point(6, 32);
            this.powerRule.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.powerRule.Name = "powerRule";
            this.powerRule.Size = new System.Drawing.Size(95, 20);
            this.powerRule.TabIndex = 35;
            // 
            // factorymetalRule
            // 
            this.factorymetalRule.DecimalPlaces = 3;
            this.factorymetalRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.factorymetalRule.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.factorymetalRule.Location = new System.Drawing.Point(6, 84);
            this.factorymetalRule.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.factorymetalRule.Name = "factorymetalRule";
            this.factorymetalRule.Size = new System.Drawing.Size(95, 20);
            this.factorymetalRule.TabIndex = 32;
            // 
            // metalstorageRule
            // 
            this.metalstorageRule.DecimalPlaces = 3;
            this.metalstorageRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.metalstorageRule.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.metalstorageRule.Location = new System.Drawing.Point(6, 188);
            this.metalstorageRule.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.metalstorageRule.Name = "metalstorageRule";
            this.metalstorageRule.Size = new System.Drawing.Size(95, 20);
            this.metalstorageRule.TabIndex = 33;
            // 
            // makermetalRule
            // 
            this.makermetalRule.DecimalPlaces = 3;
            this.makermetalRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.makermetalRule.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.makermetalRule.Location = new System.Drawing.Point(6, 214);
            this.makermetalRule.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.makermetalRule.Name = "makermetalRule";
            this.makermetalRule.Size = new System.Drawing.Size(95, 20);
            this.makermetalRule.TabIndex = 34;
            // 
            // mexRule
            // 
            this.mexRule.DecimalPlaces = 3;
            this.mexRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.mexRule.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.mexRule.Location = new System.Drawing.Point(6, 58);
            this.mexRule.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.mexRule.Name = "mexRule";
            this.mexRule.Size = new System.Drawing.Size(95, 20);
            this.mexRule.TabIndex = 36;
            // 
            // makerenergyRule
            // 
            this.makerenergyRule.DecimalPlaces = 3;
            this.makerenergyRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.makerenergyRule.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.makerenergyRule.Location = new System.Drawing.Point(6, 240);
            this.makerenergyRule.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.makerenergyRule.Name = "makerenergyRule";
            this.makerenergyRule.Size = new System.Drawing.Size(95, 20);
            this.makerenergyRule.TabIndex = 37;
            // 
            // energystorageRule
            // 
            this.energystorageRule.DecimalPlaces = 3;
            this.energystorageRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.energystorageRule.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.energystorageRule.Location = new System.Drawing.Point(6, 162);
            this.energystorageRule.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.energystorageRule.Name = "energystorageRule";
            this.energystorageRule.Size = new System.Drawing.Size(95, 20);
            this.energystorageRule.TabIndex = 38;
            // 
            // factorymetalgapRule
            // 
            this.factorymetalgapRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.factorymetalgapRule.Location = new System.Drawing.Point(6, 136);
            this.factorymetalgapRule.Maximum = new decimal(new int[] {
            30000,
            0,
            0,
            0});
            this.factorymetalgapRule.Name = "factorymetalgapRule";
            this.factorymetalgapRule.Size = new System.Drawing.Size(95, 20);
            this.factorymetalgapRule.TabIndex = 39;
            // 
            // factoryenergyRule
            // 
            this.factoryenergyRule.DecimalPlaces = 3;
            this.factoryenergyRule.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.factoryenergyRule.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.factoryenergyRule.Location = new System.Drawing.Point(6, 110);
            this.factoryenergyRule.Maximum = new decimal(new int[] {
            3,
            0,
            0,
            0});
            this.factoryenergyRule.Name = "factoryenergyRule";
            this.factoryenergyRule.Size = new System.Drawing.Size(95, 20);
            this.factoryenergyRule.TabIndex = 40;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(7, 59);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(37, 13);
            this.label3.TabIndex = 42;
            this.label3.Text = "Power";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(7, 111);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(71, 13);
            this.label4.TabIndex = 43;
            this.label4.Text = "Factory Metal";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(7, 85);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(78, 13);
            this.label5.TabIndex = 44;
            this.label5.Text = "Metal Extractor";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(7, 215);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(73, 13);
            this.label6.TabIndex = 45;
            this.label6.Text = "Metal Storage";
            // 
            // label26
            // 
            this.label26.AutoSize = true;
            this.label26.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label26.Location = new System.Drawing.Point(7, 189);
            this.label26.Name = "label26";
            this.label26.Size = new System.Drawing.Size(80, 13);
            this.label26.TabIndex = 50;
            this.label26.Text = "Energy Storage";
            // 
            // label22
            // 
            this.label22.AutoSize = true;
            this.label22.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label22.Location = new System.Drawing.Point(7, 163);
            this.label22.Name = "label22";
            this.label22.Size = new System.Drawing.Size(94, 13);
            this.label22.TabIndex = 46;
            this.label22.Text = "Factory Metal Gap";
            // 
            // label25
            // 
            this.label25.AutoSize = true;
            this.label25.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label25.Location = new System.Drawing.Point(6, 241);
            this.label25.Name = "label25";
            this.label25.Size = new System.Drawing.Size(95, 13);
            this.label25.TabIndex = 49;
            this.label25.Text = "MetalMaker  Metal";
            // 
            // label23
            // 
            this.label23.AutoSize = true;
            this.label23.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label23.Location = new System.Drawing.Point(7, 267);
            this.label23.Name = "label23";
            this.label23.Size = new System.Drawing.Size(99, 13);
            this.label23.TabIndex = 47;
            this.label23.Text = "MetalMaker Energy";
            // 
            // label24
            // 
            this.label24.AutoSize = true;
            this.label24.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label24.Location = new System.Drawing.Point(7, 137);
            this.label24.Name = "label24";
            this.label24.Size = new System.Drawing.Size(78, 13);
            this.label24.TabIndex = 48;
            this.label24.Text = "Factory Energy";
            // 
            // tabPage3
            // 
            this.tabPage3.BackColor = System.Drawing.SystemColors.Window;
            this.tabPage3.Controls.Add(this.groupBox3);
            this.tabPage3.Controls.Add(this.groupBox2);
            this.tabPage3.Controls.Add(this.groupBox1);
            this.tabPage3.Controls.Add(this.label51);
            this.tabPage3.Controls.Add(this.label50);
            this.tabPage3.Controls.Add(this.label49);
            this.tabPage3.Controls.Add(this.label36);
            this.tabPage3.Controls.Add(this.label35);
            this.tabPage3.Controls.Add(this.currentqueue);
            this.tabPage3.Controls.Add(this.keywords);
            this.tabPage3.Location = new System.Drawing.Point(4, 22);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage3.Size = new System.Drawing.Size(967, 666);
            this.tabPage3.TabIndex = 5;
            this.tabPage3.Text = "Create/Edit TaskLists";
            this.tabPage3.UseVisualStyleBackColor = true;
            this.tabPage3.Click += new System.EventHandler(this.tabPage3_Click);
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.button8);
            this.groupBox3.Controls.Add(this.InsertWord);
            this.groupBox3.Controls.Add(this.button2);
            this.groupBox3.Controls.Add(this.button3);
            this.groupBox3.Controls.Add(this.button6);
            this.groupBox3.Controls.Add(this.button7);
            this.groupBox3.Controls.Add(this.button4);
            this.groupBox3.Controls.Add(this.button5);
            this.groupBox3.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox3.ForeColor = System.Drawing.Color.Black;
            this.groupBox3.Location = new System.Drawing.Point(805, 238);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(154, 246);
            this.groupBox3.TabIndex = 26;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "List Actions";
            // 
            // button8
            // 
            this.button8.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button8.Location = new System.Drawing.Point(13, 29);
            this.button8.Name = "button8";
            this.button8.Size = new System.Drawing.Size(135, 23);
            this.button8.TabIndex = 12;
            this.button8.Text = "Insert At Start";
            this.button8.UseVisualStyleBackColor = true;
            this.button8.Click += new System.EventHandler(this.button8_Click);
            // 
            // InsertWord
            // 
            this.InsertWord.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.InsertWord.Location = new System.Drawing.Point(13, 55);
            this.InsertWord.Name = "InsertWord";
            this.InsertWord.Size = new System.Drawing.Size(135, 23);
            this.InsertWord.TabIndex = 5;
            this.InsertWord.Text = "Insert At End";
            this.InsertWord.UseVisualStyleBackColor = true;
            this.InsertWord.Click += new System.EventHandler(this.InsertWord_Click);
            // 
            // button2
            // 
            this.button2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button2.Location = new System.Drawing.Point(13, 81);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(135, 23);
            this.button2.TabIndex = 6;
            this.button2.Text = "Move Up";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // button3
            // 
            this.button3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button3.Location = new System.Drawing.Point(13, 107);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(135, 23);
            this.button3.TabIndex = 7;
            this.button3.Text = "Move Down";
            this.button3.UseVisualStyleBackColor = true;
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // button6
            // 
            this.button6.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button6.Location = new System.Drawing.Point(13, 133);
            this.button6.Name = "button6";
            this.button6.Size = new System.Drawing.Size(135, 23);
            this.button6.TabIndex = 10;
            this.button6.Text = "Move to Start";
            this.button6.UseVisualStyleBackColor = true;
            this.button6.Click += new System.EventHandler(this.button6_Click);
            // 
            // button7
            // 
            this.button7.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button7.Location = new System.Drawing.Point(13, 159);
            this.button7.Name = "button7";
            this.button7.Size = new System.Drawing.Size(135, 23);
            this.button7.TabIndex = 11;
            this.button7.Text = "Move to End";
            this.button7.UseVisualStyleBackColor = true;
            this.button7.Click += new System.EventHandler(this.button7_Click);
            // 
            // button4
            // 
            this.button4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button4.Location = new System.Drawing.Point(13, 185);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(135, 23);
            this.button4.TabIndex = 8;
            this.button4.Text = "Remove selected task";
            this.button4.UseVisualStyleBackColor = true;
            this.button4.Click += new System.EventHandler(this.button4_Click);
            // 
            // button5
            // 
            this.button5.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button5.Location = new System.Drawing.Point(13, 211);
            this.button5.Name = "button5";
            this.button5.Size = new System.Drawing.Size(135, 23);
            this.button5.TabIndex = 9;
            this.button5.Text = "Clear all tasks";
            this.button5.UseVisualStyleBackColor = true;
            this.button5.Click += new System.EventHandler(this.button5_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.currentbuildqueue);
            this.groupBox2.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox2.ForeColor = System.Drawing.Color.Black;
            this.groupBox2.Location = new System.Drawing.Point(805, 7);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(154, 92);
            this.groupBox2.TabIndex = 25;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Change TaskList";
            // 
            // currentbuildqueue
            // 
            this.currentbuildqueue.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.currentbuildqueue.FormattingEnabled = true;
            this.currentbuildqueue.Location = new System.Drawing.Point(6, 44);
            this.currentbuildqueue.MaxDropDownItems = 20;
            this.currentbuildqueue.Name = "currentbuildqueue";
            this.currentbuildqueue.Size = new System.Drawing.Size(132, 21);
            this.currentbuildqueue.Sorted = true;
            this.currentbuildqueue.TabIndex = 3;
            this.currentbuildqueue.SelectedIndexChanged += new System.EventHandler(this.currentbuildqueue_SelectedIndexChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.newtasklistbox);
            this.groupBox1.Controls.Add(this.button11);
            this.groupBox1.Controls.Add(this.btn_CreateNewTaskList);
            this.groupBox1.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox1.ForeColor = System.Drawing.Color.Black;
            this.groupBox1.Location = new System.Drawing.Point(805, 119);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(154, 113);
            this.groupBox1.TabIndex = 24;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "New TaskList";
            // 
            // newtasklistbox
            // 
            this.newtasklistbox.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.newtasklistbox.Location = new System.Drawing.Point(13, 23);
            this.newtasklistbox.Name = "newtasklistbox";
            this.newtasklistbox.Size = new System.Drawing.Size(135, 22);
            this.newtasklistbox.TabIndex = 0;
            this.newtasklistbox.PreviewKeyDown += new System.Windows.Forms.PreviewKeyDownEventHandler(this.newtasklistbox_PreviewKeyDown);
            this.newtasklistbox.TextChanged += new System.EventHandler(this.newtasklistbox_TextChanged);
            this.newtasklistbox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.newtasklistbox_KeyDown);
            // 
            // button11
            // 
            this.button11.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button11.Location = new System.Drawing.Point(13, 77);
            this.button11.Name = "button11";
            this.button11.Size = new System.Drawing.Size(135, 23);
            this.button11.TabIndex = 23;
            this.button11.Text = "Create New list from File";
            this.button11.UseVisualStyleBackColor = true;
            this.button11.Click += new System.EventHandler(this.button11_Click);
            // 
            // btn_CreateNewTaskList
            // 
            this.btn_CreateNewTaskList.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_CreateNewTaskList.Location = new System.Drawing.Point(13, 54);
            this.btn_CreateNewTaskList.Name = "btn_CreateNewTaskList";
            this.btn_CreateNewTaskList.Size = new System.Drawing.Size(135, 23);
            this.btn_CreateNewTaskList.TabIndex = 1;
            this.btn_CreateNewTaskList.Text = "Create New TaskList";
            this.btn_CreateNewTaskList.UseVisualStyleBackColor = true;
            this.btn_CreateNewTaskList.Click += new System.EventHandler(this.button1_Click);
            // 
            // label51
            // 
            this.label51.AutoSize = true;
            this.label51.Location = new System.Drawing.Point(443, 59);
            this.label51.Name = "label51";
            this.label51.Size = new System.Drawing.Size(316, 13);
            this.label51.TabIndex = 22;
            this.label51.Text = "Always check logfiles for reasons why tasks fail at /AI/NTai/logs/";
            // 
            // label50
            // 
            this.label50.AutoSize = true;
            this.label50.Location = new System.Drawing.Point(443, 46);
            this.label50.Name = "label50";
            this.label50.Size = new System.Drawing.Size(242, 13);
            this.label50.TabIndex = 21;
            this.label50.Text = "the bottom, skipping any that fail/cannot be done.";
            // 
            // label49
            // 
            this.label49.AutoSize = true;
            this.label49.Location = new System.Drawing.Point(443, 33);
            this.label49.Name = "label49";
            this.label49.Size = new System.Drawing.Size(337, 13);
            this.label49.TabIndex = 20;
            this.label49.Text = "Units following this tasklist will execute each task from the top down to";
            // 
            // label36
            // 
            this.label36.AutoSize = true;
            this.label36.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label36.Location = new System.Drawing.Point(439, 3);
            this.label36.Name = "label36";
            this.label36.Size = new System.Drawing.Size(169, 20);
            this.label36.TabIndex = 16;
            this.label36.Text = "Current TaskList Items";
            // 
            // label35
            // 
            this.label35.AutoSize = true;
            this.label35.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label35.Location = new System.Drawing.Point(6, 3);
            this.label35.Name = "label35";
            this.label35.Size = new System.Drawing.Size(167, 20);
            this.label35.TabIndex = 15;
            this.label35.Text = "Task List Items to Add";
            // 
            // currentqueue
            // 
            this.currentqueue.FormattingEnabled = true;
            this.currentqueue.Items.AddRange(new object[] {
            "please create a new tasklist to begin, type in the name of the",
            "tasklist to the right and click create new tasklist to begin.",
            "",
            "Then add tasks using the two tabs to the left.",
            "Todo this select a task item, then insert it using the buttons",
            "on the right.",
            "",
            "When you\'re done, goto the units tab, pick a unit, and give",
            "this tasklist to it.",
            "",
            "Its not enough for a tasklist to exist, you have to \'assign\'/\'give\'",
            "it to a unit, you need to tell the unit \"this is what your doing",
            "in this list, this list  and this list\"."});
            this.currentqueue.Location = new System.Drawing.Point(443, 79);
            this.currentqueue.Name = "currentqueue";
            this.currentqueue.Size = new System.Drawing.Size(356, 576);
            this.currentqueue.TabIndex = 4;
            // 
            // keywords
            // 
            this.keywords.Controls.Add(this.tabPage8);
            this.keywords.Controls.Add(this.tabPage9);
            this.keywords.Location = new System.Drawing.Point(3, 28);
            this.keywords.Multiline = true;
            this.keywords.Name = "keywords";
            this.keywords.SelectedIndex = 0;
            this.keywords.Size = new System.Drawing.Size(434, 632);
            this.keywords.TabIndex = 2;
            // 
            // tabPage8
            // 
            this.tabPage8.Controls.Add(this.universalkeywordsList);
            this.tabPage8.Location = new System.Drawing.Point(4, 22);
            this.tabPage8.Name = "tabPage8";
            this.tabPage8.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage8.Size = new System.Drawing.Size(426, 606);
            this.tabPage8.TabIndex = 0;
            this.tabPage8.Text = "Universal Keywords";
            this.tabPage8.ToolTipText = "Mod independant rule based keywords";
            this.tabPage8.UseVisualStyleBackColor = true;
            // 
            // universalkeywordsList
            // 
            this.universalkeywordsList.FormattingEnabled = true;
            this.universalkeywordsList.Location = new System.Drawing.Point(1, 3);
            this.universalkeywordsList.Name = "universalkeywordsList";
            this.universalkeywordsList.Size = new System.Drawing.Size(422, 602);
            this.universalkeywordsList.TabIndex = 0;
            // 
            // tabPage9
            // 
            this.tabPage9.Controls.Add(this.unitkeywords);
            this.tabPage9.Location = new System.Drawing.Point(4, 22);
            this.tabPage9.Name = "tabPage9";
            this.tabPage9.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage9.Size = new System.Drawing.Size(426, 606);
            this.tabPage9.TabIndex = 1;
            this.tabPage9.Text = "Build Unit";
            this.tabPage9.ToolTipText = "Units in the mod";
            this.tabPage9.UseVisualStyleBackColor = true;
            // 
            // unitkeywords
            // 
            this.unitkeywords.FormattingEnabled = true;
            this.unitkeywords.HorizontalScrollbar = true;
            this.unitkeywords.Location = new System.Drawing.Point(3, 4);
            this.unitkeywords.Name = "unitkeywords";
            this.unitkeywords.Size = new System.Drawing.Size(420, 602);
            this.unitkeywords.Sorted = true;
            this.unitkeywords.TabIndex = 0;
            // 
            // tabPage5
            // 
            this.tabPage5.Controls.Add(this.label27);
            this.tabPage5.Controls.Add(this.toolStrip1);
            this.tabPage5.Controls.Add(this.quicksetchecks);
            this.tabPage5.Location = new System.Drawing.Point(4, 22);
            this.tabPage5.Name = "tabPage5";
            this.tabPage5.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage5.Size = new System.Drawing.Size(967, 666);
            this.tabPage5.TabIndex = 9;
            this.tabPage5.Text = "Set several unit properties at once";
            this.tabPage5.UseVisualStyleBackColor = true;
            // 
            // label27
            // 
            this.label27.AutoSize = true;
            this.label27.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label27.Location = new System.Drawing.Point(8, 28);
            this.label27.Name = "label27";
            this.label27.Size = new System.Drawing.Size(824, 16);
            this.label27.TabIndex = 2;
            this.label27.Text = "Use this page to set settings for lots of units at once, select a property in the" +
                " combo box above and make your changes, then apply settings";
            // 
            // toolStrip1
            // 
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripLabel1,
            this.QuicksetCombo,
            this.toolStripButton1});
            this.toolStrip1.Location = new System.Drawing.Point(3, 3);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(961, 25);
            this.toolStrip1.TabIndex = 1;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // toolStripLabel1
            // 
            this.toolStripLabel1.Name = "toolStripLabel1";
            this.toolStripLabel1.Size = new System.Drawing.Size(154, 22);
            this.toolStripLabel1.Text = "Select a property to change:";
            // 
            // QuicksetCombo
            // 
            this.QuicksetCombo.Items.AddRange(new object[] {
            "Attackers",
            "Kamikaze",
            "Fire at will",
            "Return Fire",
            "Hold Fire",
            "Roam",
            "Maneouvre",
            "Hold Position",
            "NeverAntistall"});
            this.QuicksetCombo.Name = "QuicksetCombo";
            this.QuicksetCombo.Size = new System.Drawing.Size(121, 25);
            this.QuicksetCombo.SelectedIndexChanged += new System.EventHandler(this.QuicksetCombo_SelectedIndexChanged);
            this.QuicksetCombo.Click += new System.EventHandler(this.QuicksetCombo_Click);
            // 
            // toolStripButton1
            // 
            this.toolStripButton1.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.toolStripButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButton1.Name = "toolStripButton1";
            this.toolStripButton1.Size = new System.Drawing.Size(135, 22);
            this.toolStripButton1.Text = "Apply Changes to Units";
            this.toolStripButton1.Click += new System.EventHandler(this.toolStripButton1_Click);
            // 
            // quicksetchecks
            // 
            this.quicksetchecks.FormattingEnabled = true;
            this.quicksetchecks.Location = new System.Drawing.Point(8, 62);
            this.quicksetchecks.Name = "quicksetchecks";
            this.quicksetchecks.Size = new System.Drawing.Size(951, 589);
            this.quicksetchecks.TabIndex = 0;
            // 
            // tabPage4
            // 
            this.tabPage4.Controls.Add(this.label65);
            this.tabPage4.Controls.Add(this.refresh);
            this.tabPage4.Controls.Add(this.debugsave);
            this.tabPage4.Location = new System.Drawing.Point(4, 22);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage4.Size = new System.Drawing.Size(967, 666);
            this.tabPage4.TabIndex = 7;
            this.tabPage4.Text = "Debug";
            this.tabPage4.UseVisualStyleBackColor = true;
            // 
            // label65
            // 
            this.label65.AutoSize = true;
            this.label65.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label65.Location = new System.Drawing.Point(3, 609);
            this.label65.Name = "label65";
            this.label65.Size = new System.Drawing.Size(267, 20);
            this.label65.TabIndex = 2;
            this.label65.Text = "This Page is for debug purposes";
            // 
            // refresh
            // 
            this.refresh.Location = new System.Drawing.Point(839, 609);
            this.refresh.Name = "refresh";
            this.refresh.Size = new System.Drawing.Size(91, 24);
            this.refresh.TabIndex = 1;
            this.refresh.Text = "refresh";
            this.refresh.UseVisualStyleBackColor = true;
            this.refresh.Click += new System.EventHandler(this.refresh_Click);
            this.refresh.MouseClick += new System.Windows.Forms.MouseEventHandler(this.refresh_MouseClick);
            // 
            // debugsave
            // 
            this.debugsave.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.debugsave.BackColor = System.Drawing.Color.White;
            this.debugsave.Location = new System.Drawing.Point(3, 3);
            this.debugsave.Name = "debugsave";
            this.debugsave.ReadOnly = true;
            this.debugsave.Size = new System.Drawing.Size(956, 603);
            this.debugsave.TabIndex = 0;
            this.debugsave.Text = "";
            // 
            // openFileDialog
            // 
            this.openFileDialog.DefaultExt = "tdf";
            this.openFileDialog.Filter = "TDF configs (*.tdf)|*.tdf";
            this.openFileDialog.Title = "Open config";
            this.openFileDialog.FileOk += new System.ComponentModel.CancelEventHandler(this.openFileDialog1_FileOk);
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.DefaultExt = "txt";
            this.openFileDialog1.FileName = "openFileDialog1";
            this.openFileDialog1.Filter = "commar seperated list (*.txt)|*.txt";
            // 
            // Form1
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(224)))), ((int)(((byte)(224)))), ((int)(((byte)(224)))));
            this.ClientSize = new System.Drawing.Size(975, 692);
            this.Controls.Add(this.mainTabs);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "NTai Toolkit V0.27";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.mainTabs.ResumeLayout(false);
            this.StartTab.ResumeLayout(false);
            this.StartTab.PerformLayout();
            this.tabPage1.ResumeLayout(false);
            this.tabPage1.PerformLayout();
            this.UnitsTab.ResumeLayout(false);
            this.UnitsTab.PerformLayout();
            this.tabControl1.ResumeLayout(false);
            this.tabPage6.ResumeLayout(false);
            this.tabPage7.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.ExclusionZone)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.RepairRange)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.MaxEnergy)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.MinEnergy)).EndInit();
            this.modTDFtab.ResumeLayout(false);
            this.groupBox12.ResumeLayout(false);
            this.groupBox12.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.maxAttackSize)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.AttackIncrementPercentage)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.AttackIncrementValue)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.initialAttackSize)).EndInit();
            this.groupBox11.ResumeLayout(false);
            this.groupBox11.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.normal_handicap)).EndInit();
            this.groupBox7.ResumeLayout(false);
            this.groupBox7.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mexnoweaponradius)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.NoEnemyGeo)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.GeoSearchRadius)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.DefaultSpacing)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.DefenceSpacing)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.FactorySpacing)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.PowerSpacing)).EndInit();
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.tabPage2.ResumeLayout(false);
            this.groupBox13.ResumeLayout(false);
            this.groupBox13.PerformLayout();
            this.MaxAntiStallBox.ResumeLayout(false);
            this.MaxAntiStallBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.StallTimeMobile)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.StallTimeImMobile)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.antistallwindow)).EndInit();
            this.groupBox9.ResumeLayout(false);
            this.groupBox9.PerformLayout();
            this.groupBox10.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.powerRuleEx)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.factorymetalRuleEx)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.factoryenergyRuleEx)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.factorymetalgapRuleEx)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.metalstorageRuleEx)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.energystorageRuleEx)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.mexRuleEx)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.makermetalRuleEx)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.makerenergyRuleEx)).EndInit();
            this.groupBox8.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.powerRule)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.factorymetalRule)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.metalstorageRule)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.makermetalRule)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.mexRule)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.makerenergyRule)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.energystorageRule)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.factorymetalgapRule)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.factoryenergyRule)).EndInit();
            this.tabPage3.ResumeLayout(false);
            this.tabPage3.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.keywords.ResumeLayout(false);
            this.tabPage8.ResumeLayout(false);
            this.tabPage9.ResumeLayout(false);
            this.tabPage5.ResumeLayout(false);
            this.tabPage5.PerformLayout();
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.tabPage4.ResumeLayout(false);
            this.tabPage4.PerformLayout();
            this.ResumeLayout(false);

        }

        private void InsertWord_Click(object sender, EventArgs e)
        {
            if (this.CheckSelectedList())
            {
                if (this.keywords.SelectedIndex == 0)
                {
                    this.currentqueue.Items.Add(this.GetSelectedKeyWord());
                }
                else
                {
                    this.currentqueue.Items.Add(this.GetSelectedKeyWord());
                }
                this.SaveQueue();
            }
        }

        private void KamikazeCheck_CheckedChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                if (!this.buildtree.kamikaze.Contains(text1))
                {
                    this.buildtree.kamikaze.Add(text1);
                }
            }
        }

        String configfile1 = "";
        String configfile2 = "";
        private Label label31;
        private Label label56;
        private Label label55;
        private Label label47;
        private Label label59;
        private Label label58;
        private Label label57;
        private Label label61;
        private Label label60;
        private Label label66;
        private Label label63;
        private Label label62;
        private Label label67;
        private LinkLabel linkLabel3;
        private Label label68;
        public NumericUpDown AttackIncrementValue;
        public NumericUpDown NoEnemyGeo;
        private Label label40;
        public NumericUpDown GeoSearchRadius;
        private Label label39;
        private Label label43;
        public NumericUpDown AttackIncrementPercentage;
        private Label label69;
        public NumericUpDown maxAttackSize;
        private Label label71;
        private Label label70;
        public NumericUpDown mexnoweaponradius;
        private TabPage tabPage2;
        private GroupBox groupBox13;
        public CheckBox interpolate;
        public GroupBox MaxAntiStallBox;
        private Label label41;
        public NumericUpDown StallTimeMobile;
        public NumericUpDown StallTimeImMobile;
        private Label label15;
        private Label label17;
        public NumericUpDown antistallwindow;
        private Label label52;
        private GroupBox groupBox9;
        private GroupBox groupBox10;
        public NumericUpDown powerRuleEx;
        public NumericUpDown factorymetalRuleEx;
        public NumericUpDown factoryenergyRuleEx;
        public NumericUpDown factorymetalgapRuleEx;
        public NumericUpDown metalstorageRuleEx;
        public NumericUpDown energystorageRuleEx;
        public NumericUpDown mexRuleEx;
        public NumericUpDown makermetalRuleEx;
        public NumericUpDown makerenergyRuleEx;
        private Label label42;
        private GroupBox groupBox8;
        public NumericUpDown powerRule;
        public NumericUpDown factorymetalRule;
        public NumericUpDown metalstorageRule;
        public NumericUpDown makermetalRule;
        public NumericUpDown mexRule;
        public NumericUpDown makerenergyRule;
        public NumericUpDown energystorageRule;
        public NumericUpDown factorymetalgapRule;
        public NumericUpDown factoryenergyRule;
        private Label label3;
        private Label label4;
        private Label label5;
        private Label label6;
        private Label label26;
        private Label label22;
        private Label label25;
        private Label label23;
        private Label label24;
        private Label label72;
        private Label label73;
        String configfile3 = "";
        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            try
            {
                //fix for multiple load problem
                this.mod = new CMod();
                this.buildtree = new CBuildtree(this);
                this.mod.f = this;
                this.universalkeywordsList.Items.Clear();
                this.unitkeywords.Items.Clear();
                this.Units.Items.Clear();
                this.currentbuildqueue.Items.Clear();


                string text1 = "";
                this.openFileDialog.ShowDialog();
                if (System.IO.File.Exists(this.openFileDialog.FileName))
                {
                    String[] path = openFileDialog.FileName.Split('\\');
                    configfile1 = path[path.Length-1];//this.openFileDialog.FileName;
                    Stream stream1 = this.openFileDialog.OpenFile();
                    StreamReader reader1 = new StreamReader(stream1);
                    text1 = reader1.ReadToEnd();
                    reader1.Close();
                    TdfParser parser1 = new TdfParser(text1);
                    string[] textArray1 = this.openFileDialog.FileName.Split(new char[] { '\\' });
                    string text2 = "";
                    for (int num1 = 0; num1 < textArray1.Length; num1++)
                    {
                        if ((num1 + 1) >= textArray1.Length)
                        {
                            break;
                        }
                        text2 = text2 + textArray1[num1] + "/";
                    }

                    String s = parser1.RootSection.GetStringValue("foobar", @"ntai\modconfig");
                    if(s.Equals("foobar")){
                        MessageBox.Show("If you're trying to open a config inside the /AI/NTai/configs/ folder then your choosing the wrong file. Please select the file in the /AI/NTai/ folder", "Access exception.");
                        return;
                    }
                    if (this.buildtree.Load(text2 + parser1.RootSection.GetStringValue(@"ntai\modconfig")))
                    {
                        configfile2 = parser1.RootSection.GetStringValue(@"ntai\modconfig");
                        this.mod.LoadMod(this.openFileDialog.InitialDirectory + parser1.RootSection.GetStringValue(@"ntai\learndata"));
                        configfile3 = parser1.RootSection.GetStringValue(@"ntai\learndata");
                        this.mod.modname = parser1.RootSection.GetStringValue(@"ntai\modname");
                        this.modlabel.Text = this.mod.modname + " is loaded";
                        ArrayList list1 = this.buildtree.keywords.GetUniversalKeywords();
                        foreach (string text3 in list1)
                        {
                            this.universalkeywordsList.Items.Add(text3);
                        }
                        ArrayList list2 = new ArrayList();
                        foreach (string text4 in this.mod.units)
                        {
                            list2.Add(text4 + " - " + this.mod.human_names[text4] + " " + this.mod.descriptions[text4]);
                        }
                        this.buildtree.keywords.AddCollection(this.mod.units, EWordType.e_unitname);
                        foreach (string text5 in list2)
                        {
                            this.unitkeywords.Items.Add(text5);
                            this.Units.Items.Add(text5);
                        }
                        this.currentbuildqueue.Items.Clear();
                        foreach (string text6 in this.buildtree.tasklists.Keys)
                        {
                            this.currentbuildqueue.Items.Add(text6);
                        }
                    }
                    else
                    {
                        this.modlabel.Text = this.mod.modname + " FAILED to load";
                    }
                }
            }
            catch (UnauthorizedAccessException)
            {
                MessageBox.Show("If you're trying to open a config inside the /AI/NTai/configs/ folder then your choosing the wrong file. Please select the file in the /AI/NTai/ folder", "Access exception.");
            }
        }

        private void linkLabel2_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
        }

        private void linkLabel2_MouseClick(object sender, MouseEventArgs e)
        {
            this.buildtree.Save(true);
            this.mod.SaveMod();
        }

        public void LoadConfig(string filename)
        {
        }

        public void LoadCurrentQueue(string tasklist)
        {
            this.currentqueue.Items.Clear();
            ArrayList list1 = this.buildtree.GetTaskList(tasklist);
            foreach (string text1 in list1)
            {
                this.currentqueue.Items.Add(text1);
            }
        }

        [STAThread]
        private static void Main()
        {
            using (Form1 form1 = new Form1())
            {
                Application.EnableVisualStyles();
                Application.Run(form1);
            }
        }

        private void MaxEnergy_ValueChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                this.buildtree.MaxEnergy[text1] = this.MaxEnergy.Value;
            }
        }

        private void Maximumenergyallowed_ValueChanged(object sender, EventArgs e)
        {
        }

        private void menuItem12_Click(object sender, EventArgs e)
        {
        }

        private void Message_TextChanged(object sender, EventArgs e)
        {
            this.buildtree.message = this.Message.Text;
        }

        private void MinEnergy_ValueChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                this.buildtree.MinEnergy[text1] = this.MinEnergy.Value;
            }
        }

        private void Minimumenergyallowed_ValueChanged(object sender, EventArgs e)
        {
        }

        private void modTDFtab_Click(object sender, EventArgs e)
        {
        }

        private void Movestatecombo_SelectedIndexChanged(object sender, EventArgs e)
        {
        }

        private void Movestatecombo_SelectedValueChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                this.buildtree.movement[text1] = this.Movestatecombo.SelectedIndex;
            }
        }

        private void NeverantistallCheck_CheckedChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                if (!this.buildtree.neverantistall.Contains(text1))
                {
                    this.buildtree.neverantistall.Add(text1);
                }
            }
        }


        private void newtasklistbox_KeyDown(object sender, KeyEventArgs e)
        {
        }

        private void newtasklistbox_PreviewKeyDown(object sender, PreviewKeyDownEventArgs e)
        {
        }

        private void newtasklistbox_TextChanged(object sender, EventArgs e)
        {
            if ((this.newtasklistbox.Text != null) && (((this.newtasklistbox.Text.Contains("/") || this.newtasklistbox.Text.Contains(";")) || (this.newtasklistbox.Text.Contains(" ") || this.newtasklistbox.Text.Contains("{"))) || ((this.newtasklistbox.Text.Contains("}") || this.newtasklistbox.Text.Contains("[")) || (this.newtasklistbox.Text.Contains("]") || this.newtasklistbox.Text.Contains("=")))))
            {
                MessageBox.Show("ERROR This character will crash NTai if you require spaces, please use underscores __");
                char[] chArray1 = this.newtasklistbox.Text.ToCharArray();
                string text1 = "";
                foreach (char ch1 in chArray1)
                {
                    if ((((ch1 != '/') && (ch1 != ';')) && ((ch1 != ' ') && (ch1 != '{'))) && (((ch1 != '}') && (ch1 != '[')) && ((ch1 != ']') && (ch1 != '='))))
                    {
                        text1 = text1 + ch1;
                    }
                }
                this.newtasklistbox.Text = text1;
            }
        }

        private void openFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
        }

        private void QuicksetCombo_Click(object sender, EventArgs e)
        {
        }

        private void QuicksetCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.FillQuickSetPage();
        }

        private void refresh_Click(object sender, EventArgs e)
        {
        }

        private void refresh_MouseClick(object sender, MouseEventArgs e)
        {
            this.debugsave.Text = this.buildtree.Save(false);
        }

        public void ReloadUserTabStats()
        {
            string text1 = this.GetSelectedUnit();
            if (this.buildtree.firingstate.ContainsKey(text1))
            {
                this.Firingstatecombo.SelectedIndex = this.buildtree.firingstate[text1];
            }
            else
            {
                this.Firingstatecombo.SelectedIndex = 2;
            }
            if (this.buildtree.movement.ContainsKey(text1))
            {
                this.Movestatecombo.SelectedIndex = this.buildtree.movement[text1];
            }
            else
            {
                this.Movestatecombo.SelectedIndex = 1;
            }
            this.AttackCheck.Checked = this.buildtree.attackers.Contains(text1);
            //this.ScouterCheck.Checked = this.buildtree.scouters.Contains(text1);
            this.SolobuildCheck.Checked = this.buildtree.solobuild.Contains(text1);
            this.SinglebuildCheck.Checked = this.buildtree.singlebuild.Contains(text1);
            this.AlwaysantistallCheck.Checked = this.buildtree.alwaysantistall.Contains(text1);
            this.NeverantistallCheck.Checked = this.buildtree.neverantistall.Contains(text1);
            this.KamikazeCheck.Checked = this.buildtree.kamikaze.Contains(text1);
            if (this.buildtree.MinEnergy.ContainsKey(text1))
            {
                this.MinEnergy.Value = this.buildtree.MinEnergy[text1];
            }
            else
            {
                this.MinEnergy.Value = new decimal(0);
            }
            if (this.buildtree.MaxEnergy.ContainsKey(text1))
            {
                this.MaxEnergy.Value = this.buildtree.MaxEnergy[text1];
            }
            else
            {
                this.MaxEnergy.Value = new decimal(0);
            }
            if (this.buildtree.ConstructionRepairRanges.ContainsKey(text1))
            {
                this.RepairRange.Value = this.buildtree.ConstructionRepairRanges[text1];
            }
            else
            {
                this.RepairRange.Value = new decimal(0);
            }
            if (this.buildtree.ConstructionExclusionRange.ContainsKey(text1))
            {
                this.ExclusionZone.Value = this.buildtree.ConstructionExclusionRange[text1];
            }
            else
            {
                this.ExclusionZone.Value = new decimal(0);
            }
            foreach (string text2 in this.buildtree.tasklists.Keys)
            {
                ArrayList list1;
                ArrayList list2;
                if (this.buildtree.unittasklists.ContainsKey(text1))
                {
                    list1 = this.buildtree.unittasklists[text1];
                }
                else
                {
                    list1 = new ArrayList();
                }
                if (this.buildtree.unitcheattasklists.ContainsKey(text1))
                {
                    list2 = this.buildtree.unitcheattasklists[text1];
                }
                else
                {
                    list2 = new ArrayList();
                }
                if (list1.Contains(text2))
                {
                    this.TaskQueues.Items.Add(text2, true);
                }
                else
                {
                    this.TaskQueues.Items.Add(text2, false);
                }
                if (list2.Contains(text2))
                {
                    this.cheattaskqueue.Items.Add(text2, true);
                }
                else
                {
                    this.cheattaskqueue.Items.Add(text2, false);
                }
            }
        }

        private void RepairRange_ValueChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                this.buildtree.ConstructionRepairRanges[text1] = this.RepairRange.Value;
            }
        }

        public void ResetUserTabStats()
        {
            this.TaskQueues.Items.Clear();
            this.cheattaskqueue.Items.Clear();
        }

        public void SaveListAssignments()
        {
            string text1 = this.GetSelectedUnit();
            ArrayList list1 = new ArrayList();
            for (int num1 = 0; num1 < this.TaskQueues.Items.Count; num1++)
            {
                if (this.TaskQueues.GetItemChecked(num1))
                {
                    list1.Add(this.TaskQueues.Items[num1]);
                }
            }
            this.buildtree.unittasklists[text1] = list1;
            ArrayList list2 = new ArrayList();
            for (int num2 = 0; num2 < this.cheattaskqueue.Items.Count; num2++)
            {
                if (this.cheattaskqueue.GetItemChecked(num2))
                {
                    list2.Add(this.cheattaskqueue.Items[num2]);
                }
            }
            this.buildtree.unitcheattasklists[text1] = list2;
        }

        public void SaveQueue()
        {
            string text1 = this.currentbuildqueue.SelectedItem as string;
            ArrayList list1 = new ArrayList();
            foreach (string text2 in this.currentqueue.Items)
            {
                list1.Add(text2);
            }
            this.buildtree.SetTaskList(text1, list1);
        }

        public void SaveQuickset()
        {
            string text1 = this.QuicksetCombo.SelectedItem as string;
            ArrayList list1 = new ArrayList();
            ArrayList list2 = this.mod.units;
            foreach (string text2 in this.quicksetchecks.Items)
            {
                if (this.quicksetchecks.GetItemChecked(this.quicksetchecks.Items.IndexOf(text2)))
                {
                    list1.Add(text2);
                }
            }
            switch (text1)
            {
                case "Scouters":
                    this.buildtree.scouters.Clear();
                    this.buildtree.scouters.AddRange(list1);
                    break;

                case "Kamikaze":
                    this.buildtree.kamikaze.Clear();
                    this.buildtree.kamikaze.AddRange(list1);
                    break;

                case "NeverAntistall":
                    this.buildtree.neverantistall.Clear();
                    this.buildtree.neverantistall.AddRange(list1);
                    return;

                case "Attackers":
                    this.buildtree.attackers.Clear();
                    this.buildtree.attackers.AddRange(list1);
                    return;

                case "Fire at will":
                    foreach (string text3 in list2)
                    {
                        if (list1.Contains(text3))
                        {
                            this.buildtree.firingstate[text3] = 2;
                        }
                        else if (this.buildtree.firingstate[text3] == 2)
                        {
                            this.buildtree.firingstate[text3] = 1;
                        }
                    }
                    return;

                case "Return Fire":
                    foreach (string text4 in list2)
                    {
                        if (list1.Contains(text4))
                        {
                            this.buildtree.firingstate[text4] = 1;
                        }
                        else if (this.buildtree.firingstate[text4] == 1)
                        {
                            this.buildtree.firingstate[text4] = 2;
                        }
                    }
                    return;

                case "Hold Fire":
                    foreach (string text5 in list2)
                    {
                        if (list1.Contains(text5))
                        {
                            this.buildtree.firingstate[text5] = 0;
                        }
                        else if (this.buildtree.firingstate[text5] == 0)
                        {
                            this.buildtree.firingstate[text5] = 2;
                        }
                    }
                    return;

                case "Roam":
                    foreach (string text6 in list2)
                    {
                        if (list1.Contains(text6))
                        {
                            this.buildtree.movement[text6] = 2;
                        }
                        else if (this.buildtree.movement[text6] == 2)
                        {
                            this.buildtree.movement[text6] = 1;
                        }
                    }
                    return;

                case "Maneouvre":
                    foreach (string text7 in list2)
                    {
                        if (list1.Contains(text7))
                        {
                            this.buildtree.movement[text7] = 1;
                        }
                        else if (this.buildtree.movement[text7] == 1)
                        {
                            this.buildtree.movement[text7] = 0;
                        }
                    }
                    return;
            }
            if (text1 == "Hold Position")
            {
                foreach (string text8 in list2)
                {
                    if (list1.Contains(text8))
                    {
                        this.buildtree.movement[text8] = 0;
                    }
                    else if (this.buildtree.movement[text8] == 0)
                    {
                        this.buildtree.movement[text8] = 2;
                    }
                }
            }
        }

        private void ScouterCheck_CheckedChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                if (!this.buildtree.scouters.Contains(text1))
                {
                    this.buildtree.scouters.Add(text1);
                }
            }
        }

        private void SinglebuildCheck_CheckedChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                if (!this.buildtree.singlebuild.Contains(text1))
                {
                    this.buildtree.singlebuild.Add(text1);
                }
            }
        }

        private void SolobuildCheck_CheckedChanged(object sender, EventArgs e)
        {
            if (!this.inswitch)
            {
                string text1 = this.GetSelectedUnit();
                if (!this.buildtree.solobuild.Contains(text1))
                {
                    this.buildtree.solobuild.Add(text1);
                }
            }
        }

        private void tabPage1_Click(object sender, EventArgs e)
        {
        }

        private void tabPage3_Click(object sender, EventArgs e)
        {
        }

        private void TaskQueues_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.SaveListAssignments();
        }

        private void toolStripButton1_Click(object sender, EventArgs e)
        {
            this.SaveQuickset();
        }

        private void Units_SelectedIndexChanged(object sender, EventArgs e)
        {
        }

        private void Units_SelectedValueChanged(object sender, EventArgs e)
        {
            string text1 = this.GetSelectedUnit();
            if (text1 != this.currentselected)
            {
                this.inswitch = true;
                this.ResetUserTabStats();
                this.ReloadUserTabStats();
                this.inswitch = false;
                this.currentselected = text1;
            }
        }

        private void UnitsTab_Enter(object sender, EventArgs e)
        {
        }

        private void UnitsTab_Leave(object sender, EventArgs e)
        {
            this.SaveListAssignments();
        }

        private void UnitTagsTab_Click(object sender, EventArgs e)
        {
        }

        private void Version_TextChanged(object sender, EventArgs e)
        {
        }


        private CheckBox AlwaysantistallCheck;
        public CheckBox Antistall;
        private CheckBox AttackCheck;
        public TextBox Author;
        public CBuildtree buildtree;
        private Button btn_CreateNewTaskList;
        private Button button11;
        private Button button2;
        private Button button3;
        private Button button4;
        private Button button5;
        private Button button6;
        private Button button7;
        private Button button8;
        private CheckedListBox cheattaskqueue;
        private ComboBox currentbuildqueue;
        private ListBox currentqueue;
        public string currentselected;
        public RichTextBox debugsave;
        public NumericUpDown DefaultSpacing;
        public NumericUpDown DefenceSpacing;
        public NumericUpDown ExclusionZone;
        public NumericUpDown FactorySpacing;
        private ComboBox Firingstatecombo;
        private GroupBox groupBox1;
        private GroupBox groupBox11;
        private GroupBox groupBox12;
        private GroupBox groupBox2;
        private GroupBox groupBox3;
        private GroupBox groupBox4;
        private GroupBox groupBox5;
        private GroupBox groupBox7;
        public NumericUpDown initialAttackSize;
        private Button InsertWord;
        public bool inswitch;
        private CheckBox KamikazeCheck;
        private TabControl keywords;
        private Label label1;
        private Label label10;
        private Label label11;
        private Label label12;
        private Label label13;
        private Label label14;
        private Label label16;
        private Label label18;
        private Label label19;
        private Label label2;
        private Label label20;
        private Label label21;
        private Label label27;
        private Label label28;
        private Label label29;
        private Label label30;
        private Label label32;
        private Label label33;
        private Label label34;
        private Label label35;
        private Label label36;
        private Label label37;
        private Label label38;
        private Label label44;
        private Label label45;
        private Label label49;
        private Label label50;
        private Label label51;
        private Label label53;
        private Label label54;
        private Label label64;
        private Label label65;
        private Label label7;
        private Label label8;
        private Label label9;
        private LinkLabel ll_OpenConfigFile;
        private LinkLabel linkLabel2;
        private TabControl mainTabs;
        public NumericUpDown MaxEnergy;
        public TextBox Message;
        public NumericUpDown MinEnergy;
        public CMod mod;
        public Label modlabel;
        private TabPage modTDFtab;
        private ComboBox Movestatecombo;
        private CheckBox NeverantistallCheck;
        private TextBox newtasklistbox;
        public NumericUpDown normal_handicap;
        private OpenFileDialog openFileDialog;
        public OpenFileDialog openFileDialog1;
        public NumericUpDown PowerSpacing;
        private CheckedListBox quicksetchecks;
        private ToolStripComboBox QuicksetCombo;
        private Button refresh;
        public NumericUpDown RepairRange;
        private RichTextBox richTextBox1;
        private CheckBox SinglebuildCheck;
        private CheckBox SolobuildCheck;
        public CheckBox spacemod;
        private TabPage StartTab;
        private TabControl tabControl1;
        private TabPage tabPage1;
        private TabPage tabPage3;
        private TabPage tabPage4;
        private TabPage tabPage5;
        private TabPage tabPage6;
        private TabPage tabPage7;
        private TabPage tabPage8;
        private TabPage tabPage9;
        private CheckedListBox TaskQueues;
        private ToolStrip toolStrip1;
        private ToolStripButton toolStripButton1;
        private ListBox unitkeywords;
        public ListBox Units;
        private TabPage UnitsTab;
        private ListBox universalkeywordsList;
        public TextBox Version;
        private ToolStripLabel toolStripLabel1;
        private LinkLabel linkLabel1;
        public int VERSION;

        private void linkLabel1_LinkClicked_1(object sender, LinkLabelLinkClickedEventArgs e)
        {
            try{
                using (ZipOutputStream s = new ZipOutputStream(File.Create(mod.modname+"_config.zip"))) {
    			
				    s.SetLevel(9); // 0 - store only to 9 - means best compression
    		
				    byte[] buffer = new byte[4096];
    				
				    //foreach (string file in filenames) {
    					
					    // Using GetFileName makes the result compatible with XP
					    // as the resulting path is not absolute.
				    ZipEntry entry1 = new ZipEntry("AI\\NTai\\"+configfile1);
					
				    // Setup the entry data as required.
					
				    // Crc and size are handled by the library for seakable streams
				    // so no need to do them here.

				    // Could also use the last write time or similar for the file.
				    entry1.DateTime = DateTime.Now;
				    s.PutNextEntry(entry1);

                    using (FileStream fs = File.OpenRead(configfile1))
                    {
		
					    // Using a fixed size buffer here makes no noticeable difference for output
					    // but keeps a lid on memory usage.
					    int sourceBytes;
					    do {
						    sourceBytes = fs.Read(buffer, 0, buffer.Length);
						    s.Write(buffer, 0, sourceBytes);
					    } while ( sourceBytes > 0 );
				    }


                    ZipEntry entry2 = new ZipEntry("AI\\NTai\\"+configfile2);

                    // Setup the entry data as required.

                    // Crc and size are handled by the library for seakable streams
                    // so no need to do them here.

                    // Could also use the last write time or similar for the file.
                    entry2.DateTime = DateTime.Now;
                    s.PutNextEntry(entry2);

                    using (FileStream fs = File.OpenRead(configfile2))
                    {

                        // Using a fixed size buffer here makes no noticeable difference for output
                        // but keeps a lid on memory usage.
                        int sourceBytes;
                        do
                        {
                            sourceBytes = fs.Read(buffer, 0, buffer.Length);
                            s.Write(buffer, 0, sourceBytes);
                        } while (sourceBytes > 0);
                    }
                    ZipEntry entry3 = new ZipEntry("AI\\NTai\\"+configfile3);

                    // Setup the entry data as required.

                    // Crc and size are handled by the library for seakable streams
                    // so no need to do them here.

                    // Could also use the last write time or similar for the file.
                    entry3.DateTime = DateTime.Now;
                    s.PutNextEntry(entry3);

                    using (FileStream fs = File.OpenRead(configfile3))
                    {

                        // Using a fixed size buffer here makes no noticeable difference for output
                        // but keeps a lid on memory usage.
                        int sourceBytes;
                        do
                        {
                            sourceBytes = fs.Read(buffer, 0, buffer.Length);
                            s.Write(buffer, 0, sourceBytes);
                        } while (sourceBytes > 0);
                    }
				    //}
    				
				    // Finish/Close arent needed strictly as the using statement does this automatically
    				
				    // Finish is important to ensure trailing information for a Zip file is appended.  Without this
				    // the created file would be invalid.
				    s.Finish();
    				
				    // Close is important to wrap things up and unlock the file.
				    s.Close();
			    }
		    }
		    catch(Exception ex)
		    {
			    Console.WriteLine("Exception during processing {0}", ex);
    			
			    // No need to rethrow the exception as for our purposes its handled.
		    }

        }

        private void label7_Click(object sender, EventArgs e)
        {

        }

        private void label56_Click(object sender, EventArgs e)
        {

        }

        private void StartTab_Click(object sender, EventArgs e)
        {

        }

        private void Antistall_CheckedChanged(object sender, EventArgs e)
        {

        }
    }
}


