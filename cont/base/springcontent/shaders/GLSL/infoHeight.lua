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

		in vec2 vTexCoords;
		layout(location = 0) out vec4 fFragColor;

	#ifdef HIGH_QUALITY

		//! source: http://www.iquilezles.org/www/articles/texture/texture.htm
		vec4 getTexel(sampler2D tex, vec2 p)
		{
			int lod = int(textureQueryLOD(tex, p).x);
			vec2 texSize = vec2(textureSize(tex, lod));
			p = p * texSize + 0.5;

			vec2 i = floor(p);
			vec2 f = p - i;
			vec2 ff = f*f;

			f = ff * f * ((ff * 6.0 - f * 15.0) + 10.0);
			p = i + f;
			p = (p - 0.5) / texSize;

			return texture(tex, p);
		}
	#else
		#define getTexel texture
	#endif

		void main() {
			fFragColor = getTexel(tex0, vTexCoords);
			fFragColor.a = 0.3;
		}
	]],
	uniformInt = {
		tex0 = 0,
	},
	textures = {
		[0] = "$info:height",
	},
}
