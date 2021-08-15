using System.Collections.Generic;
using System.Drawing;

namespace MakeSpriteFont
{
    public class ByWidthXY : IComparer<Rectangle>
    {
        public int Compare(Rectangle a, Rectangle b)
        {
            var result = a.Width.CompareTo(b.Width);
            if (0 == result)
            {
                result = a.Y.CompareTo(b.Y);
                if (0 == result)
                {
                    result = a.X.CompareTo(b.X);
                }
            }
            return result;
        }
    }
}
