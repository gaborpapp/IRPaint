#version 120

uniform sampler2D drawing;
uniform sampler2D stencil;
uniform vec2 size; // window size

void main()
{
	vec2 stencilUv = gl_FragCoord.xy;
	stencilUv.y = size.y - stencilUv.y; // flip y
	stencilUv /= size;
	vec2 uv = gl_TexCoord[ 0 ].st;
	float alpha = texture2D( stencil, stencilUv ).a;
	vec4 drawingColor = texture2D( drawing, uv );
	gl_FragColor = vec4( drawingColor.rgb, alpha );
}

