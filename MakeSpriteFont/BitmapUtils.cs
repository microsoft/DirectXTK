// DirectXTK MakeSpriteFont tool
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace MakeSpriteFont
{
    // Assorted helpers for doing useful things with bitmaps.
    public static class BitmapUtils
    {
        // Copies a rectangular area from one bitmap to another.
        public static void CopyRect(Bitmap source, Rectangle sourceRegion, Bitmap output, Rectangle outputRegion)
        {
            if (sourceRegion.Width != outputRegion.Width ||
                sourceRegion.Height != outputRegion.Height)
            {
                throw new ArgumentException();
            }

            using (var sourceData = new PixelAccessor(source, ImageLockMode.ReadOnly, sourceRegion))
            using (var outputData = new PixelAccessor(output, ImageLockMode.WriteOnly, outputRegion))
            {
                for (int y = 0; y < sourceRegion.Height; y++)
                {
                    for (int x = 0; x < sourceRegion.Width; x++)
                    {
                        outputData[x, y] = sourceData[x, y];
                    }
                }
            }
        }


        // Checks whether an area of a bitmap contains entirely the specified alpha value.
        public static bool IsAlphaEntirely(byte expectedAlpha, Bitmap bitmap, Rectangle? region = null)
        {
            using (var bitmapData = new PixelAccessor(bitmap, ImageLockMode.ReadOnly, region))
            {
                for (int y = 0; y < bitmapData.Region.Height; y++)
                {
                    for (int x = 0; x < bitmapData.Region.Width; x++)
                    {
                        byte alpha = bitmapData[x, y].A;

                        if (alpha != expectedAlpha)
                            return false;
                    }
                }
            }

            return true;
        }


        // Checks whether a bitmap contains entirely the specified RGB value.
        public static bool IsRgbEntirely(Color expectedRgb, Bitmap bitmap)
        {
            using (var bitmapData = new PixelAccessor(bitmap, ImageLockMode.ReadOnly))
            {
                for (int y = 0; y < bitmap.Height; y++)
                {
                    for (int x = 0; x < bitmap.Width; x++)
                    {
                        Color color = bitmapData[x, y];

                        if (color.A == 0)
                            continue;

                        if ((color.R != expectedRgb.R) ||
                            (color.G != expectedRgb.G) ||
                            (color.B != expectedRgb.B))
                        {
                            return false;
                        }
                    }
                }
            }

            return true;
        }


        // Converts greyscale luminosity to alpha data.
        public static void ConvertGreyToAlpha(Bitmap bitmap)
        {
            using (var bitmapData = new PixelAccessor(bitmap, ImageLockMode.ReadWrite))
            {
                for (int y = 0; y < bitmap.Height; y++)
                {
                    for (int x = 0; x < bitmap.Width; x++)
                    {
                        Color color = bitmapData[x, y];

                        // Average the red, green and blue values to compute brightness.
                        int alpha = (color.R + color.G + color.B) / 3;

                        bitmapData[x, y] = Color.FromArgb(alpha, 255, 255, 255);
                    }
                }
            }
        }


        // Converts a bitmap to premultiplied alpha format.
        public static void PremultiplyAlpha(Bitmap bitmap)
        {
            using (var bitmapData = new PixelAccessor(bitmap, ImageLockMode.ReadWrite))
            {
                for (int y = 0; y < bitmap.Height; y++)
                {
                    for (int x = 0; x < bitmap.Width; x++)
                    {
                        Color color = bitmapData[x, y];

                        int a = color.A;
                        int r = color.R * a / 255;
                        int g = color.G * a / 255;
                        int b = color.B * a / 255;

                        bitmapData[x, y] = Color.FromArgb(a, r, g, b);
                    }
                }
            }
        }


        // To avoid filtering artifacts when scaling or rotating fonts that do not use premultiplied alpha,
        // make sure the one pixel border around each glyph contains the same RGB values as the edge of the
        // glyph itself, but with zero alpha. This processing is an elaborate no-op when using premultiplied
        // alpha, because the premultiply conversion will change the RGB of all such zero alpha pixels to black.
        public static void PadBorderPixels(Bitmap bitmap, Rectangle region)
        {
            using (var bitmapData = new PixelAccessor(bitmap, ImageLockMode.ReadWrite))
            {
                // Pad the top and bottom.
                for (int x = region.Left; x < region.Right; x++)
                {
                    CopyBorderPixel(bitmapData, x, region.Top, x, region.Top - 1);
                    CopyBorderPixel(bitmapData, x, region.Bottom - 1, x, region.Bottom);
                }

                // Pad the left and right.
                for (int y = region.Top; y < region.Bottom; y++)
                {
                    CopyBorderPixel(bitmapData, region.Left, y, region.Left - 1, y);
                    CopyBorderPixel(bitmapData, region.Right - 1, y, region.Right, y);
                }

                // Pad the four corners.
                CopyBorderPixel(bitmapData, region.Left, region.Top, region.Left - 1, region.Top - 1);
                CopyBorderPixel(bitmapData, region.Right - 1, region.Top, region.Right, region.Top - 1);
                CopyBorderPixel(bitmapData, region.Left, region.Bottom - 1, region.Left - 1, region.Bottom);
                CopyBorderPixel(bitmapData, region.Right - 1, region.Bottom - 1, region.Right, region.Bottom);
            }
        }


        // Copies a single pixel within a bitmap, preserving RGB but forcing alpha to zero.
        static void CopyBorderPixel(PixelAccessor bitmapData, int sourceX, int sourceY, int destX, int destY)
        {
            Color color = bitmapData[sourceX, sourceY];

            bitmapData[destX, destY] = Color.FromArgb(0, color);
        }

        
        // Converts a bitmap to the specified pixel format.
        public static Bitmap ChangePixelFormat(Bitmap bitmap, PixelFormat format)
        {
            Rectangle bounds = new Rectangle(0, 0, bitmap.Width, bitmap.Height);

            return bitmap.Clone(bounds, format);
        }


        // Helper for locking a bitmap and efficiently reading or writing its pixels.
        public sealed class PixelAccessor : IDisposable
        {
            // Constructor locks the bitmap.
            public PixelAccessor(Bitmap bitmap, ImageLockMode mode, Rectangle? region = null)
            {
                this.bitmap = bitmap;

                this.Region = region.GetValueOrDefault(new Rectangle(0, 0, bitmap.Width, bitmap.Height));

                this.data = bitmap.LockBits(Region, mode, PixelFormat.Format32bppArgb);
            }


            // Dispose unlocks the bitmap.
            public void Dispose()
            {
                if (data != null)
                {
                    bitmap.UnlockBits(data);

                    data = null;
                }
            }


            // Query what part of the bitmap is locked.
            public Rectangle Region { get; private set; }


            // Get or set a pixel value.
            public Color this[int x, int y]
            {
                get
                {
                    return Color.FromArgb(Marshal.ReadInt32(PixelAddress(x, y)));
                }

                set
                {
                    Marshal.WriteInt32(PixelAddress(x, y), value.ToArgb()); 
                }
            }


            // Helper computes the address of the specified pixel.
            IntPtr PixelAddress(int x, int y)
            {
                return data.Scan0 + (y * data.Stride) + (x * sizeof(int));
            }


            // Fields.
            Bitmap bitmap;
            BitmapData data;
        }
    }
}
