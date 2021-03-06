#version 150 core

in fromVertex {
    vec3 color;
} fromV;

out toFragment {
    vec3 color;
} toF;

out fromVertex {  // okay to reuse a block name for another block name
    vec3 color;
};

out fooB {
    vec2 color;
} fromVertex;     // ERROR, cannot reuse block name as block instance

int fromVertex;   // ERROR, cannot reuse a block name for something else

out fooC {
    vec2 color;
} fooC;           // ERROR, cannot have same name for block and instance name

void main()
{
    EmitVertex();
    EndPrimitive();
    EmitStreamVertex(1);    // ERROR
    EndStreamPrimitive(0);  // ERROR

    color = fromV.color;
    gl_ClipDistance[3] = gl_in[1].gl_ClipDistance[2];
    gl_Position = gl_in[0].gl_Position;
    gl_PointSize = gl_in[3].gl_PointSize;
    gl_PrimitiveID = gl_PrimitiveIDIn;
    gl_Layer = 2;
}

out vec4 ov0;  // stream should be 0
layout(stream = 4) out vec4 ov4;
out vec4 o1v0;  // stream should be 0

layout(stream = 3) uniform;        // ERROR
layout(stream = 3) in;             // ERROR
layout(stream = 3) uniform int ua; // ERROR
layout(stream = 3) uniform ubb { int ua; } ibb; // ERROR

layout(line_strip, points, triangle_strip, stream = 3, points, triangle_strip) out;  // just means "stream = 3, triangle_strip"
layout(stream = 3, triangle_strip) out;
out vec4 ov3;  // stream should be 3

layout(stream = 6) out ooutb { vec4 a; } ouuaa6;

layout(stream = 6) out ooutb2 {
    layout(stream = 6) vec4 a;
} ouua6;

layout(stream = 7) out ooutb3 {
    layout(stream = 6) vec4 a;  // ERROR
} ouua7;

out vec4 ov2s3;  // stream should be 3

void foo(layout(max_vertices = 4) int a)  // ERROR
{
    ouuaa6.a = vec4(1.0);
}

layout(line_strip, points, triangle_strip, stream = 3, points) out;  // ERROR, changing output primitive
layout(line_strip, points, stream = 3) out; // ERROR, changing output primitive
layout(triangle_strip) in; // ERROR, not an input primitive
layout(triangle_strip) uniform; // ERROR
layout(triangle_strip) out vec4 badv4;  // ERROR, not on a variable
layout(triangle_strip) in vec4 bad2v4;  // ERROR, not on a variable or input
layout(invocations = 3) out outbn { int a; }; // ERROR, not on a block
out outbn2 {
    layout(invocations = 3)  int a; // ERROR, not on a block member
    layout(max_vertices = 3) int b; // ERROR, not on a block member
    layout(triangle_strip)   int c; // ERROR, not on a block member
} outbi;

layout(lines) out;  // ERROR, not on output
layout(lines_adjancency) in;
layout(triangles) in;             // ERROR, can't change it
layout(triangles_adjacency) in;   // ERROR, can't change it

layout(invocations = 4, max_vertices = 300) out;

in inbn {
    layout(stream = 2) int a;     // ERROR, stream on input
} inbi;

in sameName {
    int a15;
};

out sameName {
    float f15;
};

uniform sameName {
    bool b15;
};
