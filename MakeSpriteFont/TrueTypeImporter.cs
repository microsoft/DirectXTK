// DirectXTK MakeSpriteFont tool
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Globalization;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

namespace MakeSpriteFont
{
    // Uses System.Drawing (aka GDI+) to rasterize TrueType fonts into a series of glyph bitmaps.
    public class TrueTypeImporter : IFontImporter
    {
        protected volatile int count;

        // Properties hold the imported font data.
        public IEnumerable<Glyph> Glyphs { get; private set; }

        public float LineSpacing { get; private set; }


        // Size of the temp surface used for GDI+ rasterization.
        const int MaxGlyphSize = 1024;

        struct ImportGlyphArgs
        {
            public Bitmap Bitmap { get; set; }
            public Brush Brush { get; set; }
            public Font Font { get; set; }
            public Graphics Graphics { get; set; }
            public StringFormat StringFormat { get; set; }
        }
        public void Import(CommandLineOptions options)
        {
            // Which characters do we want to include?
            var characters = CharacterRegion.Flatten(options.CharacterRegions).ToArray();

            var glyphList = new Glyph[characters.Count()];

            // Rasterize each character in turn.
            count = 0;

            if (glyphList.Length >= 500)
            {
                if (!options.FastPack)
                {
                    Console.WriteLine("WARNING: capturing a large font. This may take a long time to complete and could result in too large a texture. Consider using /FastPack.");
                }
            }

            using (var cancellationTokenSource = new CancellationTokenSource())
            {
                Task.Run(() =>
                {
                    var periods = 1;
                    while (true)
                    {
                        Thread.Sleep(TimeSpan.FromMilliseconds(89));
                        if (count >= periods * 500)
                        {
                            periods++;
                            Console.Write(".");
                        }
                        cancellationTokenSource.Token.ThrowIfCancellationRequested();
                    }
                }, cancellationTokenSource.Token);
                var partitioner = Partitioner.Create(0, characters.Count(), Math.Min(1, characters.Count() / Environment.ProcessorCount));
                Parallel.ForEach<Tuple<int, int>, ImportGlyphArgs>(
                    partitioner, () =>
                    {
                        // Create a bunch of GDI+ objects.
                        var local = new ImportGlyphArgs()
                        {
                            Bitmap = new Bitmap(MaxGlyphSize, MaxGlyphSize, PixelFormat.Format32bppArgb),
                            Brush = new SolidBrush(Color.White),
                            Font = CreateFont(options),
                            //StringFormat = new StringFormat(StringFormatFlags.NoFontFallback),
                            StringFormat = new StringFormat(),
                        };
                        var graphics = Graphics.FromImage(local.Bitmap);
                        graphics.PixelOffsetMode = options.Sharp ? PixelOffsetMode.None : PixelOffsetMode.HighQuality;
                        graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
                        graphics.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
                        local.Graphics = graphics;
                        return local;
                    }, (range, state, rangeIndex, local) =>
                    {
                        for (var i = range.Item1; i < range.Item2; i++)
                        {
                            count++;
                            Glyph glyph = ImportGlyph(characters[i], local.Font, local.Brush, local.StringFormat, local.Bitmap, local.Graphics);
                            glyphList[i] = glyph;
                        }
                        return local;
                    }, local =>
                    {
                        local.Bitmap.Dispose();
                        local.Brush.Dispose();
                        local.Font.Dispose();
                        local.Graphics.Dispose();
                        local.StringFormat.Dispose();
                    });
                cancellationTokenSource.Cancel();
            }

            if (count > 500)
            {
                Console.WriteLine();
            }

            Glyphs = glyphList;
            using (Font font = CreateFont(options))
            {
                // Store the font height.
                LineSpacing = font.GetHeight();
            }
        }


        // Attempts to instantiate the requested GDI+ font object.
        static Font CreateFont(CommandLineOptions options)
        {
            Font font = new Font(options.SourceFont, PointsToPixels(options.FontSize), options.FontStyle, GraphicsUnit.Pixel);

            try
            {
                // The font constructor automatically substitutes fonts if it can't find the one requested.
                // But we prefer the caller to know if anything is wrong with their data. A simple string compare
                // isn't sufficient because some fonts (eg. MS Mincho) change names depending on the locale.

                // Early out: in most cases the name will match the current or invariant culture.
                if (options.SourceFont.Equals(font.FontFamily.GetName(CultureInfo.CurrentCulture.LCID), StringComparison.OrdinalIgnoreCase) ||
                    options.SourceFont.Equals(font.FontFamily.GetName(CultureInfo.InvariantCulture.LCID), StringComparison.OrdinalIgnoreCase))
                {
                    return font;
                }

                // Check the font name in every culture.
                foreach (CultureInfo culture in CultureInfo.GetCultures(CultureTypes.SpecificCultures))
                {
                    if (options.SourceFont.Equals(font.FontFamily.GetName(culture.LCID), StringComparison.OrdinalIgnoreCase))
                    {
                        return font;
                    }
                }

                // A font substitution must have occurred.
                throw new Exception(string.Format("Can't find font '{0}'.", options.SourceFont));
            }
            catch
            {
                font.Dispose();
                throw;
            }
        }


