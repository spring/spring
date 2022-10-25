// SPDX-License-Identifier: GPL-2.0-or-later

//FRAGMENT SHADER
#version 400

uniform float textureFlag;
uniform sampler2D texSampler;

in VertexData {
    vec4 color;
    vec2 texCoord;
    vec4 borderColor;
} inData;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = mix(inData.color, inData.color * texture(texSampler, inData.texCoord), textureFlag);
}
