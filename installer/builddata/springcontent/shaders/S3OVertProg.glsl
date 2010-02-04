uniform vec3 lightDir;
uniform vec3 cameraPos;
// varying vec3 cameraDir;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
