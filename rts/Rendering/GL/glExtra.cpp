/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "glExtra.h"
#include "VertexArray.h"
#include "Map/Ground.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/myMath.h"
#include "System/Threading/ThreadPool.h"


/**
 *  Draws a trigonometric circle in 'resolution' steps.
 */
static void defSurfaceCircle(const float3& center, float radius, unsigned int res)
{
	CVertexArray* va = GetVertexArray();
	va->Initialize();
	for (unsigned int i = 0; i < res; ++i) {
		const float radians = math::TWOPI * (float)i / (float)res;
		float3 pos;
		pos.x = center.x + (fastmath::sin(radians) * radius);
		pos.z = center.z + (fastmath::cos(radians) * radius);
		pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + 5.0f;
		va->AddVertex0(pos);
	}
	va->DrawArray0(GL_LINE_LOOP);
}

static void defSurfaceSquare(const float3& center, float xsize, float zsize)
{
	// FIXME: terrain contouring
	const float3 p0 = center + float3(-xsize, 0.0f, -zsize);
	const float3 p1 = center + float3( xsize, 0.0f, -zsize);
	const float3 p2 = center + float3( xsize, 0.0f,  zsize);
	const float3 p3 = center + float3(-xsize, 0.0f,  zsize);

	CVertexArray* va = GetVertexArray();
	va->Initialize();
		va->AddVertex0(p0.x, CGround::GetHeightAboveWater(p0.x, p0.z, false), p0.z);
		va->AddVertex0(p1.x, CGround::GetHeightAboveWater(p1.x, p1.z, false), p1.z);
		va->AddVertex0(p2.x, CGround::GetHeightAboveWater(p2.x, p2.z, false), p2.z);
		va->AddVertex0(p3.x, CGround::GetHeightAboveWater(p3.x, p3.z, false), p3.z);
	va->DrawArray0(GL_LINE_LOOP);
}


SurfaceCircleFunc glSurfaceCircle = defSurfaceCircle;
SurfaceSquareFunc glSurfaceSquare = defSurfaceSquare;

void setSurfaceCircleFunc(SurfaceCircleFunc func)
{
	glSurfaceCircle = (func == nullptr)? defSurfaceCircle: func;
}

void setSurfaceSquareFunc(SurfaceSquareFunc func)
{
	glSurfaceSquare = (func == nullptr)? defSurfaceSquare: func;
}




// typedef float (*WeaponRangeFunc)(const CWeapon*, const WeaponDef*, float, float);
// static constexpr WeaponRangeFunc weaponRangeFuncs[] = {

static constexpr float (*weaponRangeFuncs[])(const CWeapon*, const WeaponDef*, float, float) = {
	CWeapon::GetStaticRange2D,
	CWeapon::GetLiveRange2D,
};

static void glBallisticCircle(const CWeapon* weapon, const WeaponDef* weaponDef, unsigned int resolution, const float3& center, const float3& params)
{
	constexpr int resDiv = 50;

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(resolution *= 2, 0, VA_SIZE_0);

	float3* vertices = va->GetTypedVertexArray<float3>(resolution);

	const float radius = params.x;
	const float slope = params.y;

	const float wdHeightMod = weaponDef->heightmod;
	const float wdProjGravity = mix(params.z, -weaponDef->myGravity, weaponDef->myGravity != 0.0f);

	for_mt(0, resolution, [&](const int i) {
		const float radians = math::TWOPI * (float)i / (float)resolution;

		const float sinR = fastmath::sin(radians);
		const float cosR = fastmath::cos(radians);

		float maxWeaponRange = radius;

		float3 pos;
		pos.x = center.x + (sinR * maxWeaponRange);
		pos.z = center.z + (cosR * maxWeaponRange);
		pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false);

		float posHeightDelta = (pos.y - center.y) * 0.5f;
		float posWeaponRange = weaponRangeFuncs[weapon != nullptr](weapon, weaponDef, posHeightDelta * wdHeightMod, wdProjGravity);
		float rangeIncrement = (maxWeaponRange -= (posHeightDelta * slope)) * 0.5f;
		float ydiff = 0.0f;

		// "binary search" for the maximum positional range per angle, accounting for terrain height
		for (int j = 0; j < resDiv && (std::fabs(posWeaponRange - maxWeaponRange) + ydiff) > (0.01f * maxWeaponRange); j++) {
			if (posWeaponRange > maxWeaponRange) {
				maxWeaponRange += rangeIncrement;
			} else {
				// overshot, reduce step-size
				maxWeaponRange -= rangeIncrement;
				rangeIncrement *= 0.5f;
			}

			pos.x = center.x + (sinR * maxWeaponRange);
			pos.z = center.z + (cosR * maxWeaponRange);

			const float newY = CGround::GetHeightAboveWater(pos.x, pos.z, false);
			ydiff = std::fabs(pos.y - newY);
			pos.y = newY;

			posHeightDelta = pos.y - center.y;
			posWeaponRange = weaponRangeFuncs[weapon != nullptr](weapon, weaponDef, posHeightDelta * wdHeightMod, wdProjGravity);
		}

		pos.x = center.x + (sinR * posWeaponRange);
		pos.z = center.z + (cosR * posWeaponRange);
		pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + 5.0f;

		vertices[i] = pos;
	});

	va->DrawArray0(GL_LINE_LOOP);
}


