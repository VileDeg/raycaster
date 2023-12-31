#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D topViewTex;
uniform sampler2D povTex;

void main()
{
	// Display top view texture only on left half of the screen
	if (TexCoord.x < 1.0) {
		FragColor = texture(topViewTex, TexCoord);
	} else {
		FragColor = texture(povTex, TexCoord);
	}
}