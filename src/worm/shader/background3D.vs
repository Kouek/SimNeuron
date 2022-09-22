#version 450 core

uniform float z;
uniform mat4 VP;

layout(location = 0) in vec2 posIn;
layout(location = 1) in vec2 texCoordIn;

out vec2 texCoord;

void main() {
    gl_Position = VP * vec4(posIn, z, 1.0);
    texCoord = texCoordIn;
}
