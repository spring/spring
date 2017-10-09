#version 130

in vec3 position;
uniform mat4 viewProjMatrix;

void main()
{
	gl_Position = viewProjMatrix * vec4(position, 1.0);
	gl_TexCoord[0] = gl_MultiTexCoord0;
}
