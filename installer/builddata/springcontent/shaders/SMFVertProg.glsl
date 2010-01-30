varying vec3 vertexNormal;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	vertexNormal = gl_NormalMatrix * gl_Normal;
}
