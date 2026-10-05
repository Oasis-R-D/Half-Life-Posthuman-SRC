#pragma once

char glsl_overlay_vp[] = R"(

	out vec2 frag_texcoord;
	
	void main()
	{
		frag_texcoord = aTexCoord;
		frag_color = aColor;
		gl_Position = vec4(aPosition, 1); // 2nd and third should be 0, 1?
	}

)";

const char glsl_overlay_fp[] = R"(

	uniform sampler2D texture0;

	in vec2 frag_texcoord;

	void main()
	{
		gl_FragColor = texture(texture0, frag_texcoord);
	}

)";