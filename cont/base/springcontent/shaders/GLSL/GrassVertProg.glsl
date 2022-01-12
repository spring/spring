#version 410 core

uniform mat4 turfMatrices[128];
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

uniform vec2 mapSizePO2;     // (1.0 / pwr2map{x,z} * SQUARE_SIZE)
uniform vec2 mapSize;        // (1.0 /     map{x,z} * SQUARE_SIZE)

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;

uniform vec3 camPos;
uniform vec3 camUp;
uniform vec3 camRight;

uniform float frame;
uniform vec3 windSpeed;

uniform vec3 fogParams;

uniform vec3 sunDir;
uniform vec3 ambientLightColor;
uniform vec3 diffuseLightColor;


layout(location = 0) in vec3 positionAttr;
layout(location = 1) in vec2 texCoordAttr;
layout(location = 2) in vec3   normalAttr;

out vec3 wsNormal;
out vec4 shadingTexCoords;
out vec2 bladeTexCoords;
out vec3 ambientDiffuseLightTerm;

#if (defined(HAVE_SHADOWS) || defined(SHADOW_GEN))
out vec4 shadowTexCoords;
#endif

out float fogFactor;


const float PI = 3.14159265358979323846264;



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Crytek - foliage animation
// src: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch16.html
//

const vec2 V_FREQ = vec2(1.975, 0.793);

vec2 SmoothCurve(vec2 x) { return (x * x * (3.0 - 2.0 * x)); }
vec2 TriangleWave(vec2 x) { return abs(fract(x + 0.5) * 1.99 - 1.0); }
vec2 SmoothTriangleWave(vec2 x) { return SmoothCurve(TriangleWave(x)); } // similar to sine wave, but faster

// This bends the entire plant in the direction of the wind.
// vPos is the world position of the plant *relative* to its base.
vec3 ApplyMainBending(in vec3 vPos, in vec2 vWind, in float fBendScale)
{
	float fLength = length(vPos);
	float fBendFac = vPos.y * fBendScale + 1.0;

	fBendFac *= fBendFac;
	fBendFac  = fBendFac * fBendFac - fBendFac;

	vPos.xz += (vWind.xy * fBendFac);
	return (normalize(vPos) * fLength);
}

// This provides "chaotic" motion for leaves and branches (the entire plant, really)
void ApplyDetailBending(inout vec3 vPos, vec3 vNormal, float fDetailPhase, float fTime, float fSpeed, float fDetailAmp)
{
	vec2 vPhase = vec2(fTime + fDetailPhase);
	vec2 vWaves = (fract(vPhase * V_FREQ) * 2.0 - 1.0) * fSpeed;

	vWaves = SmoothTriangleWave(vWaves);
	vPos.xyz += (vNormal.xyz * vWaves.xxy * fDetailAmp);
}




void main() {
	// NB: technically need the inverse transpose, etc
	mat4 turfMatrix = turfMatrices[gl_InstanceID];
	mat3 nrmlMatrix = mat3(turfMatrix);

	vec4 vertexPos = vec4(positionAttr, 1.0);
	vec4 worldPos = turfMatrix * vertexPos;
	vec3 objPos = nrmlMatrix * vertexPos.xyz;

	wsNormal = nrmlMatrix * normalAttr;
	// animate
	worldPos.xyz += ApplyMainBending(objPos, windSpeed.xz, texCoordAttr.s * 0.004 + 0.007) - objPos;

	ApplyDetailBending(worldPos.xyz, wsNormal,  texCoordAttr.s, frame / 30.0, 0.3, texCoordAttr.t * 0.4);

	{
		// compute ambient & diffuse lighting per-vertex, specular is per-pixel
		// TODO: make front/back surface constants customizable?
		float sunCosAngle = dot(wsNormal, sunDir);
		float diffuseTerm = sunCosAngle * 0.4 + 0.6;

		diffuseTerm = max(diffuseTerm, (-sunCosAngle * 0.3 + 0.7) * 0.8);
		ambientDiffuseLightTerm = ambientLightColor + diffuseTerm * diffuseLightColor;
	}

#if (defined(HAVE_SHADOWS) || defined(SHADOW_GEN))
	vec4 vertexShadowPos = shadowMatrix * worldPos;

	shadowTexCoords = vertexShadowPos;
#endif

#ifdef SHADOW_GEN
	{
		gl_Position = projMatrix * vertexShadowPos;
		return;
	}
#endif

	bladeTexCoords = texCoordAttr;
	shadingTexCoords = worldPos.xzxz * vec4(mapSizePO2, mapSize);

	gl_Position = projMatrix * viewMatrix * worldPos;

	{
		float eyeDepth = distance(camPos, worldPos.xyz);
		float fogRange = (fogParams.y - fogParams.x) * fogParams.z;
		float fogDepth = (eyeDepth - fogParams.x * fogParams.z) / fogRange;
		// float fogDepth = (fogParams.y * fogParams.z - eyeDepth) / fogRange;

		fogFactor = 1.0 - clamp(fogDepth, 0.0, 1.0);
		// fogFactor = clamp(fogDepth, 0.0, 1.0);
	}
}

