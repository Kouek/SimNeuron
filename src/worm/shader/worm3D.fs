#version 450 core

uniform vec4 color;
out vec4 fragColor;

in vec4 posInWdSp;
in vec4 normal;

void main() {
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

	vec3 norm = normalize(normal.xyz);
	vec3 lightDrc = normalize(vec3(0, 0, 0.5) - posInWdSp.xyz);
	float diff = max(dot(norm, lightDrc), 0.0);
	vec3 diffuse = diff * lightColor;
	
	fragColor = vec4((ambient + diffuse) * color.rgb, color.a);
}
