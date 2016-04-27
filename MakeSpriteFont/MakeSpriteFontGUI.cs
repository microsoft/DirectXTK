using System;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

namespace MakeSpriteFont
{
	public partial class FormMakeSpriteFontGui : Form
	{
		private const string SpriteFontOutputFilename = "myfile.spritefont";

		private const string InitialFontNameComicSansMs = "Comic Sans MS";

		private Font SpriteFontFromForm { get; set; }

		public FormMakeSpriteFontGui()
		{
			InitializeComponent();

			InitializeForm();
		}

		#region Logic

		/// <summary>
		/// Initialize the form with the default values from the <see cref="CommandLineOptions"/> object
		/// </summary>
		private void InitializeForm()
		{
			CommandLineOptions commandLineOptions = new CommandLineOptions();

			SpriteFontFromForm = new Font(InitialFontNameComicSansMs, commandLineOptions.FontSize, commandLineOptions.FontStyle);

			TextBoxFont.Text = SpriteFontFromForm.Name;

			TextBoxOutputFilename.Text = SpriteFontOutputFilename;

			TextBoxCharacterRegionBegin.Text = string.Empty;

			NumericLineSpacing.Value = (int)commandLineOptions.CharacterSpacing;

			NumericLineSpacing.Value = (int)commandLineOptions.LineSpacing;

			CheckBoxSharpFont.Checked = commandLineOptions.Sharp;

			CheckBoxFastPack.Checked = commandLineOptions.FastPack;

			ComboBoxTextureFormat.DataSource = Enum.GetValues(typeof(TextureFormat));

			ComboBoxTextureFormat.SelectedItem = TextureFormat.Auto;
		}

		/// <summary>
		/// Get the <see cref="CommandLineOptions"/> from the form
		/// </summary>
		/// <returns><see cref="CommandLineOptions"/> from the form</returns>
		private CommandLineOptions GetCommandLineOptionsFromForm()
		{
			CommandLineOptions retVal = new CommandLineOptions();

			retVal.SourceFont = SpriteFontFromForm.Name;

			retVal.FontSize = SpriteFontFromForm.Size;

			retVal.FontStyle = SpriteFontFromForm.Style;

			retVal.OutputFile = TextBoxOutputFilename.Text;

			CharacterRegion characterRegion = GetCharacterRegionFromForm();

			if (characterRegion != null)
			{
				retVal.CharacterRegions.Add(characterRegion);
			}

			retVal.LineSpacing = (float)NumericLineSpacing.Value;

			retVal.CharacterSpacing = (float)NumericCharacterSpacing.Value;

			retVal.Sharp = CheckBoxSharpFont.Checked;

			retVal.FastPack = CheckBoxFastPack.Checked;

			retVal.TextureFormat = (TextureFormat)ComboBoxTextureFormat.SelectedItem;

			return retVal;
		}

		/// <summary>
		/// Get the <see cref="CharacterRegion"/> from the form
		/// </summary>
		/// <returns><see cref="CharacterRegion"/>from the form</returns>
		private CharacterRegion GetCharacterRegionFromForm()
		{
			CharacterRegion retVal = null;

			if (TextBoxCharacterRegionBegin.Text.Trim().Length == 1 && TextBoxCharacterRegionEnd.Text.Trim().Length == 1)
			{
				retVal = new CharacterRegion(TextBoxCharacterRegionBegin.Text.First(), TextBoxCharacterRegionEnd.Text.First());
			}

			return retVal;
		}

		#endregion

		#region Events

		/// <summary>
		/// Reset the form with the default values from the <see cref="CommandLineOptions"/> object
		/// </summary>
		/// <param name="sender">The sender from the event</param>
		/// <param name="e">The event param</param>
		private void ButtonReset_Click(object sender, EventArgs e)
		{
			InitializeForm();
		}

		/// <summary>
		/// Create the sprite font with the given values from the form
		/// </summary>
		/// <param name="sender">The sender from the event</param>
		/// <param name="e">The event param</param>
		private void ButtonCreateSpriteFont_Click(object sender, EventArgs e)
		{
			try
			{
				Program.MakeSpriteFont(GetCommandLineOptionsFromForm());
				MessageBox.Show(
					string.Format("sprite font object with the name [{0}] created successfully.: ", TextBoxOutputFilename.Text),
					"Spritefont created", MessageBoxButtons.OK, MessageBoxIcon.Information);
			}
			catch (Exception exception)
			{
				MessageBox.Show(exception.Message, "Error creating sprite font", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
		}

		/// <summary>
		/// Select a font to make a sprite font from
		/// </summary>
		/// <param name="sender">The sender from the event</param>
		/// <param name="e">The event param</param>
		private void ButtonSelectFont_Click(object sender, EventArgs e)
		{
			FontDialog fontDialog = new FontDialog {Font = SpriteFontFromForm};

			if (fontDialog.ShowDialog() == DialogResult.OK)
			{
				SpriteFontFromForm = fontDialog.Font;
				TextBoxFont.Text = fontDialog.Font.Name;
			}
		}

		#endregion
	}
}
