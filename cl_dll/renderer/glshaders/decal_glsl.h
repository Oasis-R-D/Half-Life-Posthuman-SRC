#pragma once

// simple shader for decals (should probably make one definite shader for simple quad rendering like this)

char glsl_decal_vp[] = R"(

	uniform mat4 projviewmatrix;

	out vec2 frag_texcoord;
	out vec2 frag_lmcoord;
	
	void main()
	{
		frag_texcoord = aTexCoord;
		frag_lmcoord = aTexCoordLM;
		gl_Position = projviewmatrix * vec4(aPosition, 1);
	}



)";

const char glsl_decal_fp[] = R"(

	uniform sampler2D lmtexture;
	uniform sampler2D texture1;
	uniform bool wireframe;

	in vec2 frag_texcoord;
	in vec2 frag_lmcoord;

	void main()
	{
		if(wireframe)
		{
			gl_FragColor = vec4(0, 1, 0, 1); //green
			return;
		}
		vec4 decaltexture = texture(texture1, frag_texcoord);
		decaltexture.rgb *= 2;

		if(decaltexture.a < 0.1)
			discard;

		vec3 lmpixel = texture(lmtexture, frag_lmcoord).rgb;

		decaltexture.rgb *= lmpixel;

		gl_FragColor = decaltexture;
	}

)";