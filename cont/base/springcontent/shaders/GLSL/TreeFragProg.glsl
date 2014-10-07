uniform sampler2DShadow shadowTex;
uniform sampler2D       diffuseTex;

uniform vec3 groundAmbientColor;
uniform float groundShadowDensity;

#if !(defined(TREE_NEAR_SHADOW) || defined(TREE_DIST_SHADOW))
	varying float fogFactor;
#endif

void main() {
	vec4 diffuseCol = texture2D(diffuseTex, gl_TexCoord[1].st);

	float shadowInt = shadow2DProj(shadowTex, gl_TexCoord[0]).a;
	shadowInt = clamp(shadowInt + groundShadowDensity, 0.0, 1.0);
	vec3 shadeInt = mix(groundAmbientColor.rgb, gl_Color.rgb, shadowInt);

	gl_FragColor.rgb = diffuseCol.rgb * shadeInt.rgb;

#if !(defined(TREE_NEAR_SHADOW) || defined(TREE_DIST_SHADOW))
	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogFactor);
#endif

	gl_FragColor.a = diffuseCol.a;
}
