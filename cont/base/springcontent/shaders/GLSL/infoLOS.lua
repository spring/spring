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
		uniform vec4 alwaysColor;
		uniform vec4 losColor;
		uniform vec4 radarColor;
		uniform vec4 jamColor;

		uniform sampler2D tex0;
		uniform sampler2D tex1;
		uniform sampler2D tex2;
		varying vec2 texCoord;

		mat4 COLORMATRIX0 = mat4(losColor,   0.00,0.00,0.00,0.0, 0.0,0.0,0.0,0.0, 0.0,0.0,0.0,1.0);
		mat4 COLORMATRIX1 = mat4(losColor,   0.00,0.00,0.00,0.0, 0.0,0.0,0.0,0.0, 0.0,0.0,0.0,1.0);
		mat4 COLORMATRIX2 = mat4(radarColor, jamColor,           0.0,0.0,0.0,0.0, 0.0,0.0,0.0,1.0);

	#ifdef HIGH_QUALITY
	#extension GL_ARB_texture_query_lod : enable

		//! source: http://www.ozone3d.net/blogs/lab/20110427/glsl-random-generator/
		float rand(vec2 n)
		{
			return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
		}

		vec4 getTexel(sampler2D tex, vec2 p)
		{
			int lod = int(textureQueryLOD(tex, p).x);
			vec2 texSize = vec2(textureSize(tex, lod));
			vec2 off = vec2(0.0);
			vec4 c = vec4(0.0);

			for (int i = 0; i<8; i++) {
				c += texture2D(tex, p + off);
				off = (vec2(rand(p.st + off.st), rand(p.ts - off.ts)) * 2.0 - 1.0) / texSize;
			}
			c *= 0.125;

			return smoothstep(0.1, 1.0, c);
		}
	#else
		#define getTexel texture2D
	#endif

		void main() {
			gl_FragColor  = alwaysColor;
			gl_FragColor += COLORMATRIX0 * getTexel(tex0, texCoord);
			gl_FragColor += COLORMATRIX1 * getTexel(tex1, texCoord);

			vec4 radarJammer = getTexel(tex2, texCoord);
			radarJammer.r = fract(radarJammer.r); // needed for zk's radar edge detection

			gl_FragColor += COLORMATRIX2 * radarJammer;
			gl_FragColor.a = 0.05;
		}
	]],
	uniformFloat = {
		alwaysColor = select(1, Spring.GetLosViewColors()),
		losColor    = select(2, Spring.GetLosViewColors()),
		radarColor  = select(3, Spring.GetLosViewColors()),
		jamColor    = select(4, Spring.GetLosViewColors()),
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
