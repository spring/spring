uniform vec2 mapSizePO2; // 1.0f / (pwr2map{x,z} * SQUARE_SIZE)
varying vec4 vertexPos;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0].st = gl_MultiTexCoord0.st;
	gl_TexCoord[1].st = gl_Vertex.xz * mapSizePO2;
	gl_FrontColor = gl_Color;
	gl_FogFragCoord = dot(gl_ModelViewProjectionMatrix[2].xyz, gl_Vertex.xyz);

	vertexPos = gl_Vertex;
}
