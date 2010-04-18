#if (defined(TREE_NEAR_SHADOW) || defined(TREE_DIST_SHADOW))
uniform sampler2DShadow shadowTex;
uniform sampler2D       diffuseTex;

uniform vec3 groundAmbientColor;
uniform float groundShadowDensity;

varying float fogFactor;

void main() {
	vec4 shadowInt = shadow2DProj(shadowTex, gl_TexCoord[0]);
		shadowInt.x = max(0.0, min(shadowInt.x + groundShadowDensity, 1.0));
	vec3 shadeInt = mix(groundAmbientColor.rgb, gl_Color.rgb, shadowInt.x);
	vec4 diffuseCol = texture2D(diffuseTex, gl_TexCoord[1].st);

	gl_FragColor.rgb = diffuseCol.rgb * shadeInt.rgb;

	#if (defined(TREE_NEAR_SHADOW) || defined(TREE_DIST_SHADOW))
	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogFactor);
	#endif

	gl_FragColor.a = diffuseCol.a;
}
#endif
