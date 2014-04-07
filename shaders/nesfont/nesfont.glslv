#version 420

void main()
{
    gl_Position = vec4(
        -1.0 + 4.0 * (gl_VertexID >> 1),
        -1.0 + 4.0 * (gl_VertexID & 1),
        0.0,
        1.0);
}
