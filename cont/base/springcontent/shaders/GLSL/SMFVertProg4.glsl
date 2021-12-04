#version 410 core

#define SMF_TEXSQR_SIZE 1024.0
#define SMF_DETAILTEX_RES 0.02

uniform ivec4 texSquare;     // x, y, y * X + x (index into diffuseTex), mip
uniform vec3 cameraPos;
uniform vec4 lightDir;       // mapInfo->light.sunDir
uniform vec3 fogParams;      // .x := start, .y := end, .z := viewrange
uniform vec4 clipPlane;      // world-space

uniform mat4 viewMat;
uniform mat4 viewMatInv;
uniform mat4 viewProjMat;


layout(location = 0) in vec3 vertexPosAttr;

out vec3 halfDir;
out vec4 vertexPos;
out vec2 diffuseTexCoords;

out float gl_ClipDistance[SMF_CLIP_PLANE_IDX + 1];
out float fogFactor;


void main() {
	// vec4 viewDir = viewMatInv * vec4(0.0, 0.0, 0.0, 1.0);
	vec4 viewDir = vec4(cameraPos, 1.0);

	viewDir.xyz = normalize(viewDir.xyz - vertexPosAttr);
	halfDir.xyz = normalize(lightDir.xyz + viewDir.xyz);

	vertexPos = vec4(vertexPosAttr, 1.0);
	diffuseTexCoords = (floor(vertexPos.xz) / SMF_TEXSQR_SIZE) - texSquare.xy;

	gl_Position = viewProjMat * vertexPos;
	gl_ClipDistance[SMF_CLIP_PLANE_IDX] = dot(clipPlane, vertexPos);

	{
		float eyeDepth = length((viewMat * vertexPos).xyz);
		float fogRange = (fogParams.y - fogParams.x) * fogParams.z;
		float fogDepth = (eyeDepth - fogParams.x * fogParams.z) / fogRange;
		// float fogDepth = (fogParams.y * fogParams.z - eyeDepth) / fogRange;

		fogFactor = 1.0 - clamp(fogDepth, 0.0, 1.0);
		// fogFactor = clamp(fogDepth, 0.0, 1.0);
	}
}

