#version 120
#extension GL_EXT_gpu_shader4 : enable

uniform sampler2D tex0;

varying vec2 gsTexCoord;

void main(void)
{
	vec4 clr = texture2D(tex0, gsTexCoord.xy);
 	gl_FragColor = gl_Color; // * clr;
}
