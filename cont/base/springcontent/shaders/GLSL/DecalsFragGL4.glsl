#version 420 compatibility

#extension GL_ARB_uniform_buffer_object : enable
//#extension GL_ARB_shading_language_420pack: enable


//#define HAVE_INFO_TEX
#define HAVE_SHADOWS


struct SDecal {
	vec3 pos;
	float rot;
	vec4 texcoords;
	vec2 size;
	float alpha;
};

layout(binding=0) uniform sampler2D decalTex;
layout(binding=1) uniform sampler2D groundNormalsTex;
layout(binding=3) uniform sampler2D infoTex;
layout(binding=4) uniform sampler2D depthTex;
layout(binding=5) uniform sampler2D normalMap;
layout(binding=6) uniform sampler2D normalMap2;

uniform vec2 invScreenSize;
uniform vec2 invMapSize;
uniform vec2 invMapSizePO2; // 1.0f / (pwr2map{x,z} * SQUARE_SIZE)  needed for infotex & shadingtex

layout(std140, binding=1) uniform SGroundLighting
{
	uniform vec3 ambientColor;
	uniform vec3 diffuseColor;
	uniform vec3 specularColor;
	uniform vec3 dir;
} groundLighting;

#ifdef HAVE_SHADOWS
uniform mat4 shadowMatrix;
uniform float shadowDensity;
layout(binding=2) uniform sampler2DShadow shadowTex;
#endif

/*flat in int instanceID;
layout(binding=7) uniform samplerBuffer decalInfo;

SScar GetDecalInfo() {
	vec4 info1 = texelFetch(decalInfo, instanceID*3 + 0);
	vec4 info2 = texelFetch(decalInfo, instanceID*3 + 1);
	vec4 info3 = texelFetch(decalInfo, instanceID*3 + 2);
	SDecal decal;
	decal.pos       = info1.xyz;
	decal.rot       = info1.w;
	decal.texcoords = info2.xy;
	decal.size      = info2.zw;
	decal.alpha     = info3.x;
	return decal;
}*/

flat in SDecal decal;
flat in mat2 rot;

vec3 ReconstructWorldPos() {
	vec2 screenCoord = gl_FragCoord.st * invScreenSize;
	float z = texture2D(depthTex, screenCoord).x;

	vec4 ppos;
	ppos.xyz = vec3(screenCoord, z) * 2. - 1.;
	ppos.w   = 1.;
	vec4 worldPos4 = gl_ModelViewProjectionMatrixInverse * ppos;
	return worldPos4.xyz / worldPos4.w;
}

vec3 GetDiffuseLighting(vec3 worldPos, vec2 ntx) {
	vec3 normal = texture2D(normalMap, ntx).grr;
	normal.y = sqrt(1.0 - clamp(dot(normal.xz, normal.xz), 0.0, 1.0));

	vec3 normal2 = texture2D(normalMap2, ntx * 1.789).grr;
	normal2.y = sqrt(1.0 - clamp(dot(normal2.xz, normal2.xz), 0.0, 1.0));
	normal = normalize(normal * 3.0 + normal2);

	vec3 groundNormal;
	groundNormal.xz = texture2D(groundNormalsTex, worldPos.xz * invMapSize).ra;
	groundNormal.y  = sqrt(1.0 - dot(groundNormal.xz, groundNormal.xz));
	groundNormal = normalize(groundNormal + normal * 3.0);
	float normalSunCos = max(0.0, dot(groundNormal, groundLighting.dir));

	//vec3 diffuse = groundLighting.diffuseColor * pow(normalSunCos, 8.0f);
	vec3 diffuse = groundLighting.diffuseColor * normalSunCos;
	return diffuse;
}

float GetShadowOcclusion(vec3 worldPos) {
	float shadowInt = 1.0;
#ifdef HAVE_SHADOWS
	vec4 vertexShadowPos = shadowMatrix * vec4(worldPos, 1.0);
	vertexShadowPos.st += vec2(0.5, 0.5);
	shadowInt = shadow2DProj(shadowTex, vertexShadowPos).r;
	shadowInt = mix(1.0, shadowInt, shadowDensity);
#endif
	return shadowInt;
}


void main() {
	//SDecal decal = GetDecalInfo();
	vec3 worldPos = ReconstructWorldPos();
	vec3 relPos   = worldPos - decal.pos;

	vec2 ntx; // range: -1 .. 0 .. +1
	ntx.x = dot(relPos.xz, rot * vec2(1.0,0.0)) / decal.size.x;
	ntx.y = dot(relPos.xz, rot * vec2(0.0,1.0)) / decal.size.y;

	bool outside = any(greaterThan(abs(ntx), vec2(1.0)));
	if (outside) {
		gl_FragColor = vec4(0.0);
		return;
	}

	ntx = (ntx + 1.0) * 0.5; // range: 0..+1
	vec2 tx = mix(decal.texcoords.st, decal.texcoords.pq, ntx);

	vec4 albedo = texture2D(decalTex, tx);
	albedo.a *= 1.0 - abs(relPos.y) / decal.size.x; // make transparent when terrain is higher or lower than the decal's pos
	albedo.a *= decal.alpha;

	vec3 diffuseTerm = GetDiffuseLighting(worldPos, ntx);
	float shadowInt  = GetShadowOcclusion(worldPos);

	vec3 lightCol = groundLighting.ambientColor + diffuseTerm * shadowInt;
	gl_FragColor.rgb = albedo.rgb * lightCol;
	gl_FragColor.a   = albedo.a;

#ifdef HAVE_INFO_TEX
	gl_FragColor.rgb *= texture2D(infoTex, worldPos.xz * invMapSizePO2).rgb;
#endif

	// check if pixel is in decal dist
	gl_FragColor.a = gl_FragColor.a * (1.0 - float(outside));
}
