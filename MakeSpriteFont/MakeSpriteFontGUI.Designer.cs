namespace MakeSpriteFont
{
	partial class FormMakeSpriteFontGUI
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
			this.ButtonReset = new System.Windows.Forms.Button();
			this.TextBoxFont = new System.Windows.Forms.TextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.ButtonSelectFont = new System.Windows.Forms.Button();
			this.label2 = new System.Windows.Forms.Label();
			this.TextBoxOutputFilename = new System.Windows.Forms.TextBox();
			this.label3 = new System.Windows.Forms.Label();
			this.TextBoxCharacterRegion = new System.Windows.Forms.TextBox();
			this.label4 = new System.Windows.Forms.Label();
			this.label5 = new System.Windows.Forms.Label();
			this.NumericLineSpacing = new System.Windows.Forms.NumericUpDown();
			this.NumericCharacterSpacing = new System.Windows.Forms.NumericUpDown();
			this.CheckBoxSharpFont = new System.Windows.Forms.CheckBox();
			this.CheckBoxFastPack = new System.Windows.Forms.CheckBox();
			this.groupBox1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.NumericLineSpacing)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.NumericCharacterSpacing)).BeginInit();
			this.SuspendLayout();
			// 
			// ButtonCreateSpriteFont
			// 
			this.ButtonCreateSpriteFont.Location = new System.Drawing.Point(306, 168);
			this.ButtonCreateSpriteFont.Name = "ButtonCreateSpriteFont";
			this.ButtonCreateSpriteFont.Size = new System.Drawing.Size(144, 41);
			this.ButtonCreateSpriteFont.TabIndex = 1;
			this.ButtonCreateSpriteFont.Text = "Create Spritefont";
			this.ButtonCreateSpriteFont.UseVisualStyleBackColor = true;
			// 
			// groupBox1
			// 
			this.groupBox1.BackColor = System.Drawing.Color.Transparent;
			this.groupBox1.Controls.Add(this.CheckBoxFastPack);
			this.groupBox1.Controls.Add(this.CheckBoxSharpFont);
			this.groupBox1.Controls.Add(this.NumericCharacterSpacing);
			this.groupBox1.Controls.Add(this.NumericLineSpacing);
			this.groupBox1.Controls.Add(this.label5);
			this.groupBox1.Controls.Add(this.label4);
			this.groupBox1.Controls.Add(this.label3);
			this.groupBox1.Controls.Add(this.TextBoxCharacterRegion);
			this.groupBox1.Controls.Add(this.label2);
			this.groupBox1.Controls.Add(this.TextBoxOutputFilename);
			this.groupBox1.Controls.Add(this.ButtonSelectFont);
			this.groupBox1.Controls.Add(this.label1);
			this.groupBox1.Controls.Add(this.TextBoxFont);
			this.groupBox1.Controls.Add(this.ButtonReset);
			this.groupBox1.Controls.Add(this.ButtonCreateSpriteFont);
			this.groupBox1.Location = new System.Drawing.Point(12, 12);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(560, 224);
			this.groupBox1.TabIndex = 2;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Fill the form to create a new spritefont object";
			// 
			// ButtonReset
			// 
			this.ButtonReset.Location = new System.Drawing.Point(6, 168);
			this.ButtonReset.Name = "ButtonReset";
			this.ButtonReset.Size = new System.Drawing.Size(144, 41);
			this.ButtonReset.TabIndex = 2;
			this.ButtonReset.Text = "Reset";
			this.ButtonReset.UseVisualStyleBackColor = true;
			// 
			// TextBoxFont
			// 
			this.TextBoxFont.Location = new System.Drawing.Point(120, 30);
			this.TextBoxFont.Name = "TextBoxFont";
			this.TextBoxFont.Size = new System.Drawing.Size(300, 20);
			this.TextBoxFont.TabIndex = 1;
			this.TextBoxFont.Text = "Comic Sans MS";
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
			// ButtonSelectFont
			// 
			this.ButtonSelectFont.Location = new System.Drawing.Point(434, 19);
			this.ButtonSelectFont.Name = "ButtonSelectFont";
			this.ButtonSelectFont.Size = new System.Drawing.Size(109, 33);
			this.ButtonSelectFont.TabIndex = 5;
			this.ButtonSelectFont.Text = "Select Font";
			this.ButtonSelectFont.UseVisualStyleBackColor = true;
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
			this.TextBoxOutputFilename.Location = new System.Drawing.Point(120, 56);
			this.TextBoxOutputFilename.Name = "TextBoxOutputFilename";
			this.TextBoxOutputFilename.Size = new System.Drawing.Size(300, 20);
			this.TextBoxOutputFilename.TabIndex = 2;
			this.TextBoxOutputFilename.Text = "myfile.spritefont";
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(19, 88);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(88, 13);
			this.label3.TabIndex = 9;
			this.label3.Text = "Character region:";
			// 
			// TextBoxCharacterRegion
			// 
			this.TextBoxCharacterRegion.Location = new System.Drawing.Point(120, 83);
			this.TextBoxCharacterRegion.Name = "TextBoxCharacterRegion";
			this.TextBoxCharacterRegion.Size = new System.Drawing.Size(300, 20);
			this.TextBoxCharacterRegion.TabIndex = 8;
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
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(19, 140);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(96, 13);
			this.label5.TabIndex = 13;
			this.label5.Text = "Character spacing:";
			// 
			// NumericLineSpacing
			// 
			this.NumericLineSpacing.Location = new System.Drawing.Point(120, 111);
			this.NumericLineSpacing.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
			this.NumericLineSpacing.Name = "NumericLineSpacing";
			this.NumericLineSpacing.Size = new System.Drawing.Size(51, 20);
			this.NumericLineSpacing.TabIndex = 14;
			// 
			// NumericCharacterSpacing
			// 
			this.NumericCharacterSpacing.Location = new System.Drawing.Point(120, 137);
			this.NumericCharacterSpacing.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
			this.NumericCharacterSpacing.Name = "NumericCharacterSpacing";
			this.NumericCharacterSpacing.Size = new System.Drawing.Size(51, 20);
			this.NumericCharacterSpacing.TabIndex = 15;
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
			// FormMakeSpriteFontGUI
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(584, 251);
			this.Controls.Add(this.groupBox1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.MaximizeBox = false;
			this.Name = "FormMakeSpriteFontGUI";
			this.Text = "MakeSpriteFontGUI";
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.NumericLineSpacing)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.NumericCharacterSpacing)).EndInit();
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
		private System.Windows.Forms.TextBox TextBoxCharacterRegion;
		private System.Windows.Forms.NumericUpDown NumericCharacterSpacing;
		private System.Windows.Forms.NumericUpDown NumericLineSpacing;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.Label label4;
		public System.Windows.Forms.CheckBox CheckBoxFastPack;
		public System.Windows.Forms.CheckBox CheckBoxSharpFont;
	}
}