#version 130

uniform vec4 planeColor;
uniform vec4 fogColor;
uniform vec3 fogParams;

in vec3 worldCameraDir;
out vec4 fragColor;

void main()
{
	float fogDist = length(worldCameraDir);

	float fogFactor = (fogParams.y - fogDist) * fogParams.z;
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	fragColor = mix(fogColor, planeColor, fogFactor);
}