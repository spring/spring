using System;
using System.Runtime.InteropServices;
using System.Text;

    // this is just to get the spring window handle
    // in the future we can probably get this via the spring ai api???
    public class User32
    {
        public delegate bool EnumWindowsCallback(int hwnd, int lParam);

        [DllImport("user32.Dll")]
        public static extern int EnumWindows(EnumWindowsCallback x, int y);
        [DllImport("User32.Dll")]
        public static extern void GetWindowText(int h, StringBuilder s, int nMaxCount);
        [DllImport("User32.Dll")]
        public static extern void GetClassName(int h, StringBuilder s, int nMaxCount);
    }
