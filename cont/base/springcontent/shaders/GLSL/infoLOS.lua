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

		uniform float time;

		uniform vec4 alwaysColor;
		uniform vec4 losColor;
		uniform vec4 radarColor;
		uniform vec4 radarColor2;
		uniform vec4 jamColor;

		uniform sampler2D tex0;
		uniform sampler2D tex1;
		uniform sampler2D tex2;

		in vec2 vTexCoords;
		layout(location = 0) out vec4 fFragColor;

	#ifdef HIGH_QUALITY

		//! source: http://www.ozone3d.net/blogs/lab/20110427/glsl-random-generator/
		float rand(const in vec2 n)
		{
			return fract(sin(dot(n, vec2(12.9898, 78.233))) * 43758.5453);
		}

		vec4 getTexel(in sampler2D tex, in vec2 p)
		{
			int lod = int(textureQueryLOD(tex, p).x);
			vec2 texSize = vec2(textureSize(tex, lod));
			vec2 off = vec2(time);
			vec4 c = vec4(0.0);

			for (int i = 0; i<4; i++) {
				off = (vec2(rand(p.st + off.st), rand(p.ts - off.ts)) * 2.0 - 1.0) / texSize;
				c += texture(tex, p + off);
			}
			c *= 0.25;

			return smoothstep(0.5, 1.0, c);
		}
	#else
		#define getTexel texture
	#endif

		void main() {
			fFragColor = vec4(0.0);
			// fFragColor = alwaysColor;

			vec2 radarJammer = getTexel(tex2, vTexCoords).rg;

			fFragColor += jamColor * radarJammer.g;
			fFragColor += radarColor2 * step(0.8, radarJammer.r) * radarJammer.r;
			fFragColor.rgb = fract(fFragColor.rgb);

			float los = getTexel(tex0, vTexCoords).r;
			float airlos = getTexel(tex1, vTexCoords).r;

			fFragColor += losColor * ((los + airlos) * 0.5);
			fFragColor += radarColor * step(0.2, fract(1 - radarJammer.r));
			fFragColor += alwaysColor;
			fFragColor.a = 0.05;
		}
	]],
	uniformFloat = {
		alwaysColor = select(1, Spring.GetLosViewColors()),
		losColor    = select(2, Spring.GetLosViewColors()),
		radarColor  = select(3, Spring.GetLosViewColors()),
		jamColor    = select(4, Spring.GetLosViewColors()),
		radarColor2 = select(5, Spring.GetLosViewColors()),
	},
	uniformInt = {
		tex0 = 0,
		tex1 = 1,
		tex2 = 2,
		tex3 = 3,
	},
	textures = {
		[0] = "$info:los",
		[1] = "$info:airlos",
		[2] = "$info:radar",
	},
}
