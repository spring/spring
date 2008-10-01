// Copyright Hugh Perkins 2006
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURVector3E. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;

namespace CSharpAI
{
    public class HeightMapPersistence
    {
        public string filename = "heightmap.bmp";

        static HeightMapPersistence instance = new HeightMapPersistence();
        public static HeightMapPersistence GetInstance() { return instance; }

        HeightMapPersistence()
        {
            LogFile logfile = LogFile.GetInstance();
            logfile.WriteLine("HeightMapPersistence");
            CSAI csai = CSAI.GetInstance();
            CSAI.VoiceCommandHandler handler = new CSAI.VoiceCommandHandler(this.SaveHandler);
            logfile.WriteLine("csai == null? " + (csai == null).ToString());
            logfile.WriteLine("handler == null? " + (handler == null).ToString());
            csai.RegisterVoiceCommand("saveheightmap", handler);
        }

        public void Load()
        {
            /*
            Bitmap bitmap = Bitmap.FromFile(filename) as Bitmap;
            int width = bitmap.Width;
            int height = bitmap.Height;
            HeightMap.GetInstance().Width = width;
            HeightMap.GetInstance().Height = height;
            HeightMap.GetInstance().Map = new float[width, height];
            double minheight = Config.GetInstance().minheight;
            double maxheight = Config.GetInstance().maxheight;
            double heightmultiplier = (maxheight - minheight) / 255;
            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    HeightMap.GetInstance().Map[i, j] = (float)(minheight + heightmultiplier * bitmap.GetPixel(i, j).B);
                }
            }
             */
        }

        void Save( string filename, double[,]mesh )
        {
            int width = mesh.GetUpperBound(0) + 1;
            int height = mesh.GetUpperBound(1) + 1;
            Bitmap bitmap = new Bitmap(width, height, PixelFormat.Format24bppRgb);
            Graphics g = Graphics.FromImage(bitmap);
            Pen[] pens = new Pen[256];
            for (int i = 0; i < 256; i++)
            {
                pens[i] = new Pen(System.Drawing.Color.FromArgb(i, i, i));
            }

            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    g.DrawRectangle(pens[(int)mesh[i, j]], i, j, 1, 1);
                }
            }
            bitmap.Save(filename, ImageFormat.Bmp);
        }

        public void SaveHandler(string cmd, string[]args, int player)
        {
            double[,]mesh = HeightMap.GetInstance().GetHeightMap();
            Save(args[2], mesh);
        }
    }
}
