#version 420

void main()
{
    int u = gl_VertexID >> 1;
    int v = (gl_VertexID & 1)^1;

    gl_Position = vec4(
        -1.0 + 2.0 * u,
        -1.0 + 2.0 * v,
        0.0,
        1.0);
}
