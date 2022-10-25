// SPDX-License-Identifier: MIT

/*
 * MIT License
 *
 * Copyright (c) 2016 Victoria Rudakova
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 * GLSL geometry shader that draws thick and smooth lines in 3D.
 * Based on [shader-3dcurve](https://github.com/vicrucann/shader-3dcurve).
 */

//GEOMETRY SHADER
#version 400

uniform float miterLimit;
uniform float thickness;
uniform vec2 viewport;

uniform vec4 textureCoords;
uniform float texturePhase;
uniform float textureSpeed;

layout(lines_adjacency) in;

in VertexData {
    vec4 color;
} inData[4];

layout(triangle_strip, max_vertices = 7) out;

out VertexData {
    vec4 color;
    vec2 texCoord;
} outData;

vec2 toScreenSpace(vec4 vertex) {
    return vec2(vertex.xy / vertex.w) * viewport;
}

float toZValue(vec4 vertex) {
    return (vertex.z/vertex.w);
}

void drawSegment(vec2 points[4], vec4 colors[4], float zValues[4]) {
    vec2 p0 = points[0];
    vec2 p1 = points[1];
    vec2 p2 = points[2];
    vec2 p3 = points[3];

#if 0
    // Perform naive culling.
    vec2 area = viewport * 4;
    if(p1.x < -area.x || p1.x > area.x) return;
    if(p1.y < -area.y || p1.y > area.y) return;
    if(p2.x < -area.x || p2.x > area.x) return;
    if(p2.y < -area.y || p2.y > area.y) return;
#endif

    // Determine the direction of each of the 3 segments (previous, current, next).
    vec2 v0 = normalize(p1 - p0);
    vec2 v1 = normalize(p2 - p1);
    vec2 v2 = normalize(p3 - p2);

    // Determine the normal of each of the 3 segments (previous, current, next).
    vec2 n0 = vec2(-v0.y, v0.x);
    vec2 n1 = vec2(-v1.y, v1.x);
    vec2 n2 = vec2(-v2.y, v2.x);

    // Determine miter lines by averaging the normals of the 2 segments.
    vec2 miter_a = normalize(n0 + n1); // Miter at start of current segment.
    vec2 miter_b = normalize(n1 + n2); // Miter at end of current segment.

    // Determine the length of the miter by projecting it onto normal and then inverse it.
    float an1 = dot(miter_a, n1);
    if (an1==0) an1 = 1;
    float length_a = thickness / an1;

    // Start of line.
    if (all(equal(p0, p1))) {
        miter_a = n1;
        length_a = thickness;
    }

    float bn1 = dot(miter_b, n2);
    if (bn1==0) bn1 = 1;
    float length_b = thickness / bn1;

    // End of line.
    if (all(equal(p2, p3))) {
        miter_b = n1;
        length_b = thickness;
    }

    // Prevent excessively long miters at sharp corners.
    if(dot(v0, v1) < -miterLimit) {
        miter_a = n1;
        length_a = thickness;

        // Close the gap.
        // FIXME: Fix texture coordinates of gap filler.
        if(dot(v0, n1) > 0) {
            outData.texCoord = textureCoords.st;
            outData.color = colors[1];
            gl_Position = vec4((p1 + thickness * n0) / viewport, zValues[1], 1.0);
            EmitVertex();

            outData.texCoord = textureCoords.st;
            outData.color = colors[1];
            gl_Position = vec4((p1 + thickness * n1) / viewport, zValues[1], 1.0);
            EmitVertex();

            outData.texCoord = vec2(textureCoords.s, (textureCoords.t + textureCoords.q) / 2.0);
            outData.color = colors[1];
            gl_Position = vec4(p1 / viewport, zValues[1], 1.0);
            EmitVertex();

            EndPrimitive();
        } else {
            outData.texCoord = textureCoords.sq;
            outData.color = colors[1];
            gl_Position = vec4((p1 - thickness * n1) / viewport, zValues[1], 1.0);
            EmitVertex();

            outData.texCoord = textureCoords.sq;
            outData.color = colors[1];
            gl_Position = vec4((p1 - thickness * n0) / viewport, zValues[1], 1.0);
            EmitVertex();

            outData.texCoord = vec2(textureCoords.s, (textureCoords.t + textureCoords.q) / 2.0);
            outData.color = colors[1];
            gl_Position = vec4(p1 / viewport, zValues[1], 1.0);
            EmitVertex();

            EndPrimitive();
        }
    }

    if(dot(v1, v2) < -miterLimit) {
        miter_b = n1;
        length_b = thickness;
    }

    float s1 = textureCoords.s - (texturePhase * textureSpeed);
    float s2 = s1 + length(p2 - p1) / (thickness*2) * textureCoords.p;

    // Generate the triangle strip.
    outData.texCoord = vec2(s1, textureCoords.t);
    outData.color = colors[1];
    gl_Position = vec4((p1 + length_a * miter_a) / viewport, zValues[1], 1.0);
    EmitVertex();

    outData.texCoord = vec2(s1, textureCoords.q);
    outData.color = colors[1];
    gl_Position = vec4((p1 - length_a * miter_a) / viewport, zValues[1], 1.0);
    EmitVertex();

    outData.texCoord = vec2(s2, textureCoords.t);
    outData.color = colors[2];
    gl_Position = vec4((p2 + length_b * miter_b) / viewport, zValues[2], 1.0);
    EmitVertex();

    outData.texCoord = vec2(s2, textureCoords.q);
    outData.color = colors[2];
    gl_Position = vec4((p2 - length_b * miter_b) / viewport, zValues[2], 1.0);
    EmitVertex();

    EndPrimitive();
}

void main(void) {
    // 4 points.
    vec4 Points[4];
    Points[0] = gl_in[0].gl_Position;
    Points[1] = gl_in[1].gl_Position;
    Points[2] = gl_in[2].gl_Position;
    Points[3] = gl_in[3].gl_Position;

    // 4 attached colors.
    vec4 colors[4];
    colors[0] = inData[0].color;
    colors[1] = inData[1].color;
    colors[2] = inData[2].color;
    colors[3] = inData[3].color;

    // Screen coords.
    vec2 points[4];
    points[0] = toScreenSpace(Points[0]);
    points[1] = toScreenSpace(Points[1]);
    points[2] = toScreenSpace(Points[2]);
    points[3] = toScreenSpace(Points[3]);

    // Deepness values.
    float zValues[4];
    zValues[0] = toZValue(Points[0]);
    zValues[1] = toZValue(Points[1]);
    zValues[2] = toZValue(Points[2]);
    zValues[3] = toZValue(Points[3]);

    drawSegment(points, colors, zValues);
}
