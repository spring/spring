// SPDX-License-Identifier: GPL-2.0-or-later

//VERTEX SHADER
#version 400

uniform mat4 viewMatrix;
uniform mat4 projMatrix;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

out VertexData {
    vec4 color;
} outData;

void main(void) {
    gl_Position = projMatrix * viewMatrix * vec4(inPosition, 1.0);
    outData.color = inColor;
}
