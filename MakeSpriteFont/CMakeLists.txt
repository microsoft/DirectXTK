# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.21)

project (MakeSpriteFont
  DESCRIPTION "DirectX Tool Kit SpriteFont tool"
  HOMEPAGE_URL "https://github.com/microsoft/DirectXTK"
  LANGUAGES CSharp)

add_executable(${PROJECT_NAME}
    BitmapImporter.cs
    BitmapUtils.cs
    CharacterRegion.cs
    CommandLineOptions.cs
    CommandLineParser.cs
    Glyph.cs
    GlyphCropper.cs
    GlyphPacker.cs
    IFontImporter.cs
    Program.cs
    Properties/AssemblyInfo.cs
    SpriteFontWriter.cs
    TrueTypeImporter.cs)

  set_property(TARGET ${PROJECT_NAME}
      PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
      PROPERTY WIN32_EXECUTABLE TRUE)

  set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES
      "Microsoft.CSharp"
      "System"
      "System.Drawing")

set(CMAKE_CSharp_FLAGS "/platform:AnyCPU")      
