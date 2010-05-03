#define SMF_TEXSQR_SIZE_INT 1024
#define SMF_TEXSQR_SIZE_FLT 1024.0
#define SMF_DETAILTEX_RES 0.02

uniform vec2 mapSizePO2;    // pwr2map{x,z} * SQUARE_SIZE (programmatically #define this)
uniform vec2 mapSize;       //     map{x,z} * SQUARE_SIZE (programmatically #define this)
uniform int texSquareX;
uniform int texSquareZ;

uniform vec3 cameraPos;
uniform vec4 lightDir;      // mapInfo->light.sunDir
varying vec3 cameraDir;     // WS
varying vec3 viewDir;
varying vec3 halfDir;

varying float fogFactor;
varying float vertexHeight;

// defaults to SMF_DETAILTEX_RES per channel
uniform vec4 splatTexScales;

void main() {
	viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));
	viewDir = normalize(viewDir - gl_Vertex.xyz);
	halfDir = normalize(lightDir.xyz + viewDir);
	cameraDir = gl_Vertex.xyz - cameraPos;

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// diffuse-tex coors
	gl_TexCoord[0].s = float(int(gl_Vertex.x) - (texSquareX * SMF_TEXSQR_SIZE_INT)) / SMF_TEXSQR_SIZE_FLT;
	gl_TexCoord[0].t = float(int(gl_Vertex.z) - (texSquareZ * SMF_TEXSQR_SIZE_INT)) / SMF_TEXSQR_SIZE_FLT;
	gl_TexCoord[0].q = gl_Vertex.y;

	// normal-tex coors
	gl_TexCoord[1].s = gl_Vertex.x / mapSizePO2.x;
	gl_TexCoord[1].t = gl_Vertex.z / mapSizePO2.y;

	// specular-tex coors
	gl_TexCoord[2].s = gl_Vertex.x / mapSize.x;
	gl_TexCoord[2].t = gl_Vertex.z / mapSize.y;

	// shadow-tex coors (map vertices are already in world-space)
	gl_TexCoord[3] = gl_Vertex;

	// detail-tex coors
	#if (SMF_DETAIL_TEXTURE_SPLATTING == 0)
	gl_TexCoord[4].st  = gl_Vertex.xz * vec2(SMF_DETAILTEX_RES, SMF_DETAILTEX_RES);
	gl_TexCoord[4].s  += -floor(cameraPos.x * SMF_DETAILTEX_RES);
	gl_TexCoord[4].t  += -floor(cameraPos.z * SMF_DETAILTEX_RES);
	#else
	gl_TexCoord[4].st  = gl_Vertex.xz * vec2(splatTexScales.r, splatTexScales.r);
	gl_TexCoord[4].s  += -floor(cameraPos.x * splatTexScales.r);
	gl_TexCoord[4].t  += -floor(cameraPos.z * splatTexScales.r);
	gl_TexCoord[5].st  = gl_Vertex.xz * vec2(splatTexScales.g, splatTexScales.g);
	gl_TexCoord[5].s  += -floor(cameraPos.x * splatTexScales.g);
	gl_TexCoord[5].t  += -floor(cameraPos.z * splatTexScales.g);
	gl_TexCoord[6].st  = gl_Vertex.xz * vec2(splatTexScales.b, splatTexScales.b);
	gl_TexCoord[6].s  += -floor(cameraPos.x * splatTexScales.b);
	gl_TexCoord[6].t  += -floor(cameraPos.z * splatTexScales.b);
	gl_TexCoord[7].st  = gl_Vertex.xz * vec2(splatTexScales.a, splatTexScales.a);
	gl_TexCoord[7].s  += -floor(cameraPos.x * splatTexScales.a);
	gl_TexCoord[7].t  += -floor(cameraPos.z * splatTexScales.a);
	#endif

	// gl_FogCoord is not initialized
	gl_FogFragCoord = length((gl_ModelViewMatrix * gl_Vertex).xyz);

	// emulate linear fog
	fogFactor = (gl_Fog.end - gl_FogFragCoord) / (gl_Fog.end - gl_Fog.start);
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	vertexHeight = gl_Vertex.y;
}
