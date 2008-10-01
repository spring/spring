// From http://www.devhood.com/Tools/tool_details.aspx?tool_id=295

using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace ajma.Utils
{
	/// <summary>
	/// Summary description for InputBoxForm.
	/// </summary>
	internal class InputBoxForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button btnOK;
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Label lblText;
		private System.Windows.Forms.TextBox txtResult;
		private string strReturnValue;
		private Point pntStartLocation;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public InputBoxForm()
		{
			// Required for Windows Form Designer support
			InitializeComponent();
			this.strReturnValue = "";
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.btnOK = new System.Windows.Forms.Button();
			this.txtResult = new System.Windows.Forms.TextBox();
			this.lblText = new System.Windows.Forms.Label();
			this.btnCancel = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// btnOK
			// 
			this.btnOK.Location = new System.Drawing.Point(288, 8);
			this.btnOK.Name = "btnOK";
			this.btnOK.Size = new System.Drawing.Size(64, 24);
			this.btnOK.TabIndex = 0;
			this.btnOK.Text = "OK";
			this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
			// 
			// txtResult
			// 
			this.txtResult.Location = new System.Drawing.Point(8, 80);
			this.txtResult.Name = "txtResult";
			this.txtResult.Size = new System.Drawing.Size(344, 20);
			this.txtResult.TabIndex = 1;
			this.txtResult.Text = "";
			// 
			// lblText
			// 
			this.lblText.Location = new System.Drawing.Point(16, 8);
			this.lblText.Name = "lblText";
			this.lblText.Size = new System.Drawing.Size(264, 64);
			this.lblText.TabIndex = 2;
			this.lblText.Text = "InputBox";
			// 
			// btnCancel
			// 
			this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnCancel.Location = new System.Drawing.Point(288, 40);
			this.btnCancel.Name = "btnCancel";
			this.btnCancel.Size = new System.Drawing.Size(64, 24);
			this.btnCancel.TabIndex = 3;
			this.btnCancel.Text = "Cancel";
			this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
			// 
			// InputBoxForm
			// 
			this.AcceptButton = this.btnOK;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.btnCancel;
			this.ClientSize = new System.Drawing.Size(362, 111);
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.btnCancel,
																		  this.lblText,
																		  this.txtResult,
																		  this.btnOK});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "InputBoxForm";
			this.Text = "InputBox";
			this.Load += new System.EventHandler(this.InputBoxForm_Load);
			this.ResumeLayout(false);

		}
		#endregion

		private void InputBoxForm_Load(object sender, System.EventArgs e)
		{
			if (!this.pntStartLocation.IsEmpty) 
			{
				this.Top = this.pntStartLocation.X;
				this.Left = this.pntStartLocation.Y;
			}
		}

		private void btnOK_Click(object sender, System.EventArgs e)
		{
			this.strReturnValue = this.txtResult.Text;
			this.Close();
		}

		private void btnCancel_Click(object sender, System.EventArgs e)
		{
			this.Close();
		}

		public string Title
		{
			set
			{
				this.Text = value;
			}
		}

		public string Prompt
		{
			set
			{
				this.lblText.Text = value;
			}
		}

		public string ReturnValue
		{
			get
			{
				return strReturnValue;
			}
		}

		public string DefaultResponse
		{
			set
			{
				this.txtResult.Text = value;
				this.txtResult.SelectAll();
			}
		}

		public Point StartLocation
		{
			set
			{
				this.pntStartLocation = value;
			}
		}
	}
}
