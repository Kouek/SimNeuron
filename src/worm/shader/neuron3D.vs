#version 450 core

layout(location = 0) in vec3 posIn;

out VertDat {
    vec3 cntrPos;
} vs_out;

void main() {
    vs_out.cntrPos = posIn;
}
