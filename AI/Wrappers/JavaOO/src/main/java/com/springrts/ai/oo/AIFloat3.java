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

package com.springrts.ai.oo;


import javax.vecmath.Tuple3d;
import javax.vecmath.Tuple3f;
import javax.vecmath.Vector3d;
import javax.vecmath.Vector3f;
import java.awt.Color;

/**
 * Represents a position on the map.
 *
 * @author	hoijui.quaero@gmail.com
 * @version	0.1
 */
public class AIFloat3 extends Vector3f {

	public AIFloat3() {
		super(0.0f, 0.0f, 0.0f);
	}
	public AIFloat3(float x, float y, float z) {
		super(x, y, z);
	}
	public AIFloat3(float[] xyz) {
		super(xyz);
	}
	public AIFloat3(AIFloat3 other) {
		super(other);
	}
	public AIFloat3(Tuple3d tub3d) {
		super(tub3d);
	}
	public AIFloat3(Tuple3f tub3f) {
		super(tub3f);
	}
	public AIFloat3(Vector3d vec3d) {
		super(vec3d);
	}
	public AIFloat3(Vector3f vec3f) {
		super(vec3f);
	}
	public AIFloat3(Color color) {

		this.x = color.getRed()   / 255.0F;
		this.y = color.getGreen() / 255.0F;
		this.z = color.getBlue()  / 255.0F;
	}

	public Color toColor() {
		return new Color(x, y, z);
	}
	@Override
	public String toString() {
		return "(" + this.x + ", " + this.y + ", " + this.z + ")";
	}
	public float[] toFloatArray() {

		float[] floatArr = new float[3];
		loadInto(floatArr);
		return floatArr;
	}
	public void loadInto(float[] xyz) {

		xyz[0] = x;
		xyz[1] = y;
		xyz[2] = z;
	}

	@Override
	public int hashCode() {

		final int prime = 31;
		int result = super.hashCode();
		result = prime * result + Float.floatToIntBits(x);
		result = prime * result + Float.floatToIntBits(y);
		result = prime * result + Float.floatToIntBits(z);
		return result;
	}
	@Override
	public boolean equals(Object obj) {

		if (this == obj) {
			return true;
		} else if (!super.equals(obj)) {
			return false;
		} else if (getClass() != obj.getClass()) {
			return false;
		}

		AIFloat3 other = (AIFloat3) obj;
		if (Float.floatToIntBits(x) != Float.floatToIntBits(other.x)) {
			return false;
		} else if (Float.floatToIntBits(y) != Float.floatToIntBits(other.y)) {
			return false;
		} else if (Float.floatToIntBits(z) != Float.floatToIntBits(other.z)) {
			return false;
		} else {
			return true;
		}
	}
}
