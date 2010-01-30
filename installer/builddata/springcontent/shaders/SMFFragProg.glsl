varying vec3 vertexNormal;

void main() {
	gl_FragColor.rgb = normalize(vertexNormal);
	gl_FragColor.a = 1.0;
}
