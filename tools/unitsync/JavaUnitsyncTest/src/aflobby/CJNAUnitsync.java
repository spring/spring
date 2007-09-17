/*
 * CJNAUnitsync.java
 * 
 * Created on 17-Sep-2007, 09:07:13
 * 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package aflobby;

/**
 *
 * @author AF-StandardUsr
 */

import com.sun.jna.*;
import com.sun.jna.win32.StdCallLibrary;
 
 
public interface CJNAUnitsync extends StdCallLibrary  {
	
	CJNAUnitsync INSTANCE = (CJNAUnitsync) Native.loadLibrary("unitsync", CJNAUnitsync.class);
        CJNAUnitsync INSTANCE2 = (CJNAUnitsync) Native.loadLibrary("unitsyncVS2005", CJNAUnitsync.class);
		
	public int Init(boolean isServer, int id);
        String GetSpringVersion();
}