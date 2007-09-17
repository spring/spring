/*
 * Main.java
 * 
 * Created on 15-Sep-2007, 12:47:38
 * 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package unitsynctest;

import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;

/**
 *
 * @author AF-StandardUsr
 */
public class Main {

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        javax.swing.SwingUtilities.invokeLater (new Runnable () {
            public void run () {
                CreateGUI ();
            }
        });
    }
    
    /**
     * Creates the programs GUI window
     */
    public static void CreateGUI() {
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace ();
        } catch (UnsupportedLookAndFeelException ex) {
            ex.printStackTrace ();
        } catch (InstantiationException ex) {
            ex.printStackTrace ();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace ();
        }
        
        CGUI c =new CGUI ();
        c.setVisible (true);
    }
}
