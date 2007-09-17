/*
 * CUnitSyncJNIBindings.java
 *
 * Created on 31 May 2006, 14:43
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package aflobby;

/**
 * CUnitSyncJNIBindings.java
 *
 *
 *
 * @author AF
 */

//CUnitSyncJNIBindings
public class CUnitSyncJNIBindings {

    /**
     * Creates a new instance of CUnitSyncJNIBindings
     */
    public CUnitSyncJNIBindings() {
    }

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


    public static native String GetSpringVersion();

    public static native void Message(String p_szMessage);

    public static native int Init(boolean isServer, int id);

    public static native void UnInit();

    public static native int ProcessUnits();

    public static native int ProcessUnitsNoChecksum();

    public static native String GetCurrentList();

    public static native void AddClient(int id, String unitList);

    public static native void RemoveClient(int id);

    public static native String GetClientDiff(int id);

    public static native void InstallClientDiff(String diff);

    public static native int GetUnitCount();

    public static native String GetUnitName(int unit);

    public static native String GetFullUnitName(int unit);

    public static native int IsUnitDisabled(int unit);

    public static native int IsUnitDisabledByClient(int unit, int clientId);

    //public static native int InitArchiveScanner();
    public static native void AddArchive(String name);

    public static native void AddAllArchives(String root);

    public static native String GetArchiveChecksum(String arname); //unsigned integer

    public static native int GetMapCount();

    public static native String GetMapName(int index);

    //public static native int GetMapInfoEx(String name, MapInfo outInfo, int version);
    //public static native int GetMapInfo(String name, MapInfo outInfo);
    //public static native void* GetMinimap(String filename, int miplevel);
    public static native int GetMapArchiveCount(String mapName);

    public static native String GetMapArchiveName(int index);

    public static native String GetMapChecksum(int index); // unsigned

    public static native int GetPrimaryModCount();

    public static native String GetPrimaryModName(int index);

    public static native String GetPrimaryModArchive(int index);

    public static native int GetPrimaryModArchiveCount(int index);

    public static native String GetPrimaryModArchiveList(int arnr);

    public static native int GetPrimaryModIndex(String name);

    public static native String GetPrimaryModChecksum(int index); //unsigned

    public static native int GetSideCount();

    public static native String GetSideName(int side);


    public static native int OpenFileVFS(String name);

    public static native void CloseFileVFS(int handle);

    public static native String ReadFileVFS(int handle);

    public static native int FileSizeVFS(int handle);

    public static native int InitFindVFS ();
    public static native String SearchVFS (String pattern);////////////
    public static native int OpenArchive(String name);

    public static native void CloseArchive(int archive);

    public static native int FindFilesArchive(int archive, int cur, String nameBuf, int size);

    public static native int OpenArchiveFile(int archive, String name);

    public static native String ReadArchiveFile(int archive, int handle);

    public static native void CloseArchiveFile(int archive, int handle);

    public static native int SizeArchiveFile(int archive, int handle);

    public static native String GetDataDirs(boolean write);

    public static native boolean WriteMiniMap(String mapfile, String imagename, int miplevel);
}
