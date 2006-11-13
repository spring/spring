using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace CSharpAI
{
    // this manages the contorl panel etc， kinda
    // this class and the control panel are very experimental
    public class CSAIUserInteraction
    {
        static CSAIUserInteraction instance = new CSAIUserInteraction();
        public static CSAIUserInteraction GetInstance() { return instance; }

        public ControlPanel controlpanel;

        CSAIUserInteraction()
        {
            Init();
        }

        class Win32Window : IWin32Window
        {
            IntPtr windowhandle;
            public Win32Window(IntPtr windowhandle)
            {
                this.windowhandle = windowhandle;
            }
            public IntPtr Handle
            {
                get { return windowhandle; }
            }
        }

        public void Init()
        {
            //IntPtr springwindowhandle = GetSpringWindowHandle();
            controlpanel = new ControlPanel();
            controlpanel.Show();
            UnitController.GetInstance().UnitAddedEvent += new UnitController.UnitAddedHandler(CSAIUserInteraction_UnitAddedEvent);
            UnitController.GetInstance().UnitRemovedEvent += new UnitController.UnitRemovedHandler(CSAIUserInteraction_UnitRemovedEvent);
            //if( ControlPanel.
        }

        //static IntPtr springwindowhandle = IntPtr.Zero;

        //IntPtr GetSpringWindowHandle()
        //{
          //  User32.EnumWindows(new User32.EnumWindowsCallback(EnumWindowsCallback), 0);
//            return springwindowhandle;
  //      }

    //    public static bool EnumWindowsCallback(int hwnd, int lParam)
      //  {
        //    IntPtr windowHandle = (IntPtr)hwnd;
          //  StringBuilder sb = new StringBuilder(1024);
            //StringBuilder sbc = new StringBuilder(256);
            //User32.GetClassName(hwnd, sbc, sbc.Capacity);
            //User32.GetWindowText((int)windowHandle, sb, sb.Capacity);
            //if (sbc.ToString() == "SDL_app" && sb.ToString() == "RtsSpring")
            //{
              //  springwindowhandle = windowHandle;
            //}
          //  return true;
        //}

        void CSAIUserInteraction_UnitRemovedEvent(int deployedid)
        {
            UpdateCounts();
        }

        void UpdateCounts()
        {
            //controlpanel.labelnumScouts = 
        }

        void CSAIUserInteraction_UnitAddedEvent(int deployedid, IUnitDef unitdef)
        {
            UpdateCounts();            
        }
    }
}
