/*
 * CUnitSyncJNIVersion.java
 * 
 * Created on 15-Sep-2007, 13:35:52
 * 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aflobby;

/**
 *
 * @author AF-StandardUsr
 */
public class CUnitSyncJNIVersion {
    // load up the UnitSync.dll/UnitSync.so library
    public static boolean loaded = false;

    public static boolean LoadUnitSync(String lib) {
        try {
            //intendedpath = System.getProperty ("user.dir");
            //System.out.println ("working directory : " + intendedpath);
            //System.out.println ();
            //System.out.println (intendedpath+"\\libjavaunitsync.so");
            //System.out.println ();
                System.loadLibrary(lib);
//                java.lang.Runtime.getRuntime ().load (intendedpath+System.getProperty ("file.separator")+"unitsync.dll");

            loaded=true;
        } catch (final java.lang.UnsatisfiedLinkError e) {
            e.printStackTrace();
            loaded=false;
        } /*catch (Exception e){
        //            java.awt.EventQueue.invokeLater (new Runnable () {
        //                public void run () {
        //                    new WarningWindow ("<font face=\"Arial\" color=\""+ColourHelper.ColourToHex (Color.WHITE)+"\"><font size=20>UnitSync Exception error</font><br><br> Make sure you have a JNI aware UnitSync library in the same folder as AFLobby, and that it is compiled correctly using the latest java bindings.<br><br> AFlobby needs the unitsync library to be able to use spring.<br><br>Linux users need libjavaunitsync.so<br><br> Windows users need javaunitsync.dll<br><br> Apple Mac users need libjavaunitsync.dylib (Mac is not entirely supported and upto date mac builds of unitsync arent included due to time restrictions)<br><br><br><br><b> path variable =</b> <i>"+ System.getProperty ("user.dir")+"</i><br> <b> library path = </b><i>"+System.getProperty("java.library.path")+"</i></font>",
        //                        "UnitSync Error"
        //                        ).setVisible (true);
        //                }
        //            });
        //        }*/
        return loaded;
    }
    
    public static native int GetVersion();
}
