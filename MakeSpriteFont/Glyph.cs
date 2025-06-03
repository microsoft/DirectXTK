// DirectXTK MakeSpriteFont tool
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

using System.Drawing;

namespace MakeSpriteFont
{
    // Represents a single character within a font.
    public class Glyph
    {
        // Constructor.
        public Glyph(char character, Bitmap bitmap, Rectangle? subrect = null)
        {
            this.Character = character;
            this.Bitmap = bitmap;
            this.Subrect = subrect.GetValueOrDefault(new Rectangle(0, 0, bitmap.Width, bitmap.Height));
        }


        // Unicode codepoint.
        public char Character;


        // Glyph image data (may only use a portion of a larger bitmap).
        public Bitmap Bitmap;
        public Rectangle Subrect;


        // Layout information.
        public float XOffset;
        public float YOffset;

        public float XAdvance;
    }
}
