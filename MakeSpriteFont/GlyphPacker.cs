// DirectXTK MakeSpriteFont tool
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;

namespace MakeSpriteFont
{
    // Helper for arranging many small bitmaps onto a single larger surface.
    public static class GlyphPacker
    {
        public static Bitmap ArrangeGlyphsFast(Glyph[] sourceGlyphs)
        {
            // Build up a list of all the glyphs needing to be arranged.
            List<ArrangedGlyph> glyphs = new List<ArrangedGlyph>();

            int largestWidth = 1;
            int largestHeight = 1;

            for (int i = 0; i < sourceGlyphs.Length; i++)
            {
                ArrangedGlyph glyph = new ArrangedGlyph();

                glyph.Source = sourceGlyphs[i];

                // Leave a one pixel border around every glyph in the output bitmap.
                glyph.Width = sourceGlyphs[i].Subrect.Width + 2;
                glyph.Height = sourceGlyphs[i].Subrect.Height + 2;

                if (glyph.Width > largestWidth)
                    largestWidth = glyph.Width;

                if (glyph.Height > largestHeight)
                    largestHeight = glyph.Height;

                glyphs.Add(glyph);
            }

            // Work out how big the output bitmap should be.
            int outputWidth = GuessOutputWidth(sourceGlyphs);

            // Place each glyph in a grid based on the largest glyph size
            int curx = 0;
            int cury = 0;

            for (int i = 0; i < glyphs.Count; i++)
            {
                glyphs[i].X = curx;
                glyphs[i].Y = cury;

                curx += largestWidth;

                if (curx + largestWidth > outputWidth)
                {
                    curx = 0;
                    cury += largestHeight;
                }
            }

            // Create the merged output bitmap.
            int outputHeight = MakeValidTextureSize(cury + largestHeight, false);

            return CopyGlyphsToOutput(glyphs, outputWidth, outputHeight);
        }

        public static Bitmap ArrangeGlyphs(Glyph[] sourceGlyphs)
        {
            // Build up a list of all the glyphs needing to be arranged.
            List<ArrangedGlyph> glyphs = new List<ArrangedGlyph>();

            for (int i = 0; i < sourceGlyphs.Length; i++)
            {
                ArrangedGlyph glyph = new ArrangedGlyph();

                glyph.Source = sourceGlyphs[i];

                // Leave a one pixel border around every glyph in the output bitmap.
                glyph.Width = sourceGlyphs[i].Subrect.Width + 2;
                glyph.Height = sourceGlyphs[i].Subrect.Height + 2;

                glyphs.Add(glyph);
            }

            // Sort so the largest glyphs get arranged first.
            glyphs.Sort(CompareGlyphSizes);

            // Work out how big the output bitmap should be.
            int outputWidth = GuessOutputWidth(sourceGlyphs);
            int outputHeight = 0;

            // Choose positions for each glyph, one at a time.
            for (int i = 0; i < glyphs.Count; i++)
            {
                if (i > 0 && (i % 500) == 0)
                {
                    Console.Write(".");
                }

                PositionGlyph(glyphs, i, outputWidth);

                outputHeight = Math.Max(outputHeight, glyphs[i].Y + glyphs[i].Height);
            }

            if (glyphs.Count >= 500)
            {
                Console.WriteLine();
            }

            // Create the merged output bitmap.
            outputHeight = MakeValidTextureSize(outputHeight, false);

            return CopyGlyphsToOutput(glyphs, outputWidth, outputHeight);
        }


