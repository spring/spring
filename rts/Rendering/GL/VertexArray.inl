#ifdef DEBUG
	#define ASSERT_SIZE(x) assert(drawArraySize>=drawArrayPos+ x );
#else
	#define ASSERT_SIZE(x)
#endif


//! calls to this function will be removed by the optimizer unless the size is too small
void CVertexArray::CheckInitSize(const unsigned int vertexes, const unsigned int strips) {
	if(vertexes>VA_INIT_VERTEXES || strips>VA_INIT_STRIPS) { 
		handleerror(drawArrayPos=NULL, "Vertex array initial size is too small", "Rendering error", MBF_OK | MBF_EXCL);
	}
}

void CVertexArray::CheckEnlargeDrawArray() {
	if((char *)drawArrayPos>(char *)drawArraySize-10*sizeof(float))
		EnlargeDrawArray();
}

void CVertexArray::EnlargeArrays(const unsigned int vertexes, const unsigned int strips, const unsigned int stripsize) {
	while((char *)drawArrayPos>(char *)drawArraySize-stripsize*sizeof(float)*vertexes)
		EnlargeDrawArray();

	while((char *)stripArrayPos>(char *)stripArraySize-sizeof(unsigned int)*strips)
		EnlargeStripArray();
}

void CVertexArray::AddVertexQ0(const float x, const float y, const float z) {
	ASSERT_SIZE(VA_SIZE_0)
	*drawArrayPos++=x;
	*drawArrayPos++=y;
	*drawArrayPos++=z;
}

void CVertexArray::AddVertexQC(const float3& pos,const unsigned char* color) {
	ASSERT_SIZE(VA_SIZE_C)
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=*((float*)(color));
}

void CVertexArray::AddVertexQT(const float3& pos,const float tx,const float ty) {
	ASSERT_SIZE(VA_SIZE_T)
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
}

void CVertexArray::AddVertexQTN(const float3& pos,const float tx,const float ty,const float3& norm) {
	ASSERT_SIZE(VA_SIZE_TN)
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=norm.x;
	*drawArrayPos++=norm.y;
	*drawArrayPos++=norm.z;
}

void CVertexArray::AddVertexQTC(const float3& pos,const float tx,const float ty,const unsigned char* col) {
	ASSERT_SIZE(VA_SIZE_TC)
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=*((float*)(col));
}

void CVertexArray::AddVertex2dQT(const float x,const float y,const float tx,const float ty) {
	ASSERT_SIZE(VA_SIZE_2DT)
	*drawArrayPos++=x;
	*drawArrayPos++=y;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
}

void CVertexArray::AddVertex0(const float3& pos) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
}

void CVertexArray::AddVertex0(const float x, const float y, const float z) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=x;
	*drawArrayPos++=y;
	*drawArrayPos++=z;
}

void CVertexArray::AddVertexC(const float3& pos,const unsigned char* color) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=*((float*)(color));
}

void CVertexArray::AddVertexT(const float3& pos,const float tx,const float ty) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
}

void CVertexArray::AddVertexT2(const float3& pos,const float tx,const float ty,const float t2x,const float t2y) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=t2x;
	*drawArrayPos++=t2y;
}

void CVertexArray::AddVertexTN(const float3& pos,const float tx,const float ty,const float3& norm) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=norm.x;
	*drawArrayPos++=norm.y;
	*drawArrayPos++=norm.z;
}

void CVertexArray::AddVertexTC(const float3& pos,const float tx,const float ty,const unsigned char* col) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=*((float*)(col));
}

void CVertexArray::AddVertex2dT(const float x, const float y,const float tx,const float ty) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=x;
	*drawArrayPos++=y;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
}


void CVertexArray::CheckEndStrip() {
	if(stripArrayPos==stripArray || ((ptrdiff_t)*(stripArrayPos-1))!=((char *)drawArrayPos-(char *)drawArray))
		EndStrip();
}

unsigned int CVertexArray::drawIndex() const {
	return drawArrayPos-drawArray;
}

void CVertexArray::EndStripQ() {
	assert(stripArraySize>=stripArrayPos+1);
	*stripArrayPos++=((char *)drawArrayPos-(char *)drawArray);
}
