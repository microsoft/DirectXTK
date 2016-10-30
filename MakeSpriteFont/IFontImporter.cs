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

using System.Collections.Generic;

namespace MakeSpriteFont
{
    // Importer interface allows the conversion tool to support multiple source font formats.
    public interface IFontImporter
    {
        void Import(CommandLineOptions options);

        IEnumerable<Glyph> Glyphs { get; }

        float LineSpacing { get; }
    }
}
