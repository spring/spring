return {
	definitions = {
		Spring.GetConfigInt("HighResInfoTexture") and "#define HIGH_QUALITY" or "",
	},

	vertex = [[
		#version 410 core
		layout(location = 0) in vec3 aVertexPos;
		layout(location = 1) in vec2 aTexCoords;

		out vec2 vTexCoords;

		void main() {
			vTexCoords = aTexCoords;
			gl_Position = vec4(aVertexPos, 1.0);
		}
	]],

	fragment = [[
		#version 410 core
		#ifdef HIGH_QUALITY
		#extension GL_ARB_texture_query_lod : enable
		#endif

		uniform sampler2D tex0;
		uniform sampler2D tex1;

		in vec2 vTexCoords;
		layout(location = 0) out vec4 fFragColor;

		mat4 COLORMATRIX0 = mat4(0.80, 0.00, 0.00, 1.0,  0.00, 0.80, 0.20, 1.0,  1.0, 0.6, 0.0, 1.0,  0.0, 0.0, 0.0, 1.0);

	#ifdef HIGH_QUALITY

		//! source: http://www.ozone3d.net/blogs/lab/20110427/glsl-random-generator/
		float rand(vec2 n)
		{
			return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
		}

		//! source: http://www.iquilezles.org/www/articles/texture/texture.htm
		vec4 getTexel(sampler2D tex, vec2 p)
		{
			int lod = int(textureQueryLOD(tex, p).x);
			vec2 texSize = vec2(textureSize(tex, lod)) * 0.5;
			p = p * texSize + 0.5;

			vec2 i = floor(p);
			vec2 f = p - i;
			vec2 ff = f*f;
			f = ff * f * ((ff * 6.0 - f * 15.0) + 10.0);
			p = i + f;

			p = (p - 0.5) / texSize;

			vec2 off = vec2(0.0);
			vec4 c = vec4(0.0);

			off = (vec2(rand(p.st + off),rand(p.ts + off)) * 2.0 - 1.0) / texSize; c += texture(tex, p + off * 0.25);
			off = (vec2(rand(p.st + off),rand(p.ts + off)) * 2.0 - 1.0) / texSize; c += texture(tex, p + off * 0.25);
			off = (vec2(rand(p.st + off),rand(p.ts + off)) * 2.0 - 1.0) / texSize; c += texture(tex, p + off * 0.25);
			off = (vec2(rand(p.st + off),rand(p.ts + off)) * 2.0 - 1.0) / texSize; c += texture(tex, p + off * 0.25);

			return c * 0.25;
		}
	#else
		#define getTexel texture
	#endif

		void main() {
			vec4 null = vec4(0.5, 0.5, 0.5, 1.0);
			vec4 pathData = texture(tex0, vTexCoords);

			fFragColor = COLORMATRIX0 * pathData;

			fFragColor.r -= smoothstep(0.75, 1.0, pathData.r) * 0.3;
			fFragColor.b += smoothstep(0.75, 1.0, pathData.r) * 1.0;

			fFragColor = mix(vec4(1.0), fFragColor, getTexel(tex1, vTexCoords).r * 0.35 + 0.7);
			fFragColor.a = 0.3;
		}
	]],
	uniformInt = {
		tex0 = 0,
		tex1 = 1,
	},
	textures = {
		[0] = "$info:path",
		[1] = "$info:los",
	},
}
