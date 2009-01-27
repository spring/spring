/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package nulljavaai;


import com.clan_sy.spring.ai.*;
import com.clan_sy.spring.ai.event.*;
import com.clan_sy.spring.ai.command.*;

import com.sun.jna.Pointer;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;
import java.util.Set;
import java.util.logging.*;

/**
 * Serves as Interface for a Java Skirmish AIs for the Spring engine.
 *
 * @author	hoijui
 * @version	0.1
 */
public class NullJavaAI implements AI {

	private int teamId = -1;
	private AICallback clb = null;
	private String myDataDir = null;
	private String myLogFile = null;
	private Logger log = null;

	private static class MyCustomLogFormatter extends Formatter {

		private DateFormat dateFormat = new SimpleDateFormat("HH:mm:ss:SSS dd.MM.yyyy");

		public String format(LogRecord record) {

			// Create a StringBuffer to contain the formatted record
			// start with the date.
			StringBuffer sb = new StringBuffer();

			// Get the date from the LogRecord and add it to the buffer
			Date date = new Date(record.getMillis());
			sb.append(dateFormat.format(date));
			sb.append(" ");

			// Get the level name and add it to the buffer
			sb.append(record.getLevel().getName());
			sb.append(": ");

			// Get the formatted message (includes localization
			// and substitution of paramters) and add it to the buffer
			sb.append(formatMessage(record));
			sb.append("\n");

			return sb.toString();
		}
	}

	public static boolean isDebugging() {
		return true;
	}


	public  NullJavaAI() {}
	
	public int init(int teamId, Properties info, Properties options) {

		int ret = -1;

		// initialize the log
		try {
			myDataDir = info.getProperty("dataDir");
			myLogFile = myDataDir + "/log.txt";
			FileHandler fileLogger = new FileHandler(myLogFile, false);
			fileLogger.setFormatter(new MyCustomLogFormatter());
			fileLogger.setLevel(Level.ALL);
			log = Logger.getLogger("nulljavaai");
			if (isDebugging()) {
				log.setLevel(Level.ALL);
			} else {
				log.setLevel(Level.INFO);
			}
			log.addHandler(fileLogger);
		} catch (Exception ex) {
			System.out.println("NullJavaAI: Failed initializing the logger!");
			ex.printStackTrace();
			ret = -2;
		}

		try {
			log.info("initializing team " + teamId);

			log.info("info (items: " + info.size() + ") ...");
			Set<String> infoKeys = info.stringPropertyNames();
			for (String infoKey : infoKeys) {
				log.info(infoKey + " = " + info.getProperty(infoKey));
			}

			log.info("options (items: " + options.size() + ") ...");
			Set<String> optionsKeys = options.stringPropertyNames();
			for (String optionsKey : optionsKeys) {
				log.info(optionsKey + " = " + options.getProperty(optionsKey));
			}

			ret = 0;
		} catch (Exception ex) {
			log.log(Level.SEVERE, "Failed initializing", ex);
			ret = -3;
		}

		return ret;
	}

	public int release(int teamId) {

		int ret = -1;

		try {
			log.info("releasing team " + teamId);

			ret = 0;
		} catch (Exception ex) {
			log.log(Level.WARNING, "Failed releasing", ex);
			ret = -2;
		}

		return ret;
	}

	public int handleEvent(int teamId, int topic, Pointer event) {

		if (log == null) {
			System.out.println("out is still null");
			System.exit(-1);
		}
		log.finest("handleEvent topic: " + topic);
		try {
			if (topic == InitAIEvent.TOPIC) {
				//InitAIEvent evt = InitAIEvent.getInstance(event);
				//DefaultInitAIEvent evt = new DefaultInitAIEvent(event);
				InitAIEvent evt = new InitAIEvent(event);
				log.info("handleEvent:InitAIEvent");
				this.teamId = evt.team;
				clb = evt.callback;
				log.info("handleEvent:InitAIEvent:team: " + evt.team);
				log.info("handleEvent:InitAIEvent:sizeOptions: " + evt.options.size());
				log.info("handleEvent:InitAIEvent:options:");
//				Pointer[] pKeys = evt.optionKeys.getPointerArray(0L, evt.sizeOptions);
//				Pointer[] pValues = evt.optionValues.getPointerArray(0L, evt.sizeOptions);
//				Properties options = new Properties();
//				for (int i = 0; i < evt.sizeOptions; i++) {
//					options.setProperty(pKeys[i].getString(0L), pValues[i].getString(0L));
//				}
				Set<String> optionsKeys = evt.options.stringPropertyNames();
				for (String optionsKey : optionsKeys) {
					log.info(optionsKey + " = " + evt.options.getProperty(optionsKey));
				}
				log.info("handleEvent:InitAIEvent:stored");
			} else if (topic == UpdateAIEvent.TOPIC) {
				UpdateAIEvent evt = new UpdateAIEvent(event);
				log.finer("handleEvent UpdateAIEvent event ...");
				log.finer("frame: " + evt.frame);
			} else {
				if (clb != null) {
					log.finer("handleEvent UNKNOWN event: fetching frame...");
					int frame = clb.Clb_Game_getCurrentFrame(teamId);
					log.finer("handleEvent UNKNOWN event: frame is: " + frame);
					log.finer("handleEvent UNKNOWN event: sending chat msg...");
					SendTextMessageAICommand cmd = new SendTextMessageAICommand();
					cmd.text = "Hello Engine (from NullJavaAI.java)";
					cmd.zone = 0;
					cmd.write();
					int ret = clb.Clb_handleCommand(teamId, 0, -1, cmd.getTopic(), cmd.getPointer());
					log.finer("handleEvent UNKNOWN event: sending chat msg return: " + ret);
				}
			}
		} catch (Exception ex) {
			log.log(Level.WARNING, "Failed handling event", ex);
			return -1;
		}

		return 0;
	}
}