/*
 *  Draws a trigonometric circle in 'resolution' steps, with a slope modifier
 */
void glBallisticCircle(const CWeapon* weapon, unsigned int resolution, const float3& center, const float3& params)
{
	glBallisticCircle(weapon, weapon->weaponDef, resolution, center, params);
}

void glBallisticCircle(const WeaponDef* weaponDef, unsigned int resolution, const float3& center, const float3& params)
{
	glBallisticCircle(nullptr, weaponDef, resolution, center, params);
}


/******************************************************************************/

void glDrawVolume(DrawVolumeFunc drawFunc, const void* data)
{
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP_NV);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		glEnable(GL_STENCIL_TEST);
		glStencilMask(0x1);
		glStencilFunc(GL_ALWAYS, 0, 0x1);
		glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
		drawFunc(data); // draw

	glDisable(GL_DEPTH_TEST);

	glStencilFunc(GL_NOTEQUAL, 0, 0x1);
	glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO); // clear as we go

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	drawFunc(data);   // draw

	glDisable(GL_DEPTH_CLAMP_NV);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}



/******************************************************************************/

static constexpr std::array<float3, 2 * 4> BOX_VERTS = {
	// bottom-face, ccw
	float3{-0.5f, -0.5f,  0.5f},
	float3{ 0.5f, -0.5f,  0.5f},
	float3{ 0.5f, -0.5f, -0.5f},
	float3{-0.5f, -0.5f, -0.5f},
	// top-face, ccw
	float3{-0.5f,  0.5f,  0.5f},
	float3{ 0.5f,  0.5f,  0.5f},
	float3{ 0.5f,  0.5f, -0.5f},
	float3{-0.5f,  0.5f, -0.5f},
};

static std::vector<float3> CYL_VERTS;
static std::vector<float3> SPH_VERTS;


static constexpr std::array<uint32_t, 4 * 6> BOX_INDCS = {
	0, 1, 2, 3, // bottom
	4, 5, 6, 7, // top
	0, 1, 5, 4, // front
	1, 2, 6, 5, // right
	2, 3, 7, 6, // back
	3, 0, 4, 7, // left
};

static std::vector<uint32_t> CYL_INDCS;
static std::vector<uint32_t> SPH_INDCS;

