// DirectXTK MakeSpriteFont tool
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
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
