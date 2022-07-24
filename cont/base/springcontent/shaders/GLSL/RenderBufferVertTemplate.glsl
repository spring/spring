// Version and extensions
%s

// VS input attributes
%s

//uniform mat4 transformMatrix = mat4(1.0);

// VS output attributes
%s

void main() {
%s
	gl_Position = gl_ModelViewProjectionMatrix * %s;
}
