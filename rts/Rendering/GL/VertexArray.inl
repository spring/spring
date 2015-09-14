/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// calls to this function will be removed by the optimizer unless the size is too small
void CVertexArray::CheckInitSize(const unsigned int vertexes, const unsigned int strips) {
	if(vertexes>VA_INIT_VERTEXES || strips>VA_INIT_STRIPS) { 
		handleerror(drawArrayPos=NULL, "Vertex array initial size is too small", "Rendering error", MBF_OK | MBF_EXCL);
	}
}

void CVertexArray::CheckEnlargeDrawArray() {
	if ((char*) drawArrayPos > ((char*) drawArraySize - 10 * sizeof(float)))
		EnlargeDrawArray();
}

void CVertexArray::EnlargeArrays(const unsigned int vertexes, const unsigned int strips, const unsigned int stripsize) {
	while ((char*) drawArrayPos > ((char*) drawArraySize - stripsize * sizeof(float) * vertexes))
		EnlargeDrawArray();

	while ((char*) stripArrayPos > ((char*) stripArraySize - sizeof(unsigned int) * strips))
		EnlargeStripArray();
}


//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////

#ifdef DEBUG
	#define ASSERT_SIZE(x) assert(drawArrayPos + x <= drawArraySize);
#else
	#define ASSERT_SIZE(x)
#endif



void CVertexArray::AddVertexQ0(const float3& pos) {
	ASSERT_SIZE(VA_SIZE_0)
	VA_TYPE_0* vat = GetTypedVertexArrayQ<VA_TYPE_0>(1);
	vat->p = pos;
}

void CVertexArray::AddVertexQ0(float x, float y, float z) {
	ASSERT_SIZE(VA_SIZE_0)
	VA_TYPE_0* vat = GetTypedVertexArrayQ<VA_TYPE_0>(1);
	vat->p.x = x;
	vat->p.y = y;
	vat->p.z = z;
}

void CVertexArray::AddVertexQN(const float3& pos, const float3& normal) {
	ASSERT_SIZE(VA_SIZE_N)
	VA_TYPE_N* vat = GetTypedVertexArrayQ<VA_TYPE_N>(1);
	vat->p = pos;
	vat->n = normal;
}

void CVertexArray::AddVertexQC(const float3& pos, const unsigned char* color) {
	ASSERT_SIZE(VA_SIZE_C)
	VA_TYPE_C* vat = GetTypedVertexArrayQ<VA_TYPE_C>(1);
	vat->p = pos;
	vat->c = SColor(color);
}

void CVertexArray::AddVertexQT(const float3& pos, float tx, float ty) {
	ASSERT_SIZE(VA_SIZE_T)
	VA_TYPE_T* vat = GetTypedVertexArrayQ<VA_TYPE_T>(1);
	vat->p = pos;
	vat->s = tx;
	vat->t = ty;
}

void CVertexArray::AddVertexQTN(const float3& pos, float tx, float ty, const float3& norm) {
	ASSERT_SIZE(VA_SIZE_TN)
	VA_TYPE_TN* vat = GetTypedVertexArrayQ<VA_TYPE_TN>(1);
	vat->p = pos;
	vat->s = tx;
	vat->t = ty;
	vat->n = norm;
}

void CVertexArray::AddVertexQTNT(const float3& p, float tx, float ty, const float3& n, const float3& st, const float3& tt) {
	ASSERT_SIZE(VA_SIZE_TNT)
	VA_TYPE_TNT* vat = GetTypedVertexArrayQ<VA_TYPE_TNT>(1);
	vat->p = p;
	vat->s = tx;
	vat->t = ty;
	vat->n = n;
	vat->uv1 = st;
	vat->uv2 = tt;
}

void CVertexArray::AddVertexQTC(const float3& pos, float tx, float ty, const unsigned char* col) {
	ASSERT_SIZE(VA_SIZE_TC)
	VA_TYPE_TC* vat = GetTypedVertexArrayQ<VA_TYPE_TC>(1);
	vat->p = pos;
	vat->s = tx;
	vat->t = ty;
	vat->c = SColor(col);
}

void CVertexArray::AddVertexQ2d0(float x, float z) {
	ASSERT_SIZE(VA_SIZE_2D0)
	VA_TYPE_2d0* vat = GetTypedVertexArrayQ<VA_TYPE_2d0>(1);
	vat->x = x;
	vat->y = z;
}

void CVertexArray::AddVertexQ2dT(float x, float y, float tx, float ty) {
	ASSERT_SIZE(VA_SIZE_2DT)
	VA_TYPE_2dT* vat = GetTypedVertexArrayQ<VA_TYPE_2dT>(1);
	vat->x = x;
	vat->y = y;
	vat->s = tx;
	vat->t = ty;
}

void CVertexArray::AddVertexQ2dTC(float x, float y, float tx, float ty, const unsigned char* c) {
	ASSERT_SIZE(VA_SIZE_2DTC)
	VA_TYPE_2dTC* vat = GetTypedVertexArrayQ<VA_TYPE_2dTC>(1);
	vat->x = x;
	vat->y = y;
	vat->s = tx;
	vat->t = ty;
	vat->c = SColor(c);
}



//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////

void CVertexArray::AddVertex0(const float3& pos) {
	CheckEnlargeDrawArray();
	AddVertexQ0(pos);
}

void CVertexArray::AddVertex0(float x, float y, float z) {
	CheckEnlargeDrawArray();
	AddVertexQ0(x,y,z);
}

void CVertexArray::AddVertexN(const float3& pos, const float3& normal) {
	CheckEnlargeDrawArray();
	AddVertexQN(pos, normal);
}

void CVertexArray::AddVertexC(const float3& pos, const unsigned char* color) {
	CheckEnlargeDrawArray();
	AddVertexQC(pos, color);
}

void CVertexArray::AddVertexT(const float3& pos, float tx, float ty) {
	CheckEnlargeDrawArray();
	AddVertexQT(pos, tx, ty);
}

void CVertexArray::AddVertexTN(const float3& pos, float tx, float ty, const float3& norm) {
	CheckEnlargeDrawArray();
	AddVertexQTN(pos, tx, ty, norm);
}

void CVertexArray::AddVertexTNT(const float3& p, float tx, float ty, const float3& n, const float3& st, const float3& tt) {
	CheckEnlargeDrawArray();
	AddVertexQTNT(p, tx, ty, n, st, tt);
}

void CVertexArray::AddVertexTC(const float3& pos, float tx, float ty, const unsigned char* col) {
	CheckEnlargeDrawArray();
	AddVertexQTC(pos, tx, ty, col);
}

void CVertexArray::AddVertex2d0(float x, float z) {
	CheckEnlargeDrawArray();
	AddVertexQ2d0(x,z);
}

void CVertexArray::AddVertex2dT(float x, float y, float tx, float ty) {
	CheckEnlargeDrawArray();
	AddVertexQ2dT(x, y, tx, ty);
}

void CVertexArray::AddVertex2dTC(float x, float y, float tx, float ty, const unsigned char* col) {
	CheckEnlargeDrawArray();
	AddVertexQ2dTC(x, y, tx, ty, col);
}


//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////

void CVertexArray::CheckEndStrip() {
	if (stripArrayPos == stripArray || ((ptrdiff_t) * (stripArrayPos - 1)) != ((char*) drawArrayPos - (char*) drawArray))
		EndStrip();
}

