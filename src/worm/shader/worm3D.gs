#version 450 core

uniform uint divNum;
uniform mat4 M;
uniform mat4 VP;
uniform float coss[32];
uniform float sins[32];

layout(lines) in;
layout(triangle_strip, max_vertices = 128) out;

in VertDat {
    vec3 cntrPos;
    vec3 delta;
    int isFirstLastOrNot;
} gs_in[];

out vec4 posInWdSp;
out vec4 normal;

#define HEAD_K 0.3
#define TAIL_K 1.5

void main() {
    vec3 axis;
    vec3 pos;
    mat3 rot;
    if (gs_in[0].isFirstLastOrNot == -1) {
        vec4 endPosInWdSp;
        vec4 endPos = VP * (endPosInWdSp = posInWdSp = M * vec4(
            (1.0 + HEAD_K) * gs_in[0].cntrPos - HEAD_K * gs_in[1].cntrPos, 1.0));
        vec4 endN = normal = M * vec4(gs_in[0].cntrPos - gs_in[1].cntrPos, 0);
        gl_Position = endPos;
        EmitVertex();
        for (uint idx = 0; idx < divNum; ++idx) {
            float s = sins[idx];
            float c = coss[idx];
            float oc = 1.0 - c;

            axis = normalize(cross(gs_in[0].delta, vec3(0, 0, -1.0)));
            rot = mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                       oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                       oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
            pos = gs_in[0].cntrPos + (normal.xyz = rot * gs_in[0].delta);
            gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
            normal = M * normal;
            EmitVertex();

            gl_Position = endPos;
            posInWdSp = endPosInWdSp;
            normal = endN;
            EmitVertex();
        }
        EndPrimitive();
    } else if (gs_in[1].isFirstLastOrNot == 1) {
        vec4 endPosInWdSp;
        vec4 endPos = VP * (endPosInWdSp = posInWdSp = M * vec4(
            (1.0 + TAIL_K ) * gs_in[1].cntrPos - TAIL_K  * gs_in[0].cntrPos, 1.0));
        vec4 endN = normal = M * vec4(gs_in[1].cntrPos - gs_in[0].cntrPos, 0);
        gl_Position = endPos;
        EmitVertex();
        for (uint idx = 0; idx < divNum; ++idx) {
            float s = sins[idx];
            float c = coss[idx];
            float oc = 1.0 - c;

            axis = normalize(cross(gs_in[1].delta, vec3(0, 0, 1.0)));
            rot = mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                       oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                       oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
            pos = gs_in[1].cntrPos + (normal.xyz = rot * gs_in[1].delta);
            gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
            normal = M * normal;
            EmitVertex();

            gl_Position = endPos;
            posInWdSp = endPosInWdSp;
            normal = endN;
            EmitVertex();
        }
        EndPrimitive();
    }

    for (uint idx = 0; idx < divNum; ++idx) {
        float s = sins[idx];
        float c = coss[idx];
        float oc = 1.0 - c;

        axis = normalize(cross(gs_in[0].delta, vec3(0, 0, -1.0)));
        rot = mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                   oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                   oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
        pos = gs_in[0].cntrPos + (normal.xyz = rot * gs_in[0].delta);
        gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
        normal = M * normal;
        EmitVertex();

        axis = normalize(cross(gs_in[1].delta, vec3(0, 0, -1.0)));
        rot = mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                   oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                   oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
        pos = gs_in[1].cntrPos + (normal.xyz = rot * gs_in[1].delta);
        gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
        normal = M * normal;
        EmitVertex();
    }
    pos = gs_in[0].cntrPos + (normal.xyz = gs_in[0].delta);
    gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
    normal = M * normal;
    EmitVertex();
    pos = gs_in[1].cntrPos + gs_in[1].delta;
    gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
    normal = M * normal;
    EmitVertex();

    EndPrimitive();
}
