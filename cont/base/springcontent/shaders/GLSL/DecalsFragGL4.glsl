#version 420 compatibility
#extension GL_ARB_uniform_buffer_object : enable
#ifdef USE_SSBO
#extension GL_ARB_shader_storage_buffer_object : require
#endif
//#extension GL_ARB_shading_language_420pack: enable

//#define MAX_DECALS_PER_GROUP 48
//#define MAX_DECALS_GROUPS 300
//#define MAX_DECALS 300
//#define USE_PARALLAX
//#define USE_SSBO
//#define DEBUG

struct SDecal {
	vec3 pos;
	float alpha;
	vec2 invSize;
	vec2 rotMatrixElements;
	vec4 texCoords;
	vec4 texNormalsCoords;
};


struct SDecalGroup {
	vec4 boundAABB[2];
	int ids[MAX_DECALS_PER_GROUP];
};

//FIXME layout(std140)
layout(packed) uniform decalGroupsUBO
{
	SDecalGroup groups[MAX_DECALS_GROUPS];
};

#ifdef USE_SSBO
layout(std140) readonly buffer decalsUBO
#else
layout(std140) uniform decalsUBO
#endif
{
	SDecal decals[MAX_DECALS];
};


SDecal GetDecalInfo(int id) {
	return decals[id];
}

layout(binding=0) uniform sampler2D decalAtlasTex;
layout(binding=1) uniform sampler2D groundNormalsTex;
layout(binding=3) uniform sampler2D infoTex;
layout(binding=4) uniform sampler2D depthTex;

//FIXME make UBO?
uniform vec3 camPos;
uniform vec2 invScreenSize;
uniform mat4 viewProjMatrixInv;
uniform vec2 invMapSize;
uniform vec2 invMapSizePO2;


layout(std140) uniform SGroundLighting
{
	uniform vec3 ambientColor;
	uniform vec3 diffuseColor;
	uniform vec3 specularColor;
	uniform vec3 dir;

	uniform vec3 fogColor;
	uniform float fogEnd;
	uniform float fogScale;
} groundLighting;

#ifdef HAVE_SHADOWS
uniform mat4 shadowMatrix;
uniform float shadowDensity;
layout(binding=2) uniform sampler2DShadow shadowTex;
#endif

flat in int decalGroupId;

layout(location = 0) out vec4 fragColor;



vec3 ReconstructWorldPos() {
	vec2 screenCoord = (gl_FragCoord.st - vec2(0.5)) * invScreenSize;
	float z = texture2D(depthTex, screenCoord).x;
	vec4 worldPos4 = viewProjMatrixInv * vec4(screenCoord, z, 1.0);
	return worldPos4.xyz / worldPos4.w;
}


vec3 GroundNormal(vec3 worldPos, vec3 normal) {
	vec3 groundNormal;
	groundNormal.xz = texture2D(groundNormalsTex, worldPos.xz * invMapSize).ra;
	groundNormal.y  = sqrt(1.0 - dot(groundNormal.xz, groundNormal.xz));

	//normal = groundNormal;
	vec3 front = normalize(cross(groundNormal, vec3(1.,0.,0.)));
	vec3 right = cross(groundNormal, front);

	return (mat3x3(right, groundNormal, front) * normal);
}


vec3 GetDiffuseLighting(vec3 worldPos, vec3 normal) {
	float normalSunCos = max(0.0, dot(normal, groundLighting.dir));
	vec3 diffuse = groundLighting.diffuseColor * normalSunCos;

#ifdef HAVE_INFOTEX
	diffuse += texture2D(infoTex, worldPos.xz * invMapSizePO2).rgb;
	diffuse -= vec3(0.5);
#endif
	return diffuse;
}


float GetShadowOcclusion(vec3 worldPos) {
	float shadowInt = 1.0;
#ifdef HAVE_SHADOWS
	vec4 vertexShadowPos = shadowMatrix * vec4(worldPos, 1.0);
	shadowInt = shadow2DProj(shadowTex, vertexShadowPos).r;
	shadowInt = mix(1.0, shadowInt, shadowDensity);
#endif
	return shadowInt;
}


