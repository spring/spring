#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 T;
layout (location = 3) in vec3 B;
layout (location = 4) in vec4 uv;
layout (location = 5) in uint pieceIndex;

layout (location = 6) in uvec4 instData;

layout(std140, binding = 0) uniform UniformMatrixBuffer {
	mat4 screenView;
	mat4 screenProj;
	mat4 screenViewProj;

	mat4 cameraView;
	mat4 cameraProj;
	mat4 cameraViewProj;
	mat4 cameraBillboardView;

	mat4 cameraViewInv;
	mat4 cameraProjInv;
	mat4 cameraViewProjInv;

	mat4 shadowView;
	mat4 shadowProj;
	mat4 shadowViewProj;

	mat4 reflectionView;
	mat4 reflectionProj;
	mat4 reflectionViewProj;

	mat4 orthoProj01;

	// transforms for [0] := Draw, [1] := DrawInMiniMap, [2] := Lua DrawInMiniMap
	mat4 mmDrawView; //world to MM
	mat4 mmDrawProj; //world to MM
	mat4 mmDrawViewProj; //world to MM

	mat4 mmDrawIMMView; //heightmap to MM
	mat4 mmDrawIMMProj; //heightmap to MM
	mat4 mmDrawIMMViewProj; //heightmap to MM

	mat4 mmDrawDimView; //mm dims
	mat4 mmDrawDimProj; //mm dims
	mat4 mmDrawDimViewProj; //mm dims
};

layout(std140, binding = 1) uniform UniformParamsBuffer {
	vec3 rndVec3; //new every draw frame.
	uint renderCaps; //various render booleans

	vec4 timeInfo;     //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	vec4 viewGeometry; //vsx, vsy, vpx, vpy
	vec4 mapSize;      //xz, xzPO2
	vec4 mapHeight;    //height minCur, maxCur, minInit, maxInit

	vec4 fogColor;  //fog color
	vec4 fogParams; //fog {start, end, 0.0, scale}

	vec4 sunDir;

	vec4 sunAmbientModel;
	vec4 sunAmbientMap;
	vec4 sunDiffuseModel;
	vec4 sunDiffuseMap;
	vec4 sunSpecularModel;
	vec4 sunSpecularMap;

	vec4 shadowDensity; // {ground, units, 0.0, 0.0}

	vec4 windInfo; // windx, windy, windz, windStrength
	vec2 mouseScreenPos; //x, y. Screen space.
	uint mouseStatus; // bits 0th to 32th: LMB, MMB, RMB, offscreen, mmbScroll, locked
	uint mouseUnused;
	vec4 mouseWorldPos; //x,y,z; w=0 -- offmap. Ignores water, doesn't ignore units/features under the mouse cursor

	vec4 teamColor[255]; //all team colors
};

layout(std140, binding=0) readonly buffer MatrixBuffer {
	mat4 mat[];
};

uniform int drawMode = 0;
uniform mat4 staticModelMatrix = mat4(1.0);
uniform vec4 waterClipPlane = vec4(0.0, 0.0, 0.0, 1.0);
uniform float teamColorAlpha = 1.0;

out Data {
	vec4 uvCoord;
	vec4 teamCol;

	vec4 worldPos;
	vec3 worldNormal;

	// main light vector(s)
	vec3 worldCameraDir;
	// shadowPosition
	vec4 shadowVertexPos;
	// Auxilary
	float fogFactor;
};

mat4 mat4mix(mat4 a, mat4 b, float alpha) {
	return (a * (1.0 - alpha) + b * alpha);
}

void TransformPlayerCam(vec4 worldPos) {
	gl_Position = cameraViewProj * worldPos;
}

void TransformPlayerReflCam(vec4 worldPos) {
	gl_Position = reflectionViewProj * worldPos;
}

void TransformPlayerCamStaticMat(vec4 worldPos) {
	gl_Position = cameraViewProj * worldPos;
}


//unit, feature, projectile
mat4 GetWorldMatrixLocalModel() {
	uint baseIndex = instData.x; //ssbo offset
	mat4 modelMatrix = mat[baseIndex];

	mat4 pieceMatrix = mat4mix(mat4(1.0), mat[baseIndex + pieceIndex + 1u], modelMatrix[3][3]); //TODO: figure out why mat4mix() is needed
	return modelMatrix * pieceMatrix;
}

//static model
mat4 GetWorldMatrixModel() {
	uint baseIndex = instData.x; //ssbo offset

	mat4 pieceMatrix = mat[baseIndex + pieceIndex];
	return staticModelMatrix * pieceMatrix;
}

#line 1086

void main(void)
{
	mat4 worldMatrix = (drawMode >= 0) ? GetWorldMatrixLocalModel() : GetWorldMatrixModel();

	#if 0
		mat3 normalMatrix = mat3(transpose(inverse(worldMatrix)));
	#else
		mat3 normalMatrix = mat3(worldMatrix);
	#endif


	worldPos = worldMatrix * vec4(pos, 1.0);
	worldNormal = normalMatrix * normal;
	
	gl_ClipDistance[2] = dot(worldPos, waterClipPlane);

	teamCol = teamColor[instData.y]; // team index
	teamCol.a = teamColorAlpha;

	uvCoord = uv;

	shadowVertexPos = shadowView * worldPos;
	shadowVertexPos.xy += vec2(0.5);  //no need for shadowParams anymore

	vec4 cameraPos = cameraViewInv * vec4(0, 0, 0, 1);
	worldCameraDir = cameraPos.xyz - worldPos.xyz; //from fragment to camera, world space, not normalized(!)

	#if (DEFERRED_MODE == 0)
		float fogDist = length(worldCameraDir);
		fogFactor = (fogParams.y - fogDist) * fogParams.w;
		fogFactor = clamp(fogFactor, 0.0, 1.0);
	#endif


	switch(drawMode) {
		case  1: // water reflection
			TransformPlayerReflCam(worldPos);
			break;
		case  2: // water refraction
			TransformPlayerCam(worldPos);
			break;
		default: // player, (-1) static model, (0) normal rendering
			TransformPlayerCam(worldPos);			
			break;
	};
}