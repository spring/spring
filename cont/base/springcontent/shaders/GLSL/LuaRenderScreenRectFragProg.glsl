// SPDX-License-Identifier: GPL-2.0-or-later

//FRAGMENT SHADER
#version 400

uniform float textureFlag;
uniform sampler2D texSampler;

uniform vec2 viewport;
uniform vec4 rect;
uniform float bevel;
uniform float radius;
uniform float border;

in vec4 gl_FragCoord;

in VertexData {
    vec4 color;
    vec2 texCoord;
    vec4 borderColor;
} inData;

layout(location = 0) out vec4 outColor;

float rectangle(vec2 size, vec2 center) {
    float intensity = length(max(abs(gl_FragCoord.xy - center), size) - size);
    return ceil(clamp(intensity, 0.0, 1.0));
}

float rectangleBevel(vec4 rect, vec2 size, vec2 center, float bevel) {
    vec4 bevels = vec4(bevel + 0.1);
    if (rect.x <= 0) {
        bevels[3] = 0;
        bevels[0] = 0;
    }
    if (rect.y <= 0) {
        bevels[0] = 0;
        bevels[1] = 0;
    }
    if (rect.z >= viewport.x) {
        bevels[1] = 0;
        bevels[2] = 0;
    }
    if (rect.w >= viewport.y) {
        bevels[2] = 0;
        bevels[3] = 0;
    }

    vec2 v = gl_FragCoord.xy - center;
    vec4 b = vec4(-v.x-v.y, v.x-v.y, v.x+v.y, -v.x+v.y) - size.x - size.y + bevels;
    float intensity = max(max(max(b[0], b[1]), b[2]), b[3]);
    return ceil(clamp(intensity, 0.0, 1.0));
}

void main() {
    vec2 size = vec2((rect[2] - rect[0]) / 2.0, (rect[3] - rect[1]) / 2.0);
    vec2 center = vec2((rect[0] + rect[2]) / 2.0, (rect[1] + rect[3]) / 2.0);

    outColor = mix(inData.color, inData.color * texture(texSampler, inData.texCoord), textureFlag);

    if (bevel != 0) {
        float b = floor(border);

        float outerRect = rectangle(size, center);
        float outerBevel = rectangleBevel(rect, size, center, bevel);

        float outer = max(outerRect, outerBevel);

        float innerRect = rectangle(size - b, center);
        float innerBevel = rectangleBevel(rect, size, center, bevel + b * 1.4142);
        float inner = max(innerRect, innerBevel);

        vec4 borderColor = vec4(1);
        borderColor = vec4(inData.borderColor.rgb, 1.0 - outer);

        outColor = mix(outColor, borderColor, inner);

    } else if (radius != 0) {
        size -= radius + border;

        float d = length(max(abs(gl_FragCoord.xy - center), size) - size) - radius;
        float intensity = 1.0 - smoothstep(0.0, 1.0, d);
        outColor = mix(vec4(0), outColor, intensity);

        float borderIntensity = min(step(0.0, d), step(d, border));
        outColor = mix(outColor, inData.borderColor, borderIntensity);

    } else {
        vec4 borderColor = vec4(inData.borderColor.rgb, 1 - rectangle(size, center));
        outColor = mix(outColor, borderColor, rectangle(size - border, center));
    }
}
