#version 410 core

layout(location = 0) out vec4 f_color_rgba;

uniform vec4 u_color_rgba;

void main() {
	f_color_rgba = u_color_rgba;
}

