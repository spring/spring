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

package nulloojavaai;


import com.clan_sy.spring.ai.AICommand;
import com.clan_sy.spring.ai.command.*;
import com.clan_sy.spring.ai.AIFloat3;
import com.clan_sy.spring.ai.oo.*;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;
import java.util.ArrayList;
import java.util.logging.*;

/**
 * Serves as Interface for a Java Skirmish AIs for the Spring engine.
 *
 * @author	hoijui
 * @version	0.1
 */
public class NullOOJavaAI extends AbstractOOAI implements OOAI {

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

	private static void logProperties(Logger log, Level level, Properties props) {

		log.log(level, "properties (items: " + props.size() + "):");
		for (String key : props.stringPropertyNames()) {
			log.log(level, key + " = " + props.getProperty(key));
		}
	}

	private int teamId = -1;
	private Properties info = null;
	private Properties options = null;
	private OOAICallback clb = null;
	private String myDataDir = null;
	private String myLogFile = null;
	private Logger log = null;

	private static final int DEFAULT_ZONE = 0;


	NullOOJavaAI(int teamId, Properties info, Properties options) {

		this.teamId = teamId;
		this.info = info;
		this.options = options;
	}

	private int handleEngineCommand(AICommand command) {
		return clb.handleCommand(
				com.clan_sy.spring.ai.AICommandWrapper.COMMAND_TO_ID_ENGINE,
				-1, command);
	}
	private int sendTextMsg(String msg) {

		SendTextMessageAICommand msgCmd
				= new SendTextMessageAICommand(msg, DEFAULT_ZONE);
		return handleEngineCommand(msgCmd);
	}

	@Override
	public int init(int teamId, OOAICallback callback, Properties info,
			Properties options) {

		int ret = -1;

		// initialize the log
		try {
			myDataDir = info.getProperty("dataDir");
			myLogFile = myDataDir + "/log.txt";
			FileHandler fileLogger = new FileHandler(myLogFile, false);
			fileLogger.setFormatter(new MyCustomLogFormatter());
			fileLogger.setLevel(Level.ALL);
			log = Logger.getLogger("nulloojavaai");
			log.addHandler(fileLogger);
			if (NullOOJavaAIFactory.isDebugging()) {
				log.setLevel(Level.ALL);
			} else {
				log.setLevel(Level.INFO);
			}
		} catch (Exception ex) {
			System.out.println("NullOOJavaAI: Failed initializing the logger!");
			ex.printStackTrace();
			ret = -2;
		}

		this.clb = callback;

		try {
			log.info("initializing team " + teamId);

			log.log(Level.FINE, "info:");
			logProperties(log, Level.FINE, info);

			log.log(Level.FINE, "options:");
			logProperties(log, Level.FINE, options);

			ret = 0;
		} catch (Exception ex) {
			log.log(Level.SEVERE, "Failed initializing", ex);
			ret = -3;
		}

		return ret;
	}

	@Override
	public int update(int frame) {

		if (frame % 300 == 0) {
			ArrayList<Resource> resources = clb.getResources();
			for (Resource resource : resources) {
				sendTextMsg("Resource " + resource.getName() + " optimum: "
						+ resource.getOptimum());
				sendTextMsg("Resource " + resource.getName() + " current: "
						+ clb.getEconomy().getCurrent(resource));
				sendTextMsg("Resource " + resource.getName() + " income: "
						+ clb.getEconomy().getIncome(resource));
				sendTextMsg("Resource " + resource.getName() + " storage: "
						+ clb.getEconomy().getStorage(resource));
				sendTextMsg("Resource " + resource.getName() + " usage: "
						+ clb.getEconomy().getUsage(resource));
			}
		}

		return 0; // signaling: OK
	}

	@Override
	public int message(int player, String message) {
		return 0; // signaling: OK
	}

	@Override
	public int unitCreated(Unit unit) {

		int ret = sendTextMsg("unitCreated: " + unit.getDef().getName());

		return ret;
	}

	@Override
	public int unitFinished(Unit unit) {
		return 0; // signaling: OK
	}

	@Override
	public int unitIdle(Unit unit) {
		return 0; // signaling: OK
	}

	@Override
	public int unitMoveFailed(Unit unit) {
		return 0; // signaling: OK
	}

	@Override
	public int unitDamaged(Unit unit, Unit attacker, float damage, AIFloat3 dir) {
		return 0; // signaling: OK
	}

	@Override
	public int unitDestroyed(Unit unit, Unit attacker) {
		return 0; // signaling: OK
	}

	@Override
	public int unitGiven(Unit unit, int oldTeamId, int newTeamId) {
		return 0; // signaling: OK
	}

	@Override
	public int unitCaptured(Unit unit, int oldTeamId, int newTeamId) {
		return 0; // signaling: OK
	}

	@Override
	public int enemyEnterLOS(Unit enemy) {
		return 0; // signaling: OK
	}

	@Override
	public int enemyLeaveLOS(Unit enemy) {
		return 0; // signaling: OK
	}

	@Override
	public int enemyEnterRadar(Unit enemy) {
		return 0; // signaling: OK
	}

	@Override
	public int enemyLeaveRadar(Unit enemy) {
		return 0; // signaling: OK
	}

	@Override
	public int enemyDamaged(Unit enemy, Unit attacker, float damage, AIFloat3 dir) {
		return 0; // signaling: OK
	}

	@Override
	public int enemyDestroyed(Unit enemy, Unit attacker) {
		return 0; // signaling: OK
	}

	@Override
	public int weaponFired(Unit unit, WeaponDef weaponDef) {
		return 0; // signaling: OK
	}

	@Override
	public int playerCommand(ArrayList<Unit> units, AICommand command, int playerId) {
		return 0; // signaling: OK
	}

	@Override
	public int commandFinished(Unit unit, int commandTopicId) {
		return 0; // signaling: OK
	}

	@Override
	public int seismicPing(AIFloat3 pos, float strength) {
		return 0; // signaling: OK
	}

}
