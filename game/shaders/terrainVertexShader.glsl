
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

void directionalLight(in int i, in vec3 normal)
{
   float nDotVP;         // normal . light direction
   float nDotHV;         // normal . light half vector
   float pf;             // power factor

   nDotVP = max(0.0, dot(normal, normalize(vec3 (gl_LightSource[i].position))));
   nDotHV = max(0.0, dot(normal, vec3 (gl_LightSource[i].halfVector)));

   if (nDotVP == 0.0)
       pf = 0.0;
   else
       pf = pow(nDotHV, gl_FrontMaterial.shininess);

   Ambient  += gl_LightSource[i].ambient;
   Diffuse  += gl_LightSource[i].diffuse * nDotVP;
   Specular += gl_LightSource[i].specular * pf;
}

vec3 fnormal()
{
    //Compute the normal 
    vec3 normal = gl_NormalMatrix * gl_Normal;
    normal = normalize(normal);
    return normal;
}

void flight(in vec3 normal, in vec4 ecPosition)
{
    vec4 color;
    vec3 ecPosition3;
    vec3 eye;

    ecPosition3 = (vec3 (ecPosition)) / ecPosition.w;
    eye = vec3 (0.0, 0.0, 1.0);

    // Clear the light intensity accumulators
    Ambient  = vec4 (0.0);
    Diffuse  = vec4 (0.0);
    Specular = vec4 (0.0);

    directionalLight(0, normalize(normal));

		//gl_FrontLightModelProduct.sceneColor +
    color = Ambient * gl_FrontMaterial.ambient +
      Diffuse * gl_FrontMaterial.diffuse;
    color += Specular * gl_FrontMaterial.specular;
    color = clamp( color, 0.0, 1.0 );
    gl_FrontColor = color;
}

#ifdef UseBumpmapping

attribute mat3 TangentSpaceMatrix;

/*	vec3 eyeDir = normalize(gl_Vertex - wsEyePos);
	vec3 halfDir = normalize(normalize(wsLightDir) + eyeDir);
	tsHalfDir = TangentSpaceMatrix * halfDir;*/

#endif


void main (void)
{
    // Eye-coordinate position of vertex, needed in various calculations
    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = gl_ProjectionMatrix * ecPosition;
    
	CalculateTexCoords();
#ifdef UseBumpmapping
	tsLightDir = TangentSpaceMatrix * (-wsLightDir);
	vec3 eyeDir = normalize(gl_Vertex - wsEyePos);
	tsEyeDir = TangentSpaceMatrix * eyeDir;
#endif
}

