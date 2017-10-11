uniform sampler2DShadow shadowTex;
uniform sampler2D       diffuseTex;

uniform vec3 groundAmbientColor;
uniform float groundShadowDensity;

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;

#if (defined(TREE_SHADOW))
	varying float fogFactor;
#endif

void main() {
	#if (defined(TREE_SHADOW))
	vec4 vertexPos = gl_TexCoord[2];
	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;

	float shadowCoeff = mix(1.0, shadow2DProj(shadowTex, vertexShadowPos).r, groundShadowDensity);
	#else
	float shadowCoeff = 1.0;
	#endif

	vec4 diffuseCol = texture2D(diffuseTex, gl_TexCoord[1].st);
	vec3 shadeInt = mix(groundAmbientColor.rgb, vec3(1.0, 1.0, 1.0), shadowCoeff);


	gl_FragColor.rgb = diffuseCol.rgb * shadeInt.rgb;

#if (defined(TREE_SHADOW))
	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogFactor);
#endif

	gl_FragColor.a = diffuseCol.a;
}
