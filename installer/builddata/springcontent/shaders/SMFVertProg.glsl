uniform float mapSizeX;     // programmatically #define this
uniform float mapSizeZ;     // programmatically #define this
uniform int texSquareX;
uniform int texSquareZ;

uniform mat4 shadowMat;
uniform vec4 shadowParams;

uniform vec3 lightDir;      // mapInfo->light.sunDir
varying vec3 viewDir;
varying vec3 halfDir;

#define SMF_TEXSQR_SIZE_INT 1024
#define SMF_TEXSQR_SIZE_FLT 1024.0

void main() {
	viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));
	viewDir = normalize(viewDir - gl_Vertex.xyz);
	halfDir = normalize(lightDir + viewDir);

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// diffuse-map texture coors
	gl_TexCoord[0].s = float(int(gl_Vertex.x) - (texSquareX * SMF_TEXSQR_SIZE_INT)) / SMF_TEXSQR_SIZE_FLT;
	gl_TexCoord[0].t = float(int(gl_Vertex.z) - (texSquareZ * SMF_TEXSQR_SIZE_INT)) / SMF_TEXSQR_SIZE_FLT;

	// normal-and specular-map texture coors
	gl_TexCoord[1].s = gl_Vertex.x / mapSizeX;
	gl_TexCoord[1].t = gl_Vertex.z / mapSizeZ;

	// shadow-map texture coors
	gl_TexCoord[2] = shadowMat * gl_Vertex;
	gl_TexCoord[2].st *= (inversesqrt(abs(gl_TexCoord[2].st) + shadowParams.z) + shadowParams.w);
	gl_TexCoord[2].st += shadowParams.xy;
}
