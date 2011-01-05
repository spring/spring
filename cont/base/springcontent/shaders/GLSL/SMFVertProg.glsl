#define SMF_TEXSQR_SIZE 1024.0
#define SMF_DETAILTEX_RES 0.02

uniform vec2 mapSizePO2;    // pwr2map{x,z} * SQUARE_SIZE (programmatically #define this)
uniform vec2 mapSize;       //     map{x,z} * SQUARE_SIZE (programmatically #define this)
uniform int texSquareX;
uniform int texSquareZ;

uniform vec3 cameraPos;
uniform vec4 lightDir;      // mapInfo->light.sunDir
varying vec3 halfDir;

varying float fogFactor;

varying vec4 vertexPos;
varying vec2 diffuseTexCoords;
varying vec2 specularTexCoords;
varying vec2 normalTexCoords;

// defaults to SMF_DETAILTEX_RES per channel
uniform vec4 splatTexScales;

void main() {
	vec3 viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));
	viewDir = normalize(viewDir - gl_Vertex.xyz);
	halfDir = normalize(lightDir.xyz + viewDir);
	vertexPos = gl_Vertex;

	gl_Position = gl_ModelViewMatrix * gl_Vertex;
	float fogCoord = length(gl_Position.xyz);
	gl_Position = gl_ProjectionMatrix * gl_Position;

	diffuseTexCoords  = floor(gl_Vertex.xz) / SMF_TEXSQR_SIZE - vec2(ivec2(texSquareX, texSquareZ));
	specularTexCoords = gl_Vertex.xz / mapSize.xy;
	normalTexCoords   = gl_Vertex.xz / mapSizePO2.xy;

	// detail-tex coors
	#if (SMF_DETAIL_TEXTURE_SPLATTING == 0)
		gl_TexCoord[0].st  = gl_Vertex.xz * vec2(SMF_DETAILTEX_RES);
	#else
		gl_TexCoord[0].st  = gl_Vertex.xz * vec2(splatTexScales.r);
		gl_TexCoord[0].pq  = gl_Vertex.xz * vec2(splatTexScales.g);
		gl_TexCoord[1].st  = gl_Vertex.xz * vec2(splatTexScales.b);
		gl_TexCoord[1].pq  = gl_Vertex.xz * vec2(splatTexScales.a);
	#endif

	// emulate linear fog
	fogFactor = (gl_Fog.end - fogCoord) / (gl_Fog.end - gl_Fog.start);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
}