        // Converts a font size from points to pixels. Can't just let GDI+ do this for us,
        // because we want identical results on every machine regardless of system DPI settings.
        static float PointsToPixels(float points)
        {
            return points * 96 / 72;
        }

        
        // Rasterizes a single character glyph.
        static Glyph ImportGlyph(char character, Font font, Brush brush, StringFormat stringFormat, Bitmap bitmap, Graphics graphics)
        {
            string characterString = character.ToString();

            // Measure the size of this character.
            SizeF size = graphics.MeasureString(characterString, font, Point.Empty, stringFormat);

            int characterWidth = (int)Math.Ceiling(size.Width);
            int characterHeight = (int)Math.Ceiling(size.Height);

            // Pad to make sure we capture any overhangs (negative ABC spacing, etc.)
            int padWidth = characterWidth;
            int padHeight = characterHeight / 2;

            int bitmapWidth = characterWidth + padWidth * 2;
            int bitmapHeight = characterHeight + padHeight * 2;

            if (bitmapWidth > MaxGlyphSize || bitmapHeight > MaxGlyphSize)
                throw new Exception("Excessively large glyph won't fit in my lazily implemented fixed size temp surface.");

            // Render the character.
            graphics.Clear(Color.Black);
            graphics.DrawString(characterString, font, brush, padWidth, padHeight, stringFormat);
            graphics.Flush();

            // Clone the newly rendered image.
            Bitmap glyphBitmap = bitmap.Clone(new Rectangle(0, 0, bitmapWidth, bitmapHeight), PixelFormat.Format32bppArgb);

            BitmapUtils.ConvertGreyToAlpha(glyphBitmap);

            // Query its ABC spacing.
            float? abc = GetCharacterWidth(character, font, graphics);

            // Construct the output Glyph object.
            return new Glyph(character, glyphBitmap)
            {
                XOffset = -padWidth,
                XAdvance = abc.HasValue ? padWidth - bitmapWidth + abc.Value : -padWidth,
                YOffset = -padHeight,
            };
        }


        // Queries APC spacing for the specified character.
        static float? GetCharacterWidth(char character, Font font, Graphics graphics)
        {
            // Look up the native device context and font handles.
            IntPtr hdc = graphics.GetHdc();

            try
            {
                IntPtr hFont = font.ToHfont();

                try
                {
                    // Select our font into the DC.
                    IntPtr oldFont = NativeMethods.SelectObject(hdc, hFont);

                    try
                    {
                        // Query the character spacing.
                        var result = new NativeMethods.ABCFloat[1];

                        if (NativeMethods.GetCharABCWidthsFloat(hdc, character, character, result))
                        {
                            return result[0].A + 
                                   result[0].B + 
                                   result[0].C;
                        }
                        else
                        {
                            return null;
                        }
                    }
                    finally
                    {
                        NativeMethods.SelectObject(hdc, oldFont);
                    }
                }
                finally
                {
                    NativeMethods.DeleteObject(hFont);
                }
            }
            finally
            {
                graphics.ReleaseHdc(hdc);
            }
        }


        // Interop to the native GDI GetCharABCWidthsFloat method.
        static class NativeMethods
        {
            [DllImport("gdi32.dll")]
            public static extern IntPtr SelectObject(IntPtr hdc, IntPtr hObject);

            [DllImport("gdi32.dll")]
            public static extern bool DeleteObject(IntPtr hObject);

            [DllImport("gdi32.dll", CharSet = CharSet.Unicode)]
            public static extern bool GetCharABCWidthsFloat(IntPtr hdc, uint iFirstChar, uint iLastChar, [Out] ABCFloat[] lpABCF);


            [StructLayout(LayoutKind.Sequential)]
            public struct ABCFloat
            {
                public float A;
                public float B;
                public float C;
            }
        }
    }
}
