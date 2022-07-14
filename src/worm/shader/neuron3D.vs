#version 450 core

uniform mat4 M;
uniform mat4 VP;

layout(location = 0) in vec3 posIn;

void main() {
    gl_Position = VP * M * vec4(posIn, 1.0);
}
