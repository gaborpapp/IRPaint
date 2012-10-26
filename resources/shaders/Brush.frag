#version 120

uniform sampler2D stencil;
uniform sampler2D brush;
uniform vec4 color;
uniform vec2 windowSize;

void main()
{
	vec2 uv = gl_TexCoord[ 0 ].st;
	vec2 stencilUv = gl_FragCoord.xy / windowSize;
	float alpha = texture2D( stencil, stencilUv ).a;
	vec4 brushColor = texture2D( brush, uv ) * color;
	brushColor.a *= alpha;
	gl_FragColor = brushColor;
}

