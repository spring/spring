#version 130

uniform vec3 mapParams;

out vec3 worldCameraDir;

//To be drawn with GL_TRIANGLES
const vec2 POS[6] = vec2[6](
	vec2(-2.0, -2.0),
	vec2( 3.0, -2.0),
	vec2(-2.0,  3.0),

	vec2(-2.0,  3.0),
	vec2( 3.0, -2.0),
	vec2( 3.0,  3.0)
);

void main()
{
	vec4 worldPos;
	worldPos.xz	= POS[gl_VertexID] * mapParams.xz;
	worldPos.y  = mapParams.y;
	worldPos.w  = 1.0;
	
	vec4 cameraPos = gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0);

	worldCameraDir = cameraPos.xyz - worldPos.xyz;
	
	gl_Position = gl_ModelViewProjectionMatrix * worldPos;
}