void gleGenBoxMeshBuffer(unsigned int* meshData) {
	if (meshData == nullptr)
		return;

	glGenBuffers(1, &meshData[0]);
	glBindBuffer(GL_ARRAY_BUFFER, meshData[0]);
	glBufferData(GL_ARRAY_BUFFER, BOX_VERTS.size() * sizeof(float3), BOX_VERTS.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &meshData[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, BOX_INDCS.size() * sizeof(uint32_t), BOX_INDCS.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}



void gleGenCylMeshBuffer(unsigned int* meshData, unsigned int numDivs, float zSize) {
	assert(numDivs > 2);
	assert(zSize > 0.0f);

	CYL_VERTS.clear();
	CYL_VERTS.resize(2 + numDivs * 2);
	CYL_INDCS.clear();
	CYL_INDCS.reserve(numDivs * 2);

	CYL_VERTS[0] = ZeroVector;
	CYL_VERTS[1] = FwdVector * zSize;

	// front end-cap
	for (unsigned int n = 0; n <= numDivs; n++) {
		const unsigned int i = 2 + (n + 0) % numDivs;
		const unsigned int j = 2 + (n + 1) % numDivs;

		CYL_VERTS[i].x = std::cos(i * (math::TWOPI / numDivs));
		CYL_VERTS[i].y = std::sin(i * (math::TWOPI / numDivs));
		CYL_VERTS[i].z = 0.0f;

		CYL_INDCS.push_back(0);
		CYL_INDCS.push_back(i);
		CYL_INDCS.push_back(j);
	}

	// back end-cap
	for (unsigned int n = 0; n <= numDivs; n++) {
		const unsigned int i = 2 + (n + 0) % numDivs;
		const unsigned int j = 2 + (n + 1) % numDivs;

		CYL_VERTS[i + numDivs].x = CYL_VERTS[i].x;
		CYL_VERTS[i + numDivs].y = CYL_VERTS[i].y;
		CYL_VERTS[i + numDivs].z = zSize;

		CYL_INDCS.push_back(1);
		CYL_INDCS.push_back(i + numDivs);
		CYL_INDCS.push_back(j + numDivs);
	}


	// sides
	for (unsigned int n = 0; n < numDivs; n++) {
		const unsigned int i0 = 2 + (n + 0)                    ;
		const unsigned int i1 = 2 + (n + 0) + numDivs          ;
		const unsigned int i2 = 2 + (n + 1) % numDivs          ;
		const unsigned int i3 = 2 + (n + 1) % numDivs + numDivs;

		CYL_INDCS.push_back(i0);
		CYL_INDCS.push_back(i1);
		CYL_INDCS.push_back(i2);

		CYL_INDCS.push_back(i2);
		CYL_INDCS.push_back(i3);
		CYL_INDCS.push_back(i1);
	}

	if (meshData == nullptr)
		return;

	glGenBuffers(1, &meshData[0]);
	glBindBuffer(GL_ARRAY_BUFFER, meshData[0]);
	glBufferData(GL_ARRAY_BUFFER, CYL_VERTS.size() * sizeof(float3), CYL_VERTS.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &meshData[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, CYL_INDCS.size() * sizeof(uint32_t), CYL_INDCS.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}



void gleGenSphMeshBuffer(unsigned int* meshData, unsigned int numRows, unsigned int numCols) {
	assert(numRows > 2);
	assert(numCols > 2);

	SPH_VERTS.clear();
	SPH_VERTS.resize((numRows + 1) * numCols);
	SPH_INDCS.clear();
	SPH_INDCS.reserve((numRows + 1) * numCols);

	for (unsigned int row = 0; row <= numRows; row++) {
		for (unsigned int col = 0; col < numCols; col++) {
			const float a = (col * (math::TWOPI / numCols));
			const float b = (row * (math::PI    / numRows));

			float3& v = SPH_VERTS[row * numCols + col];

			v.x = std::cos(a) * std::sin(b);
			v.y = std::sin(a) * std::sin(b);
			v.z = std::cos(b);
		}
	}


	// top slice
	for (unsigned int col = 0; col <= numCols; col++) {
		const unsigned int i = 1 * numCols + ((col + 0) % numCols);
		const unsigned int j = 1 * numCols + ((col + 1) % numCols);

		SPH_INDCS.push_back(0);
		SPH_INDCS.push_back(i);
		SPH_INDCS.push_back(j);
	}

	// bottom slice
	for (unsigned int col = 0; col <= numCols; col++) {
		const unsigned int i = ((numRows - 1) * numCols) + ((col + 0) % numCols);
		const unsigned int j = ((numRows - 1) * numCols) + ((col + 1) % numCols);

		SPH_INDCS.push_back(SPH_VERTS.size() - 1);
		SPH_INDCS.push_back(i);
		SPH_INDCS.push_back(j);
	}

	// middle slices
	for (unsigned int row = 1; row < (numRows - 1); row++) {
		for (unsigned int col = 0; col < numCols; col++) {
			const unsigned int i0 = (row + 0) * numCols + (col + 0)          ;
			const unsigned int i1 = (row + 1) * numCols + (col + 0)          ;
			const unsigned int i2 = (row + 0) * numCols + (col + 1) % numCols;
			const unsigned int i3 = (row + 1) * numCols + (col + 1) % numCols;

			SPH_INDCS.push_back(i0);
			SPH_INDCS.push_back(i1);
			SPH_INDCS.push_back(i2);

			SPH_INDCS.push_back(i2);
			SPH_INDCS.push_back(i3);
			SPH_INDCS.push_back(i1);
		}
	}

	if (meshData == nullptr)
		return;

	glGenBuffers(1, &meshData[0]);
	glBindBuffer(GL_ARRAY_BUFFER, meshData[0]);
	glBufferData(GL_ARRAY_BUFFER, SPH_VERTS.size() * sizeof(float3), SPH_VERTS.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &meshData[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, SPH_INDCS.size() * sizeof(uint32_t), SPH_INDCS.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}



void gleGenColVolMeshBuffers(unsigned int* meshData) {
	const unsigned int cylDivs = meshData[0];
	const unsigned int sphRows = meshData[1];
	const unsigned int sphCols = meshData[2];

	gleGenBoxMeshBuffer(nullptr);
	gleGenCylMeshBuffer(nullptr, cylDivs, 1.0f);
	gleGenSphMeshBuffer(nullptr, sphRows, sphCols);

	{
		// VAO
		glGenVertexArrays(1, &meshData[2]);
		glBindVertexArray(meshData[2]);
	}
	{
		// VBO
		glGenBuffers(1, &meshData[0]);
		glBindBuffer(GL_ARRAY_BUFFER, meshData[0]);
		glBufferData(GL_ARRAY_BUFFER, (BOX_VERTS.size() + CYL_VERTS.size() + SPH_VERTS.size()) * sizeof(float3), nullptr, GL_STATIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, (                                  0) * sizeof(float3), BOX_VERTS.size() * sizeof(float3), BOX_VERTS.data());
		glBufferSubData(GL_ARRAY_BUFFER, (BOX_VERTS.size()                   ) * sizeof(float3), CYL_VERTS.size() * sizeof(float3), CYL_VERTS.data());
		glBufferSubData(GL_ARRAY_BUFFER, (BOX_VERTS.size() + CYL_VERTS.size()) * sizeof(float3), SPH_VERTS.size() * sizeof(float3), SPH_VERTS.data());
	}

	{
		for (size_t n = 0; n < CYL_INDCS.size(); n++) { CYL_INDCS[n] += (BOX_VERTS.size()                   ); }
		for (size_t n = 0; n < SPH_INDCS.size(); n++) { SPH_INDCS[n] += (BOX_VERTS.size() + CYL_VERTS.size()); }

		// IBO
		glGenBuffers(1, &meshData[1]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (BOX_INDCS.size() + CYL_INDCS.size() + SPH_INDCS.size()) * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (                                  0) * sizeof(uint32_t), BOX_INDCS.size() * sizeof(uint32_t), BOX_INDCS.data());
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (BOX_INDCS.size()                   ) * sizeof(uint32_t), CYL_INDCS.size() * sizeof(uint32_t), CYL_INDCS.data());
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (BOX_INDCS.size() + CYL_INDCS.size()) * sizeof(uint32_t), SPH_INDCS.size() * sizeof(uint32_t), SPH_INDCS.data());
	}
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,  3, GL_FLOAT,  false,  sizeof(float3), nullptr);
		glBindVertexArray(0);
		glDisableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	meshData[3] = BOX_VERTS.size();
	meshData[4] = BOX_INDCS.size();
	meshData[5] = CYL_VERTS.size();
	meshData[6] = CYL_INDCS.size();
	meshData[7] = SPH_VERTS.size();
	meshData[8] = SPH_INDCS.size();
}

void gleDelColVolMeshBuffers(unsigned int* meshData) {
	glDeleteBuffers(1, &meshData[0]);
	glDeleteBuffers(1, &meshData[1]);
	glDeleteVertexArrays(1, &meshData[2]);

	meshData[0] = 0;
	meshData[1] = 0;
	meshData[2] = 0;
}


void gleBindColVolMeshBuffers(const unsigned int* meshData) {
	if (meshData != nullptr) {
		glBindVertexArray(meshData[2]);
	} else {
		glBindVertexArray(0);
	}
}

void gleDrawColVolMeshSubBuffer(const unsigned int* meshData, unsigned int meshType) {
	constexpr unsigned int primTypes[] = {GL_QUADS, GL_TRIANGLES, GL_TRIANGLES};

	// const unsigned int numVerts = meshData[3 + meshType * 2 + 0];
	const unsigned int numIndcs = meshData[3 + meshType * 2 + 1];
	const unsigned int startIdx =
		(meshData[3 + 0 * 2 + 1] * (meshType > 0)) +
		(meshData[3 + 1 * 2 + 1] * (meshType > 1)) +
		(meshData[3 + 2 * 2 + 1] * (meshType > 2));

	glDrawElements(primTypes[meshType], numIndcs,  GL_UNSIGNED_INT, VA_TYPE_OFFSET(uint32_t, startIdx));
}

