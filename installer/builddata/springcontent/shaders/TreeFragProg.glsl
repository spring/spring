uniform sampler2D       diffuseTex;
uniform sampler2DShadow shadowTex;

uniform vec4 groundAmbientColor;
uniform float groundShadowDensity;

void main() {
	#if (defined(TREE_NEAR) || defined(TREE_DIST))
	vec4 shadowInt = shadow2DProj(shadowTex, gl_TexCoord[0]);
		shadowInt.x = min(shadowInt.x + groundShadowDensity, 1.0);
	vec4 shadeInt = mix(gl_FrontColor, groundAmbientColor, shadowInt.x);
	vec4 diffuseCol = texture2D(diffuseTex, gl_TexCoord[1]);

	gl_FragColor = diffuseCol * shadeInt;
	#else
	gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
	#endif
}
