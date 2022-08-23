#if (GL_FRAGMENT_PRECISION_HIGH == 1)
// ancient GL3 ATI drivers confuse GLSL for GLSL-ES and require this
precision highp float;
#else
precision mediump float;
#endif

void main() {
	#if 0
		mat3 normalMatrix = mat3(transpose(inverse(gl_ModelViewMatrix)));
	#else
		mat3 normalMatrix = mat3(gl_ModelViewMatrix);
	#endif

	vec4 lightVertexPos = gl_ModelViewMatrix * gl_Vertex;
	vec3 lightVertexNormal = normalize(normalMatrix * gl_Normal);

	float NdotL = clamp(dot(lightVertexNormal, vec3(0.0, 0.0, 1.0)), 0.0, 1.0);

	//use old bias formula from GetShadowPCFRandom(), but this time to write down shadow depth map values
	const float cb = 1e-6;
	float bias = cb * tan(acos(NdotL));
	bias = clamp(bias, 0.0, 100.0 * cb);

	lightVertexPos.xy += vec2(0.5);
	lightVertexPos.z  += bias;

	gl_Position = gl_ProjectionMatrix * lightVertexPos;

	gl_ClipVertex  = gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}