return {
	definitions = {
		Spring.GetConfigInt("HighResInfoTexture") and "#define HIGH_QUALITY" or "",
	},
	vertex = [[#version 130
		varying vec2 texCoord;

		void main() {
			texCoord = gl_MultiTexCoord0.st;
			gl_Position = vec4(gl_Vertex.xyz, 1.0);
		}
	]],
	fragment = [[#version 130
	#ifdef HIGH_QUALITY
	#extension GL_ARB_texture_query_lod : enable
	#endif

		uniform sampler2D tex0;
		varying vec2 texCoord;

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
			return texture2D(tex, p);
		}
	#else
		#define getTexel texture2D
	#endif

		void main() {
			gl_FragColor = getTexel(tex0, texCoord);
			gl_FragColor.a = 0.3;
		}
	]],
	uniformInt = {
		tex0 = 0,
	},
	textures = {
		[0] = "$info:height",
	},
}