        // Once arranging is complete, copies each glyph to its chosen position in the single larger output bitmap.
        static Bitmap CopyGlyphsToOutput(List<ArrangedGlyph> glyphs, int width, int height)
        {
            Bitmap output = new Bitmap(width, height, PixelFormat.Format32bppArgb);

            int usedPixels = 0;

            foreach (ArrangedGlyph glyph in glyphs)
            {
                Glyph sourceGlyph = glyph.Source;
                Rectangle sourceRegion = sourceGlyph.Subrect;
                Rectangle destinationRegion = new Rectangle(glyph.X + 1, glyph.Y + 1, sourceRegion.Width, sourceRegion.Height);

                BitmapUtils.CopyRect(sourceGlyph.Bitmap, sourceRegion, output, destinationRegion);

                BitmapUtils.PadBorderPixels(output, destinationRegion);

                sourceGlyph.Bitmap = output;
                sourceGlyph.Subrect = destinationRegion;

                usedPixels += (glyph.Width * glyph.Height);
            }

            float utilization = ( (float)usedPixels / (float)(width * height) ) * 100;

            Console.WriteLine("Packing efficiency {0}%", utilization );

            return output;
        }


        // Internal helper class keeps track of a glyph while it is being arranged.
        class ArrangedGlyph
        {
            public Glyph Source;

            public int X;
            public int Y;

            public int Width;
            public int Height;
        }


        // Works out where to position a single glyph.
        static void PositionGlyph(List<ArrangedGlyph> glyphs, int index, int outputWidth)
        {
            int x = 0;
            int y = 0;

            while (true)
            {
                // Is this position free for us to use?
                int intersects = FindIntersectingGlyph(glyphs, index, x, y);

                if (intersects < 0)
                {
                    glyphs[index].X = x;
                    glyphs[index].Y = y;

                    return;
                }

                // Skip past the existing glyph that we collided with.
                x = glyphs[intersects].X + glyphs[intersects].Width;

                // If we ran out of room to move to the right, try the next line down instead.
                if (x + glyphs[index].Width > outputWidth)
                {
                    x = 0;
                    y++;
                }
            }
        }


        // Checks if a proposed glyph position collides with anything that we already arranged.
        static int FindIntersectingGlyph(List<ArrangedGlyph> glyphs, int index, int x, int y)
        {
            int w = glyphs[index].Width;
            int h = glyphs[index].Height;

            for (int i = 0; i < index; i++)
            {
                if (glyphs[i].X >= x + w)
                    continue;

                if (glyphs[i].X + glyphs[i].Width <= x)
                    continue;

                if (glyphs[i].Y >= y + h)
                    continue;

                if (glyphs[i].Y + glyphs[i].Height <= y)
                    continue;

                return i;
            }

            return -1;
        }


        // Comparison function for sorting glyphs by size.
        static int CompareGlyphSizes(ArrangedGlyph a, ArrangedGlyph b)
        {
            const int heightWeight = 1024;

            int aSize = a.Height * heightWeight + a.Width;
            int bSize = b.Height * heightWeight + b.Width;

            if (aSize != bSize)
                return bSize.CompareTo(aSize);
            else
                return a.Source.Character.CompareTo(b.Source.Character);
        }


        // Heuristic guesses what might be a good output width for a list of glyphs.
        static int GuessOutputWidth(Glyph[] sourceGlyphs)
        {
            int maxWidth = 0;
            int totalSize = 0;

            foreach (Glyph glyph in sourceGlyphs)
            {
                maxWidth = Math.Max(maxWidth, glyph.Subrect.Width);
                totalSize += glyph.Subrect.Width * glyph.Subrect.Height;
            }

            int width = Math.Max((int)Math.Sqrt(totalSize), maxWidth);

            return MakeValidTextureSize(width, true);
        }


        // Rounds a value up to the next larger valid texture size.
        static int MakeValidTextureSize(int value, bool requirePowerOfTwo)
        {
            // In case we want to DXT compress, make sure the size is a multiple of 4.
            const int blockSize = 4;

            if (requirePowerOfTwo)
            {
                // Round up to a power of two.
                int powerOfTwo = blockSize;

                while (powerOfTwo < value)
                    powerOfTwo <<= 1;

                return powerOfTwo;
            }
            else
            {
                // Round up to the specified block size.
                return (value + blockSize - 1) & ~(blockSize - 1);
            }
        }
    }
}
