uniform float mapSizeX;     // pwr2mapx * SQUARE_SIZE (programmatically #define this)
uniform float mapSizeZ;     // pwr2mapy * SQUARE_SIZE (programmatically #define this)
uniform int texSquareX;
uniform int texSquareZ;

uniform mat4 cameraMatInv;
uniform mat4 shadowMat;
uniform vec4 shadowParams;

uniform vec3 cameraPos;
uniform vec3 lightDir;      // mapInfo->light.sunDir
varying vec3 viewDir;
varying vec3 halfDir;

#define SMF_TEXSQR_SIZE_INT 1024
#define SMF_TEXSQR_SIZE_FLT 1024.0
#define SMF_DETAILTEX_RES 0.02

void main() {
	viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));
	viewDir = normalize(viewDir - gl_Vertex.xyz);
	halfDir = normalize(lightDir + viewDir);

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// diffuse-tex coors
	gl_TexCoord[0].s = float(int(gl_Vertex.x) - (texSquareX * SMF_TEXSQR_SIZE_INT)) / SMF_TEXSQR_SIZE_FLT;
	gl_TexCoord[0].t = float(int(gl_Vertex.z) - (texSquareZ * SMF_TEXSQR_SIZE_INT)) / SMF_TEXSQR_SIZE_FLT;
	gl_TexCoord[0].q = gl_Vertex.y;

	// normal- and specular-tex coors; this is equal to
	// (D / mapD) * (mapD / pwr2mapD) for D in {x, z}
	gl_TexCoord[1].s = gl_Vertex.x / mapSizeX;
	gl_TexCoord[1].t = gl_Vertex.z / mapSizeZ;

	// shadow-tex coors; shadowParams stores
	// {x=xmid, y=ymid, z=sizeFactorA, w=sizeFactorB}
	// note: map vertices are already in world-space
	gl_TexCoord[2] = shadowMat * gl_Vertex;
	gl_TexCoord[2].st *= (inversesqrt(abs(gl_TexCoord[2].st) + shadowParams.z) + shadowParams.w);
	gl_TexCoord[2].st += shadowParams.xy;

	// detail-tex coors
	gl_TexCoord[3].st = gl_Vertex.xz * vec2(SMF_DETAILTEX_RES, SMF_DETAILTEX_RES);
	gl_TexCoord[3].s += -floor(cameraPos.x * SMF_DETAILTEX_RES);
	gl_TexCoord[3].t += -floor(cameraPos.z * SMF_DETAILTEX_RES);
}
