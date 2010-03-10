#if (defined(TREE_NEAR) || defined(TREE_DIST))
uniform sampler2DShadow shadowTex;
uniform sampler2D       diffuseTex;

uniform vec3 groundAmbientColor;
uniform float groundShadowDensity;

void main() {
	vec4 shadowInt = shadow2DProj(shadowTex, gl_TexCoord[0]);
		shadowInt.x = min(shadowInt.x + groundShadowDensity, 1.0);
	vec3 shadeInt = mix(gl_Color.rgb, groundAmbientColor.rgb, shadowInt.x);
	vec4 diffuseCol = texture2D(diffuseTex, gl_TexCoord[1]);

	gl_FragColor.rgb = diffuseCol.rgb * shadeInt.rgb;
	gl_FragColor.a = diffuseCol.a;
}
#endif
