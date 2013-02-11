
uniform sampler2D shadingTex;
uniform sampler2D grassShadingTex;
uniform sampler2D bladeTex;

#ifdef HAVE_SHADOW
uniform sampler2DShadow shadowMap;
#endif

#ifdef HAVE_INFOTEX
uniform sampler2D infoMap;
#endif

void main() {
	#ifdef SHADOW_GEN
	{
		#ifdef DISTANCE_FAR
			gl_FragColor = texture2D(bladeTex, gl_TexCoord[3].st);
		#else
			gl_FragColor = vec4(1.0);
		#endif
		return;
	}
	#endif

	//FIXME add diffuse lighting -> needs normal that is unavailable atm

	gl_FragColor.a = 1.0;
	gl_FragColor.rgb  = texture2D(shadingTex, gl_TexCoord[0].st).rgb * 2.0;
	gl_FragColor.rgb *= texture2D(grassShadingTex, gl_TexCoord[2].st).rgb;
	gl_FragColor *= texture2D(bladeTex, gl_TexCoord[3].st) * gl_Color;

#ifdef HAVE_SHADOW
	float shadowCoeff = shadow2DProj(shadowMap, gl_TexCoord[1]).a;
	gl_FragColor.rgb *= mix(gl_TextureEnvColor[1].rgb, vec3(1.0), shadowCoeff);
#endif

#ifdef HAVE_INFOTEX
	gl_FragColor.rgb += texture2D(infoMap, gl_TexCoord[0].st).rgb;
	gl_FragColor.rgb -= vec3(0.5, 0.5, 0.5);
#endif

	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, gl_FogFragCoord);
}
