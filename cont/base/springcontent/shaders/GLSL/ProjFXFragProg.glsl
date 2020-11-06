#version 130

uniform sampler2D atlasTex;
uniform sampler2D depthTex;

in vec4 vertColor;
in vec4 vsPos;

#define projMatrix gl_ProjectionMatrix

uniform vec2 invScreenSize;
uniform float softenValue;

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

#define EXTRA_UNSAFE_SOFTNESS 1

float GetViewSpaceDepth(float d) {
	#ifndef DEPTH_CLIP01
		d = NORM2SNORM(d);
	#endif
	return -projMatrix[3][2] / (projMatrix[2][2] + d);
}

void main() {
	vec4 color = texture(atlasTex, gl_TexCoord[0].xy);
	vec2 screenUV = gl_FragCoord.xy * invScreenSize;

	float depthZO = texture(depthTex, screenUV).x;
	float depthVS = GetViewSpaceDepth(depthZO);
	float edgeSmoothness;

	if (softenValue > 0.0) {
		edgeSmoothness = smoothstep(0.0, softenValue, vsPos.z - depthVS); // soften edges
		gl_FragColor  = color * vertColor;
		gl_FragColor *= edgeSmoothness;
		#if (EXTRA_UNSAFE_SOFTNESS == 1)
			//extra edge softness
			//makes some grey ghosts more visible
			gl_FragColor.a = mix(gl_FragColor.a, 0.005, smoothstep(0.01, 0.0, gl_FragColor.a));
		#endif

	} else {
		edgeSmoothness = smoothstep(softenValue, 0.0, vsPos.z - depthVS); // follow the surface up
		gl_FragColor  = color * vertColor;
		gl_FragColor *= edgeSmoothness;
	}
}