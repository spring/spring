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

#endif