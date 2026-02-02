#version 330 core

uniform sampler2D sceneTex;

uniform int isVertical;

in Vertex {
	vec3 colour;//tutorial has it as vec4
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
} IN;

out vec4 fragColor;

const float scaleFactors[7] = float[](0.006, 0.061, 0.242, 0.383, 0.242, 0.061, 0.006);

void main(void) {
	fragColor = vec4(0, 0, 0, 1);

	vec2 delta = vec2(0, 0);

	if (isVertical == 1) {
		delta = dFdy(IN.texCoord);
	}
	else {
		delta = dFdx(IN.texCoord);
	}

	for (int i = 0; i < 7; i++) {
		vec2 offset = delta * (i - 3);
		vec4 tmp = texture(sceneTex, IN.texCoord.xy + offset);
		//vec4 tmp = texture2D(sceneTex, IN.texCoord.xy + offset); doesnt work outside of lab comps
		fragColor += tmp * scaleFactors[i];
	}
}