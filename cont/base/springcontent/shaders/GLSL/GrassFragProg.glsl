//#define FLAT_SHADING

uniform sampler2D shadingTex;
uniform sampler2D grassShadingTex;
uniform sampler2D bladeTex;

#ifdef HAVE_SHADOWS
uniform sampler2DShadow shadowMap;
uniform float groundShadowDensity;
#endif

#ifdef HAVE_INFOTEX
uniform sampler2D infoMap;
#endif

uniform samplerCube specularTex;
uniform vec3 specularLightColor;
uniform vec3 ambientLightColor;
uniform vec3 camDir;

varying vec3 normal;
varying vec4 shadingTexCoords;
varying vec2 bladeTexCoords;
varying vec3 ambientDiffuseLightTerm;
#if defined(HAVE_SHADOWS) || defined(SHADOW_GEN)
  varying vec4 shadowTexCoords;
#endif


void main() {
#ifdef SHADOW_GEN
	{
  #ifdef DISTANCE_FAR
		gl_FragColor = texture2D(bladeTex, bladeTexCoords);
  #else
		gl_FragColor = vec4(1.0);
  #endif
		return;
	}
#endif

	vec4 matColor = texture2D(bladeTex, bladeTexCoords);
	matColor.rgb *= texture2D(grassShadingTex, shadingTexCoords.pq).rgb;
	matColor.rgb *= texture2D(shadingTex, shadingTexCoords.st).rgb * 2.0;

#if defined(FLAT_SHADING) || defined(DISTANCE_FAR)
	vec3 specular = vec3(0.0);
#else
	vec3 reflectDir = reflect(camDir, normalize(normal));
	vec3 specular   = textureCube(specularTex, reflectDir).rgb;
#endif
	gl_FragColor.rgb = matColor.rgb * ambientDiffuseLightTerm + 0.1 * specular * specularLightColor; //TODO make `0.1` specular distr. customizable?
	gl_FragColor.a   = matColor.a * gl_Color.a;

#ifdef HAVE_SHADOWS
	float shadowCoeff = clamp(shadow2DProj(shadowMap, shadowTexCoords).r, 1.0 - groundShadowDensity, 1.0);
	gl_FragColor.rgb *= mix(ambientLightColor, vec3(1.0), shadowCoeff);
#endif

#ifdef HAVE_INFOTEX
	gl_FragColor.rgb += texture2D(infoMap, shadingTexCoords.st).rgb - 0.5;
#endif

	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, gl_FogFragCoord);
}
