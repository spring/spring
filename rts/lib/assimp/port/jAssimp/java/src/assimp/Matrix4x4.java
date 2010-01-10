/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

 * Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

 * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

 * Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
 */

package assimp;

/**
 * Represents a 4x4 matrix.
 * 
 * There's no explicit convention for 'majorness'; objects of Matrix4x4 can be
 * either row-major or column-major.
 * 
 * @author Alexander Gessler (aramis_acg@users.sourceforge.net)
 * @version 1.0
 * @see Node#getTransformColumnMajor()
 * @see Node#getTransformRowMajor()()
 */
public class Matrix4x4 {

	/**
	 * Default constructor. Initializes the matrix with its identity
	 */
	public Matrix4x4() {
		coeff[0] = coeff[5] = coeff[10] = coeff[15] = 1.0f;
	}

	/**
	 * Copy constructor
	 * 
	 * @param other
	 *            Matrix to be copied
	 */
	public Matrix4x4(Matrix4x4 other) {
		System.arraycopy(other.coeff, 0, this.coeff, 0, 16);
	}

	/**
	 * Construction from 16 given coefficients
	 * 
	 * @param a1
	 * @param a2
	 * @param a3
	 * @param a4
	 * @param b1
	 * @param b2
	 * @param b3
	 * @param b4
	 * @param c1
	 * @param c2
	 * @param c3
	 * @param c4
	 * @param d1
	 * @param d2
	 * @param d3
	 * @param d4
	 */
	public Matrix4x4(float a1, float a2, float a3, float a4, float b1,
			float b2, float b3, float b4, float c1, float c2, float c3,
			float c4, float d1, float d2, float d3, float d4) {

		coeff[0] = a1;
		coeff[1] = a2;
		coeff[2] = a3;
		coeff[3] = a4;
		coeff[4] = b1;
		coeff[5] = b2;
		coeff[6] = b3;
		coeff[7] = b4;
		coeff[8] = c1;
		coeff[9] = c2;
		coeff[10] = c3;
		coeff[11] = c4;
		coeff[12] = d1;
		coeff[13] = d2;
		coeff[14] = d3;
		coeff[15] = d4;
	}

	/**
	 * Copy constructor (construction from a 4x4 matrix)
	 * 
	 * @param other
	 *            Matrix to be copied
	 */
	public Matrix4x4(Matrix3x3 other) {
		coeff[0] = other.coeff[0];
		coeff[1] = other.coeff[1];
		coeff[2] = other.coeff[2];
		coeff[4] = other.coeff[3];
		coeff[5] = other.coeff[4];
		coeff[6] = other.coeff[5];
		coeff[8] = other.coeff[6];
		coeff[9] = other.coeff[7];
		coeff[10] = other.coeff[8];
		coeff[15] = 1.0f;
	}

	/**
	 * The coefficients of the matrix
	 */
	public float[] coeff = new float[16];

	/**
	 * Returns a field in the matrix
	 * 
	 * @param row
	 *            Row index
	 * @param column
	 *            Column index
	 * @return The corresponding field value
	 */
	public final float get(int row, int column) {
		assert (row <= 3 && column <= 3);
		return coeff[row * 4 + column];
	}

	/**
	 * Computes the transpose of *this* matrix.
	 */
	public final Matrix4x4 transpose() {

		float fTemp = coeff[1 * 4 + 0];
		coeff[1 * 4 + 0] = coeff[0 * 4 + 1];
		coeff[0 * 4 + 1] = fTemp;

		fTemp = coeff[2 * 4 + 0];
		coeff[2 * 4 + 0] = coeff[0 * 4 + 2];
		coeff[0 * 4 + 2] = fTemp;

		fTemp = coeff[2 * 4 + 1];
		coeff[2 * 4 + 1] = coeff[1 * 4 + 2];
		coeff[1 * 4 + 2] = fTemp;

		fTemp = coeff[3 * 4 + 0];
		coeff[3 * 4 + 0] = coeff[0 * 4 + 3];
		coeff[0 * 4 + 3] = fTemp;

		fTemp = coeff[3 * 4 + 1];
		coeff[3 * 4 + 1] = coeff[1 * 4 + 3];
		coeff[1 * 4 + 3] = fTemp;

		fTemp = coeff[3 * 4 + 2];
		coeff[3 * 4 + 2] = coeff[2 * 4 + 3];
		coeff[2 * 4 + 3] = fTemp;
		return this;
	}
}
