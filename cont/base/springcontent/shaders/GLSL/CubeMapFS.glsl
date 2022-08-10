#version 130

uniform samplerCube skybox;

in vec3 uvw;
out vec4 fragColor;

void main()
{    
    fragColor = texture(skybox, uvw);
}