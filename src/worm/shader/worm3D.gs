#version 450 core

uniform uint divNum;
uniform bool reverseNormal;
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
}
gs_in[];

out vec4 posInWdSp;
out vec4 normal;

#define HEAD_K 0.2
#define TAIL_K 3.0

void main() {
    vec3 axis;
    vec3 pos;
    mat3 rot;
    normal.w = 0;

    if (gs_in[0].isFirstLastOrNot == -1) {
        vec4 endPos = vec4(
            (1.0 + HEAD_K) * gs_in[0].cntrPos - HEAD_K * gs_in[1].cntrPos, 1.0);
        vec4 endPosInWdSp = M * endPos;
        endPos = VP * endPosInWdSp;
        vec4 N0, N1;
        N0.w = N1.w = 0;
        vec4 pos0;
        vec4 posInWdSp0;

        pos = gs_in[0].cntrPos + (N0.xyz = gs_in[0].delta);
        pos0 = VP * (posInWdSp0 = M * vec4(pos, 1.0));
        N0 = M * (reverseNormal ? -N0 : N0);

        for (uint idx = 1; idx < divNum; ++idx) {
            gl_Position = pos0;
            posInWdSp = posInWdSp0;
            normal = N0;
            EmitVertex(); // last vertex

            float s = sins[idx];
            float c = coss[idx];
            float oc = 1.0 - c;
            axis = normalize(cross(gs_in[0].delta, vec3(0, 0, -1.0)));
            rot = mat3(
                oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s,
                oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c,
                oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,
                oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c);
            pos = gs_in[0].cntrPos + (N1.xyz = rot * gs_in[0].delta);
            gl_Position = pos0 =
                VP * (posInWdSp = posInWdSp0 = M * vec4(pos, 1.0));
            normal = N1 = M * (reverseNormal ? -N1 : N1);
            EmitVertex();

            gl_Position = endPos;
            posInWdSp = endPosInWdSp;
            normal = (N0 + N1) / 2; // avg normal of 2 vertices
            EmitVertex();

            EndPrimitive();
            N0 = N1;
        }

        gl_Position = pos0;
        posInWdSp = posInWdSp0;
        normal = N0;
        EmitVertex();

        pos = gs_in[0].cntrPos + (N1.xyz = gs_in[0].delta);
        gl_Position = pos0 = VP * (posInWdSp = posInWdSp0 = M * vec4(pos, 1.0));
        normal = N1 = M * (reverseNormal ? -N1 : N1);
        EmitVertex();

        gl_Position = endPos;
        posInWdSp = endPosInWdSp;
        normal = (N0 + N1) / 2; // avg normal of 2 vertices
        EmitVertex(); // end vertex

        EndPrimitive();
    } else if (gs_in[1].isFirstLastOrNot == 1) {
        vec4 endPos = vec4(
            (1.0 + TAIL_K) * gs_in[1].cntrPos - TAIL_K * gs_in[0].cntrPos, 1.0);
        vec4 endPosInWdSp = M * endPos;
        endPos = VP * endPosInWdSp;
        vec4 N0, N1;
        N0.w = N1.w = 0;
        vec4 pos0;
        vec4 posInWdSp0;

        pos = gs_in[1].cntrPos + (N0.xyz = gs_in[1].delta);
        pos0 = VP * (posInWdSp0 = M * vec4(pos, 1.0));
        N0 = M * (reverseNormal ? -N0 : N0);

        for (uint idx = 1; idx < divNum; ++idx) {
            gl_Position = pos0;
            posInWdSp = posInWdSp0;
            normal = N0;
            EmitVertex(); // last vertex

            float s = sins[idx];
            float c = coss[idx];
            float oc = 1.0 - c;
            axis = normalize(cross(gs_in[1].delta, vec3(0, 0, -1.0)));
            rot = mat3(
                oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s,
                oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c,
                oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,
                oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c);
            pos = gs_in[1].cntrPos + (N1.xyz = rot * gs_in[1].delta);
            N1 = M * (reverseNormal ? -N1 : N1);

            gl_Position = endPos;
            posInWdSp = endPosInWdSp;
            normal = (N0 + N1) / 2; // avg normal of 2 vertices
            EmitVertex(); // end vertex

            gl_Position = pos0 =
                VP * (posInWdSp = posInWdSp0 = M * vec4(pos, 1.0));
            normal = N1;
            EmitVertex();

            EndPrimitive();
            N0 = N1;
        }

        gl_Position = pos0;
        posInWdSp = posInWdSp0;
        normal = N0;
        EmitVertex();

        pos = gs_in[1].cntrPos + (N1.xyz = gs_in[1].delta);
        N1 = M * (reverseNormal ? -N1 : N1);

        gl_Position = endPos;
        posInWdSp = endPosInWdSp;
        normal = (N0 + N1) / 2; // avg normal of 2 vertices
        EmitVertex(); // end vertex

        gl_Position = pos0 = VP * (posInWdSp = posInWdSp0 = M * vec4(pos, 1.0));
        normal = N1;
        EmitVertex();

        EndPrimitive();
    }

    for (uint idx = 0; idx < divNum; ++idx) {
        float s = sins[idx];
        float c = coss[idx];
        float oc = 1.0 - c;
        axis = normalize(cross(gs_in[0].delta, vec3(0, 0, -1.0)));
        rot = mat3(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s,
                   oc * axis.z * axis.x + axis.y * s,
                   oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c,
                   oc * axis.y * axis.z - axis.x * s,
                   oc * axis.z * axis.x - axis.y * s,
                   oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c);
        pos = gs_in[0].cntrPos + (normal.xyz = rot * gs_in[0].delta);
        gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
        normal = M * (reverseNormal ? -normal : normal);
        EmitVertex();

        axis = normalize(cross(gs_in[1].delta, vec3(0, 0, -1.0)));
        rot = mat3(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s,
                   oc * axis.z * axis.x + axis.y * s,
                   oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c,
                   oc * axis.y * axis.z - axis.x * s,
                   oc * axis.z * axis.x - axis.y * s,
                   oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c);
        pos = gs_in[1].cntrPos + (normal.xyz = rot * gs_in[1].delta);
        gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
        normal = M * (reverseNormal ? -normal : normal);
        EmitVertex();
    }
    pos = gs_in[0].cntrPos + gs_in[0].delta;
    gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
    normal.xyz = gs_in[0].delta;
    normal = M * (reverseNormal ? -normal : normal);
    EmitVertex();

    pos = gs_in[1].cntrPos + gs_in[1].delta;
    gl_Position = VP * (posInWdSp = M * vec4(pos, 1.0));
    normal.xyz = gs_in[1].delta;
    normal = M * (reverseNormal ? -normal : normal);
    EmitVertex();

    EndPrimitive();
}
