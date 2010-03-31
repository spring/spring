uniform vec2 mapSizePO2;     // (1.0 / pwr2map{x,z} * SQUARE_SIZE)
uniform vec2 mapSize;        // (1.0 /     map{x,z} * SQUARE_SIZE)

uniform vec2 texOffset;

uniform vec3 billboardDirX;
uniform vec3 billboardDirY;
uniform vec3 billboardDirZ;

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;

void main() {
	#ifdef GRASS_BASIC
	vec4 vertexPos = gl_Vertex;
		vertexPos += billboardDirX * gl_Normal.x;
		vertexPos += billboardDirY * gl_Normal.y;
		vertexPos += billboardDirZ;

	gl_TexCoord[0].st = gl_MultiTexCoord0.st + texOffset;
	gl_TexCoord[1].st = vertexPos.xz * mapSize;
	gl_TexCoord[2].st = vertexPos.xz * mapSize;
	gl_TexCoord[3].st = vertexPos.xz * mapSizePO2;

	gl_FrontColor = gl_Color;
	gl_FogFragCoord = length((gl_ModelViewProjectionMatrix * vertexPos).xyz);
	gl_Position = gl_ModelViewProjectionMatrix * vertexPos;
	#endif


	#ifdef GRASS_NEAR
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexPos = gl_ModelViewMatrix * gl_Vertex;
	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	gl_TexCoord[0].st = vertexPos.xz * mapSizePO2;
	gl_TexCoord[1]    = vertexShadowPos;
	gl_TexCoord[1].z  = shadowMatrix[2] * vertexPos; // ?
	gl_TexCoord[1].w  = shadowMatrix[3] * vertexPos; // ?
	gl_TexCoord[2].st = vertexPos.xz * mapSize;
	gl_TexCoord[3].st = gl_TexCoord[0].st;

	gl_FrontColor = gl_Color;
	gl_FogFragCoord = length((gl_ProjectionMatrix * vertexPos).xyz);
	gl_Position = gl_ProjectionMatrix * vertexPos;
	#endif


	#ifdef GRASS_DIST
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexPos = gl_Vertex;
		vertexPos += billboardDirX * gl_Normal.x;
		vertexPos += billboardDirY * gl_Normal.y;
		vertexPos += billboardDirZ;
	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	// prevent grass from being shadowed by the ground beneath it
	gl_TexCoord[0].st = vertexPos.xz * mapSizePO2;
	gl_TexCoord[1]    = vertexShadowPos;
	gl_TexCoord[1].z  = (shadowMatrix[2] * vertexPos) - 0.0005;
	gl_TexCoord[1].w  = (shadowMatrix[3] * vertexPos);
	gl_TexCoord[2].st = vertexPos.xz * mapSize;
	gl_TexCoord[3].st = vertexPos.xz * texOffset;

	gl_FrontColor = gl_Color;
	gl_FogFragCoord = length((gl_ModelViewProjectionMatrix * vertexPos).xyz);
	gl_Position = gl_ModelViewProjectionMatrix * vertexPos;
	#endif
}
