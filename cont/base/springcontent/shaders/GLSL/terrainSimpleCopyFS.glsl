
#ifdef UseTextureRECT
#extension GL_ARB_texture_rectangle : enable

	uniform sampler2DRect sourceTexture;

	void main()
	{
		gl_FragColor=texture2DRect(sourceTexture, gl_FragCoord.xy);
	}

#else

	uniform sampler2D sourceTexture;
	uniform vec2 invScreenDim; 

	void main()
	{
		gl_FragColor = texture2D(sourceTexture, gl_FragCoord.xy * invScreenDim);
	}

#endif
