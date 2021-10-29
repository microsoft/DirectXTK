// DirectXTK MakeSpriteFont tool
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

using System;
using System.IO;
using System.Linq;
using System.Drawing;

namespace MakeSpriteFont
{
    public class Program
    {
        public static int Main(string[] args)
        {
            // Parse the commandline options.
            var options = new CommandLineOptions();
            var parser = new CommandLineParser(options);

            if (!parser.ParseCommandLine(args))
                return 1;

            try
            {
                // Convert the font.
                MakeSpriteFont(options);
                
                return 0;
            }
            catch (Exception e)
            {
                // Print an error message if conversion failed.
                Console.WriteLine();
                Console.Error.WriteLine("Error: {0}", e.Message);
     
                return 1;
            }
        }


        static void MakeSpriteFont(CommandLineOptions options)
        {
            // Import.
            Console.WriteLine("Importing {0}", options.SourceFont);

            float lineSpacing;

            Glyph[] glyphs = ImportFont(options, out lineSpacing);

            Console.WriteLine("Captured {0} glyphs", glyphs.Length);

            // Optimize.
            Console.WriteLine("Cropping glyph borders");

            foreach (Glyph glyph in glyphs)
            {
                GlyphCropper.Crop(glyph);
            }

            Console.WriteLine("Packing glyphs into sprite sheet");

            Bitmap bitmap;

            if (options.FastPack)
            {
                bitmap = GlyphPacker.ArrangeGlyphsFast(glyphs);
            }
            else
            {
                bitmap = GlyphPacker.ArrangeGlyphs(glyphs);
            }

            // Emit texture size warning based on known Feature Level limits.
            if (bitmap.Width > 16384 || bitmap.Height > 16384)
            {
                Console.WriteLine("WARNING: Resulting texture is too large for all known Feature Levels (9.1 - 12.2)");
            }
            else if (bitmap.Width > 8192 || bitmap.Height > 8192)
            {
                if (options.FeatureLevel < FeatureLevel.FL11_0)
                {
                    Console.WriteLine("WARNING: Resulting texture requires a Feature Level 11.0 or later device.");
                }
            }
            else if (bitmap.Width > 4096 || bitmap.Height > 4096)
            {
                if (options.FeatureLevel < FeatureLevel.FL10_0)
                {
                    Console.WriteLine("WARNING: Resulting texture requires a Feature Level 10.0 or later device.");
                }
            }
            else if (bitmap.Width > 2048 || bitmap.Height > 2048)
            {
                if (options.FeatureLevel < FeatureLevel.FL9_3)
                {
                    Console.WriteLine("WARNING: Resulting texture requires a Feature Level 9.3 or later device.");
                }
            }

            // Adjust line and character spacing.
            lineSpacing += options.LineSpacing;

            foreach (Glyph glyph in glyphs)
            {
                glyph.XAdvance += options.CharacterSpacing;
            }

            // Automatically detect whether this is a monochromatic or color font?
            if (options.TextureFormat == TextureFormat.Auto)
            {
                bool isMono = BitmapUtils.IsRgbEntirely(Color.White, bitmap);

                options.TextureFormat = isMono ? TextureFormat.CompressedMono :
                                                 TextureFormat.Rgba32;
            }

            // Convert to premultiplied alpha format.
            if (!options.NoPremultiply)
            {
                Console.WriteLine("Premultiplying alpha");

                BitmapUtils.PremultiplyAlpha(bitmap);
            }

            // Save output files.
            if (!string.IsNullOrEmpty(options.DebugOutputSpriteSheet))
            {
                Console.WriteLine("Saving debug output spritesheet {0}", options.DebugOutputSpriteSheet);

                bitmap.Save(options.DebugOutputSpriteSheet);
            }

            Console.WriteLine("Writing {0} ({1} format)", options.OutputFile, options.TextureFormat);

            SpriteFontWriter.WriteSpriteFont(options, glyphs, lineSpacing, bitmap);
        }


        static Glyph[] ImportFont(CommandLineOptions options, out float lineSpacing)
        {
            // Which importer knows how to read this source font?
            IFontImporter importer;

            string fileExtension = Path.GetExtension(options.SourceFont).ToLowerInvariant();

            string[] BitmapFileExtensions = { ".bmp", ".png", ".gif" };

            if (BitmapFileExtensions.Contains(fileExtension))
            {
                importer = new BitmapImporter();
            }
            else
            {
                importer = new TrueTypeImporter();
            }

            // Import the source font data.
            importer.Import(options);

            lineSpacing = importer.LineSpacing;

            var glyphs = importer.Glyphs
                                 .OrderBy(glyph => glyph.Character)
                                 .ToArray();

            // Validate.
            if (glyphs.Length == 0)
            {
                throw new Exception("Font does not contain any glyphs.");
            }

            if ((options.DefaultCharacter != 0) && !glyphs.Any(glyph => glyph.Character == options.DefaultCharacter))
            {
                throw new Exception("The specified DefaultCharacter is not part of this font.");
            }

            return glyphs;
        }
    }
}
