uniform vec3 cameraPos;     // world-space
varying vec3 cameraDir;     // world-space
varying vec3 vertexNormal;  // model-space

uniform mat4 cameraMatInv;  // unused

void main() {
	// note: gl_ModelViewMatrix actually only contains the
	// model matrix, view matrix is on the projection stack
	//
	// todo: clip gl_Position against gl_ClipPlane[3] if advFade
	vec4 vertexPos = gl_ModelViewMatrix * gl_Vertex;

	cameraDir = vertexPos.xyz - cameraPos;
	vertexNormal = gl_Normal;

	// diffuse- and shading-tex coors, shadow-tex coors
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;
	gl_TexCoord[1] = vertexPos;

	gl_FogFragCoord = length(vertexPos);
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
