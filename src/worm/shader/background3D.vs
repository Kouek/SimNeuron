#version 450 core

uniform float z;
uniform mat4 VP;

layout(location = 0) in vec2 posIn;

out vec4 posInWdSp;

void main() {
    posInWdSp = vec4(posIn, z, 1.0);
    gl_Position = VP * posInWdSp;
}
