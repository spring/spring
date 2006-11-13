using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace CSharpAI
{
    public partial class ControlPanel : Form
    {
        public ControlPanel()
        {
            InitializeComponent();
        }

        private void TestForm_Load(object sender, EventArgs e)
        {

        }

        private void radiobuttonTankRush_CheckedChanged(object sender, EventArgs e)
        {
            PlayStyleManager.GetInstance().ChoosePlayStyle("tankrush");
        }

        private void radioButtonRaider_CheckedChanged(object sender, EventArgs e)
        {
            PlayStyleManager.GetInstance().ChoosePlayStyle("raider");

        }

        private void radioButtonDefendOnly_CheckedChanged(object sender, EventArgs e)
        {
            PlayStyleManager.GetInstance().ChoosePlayStyle("defendonly");

        }

    }
}