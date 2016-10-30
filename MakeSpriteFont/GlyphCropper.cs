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

using System.Drawing;

namespace MakeSpriteFont
{
    // Crops unused space from around the edge of a glyph bitmap.
    public static class GlyphCropper
    {
        public static void Crop(Glyph glyph)
        {
            // Crop the top.
            while ((glyph.Subrect.Height > 1) && BitmapUtils.IsAlphaEntirely(0, glyph.Bitmap, new Rectangle(glyph.Subrect.X, glyph.Subrect.Y, glyph.Subrect.Width, 1)))
            {
                glyph.Subrect.Y++;
                glyph.Subrect.Height--;

                glyph.YOffset++;
            }

            // Crop the bottom.
            while ((glyph.Subrect.Height > 1) && BitmapUtils.IsAlphaEntirely(0, glyph.Bitmap, new Rectangle(glyph.Subrect.X, glyph.Subrect.Bottom - 1, glyph.Subrect.Width, 1)))
            {
                glyph.Subrect.Height--;
            }

            // Crop the left.
            while ((glyph.Subrect.Width > 1) && BitmapUtils.IsAlphaEntirely(0, glyph.Bitmap, new Rectangle(glyph.Subrect.X, glyph.Subrect.Y, 1, glyph.Subrect.Height)))
            {
                glyph.Subrect.X++;
                glyph.Subrect.Width--;

                glyph.XOffset++;
            }

            // Crop the right.
            while ((glyph.Subrect.Width > 1) && BitmapUtils.IsAlphaEntirely(0, glyph.Bitmap, new Rectangle(glyph.Subrect.Right - 1, glyph.Subrect.Y, 1, glyph.Subrect.Height)))
            {
                glyph.Subrect.Width--;

                glyph.XAdvance++;
            }
        }
    }
}
