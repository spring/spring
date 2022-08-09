#version 130

in vec3 uvw;

uniform samplerCube skybox;

void main()
{    
    gl_FragColor = texture(skybox, uvw);
}