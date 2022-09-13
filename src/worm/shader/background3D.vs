#version 450 core

uniform float z;
uniform mat4 VP;

layout(location = 0) in vec2 posIn;

void main() {
    gl_Position = VP * vec4(posIn, z, 1.0);
}
