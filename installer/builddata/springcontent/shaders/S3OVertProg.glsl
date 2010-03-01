uniform vec3 cameraPos;     // world-space
varying vec3 cameraDir;     // world-space
varying vec3 vertexNormal;  // model-space

varying float fogFactor;

uniform mat4 cameraMatInv;  // unused
uniform mat4 shadowMat;
uniform vec4 shadowParams;

void main() {
	// note: gl_ModelViewMatrix actually only contains the
	// model matrix, view matrix is on the projection stack
	//
	// todo: clip gl_Position against gl_ClipPlane[3] if advFade
	//
	// note: shadow-map texture coordinates should be generated
	// per fragment (the non-linear projection used can produce
	// shifting artefacts with large triangles due to the linear
	// interpolation of vertex positions), but this is a source
	// of acne itself
	vec4 vertexPos = gl_ModelViewMatrix * gl_Vertex;
	vec4 vertexShadowPos = shadowMat * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + shadowParams.z) + shadowParams.w);
		vertexShadowPos.st += shadowParams.xy;

	cameraDir = vertexPos.xyz - cameraPos;
	vertexNormal = gl_Normal;

	// diffuse- and shading-tex coors, shadow-tex coors
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;
	gl_TexCoord[1] = vertexShadowPos;

	gl_FogFragCoord = length(((gl_ProjectionMatrix * gl_ModelViewMatrix) * gl_Vertex).xyz);
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	fogFactor = (gl_Fog.end - gl_FogFragCoord) / (gl_Fog.end - gl_Fog.start);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
}
