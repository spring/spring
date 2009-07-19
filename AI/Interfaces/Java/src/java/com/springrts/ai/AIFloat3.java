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

package com.springrts.ai;


import com.sun.jna.Structure;
import java.awt.Color;
import javax.vecmath.Vector3f;

/**
 * Wrapper for file:
 * rts/ExternalAI/Interface/SAIFloat3.h
 *
 * @author	hoijui.quaero@gmail.com
 * @version	0.1
 */
public final class AIFloat3 extends Structure implements Structure.ByValue {

	public float x, y, z;

	public AIFloat3() {

		this.x = 0.0f;
		this.y = 0.0f;
		this.z = 0.0f;
	}
	public AIFloat3(final AIFloat3 float3) {

		this.x = float3.x;
		this.y = float3.y;
		this.z = float3.z;
	}
	public AIFloat3(float x, float y, float z) {

		this.x = x;
		this.y = y;
		this.z = z;
	}
	public AIFloat3(final float[] xyz) {

		this.x = xyz[0];
		this.y = xyz[1];
		this.z = xyz[2];
	}
	public AIFloat3(final Color color) {

		this.x = color.getRed()   / 255.0F;
		this.y = color.getGreen() / 255.0F;
		this.z = color.getBlue()  / 255.0F;
	}
	public AIFloat3(final Vector3f vec3f) {

		float[] tmpXyz = new float[3];
		vec3f.get(tmpXyz);
		this.x = tmpXyz[0];
		this.y = tmpXyz[1];
		this.z = tmpXyz[2];
	}

	public Color toColor() {
		return new Color(x, y, z);
	}
	public Vector3f toVector3f() {
		return new Vector3f(x, y, z);
	}
	@Override
	public String toString() {
		return "(" + this.x + ", " + this.y + ", " + this.z + ")";
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
