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
			this.label7 = new System.Windows.Forms.Label();
			this.TextBoxCharacterRegionEnd = new System.Windows.Forms.TextBox();
			this.label6 = new System.Windows.Forms.Label();
			this.ComboBoxTextureFormat = new System.Windows.Forms.ComboBox();
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
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.TextBoxFontSize = new System.Windows.Forms.TextBox();
			this.label8 = new System.Windows.Forms.Label();
			this.TextBoxFontStyle = new System.Windows.Forms.TextBox();
			this.label9 = new System.Windows.Forms.Label();
			this.groupBox3 = new System.Windows.Forms.GroupBox();
			this.groupBox1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.NumericCharacterSpacing)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.NumericLineSpacing)).BeginInit();
			this.groupBox2.SuspendLayout();
			this.groupBox3.SuspendLayout();
			this.SuspendLayout();
			// 
			// ButtonCreateSpriteFont
			// 
			this.ButtonCreateSpriteFont.Location = new System.Drawing.Point(506, 332);
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
			this.groupBox1.Controls.Add(this.groupBox3);
			this.groupBox1.Controls.Add(this.groupBox2);
			this.groupBox1.Controls.Add(this.ButtonReset);
			this.groupBox1.Controls.Add(this.ButtonCreateSpriteFont);
			this.groupBox1.Location = new System.Drawing.Point(12, 12);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(741, 412);
			this.groupBox1.TabIndex = 2;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Fill the form to create a new spritefont object";
			// 
			// label7
			// 
			this.label7.AutoSize = true;
			this.label7.Location = new System.Drawing.Point(8, 88);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(109, 13);
			this.label7.TabIndex = 21;
			this.label7.Text = "Character region end:";
			// 
			// TextBoxCharacterRegionEnd
			// 
			this.TextBoxCharacterRegionEnd.Location = new System.Drawing.Point(131, 85);
			this.TextBoxCharacterRegionEnd.Name = "TextBoxCharacterRegionEnd";
			this.TextBoxCharacterRegionEnd.Size = new System.Drawing.Size(151, 20);
			this.TextBoxCharacterRegionEnd.TabIndex = 20;
			// 
			// label6
			// 
			this.label6.AutoSize = true;
			this.label6.Location = new System.Drawing.Point(8, 187);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(78, 13);
			this.label6.TabIndex = 19;
			this.label6.Text = "Texture format:";
			// 
			// ComboBoxTextureFormat
			// 
			this.ComboBoxTextureFormat.FormattingEnabled = true;
			this.ComboBoxTextureFormat.Location = new System.Drawing.Point(131, 184);
			this.ComboBoxTextureFormat.Name = "ComboBoxTextureFormat";
			this.ComboBoxTextureFormat.Size = new System.Drawing.Size(151, 21);
			this.ComboBoxTextureFormat.TabIndex = 18;
			// 
			// CheckBoxFastPack
			// 
			this.CheckBoxFastPack.AutoSize = true;
			this.CheckBoxFastPack.Location = new System.Drawing.Point(131, 248);
			this.CheckBoxFastPack.Name = "CheckBoxFastPack";
			this.CheckBoxFastPack.Size = new System.Drawing.Size(129, 17);
			this.CheckBoxFastPack.TabIndex = 17;
			this.CheckBoxFastPack.Text = "FastPack (large fonts)";
			this.CheckBoxFastPack.UseVisualStyleBackColor = true;
			// 
			// CheckBoxSharpFont
			// 
			this.CheckBoxSharpFont.AutoSize = true;
			this.CheckBoxSharpFont.Location = new System.Drawing.Point(131, 218);
			this.CheckBoxSharpFont.Name = "CheckBoxSharpFont";
			this.CheckBoxSharpFont.Size = new System.Drawing.Size(75, 17);
			this.CheckBoxSharpFont.TabIndex = 16;
			this.CheckBoxSharpFont.Text = "Sharp font";
			this.CheckBoxSharpFont.UseVisualStyleBackColor = true;
			// 
			// NumericCharacterSpacing
			// 
			this.NumericCharacterSpacing.Location = new System.Drawing.Point(131, 151);
			this.NumericCharacterSpacing.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
			this.NumericCharacterSpacing.Name = "NumericCharacterSpacing";
			this.NumericCharacterSpacing.Size = new System.Drawing.Size(151, 20);
			this.NumericCharacterSpacing.TabIndex = 15;
			// 
			// NumericLineSpacing
			// 
			this.NumericLineSpacing.Location = new System.Drawing.Point(131, 118);
			this.NumericLineSpacing.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
			this.NumericLineSpacing.Name = "NumericLineSpacing";
			this.NumericLineSpacing.Size = new System.Drawing.Size(151, 20);
			this.NumericLineSpacing.TabIndex = 14;
			// 
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(8, 153);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(96, 13);
			this.label5.TabIndex = 13;
			this.label5.Text = "Character spacing:";
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Location = new System.Drawing.Point(8, 120);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(70, 13);
			this.label4.TabIndex = 11;
			this.label4.Text = "Line spacing:";
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(8, 55);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(117, 13);
			this.label3.TabIndex = 9;
			this.label3.Text = "Character region begin:";
			// 
			// TextBoxCharacterRegionBegin
			// 
			this.TextBoxCharacterRegionBegin.Location = new System.Drawing.Point(131, 52);
			this.TextBoxCharacterRegionBegin.Name = "TextBoxCharacterRegionBegin";
			this.TextBoxCharacterRegionBegin.Size = new System.Drawing.Size(151, 20);
			this.TextBoxCharacterRegionBegin.TabIndex = 8;
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(8, 22);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(84, 13);
			this.label2.TabIndex = 7;
			this.label2.Text = "Output filename:";
			// 
			// TextBoxOutputFilename
			// 
			this.TextBoxOutputFilename.Location = new System.Drawing.Point(131, 19);
			this.TextBoxOutputFilename.Name = "TextBoxOutputFilename";
			this.TextBoxOutputFilename.Size = new System.Drawing.Size(151, 20);
			this.TextBoxOutputFilename.TabIndex = 2;
			this.TextBoxOutputFilename.Text = "myfile.spritefont";
			// 
			// ButtonSelectFont
			// 
			this.ButtonSelectFont.Location = new System.Drawing.Point(15, 232);
			this.ButtonSelectFont.Name = "ButtonSelectFont";
			this.ButtonSelectFont.Size = new System.Drawing.Size(294, 33);
			this.ButtonSelectFont.TabIndex = 5;
			this.ButtonSelectFont.Text = "Select Font";
			this.ButtonSelectFont.UseVisualStyleBackColor = true;
			this.ButtonSelectFont.Click += new System.EventHandler(this.ButtonSelectFont_Click);
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(12, 22);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(31, 13);
			this.label1.TabIndex = 4;
			this.label1.Text = "Font:";
			// 
			// TextBoxFont
			// 
			this.TextBoxFont.Location = new System.Drawing.Point(81, 19);
			this.TextBoxFont.Name = "TextBoxFont";
			this.TextBoxFont.ReadOnly = true;
			this.TextBoxFont.Size = new System.Drawing.Size(222, 20);
			this.TextBoxFont.TabIndex = 1;
			this.TextBoxFont.Text = "Comic Sans MS";
			// 
			// ButtonReset
			// 
			this.ButtonReset.Location = new System.Drawing.Point(75, 347);
			this.ButtonReset.Name = "ButtonReset";
			this.ButtonReset.Size = new System.Drawing.Size(144, 41);
			this.ButtonReset.TabIndex = 2;
			this.ButtonReset.Text = "Reset";
			this.ButtonReset.UseVisualStyleBackColor = true;
			this.ButtonReset.Click += new System.EventHandler(this.ButtonReset_Click);
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.TextBoxFontStyle);
			this.groupBox2.Controls.Add(this.label9);
			this.groupBox2.Controls.Add(this.TextBoxFontSize);
			this.groupBox2.Controls.Add(this.label8);
			this.groupBox2.Controls.Add(this.TextBoxFont);
			this.groupBox2.Controls.Add(this.label1);
			this.groupBox2.Controls.Add(this.ButtonSelectFont);
			this.groupBox2.Location = new System.Drawing.Point(347, 27);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(309, 275);
			this.groupBox2.TabIndex = 22;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Select a font";
			// 
			// TextBoxFontSize
			// 
			this.TextBoxFontSize.Location = new System.Drawing.Point(81, 52);
			this.TextBoxFontSize.Name = "TextBoxFontSize";
			this.TextBoxFontSize.ReadOnly = true;
			this.TextBoxFontSize.Size = new System.Drawing.Size(222, 20);
			this.TextBoxFontSize.TabIndex = 5;
			this.TextBoxFontSize.Text = "23";
			// 
			// label8
			// 
			this.label8.AutoSize = true;
			this.label8.Location = new System.Drawing.Point(12, 55);
			this.label8.Name = "label8";
			this.label8.Size = new System.Drawing.Size(52, 13);
			this.label8.TabIndex = 6;
			this.label8.Text = "Font size:";
			// 
			// TextBoxFontStyle
			// 
			this.TextBoxFontStyle.Location = new System.Drawing.Point(81, 85);
			this.TextBoxFontStyle.Name = "TextBoxFontStyle";
			this.TextBoxFontStyle.ReadOnly = true;
			this.TextBoxFontStyle.Size = new System.Drawing.Size(222, 20);
			this.TextBoxFontStyle.TabIndex = 7;
			this.TextBoxFontStyle.Text = "Regular";
			// 
			// label9
			// 
			this.label9.AutoSize = true;
			this.label9.Location = new System.Drawing.Point(12, 88);
			this.label9.Name = "label9";
			this.label9.Size = new System.Drawing.Size(55, 13);
			this.label9.TabIndex = 8;
			this.label9.Text = "Font style:";
			// 
			// groupBox3
			// 
			this.groupBox3.Controls.Add(this.TextBoxOutputFilename);
			this.groupBox3.Controls.Add(this.label2);
			this.groupBox3.Controls.Add(this.label7);
			this.groupBox3.Controls.Add(this.TextBoxCharacterRegionBegin);
			this.groupBox3.Controls.Add(this.TextBoxCharacterRegionEnd);
			this.groupBox3.Controls.Add(this.label3);
			this.groupBox3.Controls.Add(this.label6);
			this.groupBox3.Controls.Add(this.label4);
			this.groupBox3.Controls.Add(this.ComboBoxTextureFormat);
			this.groupBox3.Controls.Add(this.label5);
			this.groupBox3.Controls.Add(this.CheckBoxFastPack);
			this.groupBox3.Controls.Add(this.NumericLineSpacing);
			this.groupBox3.Controls.Add(this.CheckBoxSharpFont);
			this.groupBox3.Controls.Add(this.NumericCharacterSpacing);
			this.groupBox3.Location = new System.Drawing.Point(13, 27);
			this.groupBox3.Name = "groupBox3";
			this.groupBox3.Size = new System.Drawing.Size(311, 275);
			this.groupBox3.TabIndex = 23;
			this.groupBox3.TabStop = false;
			this.groupBox3.Text = "Spritefont settings";
			// 
			// FormMakeSpriteFontGui
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(762, 432);
			this.Controls.Add(this.groupBox1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.MaximizeBox = false;
			this.Name = "FormMakeSpriteFontGui";
			this.Text = "MakeSpriteFontGUI";
			this.Paint += new System.Windows.Forms.PaintEventHandler(this.FormMakeSpriteFont_Paint);
			this.groupBox1.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.NumericCharacterSpacing)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.NumericLineSpacing)).EndInit();
			this.groupBox2.ResumeLayout(false);
			this.groupBox2.PerformLayout();
			this.groupBox3.ResumeLayout(false);
			this.groupBox3.PerformLayout();
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
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.TextBox TextBoxFontStyle;
		private System.Windows.Forms.Label label9;
		private System.Windows.Forms.TextBox TextBoxFontSize;
		private System.Windows.Forms.Label label8;
		private System.Windows.Forms.GroupBox groupBox3;
	}
}