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


import com.springrts.ai.oo.*;

/**
 * Serves as Interface for a Java Skirmish AIs for the Spring engine.
 *
 * @author	hoijui
 * @version	0.1
 */
public class NullOOJavaAIFactory extends OOAIFactory {

	public NullOOJavaAIFactory() {}

	public static boolean isDebugging() {
		return true;
	}

	@Override
	public OOAI createAI(int teamId, OOAICallback callback) {
		return new NullOOJavaAI(teamId, callback);
	}
}
