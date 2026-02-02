#version 330 core

uniform sampler2D stainedGlass;
uniform float transAlpha;

in vec2 texCoord;

out vec4 fragColour;

void main() {
	vec4 texColour = texture(stainedGlass, texCoord);

	fragColour = vec4(texColour.rgb, texColour.a * transAlpha);
}