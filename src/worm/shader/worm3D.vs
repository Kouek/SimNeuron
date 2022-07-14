#version 450 core

layout(location = 0) in vec3 cntrPosIn;
layout(location = 1) in vec3 deltaIn;

out VertDat {
    vec3 cntrPos;
    vec3 delta;
    int isFirstLastOrNot;
} vs_out;

void main() {
    vs_out.cntrPos = vec3(cntrPosIn.xy, 0);
    vs_out.delta = deltaIn;
    vs_out.isFirstLastOrNot = cntrPosIn.z == -1.0 ? -1
        : (cntrPosIn.z == 1.0 ? 1 : 0);
}
