#ifndef s3oH
#define s3oH



#define S3OSIGNATURE 0x676F

#define OBJECT_TYPE_UNDEFINED 0
#define OBJECT_TYPE_GEOMETRY 1
#define OBJECT_TYPE_POINT 2
#define OBJECT_TYPE_LIGHT 3

#define LIGHT_TYPE_DIFFUSE 1
#define LIGHT_TYPE_SPOT 2

#define PRIMITIVE_TYPE_POLYGON 1

/*struct float3{
	float x;
	float y;
	float z;
};*/

struct Texcoord{
	float x;
	float y;
};

struct S3OPrimitive{
	int numberOfVertexIndexes;
    int offsetToVertexIndexArray;
};

struct VertexIndexStruct{
	int vertexIndex;
};

struct VertexData{
	float3 vertex;
	float3 normal;
	Texcoord texcoord;
};

struct S3OObject{
	struct Object
	{
		int type;
		float3 offsetFromParrent;
		int offsetToSiblingObject;
		int offsetToChildObject;
		int offsetToObjectName;
	}object;

	struct Geometry
	{
		int numVertex;
		int offsetToVertexArray;
		int numPrimitives;
		int offsetToPrimitiveArray;
	}geometry;
};

struct S3OHeader{
	int signature;
	int version;
	int offsetToBaseObject;

};

#define FREAD_S3OHEADER(s3oh,src)				\
do {								\
	unsigned int __tmpdw;					\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3oh).signature = (int)swabdword(__tmpdw);		\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3oh).version = (int)swabdword(__tmpdw);		\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3oh).offsetToBaseObject = (int)swabdword(__tmpdw);	\
} while (0)

#define FREAD_S3OOBJECT_OBJECT(s3oo,src)			\
do {								\
	unsigned int __tmpdw;					\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3oo).type = (int)swabdword(__tmpdw);			\
	fread(&(s3oo).offsetFromParrent,sizeof(float3),1,(src));\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3oo).offsetToSiblingObject = (int)swabdword(__tmpdw);	\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3oo).offsetToChildObject = (int)swabdword(__tmpdw);	\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3oo).offsetToObjectName = (int)swabdword(__tmpdw);	\
} while (0)

#define FREAD_S3OOBJECT_GEOMETRY(s3og,src)			\
do {								\
	unsigned int __tmpdw;					\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3og).numVertex = (int)swabdword(__tmpdw);		\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3og).offsetToVertexArray = (int)swabdword(__tmpdw);	\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3og).numPrimitives = (int)swabdword(__tmpdw);		\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));		\
	(s3og).offsetToPrimitiveArray = (int)swabdword(__tmpdw);\
} while (0)

#define FREAD_S3OPRIMITIVE(s3op,src)					\
do {									\
	unsigned int __tmpdw;						\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));			\
	(s3op).numberOfVertexIndexes = (int)swabdword(__tmpdw);		\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));			\
	(s3op).offsetToVertexIndexArray = (int)swabdword(__tmpdw);	\
} while (0)

#define FREAD_VERTEXINDEXSTRUCT(vis,src)		\
do {							\
	unsigned int __tmpdw;				\
	fread(&__tmpdw,sizeof(unsigned int),1,(src));	\
	(vis).vertexIndex = (int)swabdword(__tmpdw);	\
} while (0)

#define FREAD_VERTEXDATA(vd,src)			\
do {							\
	fread(&(vd).vertex,sizeof(float3),1,(src));	\
	fread(&(vd).normal,sizeof(float3),1,(src));	\
	fread(&(vd).texcoord,sizeof(Texcoord),1,src);	\
} while (0)

#endif
