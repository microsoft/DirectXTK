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
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;

namespace MakeSpriteFont
{
    // Helper for arranging many small bitmaps onto a single larger surface.
    public static class GlyphPacker
    {
        static int currentBinHeight;
        static SortedSet<Rectangle> currentBins;
        static readonly SortedDictionary<int, SortedSet<Rectangle>> heightBinSetMap = new SortedDictionary<int, SortedSet<Rectangle>>();


        static void AllocateBin(int h, int w)
        {
            // Select the first bin set that's tall enough and the first bin that's wide enough.
            var q = (
                from bins in (
                    from x in heightBinSetMap
                    where x.Key > h
                    select x.Value)
                from b in bins
                where b.Width >= w
                select new { BinToSplit = b, Bins = bins }).First();
            q.Bins.Remove(q.BinToSplit);

            // Remove the bin set if it's empty.
            if (!q.Bins.Any())
            {
                heightBinSetMap.Remove(q.BinToSplit.Height);
            }

            // Allocate space by splitting the bin.
            var remainderBin = q.BinToSplit;
            remainderBin.Y += h;
            remainderBin.Height -= h;
            bool found;
            if (remainderBin.Height > 2) // Glyphs are padded with a single pixel border so they must be at least 3 high. Shorter bins are discarded.
            {
                SortedSet<Rectangle> remainderBins;
                found = heightBinSetMap.TryGetValue(remainderBin.Height, out remainderBins);
                if (!found)
                {
                    remainderBins = CreateBinSet();
                    heightBinSetMap.Add(remainderBin.Height, remainderBins); // Return free space.
                }
                remainderBins.Add(remainderBin);
            }
            var newBin = q.BinToSplit;
            newBin.Height = h;
            SortedSet<Rectangle> binSet;
            found = heightBinSetMap.TryGetValue(h, out binSet);
            if (!found)
            {
                binSet = CreateBinSet();
                heightBinSetMap.Add(h, binSet);
            }
            binSet.Add(newBin);
        }


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
            var maxOutputHeight = 16384; // D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION
            var binSet = CreateBinSet();
            binSet.Add(new Rectangle(0, 0, outputWidth, maxOutputHeight));
            heightBinSetMap.Add(maxOutputHeight, binSet);
            int outputHeight = 0;

            currentBinHeight = -1;

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


        static SortedSet<Rectangle> CreateBinSet()
        {
            var binSet = new SortedSet<Rectangle>(new ByWidthXY());
            return binSet;
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
            // Bins are grouped according to height. Glyphs are also sorted by height. Switch bin sets when glyph height changes.
            if (currentBinHeight != glyphs[index].Height)
            {
                currentBinHeight = glyphs[index].Height;
                var found = heightBinSetMap.TryGetValue(currentBinHeight, out currentBins);
                if (!found)
                {
                    AllocateBin(currentBinHeight, glyphs[index].Width);
                    currentBins = heightBinSetMap[currentBinHeight];
                }
            }

            // Select the first bin that will fit the glyph.
            var q =
                from b in currentBins
                where b.Width >= glyphs[index].Width
                select b;
            if (!q.Any())
            {
                AllocateBin(currentBinHeight, glyphs[index].Width);
            }
            var binToSplit = q.First();

            // Allocate space for the glyph by splitting the bin.
            currentBins.Remove(binToSplit);
            var remainderBin = binToSplit;
            remainderBin.X += glyphs[index].Width;
            remainderBin.Width -= glyphs[index].Width;
            if (remainderBin.Width > 2) // Glyphs are padded with a single pixel border so they must be at least 3 wide. Narrower bins are discarded.
            {
                currentBins.Add(remainderBin); // Return free space.
            }
            glyphs[index].X = binToSplit.X;
            glyphs[index].Y = binToSplit.Y;
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
