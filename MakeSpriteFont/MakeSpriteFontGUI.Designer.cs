namespace MakeSpriteFont
{
	partial class FormMakeSpriteFontGui
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
			this.ButtonCreateSpriteFont = new System.Windows.Forms.Button();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.CheckBoxFastPack = new System.Windows.Forms.CheckBox();
			this.CheckBoxSharpFont = new System.Windows.Forms.CheckBox();
			this.NumericCharacterSpacing = new System.Windows.Forms.NumericUpDown();
			this.NumericLineSpacing = new System.Windows.Forms.NumericUpDown();
			this.label5 = new System.Windows.Forms.Label();
			this.label4 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.TextBoxCharacterRegionBegin = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.TextBoxOutputFilename = new System.Windows.Forms.TextBox();
			this.ButtonSelectFont = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.TextBoxFont = new System.Windows.Forms.TextBox();
			this.ButtonReset = new System.Windows.Forms.Button();
			this.ComboBoxTextureFormat = new System.Windows.Forms.ComboBox();
			this.label6 = new System.Windows.Forms.Label();
			this.label7 = new System.Windows.Forms.Label();
			this.TextBoxCharacterRegionEnd = new System.Windows.Forms.TextBox();
			this.groupBox1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.NumericCharacterSpacing)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.NumericLineSpacing)).BeginInit();
			this.SuspendLayout();
			// 
			// ButtonCreateSpriteFont
			// 
			this.ButtonCreateSpriteFont.Location = new System.Drawing.Point(306, 234);
			this.ButtonCreateSpriteFont.Name = "ButtonCreateSpriteFont";
			this.ButtonCreateSpriteFont.Size = new System.Drawing.Size(144, 41);
			this.ButtonCreateSpriteFont.TabIndex = 1;
			this.ButtonCreateSpriteFont.Text = "Create Spritefont";
			this.ButtonCreateSpriteFont.UseVisualStyleBackColor = true;
			this.ButtonCreateSpriteFont.Click += new System.EventHandler(this.ButtonCreateSpriteFont_Click);
			// 
			// groupBox1
			// 
			this.groupBox1.BackColor = System.Drawing.Color.Transparent;
			this.groupBox1.Controls.Add(this.label7);
			this.groupBox1.Controls.Add(this.TextBoxCharacterRegionEnd);
			this.groupBox1.Controls.Add(this.label6);
			this.groupBox1.Controls.Add(this.ComboBoxTextureFormat);
			this.groupBox1.Controls.Add(this.CheckBoxFastPack);
			this.groupBox1.Controls.Add(this.CheckBoxSharpFont);
			this.groupBox1.Controls.Add(this.NumericCharacterSpacing);
			this.groupBox1.Controls.Add(this.NumericLineSpacing);
			this.groupBox1.Controls.Add(this.label5);
			this.groupBox1.Controls.Add(this.label4);
			this.groupBox1.Controls.Add(this.label3);
			this.groupBox1.Controls.Add(this.TextBoxCharacterRegionBegin);
			this.groupBox1.Controls.Add(this.label2);
			this.groupBox1.Controls.Add(this.TextBoxOutputFilename);
			this.groupBox1.Controls.Add(this.ButtonSelectFont);
			this.groupBox1.Controls.Add(this.label1);
			this.groupBox1.Controls.Add(this.TextBoxFont);
			this.groupBox1.Controls.Add(this.ButtonReset);
			this.groupBox1.Controls.Add(this.ButtonCreateSpriteFont);
			this.groupBox1.Location = new System.Drawing.Point(12, 12);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(560, 299);
			this.groupBox1.TabIndex = 2;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Fill the form to create a new spritefont object";
			// 
			// CheckBoxFastPack
			// 
			this.CheckBoxFastPack.AutoSize = true;
			this.CheckBoxFastPack.Location = new System.Drawing.Point(306, 139);
			this.CheckBoxFastPack.Name = "CheckBoxFastPack";
			this.CheckBoxFastPack.Size = new System.Drawing.Size(129, 17);
			this.CheckBoxFastPack.TabIndex = 17;
			this.CheckBoxFastPack.Text = "FastPack (large fonts)";
			this.CheckBoxFastPack.UseVisualStyleBackColor = true;
			// 
			// CheckBoxSharpFont
			// 
			this.CheckBoxSharpFont.AutoSize = true;
			this.CheckBoxSharpFont.Location = new System.Drawing.Point(306, 114);
			this.CheckBoxSharpFont.Name = "CheckBoxSharpFont";
			this.CheckBoxSharpFont.Size = new System.Drawing.Size(75, 17);
			this.CheckBoxSharpFont.TabIndex = 16;
			this.CheckBoxSharpFont.Text = "Sharp font";
			this.CheckBoxSharpFont.UseVisualStyleBackColor = true;
			// 
			// NumericCharacterSpacing
			// 
			this.NumericCharacterSpacing.Location = new System.Drawing.Point(142, 137);
			this.NumericCharacterSpacing.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
			this.NumericCharacterSpacing.Name = "NumericCharacterSpacing";
			this.NumericCharacterSpacing.Size = new System.Drawing.Size(51, 20);
			this.NumericCharacterSpacing.TabIndex = 15;
			// 
			// NumericLineSpacing
			// 
			this.NumericLineSpacing.Location = new System.Drawing.Point(142, 111);
			this.NumericLineSpacing.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
			this.NumericLineSpacing.Name = "NumericLineSpacing";
			this.NumericLineSpacing.Size = new System.Drawing.Size(51, 20);
			this.NumericLineSpacing.TabIndex = 14;
			// 
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(19, 140);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(96, 13);
			this.label5.TabIndex = 13;
			this.label5.Text = "Character spacing:";
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Location = new System.Drawing.Point(19, 114);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(70, 13);
			this.label4.TabIndex = 11;
			this.label4.Text = "Line spacing:";
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(19, 88);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(117, 13);
			this.label3.TabIndex = 9;
			this.label3.Text = "Character region begin:";
			// 
			// TextBoxCharacterRegionBegin
			// 
			this.TextBoxCharacterRegionBegin.Location = new System.Drawing.Point(142, 83);
			this.TextBoxCharacterRegionBegin.Name = "TextBoxCharacterRegionBegin";
			this.TextBoxCharacterRegionBegin.Size = new System.Drawing.Size(51, 20);
			this.TextBoxCharacterRegionBegin.TabIndex = 8;
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(19, 59);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(84, 13);
			this.label2.TabIndex = 7;
			this.label2.Text = "Output filename:";
			// 
			// TextBoxOutputFilename
			// 
			this.TextBoxOutputFilename.Location = new System.Drawing.Point(142, 56);
			this.TextBoxOutputFilename.Name = "TextBoxOutputFilename";
			this.TextBoxOutputFilename.Size = new System.Drawing.Size(300, 20);
			this.TextBoxOutputFilename.TabIndex = 2;
			this.TextBoxOutputFilename.Text = "myfile.spritefont";
			// 
			// ButtonSelectFont
			// 
			this.ButtonSelectFont.Location = new System.Drawing.Point(462, 33);
			this.ButtonSelectFont.Name = "ButtonSelectFont";
			this.ButtonSelectFont.Size = new System.Drawing.Size(92, 33);
			this.ButtonSelectFont.TabIndex = 5;
			this.ButtonSelectFont.Text = "Select Font";
			this.ButtonSelectFont.UseVisualStyleBackColor = true;
			this.ButtonSelectFont.Click += new System.EventHandler(this.ButtonSelectFont_Click);
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(19, 33);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(31, 13);
			this.label1.TabIndex = 4;
			this.label1.Text = "Font:";
			// 
			// TextBoxFont
			// 
			this.TextBoxFont.Enabled = false;
			this.TextBoxFont.Location = new System.Drawing.Point(142, 30);
			this.TextBoxFont.Name = "TextBoxFont";
			this.TextBoxFont.Size = new System.Drawing.Size(300, 20);
			this.TextBoxFont.TabIndex = 1;
			this.TextBoxFont.Text = "Comic Sans MS";
			// 
			// ButtonReset
			// 
			this.ButtonReset.Location = new System.Drawing.Point(6, 234);
			this.ButtonReset.Name = "ButtonReset";
			this.ButtonReset.Size = new System.Drawing.Size(144, 41);
			this.ButtonReset.TabIndex = 2;
			this.ButtonReset.Text = "Reset";
			this.ButtonReset.UseVisualStyleBackColor = true;
			this.ButtonReset.Click += new System.EventHandler(this.ButtonReset_Click);
			// 
			// ComboBoxTextureFormat
			// 
			this.ComboBoxTextureFormat.FormattingEnabled = true;
			this.ComboBoxTextureFormat.Location = new System.Drawing.Point(142, 167);
			this.ComboBoxTextureFormat.Name = "ComboBoxTextureFormat";
			this.ComboBoxTextureFormat.Size = new System.Drawing.Size(151, 21);
			this.ComboBoxTextureFormat.TabIndex = 18;
			// 
			// label6
			// 
			this.label6.AutoSize = true;
			this.label6.Location = new System.Drawing.Point(18, 170);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(78, 13);
			this.label6.TabIndex = 19;
			this.label6.Text = "Texture format:";
			// 
			// label7
			// 
			this.label7.AutoSize = true;
			this.label7.Location = new System.Drawing.Point(303, 88);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(109, 13);
			this.label7.TabIndex = 21;
			this.label7.Text = "Character region end:";
			// 
			// TextBoxCharacterRegionEnd
			// 
			this.TextBoxCharacterRegionEnd.Location = new System.Drawing.Point(426, 83);
			this.TextBoxCharacterRegionEnd.Name = "TextBoxCharacterRegionEnd";
			this.TextBoxCharacterRegionEnd.Size = new System.Drawing.Size(51, 20);
			this.TextBoxCharacterRegionEnd.TabIndex = 20;
			// 
			// FormMakeSpriteFontGui
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(584, 394);
			this.Controls.Add(this.groupBox1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.MaximizeBox = false;
			this.Name = "FormMakeSpriteFontGui";
			this.Text = "MakeSpriteFontGUI";
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.NumericCharacterSpacing)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.NumericLineSpacing)).EndInit();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Button ButtonCreateSpriteFont;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Button ButtonReset;
		private System.Windows.Forms.Button ButtonSelectFont;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox TextBoxFont;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.TextBox TextBoxOutputFilename;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.TextBox TextBoxCharacterRegionBegin;
		private System.Windows.Forms.NumericUpDown NumericCharacterSpacing;
		private System.Windows.Forms.NumericUpDown NumericLineSpacing;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.Label label4;
		public System.Windows.Forms.CheckBox CheckBoxFastPack;
		public System.Windows.Forms.CheckBox CheckBoxSharpFont;
		private System.Windows.Forms.Label label6;
		private System.Windows.Forms.ComboBox ComboBoxTextureFormat;
		private System.Windows.Forms.Label label7;
		private System.Windows.Forms.TextBox TextBoxCharacterRegionEnd;
	}
}