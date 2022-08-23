#version 450 core

uniform vec3 color;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform float ambientStrength;

in vec4 posInWdSp;

out vec4 fragColor;

void main() {
    vec3 ambient = ambientStrength * lightColor;

	vec3 norm = vec3(0, -1.0, 0);
	vec3 lightDrc = normalize(lightPos - posInWdSp.xyz);
	float diff = max(dot(norm, lightDrc), 0.0);
	vec3 diffuse = diff * lightColor;
	
	fragColor = vec4((ambient + diffuse) * color.rgb, 1.0);
}
