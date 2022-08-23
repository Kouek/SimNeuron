#version 450 core

uniform mat4 M;
uniform mat4 VP;
uniform float halfWid;

layout(points) in;
layout(triangle_strip, max_vertices = 18) out;

in VertDat { vec3 cntrPos; }
gs_in[];

void main() {
    mat4 MVP = VP * M;
    vec3 cntrPos = gs_in[0].cntrPos;

    // front
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y + halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y + halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y - halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y - halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    // bottom
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y - halfWid,
                       cntrPos.z - halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y - halfWid,
                       cntrPos.z - halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    // back
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y + halfWid,
                       cntrPos.z - halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y + halfWid,
                       cntrPos.z - halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    // top
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y + halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y + halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();

    EndPrimitive();

    // left
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y + halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y + halfWid,
                       cntrPos.z - halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y - halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x - halfWid, cntrPos.y - halfWid,
                       cntrPos.z - halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();

    EndPrimitive();

    // right
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y + halfWid,
                       cntrPos.z - halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y + halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y - halfWid,
                       cntrPos.z - halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();
    gl_Position = vec4(cntrPos.x + halfWid, cntrPos.y - halfWid,
                       cntrPos.z + halfWid, 1.0);
    gl_Position = MVP * gl_Position;
    EmitVertex();

    EndPrimitive();
}
