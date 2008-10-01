using System;
using System.Collections.Generic;
using System.Text;

namespace CSharpAI
{
    class HeightMap
    {
        static HeightMap instance = new HeightMap();
        public static HeightMap GetInstance() { return instance; }

        HeightMap()
        {
        }

        public double[,] GetHeightMap()
        {
            LogFile.GetInstance().WriteLine("Getting heightmap, this could take a while... ");
            IAICallback aicallback = CSAI.GetInstance().aicallback;
            int mapwidth = aicallback.GetMapWidth();
            int mapheight = aicallback.GetMapHeight();
            double[,] HeightMap = new double[mapwidth + 1, mapheight + 1];
            for (int x = 0; x < mapwidth + 1; x++)
            {
                for (int y = 0; y < mapheight + 1; y++)
                {
                    HeightMap[x, y] = aicallback.GetElevation(x * MovementMaps.SQUARE_SIZE, y * MovementMaps.SQUARE_SIZE);
                }
            }
            return HeightMap;
        }
    }
}
