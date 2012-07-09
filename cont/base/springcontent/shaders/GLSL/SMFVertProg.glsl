#define SMF_TEXSQR_SIZE 1024.0
#define SMF_DETAILTEX_RES 0.02

// uniform vec2 mapSizePO2;   // pwr2map{x,z} * SQUARE_SIZE (TODO programmatically #define this)
// uniform vec2 mapSize;      //     map{x,z} * SQUARE_SIZE (TODO programmatically #define this)
// uniform vec2 mapHeights;   // readmap->curr{Min, Max}Height

uniform vec2 normalTexGen;   // either 1.0/mapSize (when NPOT are supported) or 1.0/mapSizePO2
uniform vec2 specularTexGen; // 1.0/mapSize
uniform vec2 infoTexGen;     // 1.0/(pwr2map{x,z} * SQUARE_SIZE)
uniform ivec2 texSquare;

uniform vec4 splatTexScales; // defaults to SMF_DETAILTEX_RES per channel
uniform vec3 cameraPos;
uniform vec4 lightDir;       // mapInfo->light.sunDir


varying vec3 halfDir;
varying float fogFactor;
varying vec4 vertexWorldPos;
varying vec2 diffuseTexCoords;
varying vec2 specularTexCoords;
varying vec2 normalTexCoords;
varying vec2 infoTexCoords;



void main() {
	// calc some lighting variables
	vec3 viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));
	viewDir = normalize(viewDir - gl_Vertex.xyz);
	halfDir = normalize(lightDir.xyz + viewDir);
	vertexWorldPos = gl_Vertex;

	// calc texcoords
	diffuseTexCoords = (floor(gl_Vertex.xz) / SMF_TEXSQR_SIZE) - vec2(texSquare);
	specularTexCoords = gl_Vertex.xz * specularTexGen;
	normalTexCoords   = gl_Vertex.xz * normalTexGen;
	infoTexCoords     = gl_Vertex.xz * infoTexGen;

	// detail-tex coords
	#if (SMF_DETAIL_TEXTURE_SPLATTING == 0)
		gl_TexCoord[0].st  = gl_Vertex.xz * vec2(SMF_DETAILTEX_RES);
	#else
		gl_TexCoord[0].st  = gl_Vertex.xz * vec2(splatTexScales.r);
		gl_TexCoord[0].pq  = gl_Vertex.xz * vec2(splatTexScales.g);
		gl_TexCoord[1].st  = gl_Vertex.xz * vec2(splatTexScales.b);
		gl_TexCoord[1].pq  = gl_Vertex.xz * vec2(splatTexScales.a);
	#endif

	// transform vertex pos
	gl_Position = gl_ModelViewMatrix * gl_Vertex;
	gl_ClipVertex = gl_Position;
	float fogCoord = length(gl_Position.xyz);
	gl_Position = gl_ProjectionMatrix * gl_Position;

	// emulate linear fog
	fogFactor = (gl_Fog.end - fogCoord) * gl_Fog.scale; // gl_Fog.scale == 1.0 / (gl_Fog.end - gl_Fog.start)
	fogFactor = clamp(fogFactor, 0.0, 1.0);
}

