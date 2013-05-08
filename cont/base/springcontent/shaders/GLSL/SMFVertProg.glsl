#define SMF_TEXSQR_SIZE 1024.0
#define SMF_DETAILTEX_RES 0.02

// uniform vec2 mapSizePO2;   // pwr2map{x,z} * SQUARE_SIZE (TODO programmatically #define this)
// uniform vec2 mapSize;      //     map{x,z} * SQUARE_SIZE (TODO programmatically #define this)
// uniform vec2 mapHeights;   // readmap->curr{Min, Max}Height

uniform ivec2 texSquare;
uniform vec3 cameraPos;
uniform vec4 lightDir;       // mapInfo->light.sunDir

varying vec3 halfDir;
varying float fogFactor;
varying vec4 vertexWorldPos;
varying vec2 diffuseTexCoords;


void main() {
	// calc some lighting variables
	vec3 viewDir = vec3(gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));
	viewDir = normalize(viewDir - gl_Vertex.xyz);
	halfDir = normalize(lightDir.xyz + viewDir);
	vertexWorldPos = gl_Vertex;

	// calc texcoords
	diffuseTexCoords = (floor(gl_Vertex.xz) / SMF_TEXSQR_SIZE) - vec2(texSquare);

	// transform vertex pos
	gl_Position = gl_ModelViewMatrix * gl_Vertex;
	gl_ClipVertex = gl_Position;
	float fogCoord = length(gl_Position.xyz);
	gl_Position = gl_ProjectionMatrix * gl_Position;

	// emulate linear fog
	fogFactor = (gl_Fog.end - fogCoord) * gl_Fog.scale; // gl_Fog.scale == 1.0 / (gl_Fog.end - gl_Fog.start)
	fogFactor = clamp(fogFactor, 0.0, 1.0);
}

