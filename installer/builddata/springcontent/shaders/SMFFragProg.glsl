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
// ARB shader receives dimmed groundAmbientColor
#define GROUND_AMBIENT_COLOR_MUL (210 / 255.0)

void main() {
	vec2 tc0 = gl_TexCoord[0].st;
	vec2 tc1 = gl_TexCoord[1].st;
	vec4 tc2 = gl_TexCoord[2];

	vec3 diffuse = texture2D(diffuseTex, tc0).rgb;
	vec3 specular = texture2D(specularTex, tc1).rgb;
	vec3 normal = normalize((texture2D(normalsTex, tc1) * 2.0).rgb - 1.0);
	vec3 detail = vec3(0.0, 0.0, 0.0);
	// vec3 detail = normalize((texture2D(detailTex, tc1) * 2.0).rgb - 1.0);

	/*
	vec4 unitAmbient  = gl_LightSource[1].ambient;  // needs to be groundAmbientColor
	vec4 unitDiffuse  = gl_LightSource[1].diffuse;  // needs to be groundSunColor
	vec4 unitSpecular = gl_LightSource[1].specular; // also unitAmbient, needs to be groundSpecularColor
	*/

	float fragDepth = shadow2DProj(shadowTex, tc2).r;
	float fragShade = 1.0 - ((1.0 - fragDepth) * GROUND_SHADOW_DENSITY);

	float cosAngleDiffuse = max(dot(normalize(lightDir), normal), 0.0);
	float cosAngleSpecular = max(dot(normalize(halfDir), normal), 0.0);
	float specularPower = pow(cosAngleSpecular, SPECULAR_EXPONENT);

	#ifdef SMF_ARB_LIGHTING
	gl_FragColor.rgb =
		groundAmbientColor * GROUND_AMBIENT_COLOR_MUL +
		groundDiffuseColor * (diffuse + detail) * cosAngleDiffuse * fragShade;
	#else
	gl_FragColor.rgb =
		groundAmbientColor * GROUND_AMBIENT_COLOR_MUL * 0.25 +
		groundDiffuseColor * (diffuse + detail) * cosAngleDiffuse * fragShade +
		/* groundSpecularColor * */ specular * specularPower;
	#endif

	// MOV result.texcoord[2].z, pos.y;
	// MAD result.color.w, fragment.texcoord[2].z, 0.1, 1;
	gl_FragColor.a = 1.0;
}
