namespace CSharpAI
{
    partial class ControlPanel
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.radiobuttonTankRush = new System.Windows.Forms.RadioButton();
            this.radioButtonRaider = new System.Windows.Forms.RadioButton();
            this.radioButtonDefendOnly = new System.Windows.Forms.RadioButton();
            this.groupPlayStyle = new System.Windows.Forms.GroupBox();
            this.labelnumScouts = new System.Windows.Forms.Label();
            this.labelNumTanks = new System.Windows.Forms.Label();
            this.labelNumFactories = new System.Windows.Forms.Label();
            this.textboxlogarea = new System.Windows.Forms.TextBox();
            this.groupPlayStyle.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(24, 16);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(443, 12);
            this.label1.TabIndex = 0;
            this.label1.Text = "This control panel is just a prototype for now really.  Watch this space.";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(24, 176);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(89, 12);
            this.label2.TabIndex = 1;
            this.label2.Text = "Number scouts:";
            this.label2.Visible = false;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(24, 144);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(77, 12);
            this.label3.TabIndex = 2;
            this.label3.Text = "Chat history";
            this.label3.Visible = false;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(24, 208);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(83, 12);
            this.label4.TabIndex = 1;
            this.label4.Text = "Number tanks:";
            this.label4.Visible = false;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(24, 240);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(107, 12);
            this.label5.TabIndex = 3;
            this.label5.Text = "Number factories:";
            this.label5.Visible = false;
            // 
            // radiobuttonTankRush
            // 
            this.radiobuttonTankRush.AutoSize = true;
            this.radiobuttonTankRush.Checked = true;
            this.radiobuttonTankRush.Location = new System.Drawing.Point(8, 24);
            this.radiobuttonTankRush.Name = "radiobuttonTankRush";
            this.radiobuttonTankRush.Size = new System.Drawing.Size(131, 16);
            this.radiobuttonTankRush.TabIndex = 4;
            this.radiobuttonTankRush.TabStop = true;
            this.radiobuttonTankRush.Text = "TankRush playstyle";
            this.radiobuttonTankRush.UseVisualStyleBackColor = true;
            this.radiobuttonTankRush.CheckedChanged += new System.EventHandler(this.radiobuttonTankRush_CheckedChanged);
            // 
            // radioButtonRaider
            // 
            this.radioButtonRaider.AutoSize = true;
            this.radioButtonRaider.Location = new System.Drawing.Point(8, 48);
            this.radioButtonRaider.Name = "radioButtonRaider";
            this.radioButtonRaider.Size = new System.Drawing.Size(119, 16);
            this.radioButtonRaider.TabIndex = 5;
            this.radioButtonRaider.Text = "Raider playstyle";
            this.radioButtonRaider.UseVisualStyleBackColor = true;
            this.radioButtonRaider.CheckedChanged += new System.EventHandler(this.radioButtonRaider_CheckedChanged);
            // 
            // radioButtonDefendOnly
            // 
            this.radioButtonDefendOnly.AutoSize = true;
            this.radioButtonDefendOnly.Location = new System.Drawing.Point(8, 72);
            this.radioButtonDefendOnly.Name = "radioButtonDefendOnly";
            this.radioButtonDefendOnly.Size = new System.Drawing.Size(305, 16);
            this.radioButtonDefendOnly.TabIndex = 6;
            this.radioButtonDefendOnly.Text = "DefendOnly playstyle (in progress, not working)";
            this.radioButtonDefendOnly.UseVisualStyleBackColor = true;
            this.radioButtonDefendOnly.CheckedChanged += new System.EventHandler(this.radioButtonDefendOnly_CheckedChanged);
            // 
            // groupPlayStyle
            // 
            this.groupPlayStyle.Controls.Add(this.radiobuttonTankRush);
            this.groupPlayStyle.Controls.Add(this.radioButtonDefendOnly);
            this.groupPlayStyle.Controls.Add(this.radioButtonRaider);
            this.groupPlayStyle.Location = new System.Drawing.Point(24, 32);
            this.groupPlayStyle.Name = "groupPlayStyle";
            this.groupPlayStyle.Size = new System.Drawing.Size(440, 100);
            this.groupPlayStyle.TabIndex = 7;
            this.groupPlayStyle.TabStop = false;
            this.groupPlayStyle.Text = "Playstyle";
            // 
            // labelnumScouts
            // 
            this.labelnumScouts.AutoSize = true;
            this.labelnumScouts.Location = new System.Drawing.Point(144, 176);
            this.labelnumScouts.Name = "labelnumScouts";
            this.labelnumScouts.Size = new System.Drawing.Size(41, 12);
            this.labelnumScouts.TabIndex = 8;
            this.labelnumScouts.Text = "label6";
            this.labelnumScouts.Visible = false;
            // 
            // labelNumTanks
            // 
            this.labelNumTanks.AutoSize = true;
            this.labelNumTanks.Location = new System.Drawing.Point(144, 208);
            this.labelNumTanks.Name = "labelNumTanks";
            this.labelNumTanks.Size = new System.Drawing.Size(41, 12);
            this.labelNumTanks.TabIndex = 9;
            this.labelNumTanks.Text = "label6";
            this.labelNumTanks.Visible = false;
            // 
            // labelNumFactories
            // 
            this.labelNumFactories.AutoSize = true;
            this.labelNumFactories.Location = new System.Drawing.Point(144, 240);
            this.labelNumFactories.Name = "labelNumFactories";
            this.labelNumFactories.Size = new System.Drawing.Size(41, 12);
            this.labelNumFactories.TabIndex = 10;
            this.labelNumFactories.Text = "label6";
            this.labelNumFactories.Visible = false;
            // 
            // textboxlogarea
            // 
            this.textboxlogarea.AcceptsReturn = true;
            this.textboxlogarea.Location = new System.Drawing.Point(16, 168);
            this.textboxlogarea.Multiline = true;
            this.textboxlogarea.Name = "textboxlogarea";
            this.textboxlogarea.ReadOnly = true;
            this.textboxlogarea.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textboxlogarea.Size = new System.Drawing.Size(432, 200);
            this.textboxlogarea.TabIndex = 11;
            // 
            // ControlPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(474, 384);
            this.Controls.Add(this.textboxlogarea);
            this.Controls.Add(this.labelNumFactories);
            this.Controls.Add(this.labelNumTanks);
            this.Controls.Add(this.labelnumScouts);
            this.Controls.Add(this.groupPlayStyle);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Name = "ControlPanel";
            this.Text = "CSAI Control Panel";
            this.Load += new System.EventHandler(this.TestForm_Load);
            this.groupPlayStyle.ResumeLayout(false);
            this.groupPlayStyle.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        public System.Windows.Forms.RadioButton radiobuttonTankRush;
        public System.Windows.Forms.RadioButton radioButtonRaider;
        public System.Windows.Forms.RadioButton radioButtonDefendOnly;
        public System.Windows.Forms.GroupBox groupPlayStyle;
        public System.Windows.Forms.Label labelnumScouts;
        public System.Windows.Forms.Label labelNumTanks;
        public System.Windows.Forms.Label labelNumFactories;
        public System.Windows.Forms.TextBox textboxlogarea;
    }
}