uniform vec2 mapSizePO2;     // (1.0 / pwr2map{x,z} * SQUARE_SIZE)
uniform vec2 mapSize;        // (1.0 /     map{x,z} * SQUARE_SIZE)

uniform vec2 texOffset;

uniform vec3 billboardDirX;
uniform vec3 billboardDirY;
uniform vec3 billboardDirZ;

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;

uniform vec3 camPos;

uniform float simFrame;
uniform vec3 windSpeed;

void main() {
	#ifdef DISTANCE_NEAR
		#ifdef SHADOW_GEN
		vec4 vertexPos = gl_Vertex;
		#else
		vec4 vertexPos = gl_ModelViewMatrix * gl_Vertex;
		#endif
		#ifdef ANIMATION
		vec2 windScale;
		windScale.x = sin((simFrame + gl_MultiTexCoord0.s) * 5.0 / (10.0 + floor(gl_MultiTexCoord0.s))) * 0.01;
		windScale.y = (1.0 + sin((vertexPos.x + vertexPos.z) / 45.0 + simFrame / 15.0)) * 0.025;
		windScale *= gl_MultiTexCoord0.tt;

		vertexPos.x += dot(windSpeed.zx, windScale);
		vertexPos.z += dot(windSpeed.xz, windScale);
		#endif

	vec4 worldPos = vertexPos;
	#endif

	#ifdef DISTANCE_FAR
	vec4 worldPos = gl_Vertex;
		#ifdef ANIMATION
		float windScale =
			0.005 * (1.0 + sin((worldPos.x + worldPos.z) / 45.0 + simFrame / 15.0)) *
			gl_Normal.y * (1.0 - texOffset.x);

		 worldPos.xz += (windSpeed.xz * windScale);
		#endif

		worldPos.xyz += billboardDirX * gl_Normal.x * (1.0 + windScale);
		worldPos.xyz += billboardDirY * gl_Normal.y * (1.0 + windScale);
		worldPos.xyz += billboardDirZ;

	vec4 vertexPos = gl_ModelViewMatrix * worldPos;
	#endif

	#if defined(HAVE_SHADOW) || defined(SHADOW_GEN)
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);
	#ifdef SHADOW_GEN
	vec4 vertexShadowPos = gl_ModelViewMatrix * worldPos;
	#else
	vec4 vertexShadowPos = shadowMatrix * worldPos;
	#endif
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	gl_TexCoord[1] = vertexShadowPos;
	#endif

	#ifdef SHADOW_GEN
	{
		gl_TexCoord[3].st = gl_MultiTexCoord0.st + texOffset;
		gl_Position = gl_ProjectionMatrix * vertexShadowPos;
		return;
	}
	#endif

	gl_TexCoord[0].st = worldPos.xz * mapSizePO2;
	gl_TexCoord[2].st = worldPos.xz * mapSize;
	gl_TexCoord[3].st = gl_MultiTexCoord0.st + texOffset;

	gl_FrontColor = gl_Color;
	gl_Position = gl_ProjectionMatrix * vertexPos;

	gl_FogFragCoord = distance(camPos, worldPos.xyz);
	gl_FogFragCoord = (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale;
	gl_FogFragCoord = clamp(gl_FogFragCoord, 0.0, 1.0);
}
