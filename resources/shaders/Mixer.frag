#version 120

uniform sampler2D drawing;
uniform sampler2D stencil;
uniform sampler2D glow0;
uniform sampler2D glow1;

void main()
{
	vec2 uv = gl_TexCoord[ 0 ].st;
	float alpha = texture2D( stencil, uv ).a;
	vec4 drawingColor = texture2D( drawing, uv );
	vec4 glowColor = texture2D( glow0, uv ) + texture2D( glow1, uv );
	gl_FragColor = mix( glowColor, drawingColor, alpha );
}

