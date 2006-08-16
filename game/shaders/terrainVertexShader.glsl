
vec4 Ambient;
vec4 Diffuse;
vec4 Specular;

#ifdef UseShadowMapping

vec4 calcShadowTexCoord()
{
	vec2 temp, at;

	temp.x = dot(gl_Vertex, shadowMatrix[0]);
	temp.y = dot(gl_Vertex, shadowMatrix[1]);
	
	at = abs(temp);
	at += env[17];
	at = inversesqrt(at);
	at += env[18];

	vec4 tc;
	tc.xy = temp * at + env[16];
	
	tc.z = dot(gl_Vertex, shadowMatrix[2]);
	tc.w = dot(gl_Vertex, shadowMatrix[3]);
	
	return tc;
}

#endif

#ifdef UseBumpmapping
	attribute mat3 TangentSpaceMatrix;
#endif


void main (void)
{
    // Eye-coordinate position of vertex, needed in various calculations
    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = gl_ProjectionMatrix * ecPosition;
    
	CalculateTexCoords();
#ifdef UseBumpmapping
	tsLightDir = TangentSpaceMatrix * (-wsLightDir);
	vec3 eyeDir = normalize(gl_Vertex.xyz - wsEyePos);
	tsEyeDir = TangentSpaceMatrix * eyeDir;
#endif
}

