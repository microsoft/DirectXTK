// DirectXTK MakeSpriteFont tool
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

using System;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;

namespace MakeSpriteFont
{
    // Writes the output spritefont binary file.
    public static class SpriteFontWriter
    {
        const string spriteFontMagic = "DXTKfont";

        const int DXGI_FORMAT_R8G8B8A8_UNORM = 28;
        const int DXGI_FORMAT_B4G4R4A4_UNORM = 115;
        const int DXGI_FORMAT_BC2_UNORM = 74;


        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2202:Do not dispose objects multiple times")]
        public static void WriteSpriteFont(CommandLineOptions options, Glyph[] glyphs, float lineSpacing, Bitmap bitmap)
        {
            using (FileStream file = File.OpenWrite(options.OutputFile))
            using (BinaryWriter writer = new BinaryWriter(file))
            {
                WriteMagic(writer);
                WriteGlyphs(writer, glyphs);

                writer.Write(lineSpacing);
                writer.Write(options.DefaultCharacter);
                
                WriteBitmap(writer, options, bitmap);
            }
        }


        static void WriteMagic(BinaryWriter writer)
        {
            foreach (char magic in spriteFontMagic)
            {
                writer.Write((byte)magic);
            }
        }


        static void WriteGlyphs(BinaryWriter writer, Glyph[] glyphs)
        {
            writer.Write(glyphs.Length);

            foreach (Glyph glyph in glyphs)
            {
                writer.Write((int)glyph.Character);

                writer.Write(glyph.Subrect.Left);
                writer.Write(glyph.Subrect.Top);
                writer.Write(glyph.Subrect.Right);
                writer.Write(glyph.Subrect.Bottom);

                writer.Write(glyph.XOffset);
                writer.Write(glyph.YOffset);
                writer.Write(glyph.XAdvance);
            }
        }


        static void WriteBitmap(BinaryWriter writer, CommandLineOptions options, Bitmap bitmap)
        {
            writer.Write(bitmap.Width);
            writer.Write(bitmap.Height);

            switch (options.TextureFormat)
            {
                case TextureFormat.Rgba32:
                    WriteRgba32(writer, bitmap);
                    break;
             
                case TextureFormat.Bgra4444:
                    WriteBgra4444(writer, bitmap);
                    break;
                
                case TextureFormat.CompressedMono:
                    WriteCompressedMono(writer, bitmap, options);
                    break;
                
                default:
                    throw new NotSupportedException();
            }
        }


        // Writes an uncompressed 32 bit font texture.
        static void WriteRgba32(BinaryWriter writer, Bitmap bitmap)
        {
            writer.Write(DXGI_FORMAT_R8G8B8A8_UNORM);

            writer.Write(bitmap.Width * 4);
            writer.Write(bitmap.Height);

            using (var bitmapData = new BitmapUtils.PixelAccessor(bitmap, ImageLockMode.ReadOnly))
            {
                for (int y = 0; y < bitmap.Height; y++)
                {
                    for (int x = 0; x < bitmap.Width; x++)
                    {
                        Color color = bitmapData[x, y];

                        writer.Write(color.R);
                        writer.Write(color.G);
                        writer.Write(color.B);
                        writer.Write(color.A);
                    }
                }
            }
        }


        // Writes a 16 bit font texture.
        static void WriteBgra4444(BinaryWriter writer, Bitmap bitmap)
        {
            writer.Write(DXGI_FORMAT_B4G4R4A4_UNORM);

            writer.Write(bitmap.Width * sizeof(ushort));
            writer.Write(bitmap.Height);

            using (var bitmapData = new BitmapUtils.PixelAccessor(bitmap, ImageLockMode.ReadOnly))
            {
                for (int y = 0; y < bitmap.Height; y++)
                {
                    for (int x = 0; x < bitmap.Width; x++)
                    {
                        Color color = bitmapData[x, y];

                        int r = color.R >> 4;
                        int g = color.G >> 4;
                        int b = color.B >> 4;
                        int a = color.A >> 4;

                        int packed = b | (g << 4) | (r << 8) | (a << 12);

                        writer.Write((ushort)packed);
                    }
                }
            }
        }