float ParallaxMapping(inout vec2 ntx, const SDecal d, const mat2 rotMatrix, const vec3 eyeDir)
{
#ifdef USE_PARALLAX
	const float layerDepth = 1.0 / 5.0;

	vec2 texViewDir = (rotMatrix * eyeDir.xz) * 0.06 * layerDepth;

	vec2 txn = mix(d.texNormalsCoords.st, d.texNormalsCoords.pq, ntx);
	float height = 1.0 - texture2D(decalAtlasTex, txn).a;
	float curHeight  = 0.0;
	float prevHeight = 0.0;

	//FIXME for-loop
	// steep mapping
	while (curHeight < height) {
		ntx -= texViewDir;
		txn = mix(d.texNormalsCoords.st, d.texNormalsCoords.pq, ntx); //FIXME
		prevHeight = height;
		height = 1.0 - texture2D(decalAtlasTex, txn).a;
		curHeight += layerDepth;
	}

	// Parallax Occlusion Mapping
	float nextH = height - curHeight;
	float prevH = prevHeight - curHeight + layerDepth;
	float weight = nextH / (nextH - prevH);
	ntx = mix(ntx, ntx + texViewDir, weight);
	return 1.0;







	// soft self-shadow mapping
	/*height = mix(height, prevHeight + layerDepth, weight);


	float shadowMultiplier = 0.0;

	// calculate initial parameters
	float invNumLayers  = 1.0 / 4.0;
	float layerHeight   = height * invNumLayers;
	vec2 texStep        = (rotMatrix * groundLighting.dir.xz) * 0.06 * invNumLayers;

	// current parameters
	curHeight = height - layerHeight;
	float stepIndex = 1.0;
	vec2 stx = ntx;

	// while point is below depth 0.0 )
	while (curHeight > 0.0) {
		stx += texStep;
		vec2 stxn = mix(d.texNormalsCoords.st, d.texNormalsCoords.pq, stx); //FIXME
		float diffHeight = curHeight - texture(decalAtlasTex, stxn).r;

		// if point is under the surface
		float underSurface = step(diffHeight, 0.0);

		// calculate partial shadowing factor
		float newShadowMultiplier = diffHeight * (1.0 - stepIndex * invNumLayers);
		shadowMultiplier = max(shadowMultiplier, newShadowMultiplier * underSurface);

		// offset to the next layer
		stepIndex += 1.0; //FIXME optimize
		curHeight -= layerHeight;
	}

	// calculate lighting only for surface oriented to the light source
	//if(dot(vec3(0, 0, 1), L) > 0)
	return (1.0 - shadowMultiplier);*/
#else
	return 1.0;
#endif
}


#ifdef DEBUG
const vec4 colors[8] = {
	vec4(1.0, 0.0, 0.0, 0.75),
	vec4(0.0, 1.0, 0.0, 0.75),
	vec4(0.0, 0.0, 1.0, 0.75),
	vec4(1.0, 1.0, 0.0, 0.75),
	vec4(1.0, 0.0, 1.0, 0.75),
	vec4(0.0, 1.0, 1.0, 0.75),
	vec4(1.0, 1.0, 1.0, 0.75),
	vec4(0.0, 0.0, 0.0, 0.75)
};
#endif


