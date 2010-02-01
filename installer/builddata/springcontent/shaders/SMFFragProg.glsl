uniform vec3 lightDir;
varying vec3 viewDir;
varying vec3 halfDir;

uniform sampler2D       diffuseTex;
uniform sampler2D       normalsTex;
uniform sampler2DShadow shadowTex;
uniform sampler2D       detailTex;
uniform sampler2D       specularTex;

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;
uniform vec3 groundSpecularColor;

// these need to be uniforms
#define GROUND_SHADOW_DENSITY 0.5
#define SPECULAR_EXPONENT 16.0

void main() {
	vec2 tc0 = gl_TexCoord[0].st;
	vec2 tc1 = gl_TexCoord[1].st;
	vec3 normal = normalize((texture2D(normalsTex, tc1) * 2.0).rgb - 1.0);

	float fragDepth = shadow2DProj(shadowTex, gl_TexCoord[2]).r;
	float fragShade = 1.0 - ((1.0 - fragDepth) * GROUND_SHADOW_DENSITY);

	float cosAngleDiffuse = max(dot(normalize(lightDir), normal), 0.0);
	float cosAngleSpecular = max(dot(normalize(halfDir), normal), 0.0);
	float specularPower = pow(cosAngleSpecular, SPECULAR_EXPONENT);

	/*
	vec4 unitAmbient  = gl_LightSource[1].ambient;  // needs to be groundAmbientColor
	vec4 unitDiffuse  = gl_LightSource[1].diffuse;  // needs to be groundSunColor
	vec4 unitSpecular = gl_LightSource[1].specular; // also unitAmbient, needs to be groundSpecularColor
	*/

	#ifdef SMF_ARB_LIGHTING
	gl_FragColor.rgb =
		(groundAmbientColor +
		groundDiffuseColor * texture2D(diffuseTex, tc0).rgb * cosAngleDiffuse);
	#else
	gl_FragColor.rgb =
		groundAmbientColor * 0.25 +
		groundDiffuseColor * texture2D(diffuseTex, tc0).rgb * cosAngleDiffuse +
		/* groundSpecularColor * texture2D(specularTex, tc1).rgb * */ specularPower;
	#endif

	gl_FragColor.rgb *= fragShade;
	gl_FragColor.a = 1.0;
}