        // Writes a block compressed monochromatic font texture.
        static void WriteCompressedMono(BinaryWriter writer, Bitmap bitmap, CommandLineOptions options)
        {
            if ((bitmap.Width & 3) != 0 ||
                (bitmap.Height & 3) != 0)
            {
                throw new ArgumentException("Block compression requires texture size to be a multiple of 4.");
            }

            writer.Write(DXGI_FORMAT_BC2_UNORM);

            writer.Write(bitmap.Width * 4);
            writer.Write(bitmap.Height / 4);

            using (var bitmapData = new BitmapUtils.PixelAccessor(bitmap, ImageLockMode.ReadOnly))
            {
                for (int y = 0; y < bitmap.Height; y += 4)
                {
                    for (int x = 0; x < bitmap.Width; x += 4)
                    {
                        CompressBlock(writer, bitmapData, x, y, options);
                    }
                }
            }
        }


        // We want to compress our font textures, because, like, smaller is better, 
        // right? But a standard DXT compressor doesn't do a great job with fonts that 
        // are in premultiplied alpha format. Our font data is greyscale, so all of the 
        // RGBA channels have the same value. If one channel is compressed differently 
        // to another, this causes an ugly variation in brightness of the rendered text. 
        // Also, fonts are mostly either black or white, with grey values only used for 
        // antialiasing along their edges. It is very important that the black and white 
        // areas be accurately represented, while the precise value of grey is less 
        // important.
        //
        // Trouble is, your average DXT compressor knows nothing about these 
        // requirements. It will optimize to minimize a generic error metric such as 
        // RMS, but this will often sacrifice crisp black and white in exchange for 
        // needless accuracy of the antialiasing pixels, or encode RGB differently to 
        // alpha. UGLY!
        //
        // Fortunately, encoding monochrome fonts turns out to be trivial. Using DXT3, 
        // we can fix the end colors as black and white, which gives guaranteed exact 
        // encoding of the font inside and outside, plus two fractional values for edge 
        // antialiasing. Also, these RGB values (0, 1/3, 2/3, 1) map exactly to four of 
        // the possible 16 alpha values available in DXT3, so we can ensure the RGB and 
        // alpha channels always exactly match.

        static void CompressBlock(BinaryWriter writer, BitmapUtils.PixelAccessor bitmapData, int blockX, int blockY, CommandLineOptions options)
        {
            long alphaBits = 0;
            int rgbBits = 0;

            int pixelCount = 0;

            for (int y = 0; y < 4; y++)
            {
                for (int x = 0; x < 4; x++)
                {
                    long alpha;
                    int rgb;

                    int value = bitmapData[blockX + x, blockY + y].A;

                    if (options.NoPremultiply)
                    {
                        // If we are not premultiplied, RGB is always white and we have 4 bit alpha.
                        alpha = value >> 4;
                        rgb = 0;
                    }
                    else
                    {
                        // For premultiplied encoding, quantize the source value to 2 bit precision.
                        if (value < 256 / 6)
                        {
                            alpha = 0;
                            rgb = 1;
                        }
                        else if (value < 256 / 2)
                        {
                            alpha = 5;
                            rgb = 3;
                        }
                        else if (value < 256 * 5 / 6)
                        {
                            alpha = 10;
                            rgb = 2;
                        }
                        else
                        {
                            alpha = 15;
                            rgb = 0;
                        }
                    }

                    // Add this pixel to the alpha and RGB bit masks.
                    alphaBits |= alpha << (pixelCount * 4);
                    rgbBits |= rgb << (pixelCount * 2);

                    pixelCount++;
                }
            }

            // Output the alpha bit mask.
            writer.Write(alphaBits);

            // Output the two endpoint colors (black and white in 5.6.5 format).
            writer.Write((ushort)0xFFFF);
            writer.Write((ushort)0);

            // Output the RGB bit mask.
            writer.Write(rgbBits);
        }
    }
}
