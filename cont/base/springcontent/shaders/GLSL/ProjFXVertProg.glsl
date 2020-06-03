#version 130

out vec4 vertColor;
out vec4 vsPos;

void main() {
	gl_TexCoord[0] = gl_MultiTexCoord0;
	vertColor = gl_Color;
	vsPos = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = gl_ProjectionMatrix * vsPos;
}