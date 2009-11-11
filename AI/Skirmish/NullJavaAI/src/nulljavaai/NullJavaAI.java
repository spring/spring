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


import com.springrts.ai.*;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.*;

/**
 * Serves as Interface for a Java Skirmish AIs for the Spring engine.
 *
 * @author	hoijui
 * @version	0.1
 */
public class NullJavaAI extends AbstractAI implements AI {

	private int skirmishAIId = -1;
	private AICallback clb = null;
	private String myLogFile = null;
	private Logger log = null;
	private int frame = -1;

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

	public NullJavaAI() {}


	@Override
	public int init(int skirmishAIId, AICallback callback) {

		int ret = -1;

		this.clb = callback;

		// initialize the log
		try {
			int teamId = clb.SkirmishAI_getTeamId();
			// most likely, this causes a memory leak, as the C string
			// allocated by this, is never freed
			myLogFile = callback.DataDirs_allocatePath("log-team-" + teamId + "-" + skirmishAIId + ".txt", true, true, false, false);
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
			int teamId = clb.SkirmishAI_getTeamId();
			log.info("initializing team " + teamId + ", id " + skirmishAIId);

			int numInfo = callback.SkirmishAI_Info_getSize();
			log.info("info (items: " + numInfo + ") ...");
			for (int i = 0; i < numInfo; i++) {
				String key = callback.SkirmishAI_Info_getKey(i);
				String value = callback.SkirmishAI_Info_getValue(i);
				log.info(key + " = " + value);
			}

			int numOptions = callback.SkirmishAI_OptionValues_getSize();
			log.info("options (items: " + numOptions + ") ...");
			for (int i = 0; i < numOptions; i++) {
				String key = callback.SkirmishAI_OptionValues_getKey(i);
				String value = callback.SkirmishAI_OptionValues_getValue(i);
				log.info(key + " = " + value);
			}

			ret = 0;
		} catch (Exception ex) {
			log.log(Level.SEVERE, "Failed initializing", ex);
			log.log(Level.SEVERE, "msg: " + ex.getMessage());
			StackTraceElement[] stackTrace = ex.getStackTrace();
			for (int i = 0; i < stackTrace.length; i++) {
				log.log(Level.SEVERE, "ste: " + stackTrace[i].toString());
			}
			ret = -3;
		}

		{
			int numOptions = clb.SkirmishAI_OptionValues_getSize();
			log.info("sizeOptions: " + numOptions);
			log.info("options:");
//				Pointer[] pKeys = evt.optionKeys.getPointerArray(0L, evt.sizeOptions);
//				Pointer[] pValues = evt.optionValues.getPointerArray(0L, evt.sizeOptions);
//				Properties options = new Properties();
//				for (int i = 0; i < evt.sizeOptions; i++) {
//					options.setProperty(pKeys[i].getString(0L), pValues[i].getString(0L));
//				}
			for (int i = 0; i < numOptions; i++) {
				String key = clb.SkirmishAI_OptionValues_getKey(i);
				String value = clb.SkirmishAI_OptionValues_getValue(i);
				log.info(key + " = " + value);
			}
			log.info("stored");
		}

		return ret;
	}

	@Override
	public int update(int frame) {

		try {
			log.finer("update event ...");
			log.finer("frame: " + frame);
		} catch (Exception ex) {
			log.log(Level.WARNING, "Failed handling event", ex);
			return -1;
		}

		return 0;
	}

	@Override
	public int release(int reason) {

		int ret = -1;

		try {
			int teamId = clb.SkirmishAI_getTeamId();
			log.info("releasing team " + teamId + ", id " + skirmishAIId);

			ret = 0;
		} catch (Exception ex) {
			log.log(Level.WARNING, "Failed releasing", ex);
			ret = -2;
		}

		return ret;
	}
}
