// DirectXTK MakeSpriteFont tool
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929

using System;
using System.Linq;
using System.ComponentModel;
using System.Globalization;
using System.Collections.Generic;

namespace MakeSpriteFont
{
    // Describes a range of consecutive characters that should be included in the font.
    [TypeConverter(typeof(CharacterRegionTypeConverter))]
    public class CharacterRegion
    {
        // Constructor.
        public CharacterRegion(char start, char end)
        {
            if (start > end)
                throw new ArgumentException();

            this.Start = start;
            this.End = end;
        }


        // Fields.
        public char Start;
        public char End;


        // Enumerates all characters within the region.
        public IEnumerable<Char> Characters
        {
            get
            {
                for (char c = Start; c <= End; c++)
                {
                    yield return c;
                }
            }
        }


        // Flattens a list of character regions into a combined list of individual characters.
        public static IEnumerable<Char> Flatten(IEnumerable<CharacterRegion> regions)
        {
            if (regions.Any())
            {
                // If we have any regions, flatten them and remove duplicates.
                return regions.SelectMany(region => region.Characters).Distinct();
            }
            else
            {
                // If no regions were specified, use the default.
                return defaultRegion.Characters;
            }
        }

        
        // Default to just the base ASCII character set.
        static CharacterRegion defaultRegion = new CharacterRegion(' ', '~');
    }



    // Custom type converter enables CommandLineParser to parse CharacterRegion command line options.
    public class CharacterRegionTypeConverter : TypeConverter
    {
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            return sourceType == typeof(string);
        }


        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            // Input must be a string.
            string source = value as string;

            if (string.IsNullOrEmpty(source))
            {
                throw new ArgumentException();
            }

            // Supported input formats:
            //  A
            //  A-Z
            //  32-127
            //  0x20-0x7F

            char[] split = source.Split('-')
                                 .Select(ConvertCharacter)
                                 .ToArray();

            switch (split.Length)
            {
                case 1:
                    // Only a single character (eg. "a").
                    return new CharacterRegion(split[0], split[0]);

                case 2:
                    // Range of characters (eg. "a-z").
                    return new CharacterRegion(split[0], split[1]);
             
                default:
                    throw new ArgumentException();
            }
        }


        static char ConvertCharacter(string value)
        {
            if (value.Length == 1)
            {
                // Single character directly specifies a codepoint.
                return value[0];
            }
            else
            {
                // Otherwise it must be an integer (eg. "32" or "0x20").
                return (char)(int)intConverter.ConvertFromInvariantString(value);
            }
        }


        static TypeConverter intConverter = TypeDescriptor.GetConverter(typeof(int));
    }
}
