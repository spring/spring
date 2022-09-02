#version 130
//#extension GL_ARB_explicit_attrib_location : require

in vec3 pos;
in vec3 layer;
in vec4 uvmm;
in vec3 aparams;
in vec4 color;

out vec4 vCol;
out vec3 vUVW;
out vec4 vUVb;
out float vBF;

// gl_VertexID with glDrawElements represents vertex index not indices, so just specify the full quad
const vec2 myUVs[4] = vec2[4](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(1.0, 1.0),
	vec2(0.0, 1.0)
);

void main() {
	vec2 uvDiff = uvmm.zw - uvmm.xy;

	float ap = fract(aparams.z);

	float maxImgIdx = aparams.x * aparams.y - 1.0;
	ap *= maxImgIdx;

	float i0 = floor(ap);
	float i1 = i0 + 1.0;

	vBF = fract(ap); //blending factor

	// unscaled "integer" quadrants
	vUVb = vec4(
		floor(mod(i0 , aparams.x)),
		floor(   (i0 / aparams.x)),
		floor(mod(i1 , aparams.x)),
		floor(   (i1 / aparams.x))
	);

	// scale back to 0-1
	vUVb /= aparams.xyxy;

	// scale to uvDiff
	vUVb *= uvDiff.xyxy;

	vec2 myUV = myUVs[gl_VertexID % 4];

	vCol = color;
	vUVW = vec3(uvmm.xy + myUV * uvDiff / aparams.xy, layer);
	vec4 lightVertexPos = gl_ModelViewMatrix * vec4(pos, 1.0);
	lightVertexPos.xy += vec2(0.5);
	gl_Position = gl_ProjectionMatrix * lightVertexPos;
}