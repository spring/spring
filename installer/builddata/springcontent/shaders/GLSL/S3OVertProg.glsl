uniform vec3 cameraPos;      // WS
varying vec3 cameraDir;      // WS
varying vec3 vertexNormal;   // OS

varying float fogFactor;

varying mat3 normalMatrix;
uniform mat4 cameraMat;
uniform mat4 cameraMatInv;
uniform mat4 shadowMat;
uniform vec4 shadowParams;

void main() {
	// note: gl_ModelViewMatrix actually only contains the
	// model matrix, view matrix is on the projection stack
	// because of this, gl_NormalMatrix is not what we want
	//
	// todo: clip gl_Position against gl_ClipPlane[3] if advFade
	//
	// note: shadow-map texture coordinates should be generated
	// per fragment (the non-linear projection used can produce
	// shifting artefacts with large triangles due to the linear
	// interpolation of vertex positions), but this is a source
	// of acne itself
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexPos = gl_ModelViewMatrix * gl_Vertex;
	vec4 vertexShadowPos = shadowMat * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	cameraDir = vertexPos.xyz - cameraPos;
	vertexNormal = gl_Normal;

	// M and V are both (always) orthonormal
	normalMatrix = mat3(cameraMat * gl_ModelViewMatrix);

	// diffuse- and shading-tex coors, shadow-tex coors
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;
	gl_TexCoord[1] = vertexShadowPos;

	gl_FogFragCoord = length(((gl_ProjectionMatrix * gl_ModelViewMatrix) * gl_Vertex).xyz);
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;

	fogFactor = (gl_Fog.end - gl_FogFragCoord) / (gl_Fog.end - gl_Fog.start);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
}