void main() {
	fragColor = vec4(0.0);
	SDecalGroup g = groups[decalGroupId];

	// PROJECTION
	vec3 worldPos = ReconstructWorldPos();
	vec3 eyeDir   = (camPos - worldPos);
	float eyeDist = length(eyeDir);
	eyeDir /= eyeDist;

	/*bool outside1 = any(greaterThan(worldPos.xz, g.boundAABB[1].xy)); //FIXME
	bool outside2 = any(lessThan(worldPos.xz, g.boundAABB[0].xy));
	if (any(bvec2(outside1, outside2))) {
		discard;
	}*/

	vec4 albedo = vec4(vec3(0.0), 1.0);
	vec3 normal = vec3(0., 1., 0.);
	for (int i = 0; i<MAX_DECALS_PER_GROUP; ++i) {
		SDecal d = GetDecalInfo(g.ids[i]);
		mat2 rotMatrix = mat2(d.rotMatrixElements.x, -d.rotMatrixElements.y, d.rotMatrixElements.y, d.rotMatrixElements.x);

		// WORLD SPACE -> DECAL OBJECT SPACE
		vec3 relPos = worldPos - d.pos;
		vec2 ntx = (rotMatrix * relPos.xz) * d.invSize.xy; // range: -1 .. 0 .. +1
		ntx = ntx * 0.5 + 0.5; // range: 0..+1

		//
		float shadowMultiplier = ParallaxMapping(ntx, d, rotMatrix, eyeDir);
		bool outside = any(greaterThan(abs(ntx - vec2(0.5)), vec2(0.5)));

		// improves cache hit-rate for non-rendered texels
		ntx = clamp(ntx, vec2(0.0), vec2(1.0));
		//ntx = mix(ntx, vec2(0.5), outside);

		// TEXTURING
		vec2 tx = mix(d.texCoords.st, d.texCoords.pq, ntx);
		vec4 albedoD = texture2D(decalAtlasTex, tx);
		albedoD.rgb *= shadowMultiplier; //FIXME
	#ifdef DEBUG
		albedoD = colors[i];
	#endif
		albedoD.a *= clamp(1.0 - abs(relPos.y) * d.invSize.x, 0.0, 1.0); // make transparent when terrain is higher or lower than the decal's pos
		//albedoC.rgb = mix(albedoC.rgb, vec3(cos(float(i) *1.5), sin(float(i) *1.5), 0.0), 0.5);
		albedoD.a = mix(albedoD.a * d.alpha, 0.0, outside);
		albedo.rgb = mix(albedo.rgb, albedoD.rgb, albedoD.a);
		albedo.a *= (1.0 - albedoD.a);

		// NORMAL MAPPING
		vec2 txn = mix(d.texNormalsCoords.st, d.texNormalsCoords.pq, ntx);
		vec3 normalD = texture2D(decalAtlasTex, txn).rbg;
		normalD = (normalD * 2.0) - 1.0;
		//normalD.y = sqrt(1.0 - dot(normalD.xz, normalD.xz));
		normalD.xz = rotMatrix * normalD.xz;
		normal = mix(normal, normalD, albedoD.a);
	}

	// AlphaFunc(GL_LESS, 1.0)
	if (albedo.a >= 1.0)
		discard;


	// transform normal to GroundNormal space
	normal = normalize(normal);
	normal = GroundNormal(worldPos, normal);

	// LIGHTING (albedo & shadow)
	vec3 diffuseTerm = GetDiffuseLighting(worldPos, normal);
	float shadowInt  = GetShadowOcclusion(worldPos);

	// calculate Specular Term:
	//FIXME add glossyMap
	vec3 R = normalize(-reflect(groundLighting.dir, normal));
	vec3 specTerm = groundLighting.specularColor * 2.0 * clamp( pow(max(dot(R, eyeDir),0.0), 5.13), 0.0, 1.0);

	// COMBINE
	vec3 lightCol = (specTerm + diffuseTerm) * shadowInt + groundLighting.ambientColor;
	fragColor.rgb = albedo.rgb * lightCol;
	fragColor.a   = albedo.a;

	// FOG
	float fogFactor = (groundLighting.fogEnd - eyeDist) * groundLighting.fogScale;
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	fragColor.rgb = mix(groundLighting.fogColor * (1.0 - albedo.a), fragColor.rgb, fogFactor); //FIXME

#ifdef DEBUG
	fragColor = mix(fragColor, vec4(1.0, 0.0, 0.0, 0.0), 0.2);
#endif
}

