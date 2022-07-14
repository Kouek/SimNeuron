#version 450 core

uniform vec2 offset;
uniform vec2 scale;

layout(lines) in;
layout(line_strip, max_vertices = 4) out;

in VertDat {
    vec3 cntrPos;
    vec3 delta;
} gs_in[];

void main() {
    vec2 pos2 = vec2(gs_in[0].cntrPos + gs_in[0].delta);
    pos2 = scale * (pos2 + offset);
	gl_Position = vec4(pos2, 0, 1.0);
    EmitVertex();
    pos2 = vec2(gs_in[1].cntrPos + gs_in[1].delta);
    pos2 = scale * (pos2 + offset);
	gl_Position = vec4(pos2, 0, 1.0);
    EmitVertex();
    EndPrimitive();

    pos2 = vec2(gs_in[0].cntrPos - gs_in[0].delta);
    pos2 = scale * (pos2 + offset);
	gl_Position = vec4(pos2, 0, 1.0);
    EmitVertex();
    pos2 = vec2(gs_in[1].cntrPos - gs_in[1].delta);
    pos2 = scale * (pos2 + offset);
	gl_Position = vec4(pos2, 0, 1.0);
    EmitVertex();
    EndPrimitive();
}
