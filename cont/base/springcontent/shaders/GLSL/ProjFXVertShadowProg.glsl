#version 130
//#extension GL_ARB_explicit_attrib_location : require

in vec3 pos;
in vec3 uvw;
in vec2 uvDiff;
in vec3 aparams;
in vec4 color;

out vec4 vCol;
out vec4 vUV;
out float vLayer;
out float vBF;

void main() {
	float ap = fract(aparams.z);

	float maxImgIdx = aparams.x * aparams.y - 1.0;
	ap *= maxImgIdx;

	float i0 = floor(ap);
	float i1 = i0 + 1.0;

	vBF = fract(ap); //blending factor

	vUV = uvw.xyxy + vec4(
		floor(mod(i0 , aparams.x)),
		floor(   (i0 / aparams.x)),
		floor(mod(i1 , aparams.x)),
		floor(   (i1 / aparams.x))
	) * uvDiff.xyxy;

	vUV /= aparams.xyxy; //scale

	vLayer = uvw.z;
	vCol = color;

	vec4 lightVertexPos = gl_ModelViewMatrix * vec4(pos, 1.0);
	lightVertexPos.xy += vec2(0.5);
	gl_Position = gl_ProjectionMatrix * lightVertexPos;
}