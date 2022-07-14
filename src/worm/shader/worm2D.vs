#version 450 core

layout(location = 0) in vec3 cntrPosIn;
layout(location = 1) in vec3 deltaIn;

out VertDat {
    vec3 cntrPos;
    vec3 delta;
} vs_out;

void main() {
    vs_out.cntrPos = cntrPosIn;
    vs_out.delta = deltaIn;
}