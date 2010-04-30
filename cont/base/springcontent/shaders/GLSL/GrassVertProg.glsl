uniform vec2 mapSizePO2;     // (1.0 / pwr2map{x,z} * SQUARE_SIZE)
uniform vec2 mapSize;        // (1.0 /     map{x,z} * SQUARE_SIZE)

uniform vec2 texOffset;

uniform vec3 billboardDirX;
uniform vec3 billboardDirY;
uniform vec3 billboardDirZ;

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;

uniform float simFrame;
uniform vec3 windSpeed;

void main() {
	#ifdef GRASS_DIST_BASIC
	vec4 vertexPos = gl_Vertex;

		#ifdef GRASS_ANIMATION
		float windScale =
			0.005 * (1.0 + sin((vertexPos.x + vertexPos.z) / 45.0 + simFrame / 15.0)) *
			gl_Normal.y * (1.0 - texOffset.x);

		 vertexPos.x += (windSpeed.x * windScale);
		 vertexPos.z += (windSpeed.z * windScale);
		#endif

		vertexPos.xyz += billboardDirX * gl_Normal.x;
		vertexPos.xyz += billboardDirY * gl_Normal.y;
		vertexPos.xyz += billboardDirZ;

	gl_TexCoord[0].st = gl_MultiTexCoord0.st + texOffset;
	gl_TexCoord[1].st = vertexPos.xz * mapSize;
	gl_TexCoord[2].st = vertexPos.xz * mapSize;
	gl_TexCoord[3].st = vertexPos.xz * mapSizePO2;

	gl_FrontColor = gl_Color;
	gl_FogFragCoord = length((gl_ModelViewProjectionMatrix * vertexPos).xyz);
	gl_Position = gl_ModelViewProjectionMatrix * vertexPos;
	#endif


	#ifdef GRASS_NEAR_SHADOW
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexPos = gl_ModelViewMatrix * gl_Vertex;
		#ifdef GRASS_ANIMATION
		vec2 windScale = vec2(
			sin((simFrame + gl_MultiTexCoord0.s) * 5.0 / (10.0 + floor(gl_MultiTexCoord0.s))) * (gl_MultiTexCoord0.t) * 0.01,
			(1.0 + sin((vertexPos.x + vertexPos.z) / 45.0 + simFrame / 15.0)) * 0.025 * gl_MultiTexCoord0.t
		);

		vertexPos.x += dot(windSpeed.zx, windScale);
		vertexPos.z += dot(windSpeed.xz, windScale);
		#endif
	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	gl_TexCoord[0].st = vertexPos.xz * mapSizePO2;
	gl_TexCoord[1]    = vertexShadowPos;
	gl_TexCoord[1].z  = dot(shadowMatrix[2], vertexPos);
	gl_TexCoord[1].w  = dot(shadowMatrix[3], vertexPos);
	gl_TexCoord[2].st = vertexPos.xz * mapSize;
	gl_TexCoord[3].st = gl_MultiTexCoord0.st;

	gl_FrontColor = gl_Color;
	gl_FogFragCoord = length((gl_ProjectionMatrix * vertexPos).xyz);
	gl_Position = gl_ProjectionMatrix * vertexPos;
	#endif


	#ifdef GRASS_DIST_SHADOW
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexPos = gl_Vertex;
		#ifdef GRASS_ANIMATION
		float windScale =
			0.005 * (1.0 + sin((vertexPos.x + vertexPos.z) / 45.0 + simFrame / 15.0)) *
			gl_Normal.y * (1.0 - texOffset.x);

		 vertexPos.x += (windSpeed.x * windScale);
		 vertexPos.z += (windSpeed.z * windScale);
		#endif

		vertexPos.xyz += billboardDirX * gl_Normal.x;
		vertexPos.xyz += billboardDirY * gl_Normal.y;
		vertexPos.xyz += billboardDirZ;
	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	// prevent grass from being shadowed by the ground beneath it
	gl_TexCoord[0].st = vertexPos.xz * mapSizePO2;
	gl_TexCoord[1]    = vertexShadowPos;
	gl_TexCoord[1].z  = dot(shadowMatrix[2], vertexPos) - 0.0005;
	gl_TexCoord[1].w  = dot(shadowMatrix[3], vertexPos);
	gl_TexCoord[2].st = vertexPos.xz * mapSize;
	gl_TexCoord[3].st = gl_MultiTexCoord0.st + texOffset;

	gl_FrontColor = gl_Color;
	gl_FogFragCoord = length((gl_ModelViewProjectionMatrix * vertexPos).xyz);
	gl_Position = gl_ModelViewProjectionMatrix * vertexPos;
	#endif
}
