#version 120

uniform sampler2D drawing;
uniform sampler2D stencil;

void main()
{
	vec2 uv = gl_TexCoord[ 0 ].st;
	float alpha = texture2D( stencil, uv ).a;
	vec4 drawingColor = texture2D( drawing, uv );
	gl_FragColor = drawingColor * alpha;
}

